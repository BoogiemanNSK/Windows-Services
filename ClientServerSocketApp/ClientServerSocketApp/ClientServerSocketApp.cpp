#include "pch.h"
#include "server.h"
#include "client.h"

#include <iostream>

int main(const int argc, char* argv[]) {
    if (argc < 2) {
		std::cout << "Specify either --server, --client or --service key.\n";
		return -1;
	}

	if (strncmp(argv[1], "--server", 10) == 0) {
		auto s_object = new server();
		s_object->run_server();
	} else if (strncmp(argv[1], "--client", 10) == 0) {
		auto c_object = new client();
		c_object->run_client(argc, argv);
	} else {
		std::cout << "Specify either --server, --client or --service key.\n";
		return -1;
	}
	
	return 0;
}
