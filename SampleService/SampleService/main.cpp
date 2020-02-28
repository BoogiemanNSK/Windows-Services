#include "pch.h"
#include "ServiceWrapper.h"
#include "PipeClient.h"
#include "PipeServer.h"
#include "PipeParent.h"

#include <iostream>

int _tmain(int argc, char* argv[]) {
	printf("Path to app: %s", argv[0]);
		
	if (argc < 2) {
		std::cout << "Specify either --server, --client, --parent or --service key.\n";
		return -1;
	}

	if (strncmp(argv[1], "--server", 10) == 0) {

		auto serverObject= new PipeServer();
		if (serverObject->MyInit()) {
			serverObject->MyDone();
		}
		
	} else if (strncmp(argv[1], "--client", 10) == 0) {
		
		auto clientObject = new PipeClient();
		clientObject->RunClient();
		
	} else if (strncmp(argv[1], "--service", 10) == 0) {

		auto serviceObj = new ServiceWrapper();
		serviceObj->RunService(argc, argv);
		
	} else if (strncmp(argv[1], "--parent", 10) == 0) {

		auto parentObj = new PipeParent();
		parentObj->RunParent(--argc, ++argv);
		
	} else {
		
		std::cout << "Specify either --PipeServer, --PipeClient, --PipeParent or --ServiceWrapper key.\n";
		return -1;
		
	}
	
	return 0;
}
