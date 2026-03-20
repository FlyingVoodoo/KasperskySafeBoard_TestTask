#include "StatsClient.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
	std::string request_fifo_path = "/tmp/server_req_fifo";
	std::string response_fifo_path = "/tmp/server_resp_fifo";

	if (argc == 3) {
		request_fifo_path = argv[1];
		response_fifo_path = argv[2];
	} else if (argc != 1) {
		std::cerr << "Usage: " << argv[0] << " [<request_fifo_path> <response_fifo_path>]" << std::endl;
		return 1;
	}

	StatsClient client(request_fifo_path, response_fifo_path);
	const std::vector<StatEntry> stats = client.fetch_stats();

	if (stats.empty()) {
		std::cerr << "No stats received" << std::endl;
		return 1;
	}

	for (const StatEntry& entry : stats) {
		std::cout << entry.name << ": " << entry.value << std::endl;
	}

	return 0;
}