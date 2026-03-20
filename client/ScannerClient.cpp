#include "ScannerClient.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

ScannerClient::ScannerClient(int port) : port_(port) {
}

std::string ScannerClient::scan_file(const std::string& file_path) const {
	const std::string payload = read_file_contents(file_path);

	const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		throw std::runtime_error("Failed to create socket");
	}

	sockaddr_in server_address{};
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(static_cast<uint16_t>(port_));
	if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) != 1) {
		close(socket_fd);
		throw std::runtime_error("Failed to parse server address");
	}

	if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
		const std::string reason = std::strerror(errno);
		close(socket_fd);
		throw std::runtime_error("Failed to connect to server: " + reason);
	}

	if (!send_all(socket_fd, payload.data(), payload.size())) {
		const std::string reason = std::strerror(errno);
		close(socket_fd);
		throw std::runtime_error("Failed to send file data: " + reason);
	}

	shutdown(socket_fd, SHUT_WR);

	std::string response;
	char buffer[1024];
	while (true) {
		const ssize_t bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			const std::string reason = std::strerror(errno);
			close(socket_fd);
			throw std::runtime_error("Failed to read server response: " + reason);
		}
		if (bytes_read == 0) {
			break;
		}
		response.append(buffer, static_cast<size_t>(bytes_read));
	}

	close(socket_fd);
	return response;
}

bool ScannerClient::send_all(int socket_fd, const char* data, size_t total_bytes) const {
	size_t sent_total = 0;
	while (sent_total < total_bytes) {
		const ssize_t sent = send(socket_fd, data + sent_total, total_bytes - sent_total, 0);
		if (sent < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		if (sent == 0) {
			return false;
		}
		sent_total += static_cast<size_t>(sent);
	}

	return true;
}

std::string ScannerClient::read_file_contents(const std::string& file_path) const {
	std::ifstream input(file_path, std::ios::binary);
	if (!input.is_open()) {
		throw std::runtime_error("Failed to open file: " + file_path);
	}

	return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}
