#include "ScannerClient.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
	try {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " <file_path> <port>" << std::endl;
			return 1;
		}

		const std::string file_path = argv[1];
		const int port = std::stoi(argv[2]);
		if (port <= 0 || port > 65535) {
			std::cerr << "Invalid port: " << port << std::endl;
			return 1;
		}

		const ScannerClient client(port);
		const std::string verdict = client.scan_file(file_path);

		std::cout << verdict;
	} catch (const std::exception& error) {
		std::cerr << "Error: " << error.what() << std::endl;
		return 1;
	}

	return 0;
}
