// mtechosvr.cpp
// 
// A multi-threaded echo server for sockpp library.
// This is a simple thread-per-connection server.
// 
// USAGE:
// 		mtechosvr [port]
//

#include <iostream>
#include <thread>
#include "sockpp/tcp_acceptor.h"

using namespace std;

// --------------------------------------------------------------------------
// The thread function. This is run in a separate thread for each socket.
// Ownership of the socket object is transferred to the thread, so when this
// function exits, the socket is automatically closed.

void run_echo(sockpp::tcp_socket sock)
{
	int n;
	char buf[512];

	while ((n = sock.read(buf, sizeof(buf))) > 0)
		sock.write_n(buf, n);
}

// --------------------------------------------------------------------------
// The main thread runs the TCP port acceptor. Each time a connection is
// made, a new thread is spawned to handle it, leaving this main thread to
// immediately wait for the next connection.

int main(int argc, char* argv[])
{
	in_port_t port = (argc > 1) ? atoi(argv[1]) : 12345;

	sockpp::tcp_acceptor acc(port);

	if (!acc) {
		cerr << "Error creating the acceptor: " << acc.last_error() << endl;
		return 1;
	}
	cout << "Awaiting connections on port " << port << "..." << endl;

	while (true) {
		// Accept a new client connection
		sockpp::tcp_socket sock = acc.accept();

		if (!sock)
			cerr << "Error accepting incoming connection" << endl;
		else {
			// Create a thread and transfer the new stream to it.
			thread thr(run_echo, std::move(sock));
			thr.detach();
		}
	}

	return 0;
}



