// udptst.cpp

#include <iostream>
#include "sockpp/socket.h"

using namespace std;

// --------------------------------------------------------------------------

int do_recv(sockpp::udp_socket& sock)
{
	char buf[6];
	int n = sock.recv(buf, sizeof(buf));

	if (n < 0) {
		cerr << "Error sending packet: ["
			<< sock.last_error() << "]" << endl;
		return -1;
	}

	cout << "Received " << n << " bytes" << flush;
	buf[n] = '\0';
	cout << " '" << buf << "'" << endl;
	return 0;
}

// --------------------------------------------------------------------------

int main()
{
	in_port_t port = 12345;

	cout << "Testing UDP sockets" << endl;
	sockpp::udp_socket srvrSock;

	if (!srvrSock) {
		cerr << "Error creating server socket [" 
			<< srvrSock.last_error() << "]" << endl;
		return 1;
	}

	if (!srvrSock.bind(port)) {
		cerr << "Error binding to port: " << port << " [" 
			<< srvrSock.last_error() << "]" << endl;
		return 1;
	}

	sockpp::udp_socket cliSock;

	if (!cliSock) {
		cerr << "Error creating server socket [" 
			<< cliSock.last_error() << "]" << endl;
		return 1;
	}

	sockpp::inet_address localAddr("localhost", port);

	if (!cliSock.connect(localAddr)) {
		cerr << "Error connecting to port: " << port << " [" 
			<< cliSock.last_error() << "]" << endl;
		return 1;
	}

	cliSock.send("Hello");
	do_recv(srvrSock);

	cliSock.close();

	sockpp::udp_socket sock;
	sock.sendto("bubba", localAddr);
	do_recv(srvrSock);

	return 0;
}

