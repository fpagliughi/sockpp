#include <iostream>

#include "sockpp/error.h"
#include "sockpp/inet_address.h"
#include "sockpp/socket.h"
#include "sockpp/version.h"

int main() {
    sockpp::socket_initializer::initialize();

    sockpp::error_code ec;
    sockpp::inet_address addr("localhost", 12345, ec);

    std::cout << "sockpp version: " << sockpp::SOCKPP_VERSION << '\n';
    if (!ec)
        std::cout << "resolved address: " << addr << '\n';
    else
        std::cout << "address resolution skipped: " << ec.message() << '\n';
    return 0;
}
