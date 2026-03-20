#include "EchoServer.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
            return 1;
        }

        const int port = std::stoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port: " << port << std::endl;
            return 1;
        }

        configure_server_signals();

        EchoServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}