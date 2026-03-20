#include "Server.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " <config_path> <port>" << std::endl;
            return 1;
        }

        const std::string config_path = argv[1];
        const int port = std::stoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port: " << port << std::endl;
            return 1;
        }

        configure_server_signals();

        Server server(port, config_path);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}