#pragma once

#include <string>

class ScannerClient {
public:
	explicit ScannerClient(int port);

	std::string scan_file(const std::string& file_path) const;

private:
	bool send_all(int socket_fd, const char* data, size_t total_bytes) const;
	std::string read_file_contents(const std::string& file_path) const;

	int port_;
};
