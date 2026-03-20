#include "StatsClient.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
	const std::string fifo_path = argc >= 2 ? argv[1] : "/tmp/echo_server_fifo";

	StatsClient client(fifo_path);
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