#!/usr/bin/env bash
# mk_keys.sh
#
# Regenerates the sockpp test PKI:
#
#   sockpp-ca.key / sockpp-ca.crt  — self-signed CA (trust anchor)
#   server.key    / server.crt     — TLS server certificate (serverAuth)
#   client.key    / client.crt     — mTLS client certificate #1 (clientAuth)
#   client2.key   / client2.crt    — mTLS client certificate #2 (clientAuth)
#
# All keys are P-256 (ECDSA).  Run this script from the directory that
# contains it, or from anywhere — it always writes into its own directory.
#
# Usage:
#   ./mk_keys.sh
#
# --------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DAYS=3652
SUBJ_BASE="/O=sockpp/L=Salem/ST=MA/C=US"

# ---------------------------------------------------------------------------
# 1. CA — self-signed
# ---------------------------------------------------------------------------
echo "Generating CA key and certificate..."

openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 \
    -out sockpp-ca.key

openssl req -x509 -new -key sockpp-ca.key \
    -out sockpp-ca.crt \
    -days "$DAYS" -nodes \
    -subj "/CN=sockpp CA${SUBJ_BASE}" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" \
    -addext "subjectKeyIdentifier=hash" \
    -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"

# ---------------------------------------------------------------------------
# 2. Server certificate — signed by CA
# ---------------------------------------------------------------------------
echo "Generating server key and certificate..."

openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 \
    -out server.key

openssl req -new -key server.key \
    -subj "/CN=localhost${SUBJ_BASE}" \
    -out server.csr

openssl x509 -req -in server.csr \
    -CA sockpp-ca.crt -CAkey sockpp-ca.key -CAcreateserial \
    -out server.crt \
    -days "$DAYS" \
    -extfile <(cat <<'EXT'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyAgreement
extendedKeyUsage=serverAuth
subjectAltName=IP:127.0.0.1,DNS:localhost
EXT
)
rm server.csr

# ---------------------------------------------------------------------------
# 3. Client certificates — signed by CA
# ---------------------------------------------------------------------------
for NAME in client client2; do
    echo "Generating ${NAME} key and certificate..."

    openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 \
        -out "${NAME}.key"

    openssl req -new -key "${NAME}.key" \
        -subj "/CN=localhost${SUBJ_BASE}" \
        -out "${NAME}.csr"

    openssl x509 -req -in "${NAME}.csr" \
        -CA sockpp-ca.crt -CAkey sockpp-ca.key -CAcreateserial \
        -out "${NAME}.crt" \
        -days "$DAYS" \
        -extfile <(cat <<'EXT'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=clientAuth
subjectAltName=IP:127.0.0.1,DNS:localhost
EXT
)
    rm "${NAME}.csr"
done

# ---------------------------------------------------------------------------
# 4. Verify all leaf certificates chain up to the CA
# ---------------------------------------------------------------------------
echo "Verifying certificate chain..."
openssl verify -CAfile sockpp-ca.crt server.crt client.crt client2.crt

echo "Done."
