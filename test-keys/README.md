# sockpp Test PKI

> **WARNING: For testing only.**
> These keys and certificates are committed to a public repository and must
> never be used in a production system or any environment where real security
> is required.

This directory contains a small two-tier Public Key Infrastructure (PKI) used
by the sockpp unit tests and example programs to exercise TLS, and in the
future, mutual TLS (mTLS).

All keys use P-256 (ECDSA), all certificates are valid for 3652 days (~10
years) from the date they were last generated, and all carry
`subjectAltName=IP:127.0.0.1,DNS:localhost` so they are accepted by TLS
clients connecting to a local server.

## Files

| File | Type | Description |
|------|------|-------------|
| `sockpp-ca.key` | Private key | CA signing key |
| `sockpp-ca.crt` | Self-signed certificate | Trust anchor for all other certificates |
| `server.key` | Private key | Server identity key |
| `server.crt` | CA-signed certificate | TLS server certificate (`serverAuth`) |
| `client.key` | Private key | Client identity key #1 |
| `client.crt` | CA-signed certificate | mTLS client certificate #1 (`clientAuth`) |
| `client2.key` | Private key | Client identity key #2 |
| `client2.crt` | CA-signed certificate | mTLS client certificate #2 (`clientAuth`) |

## Trust hierarchy

```
sockpp-ca (self-signed CA)
├── server.crt   — presented by the server during the TLS handshake
├── client.crt   — presented by client #1 during an mTLS handshake
└── client2.crt  — presented by client #2 during an mTLS handshake
```

A TLS client that trusts `sockpp-ca.crt` will accept connections to any server
presenting `server.crt`.  A TLS server configured to require client
authentication and trust `sockpp-ca.crt` will accept either client certificate.

## Using the certificates with the example apps

Start the echo server:
```sh
./tlssvr test-keys/server.crt test-keys/server.key
```

Connect with a client (pass the CA so the server certificate is trusted):
```sh
./tlscli --trust test-keys/sockpp-ca.crt localhost 4433
```

## Regenerating the keys

When the certificates approach expiry, regenerate the entire PKI by running:

```sh
test-keys/mk_keys.sh
```

The script can be run from anywhere in the repository; it always writes into
its own directory.  It replaces all keys and certificates atomically and
verifies the new chain before exiting.
