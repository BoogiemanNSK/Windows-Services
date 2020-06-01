#include "pch.h"
#include "CClient.h"
#include "CService.h"

int _tmain(int argc, char* argv[])
{
	if (argc > 1) 
	{
		auto clientObj = new CClient();
		if (!clientObj->MyInit()) 
		{
			printf("Unable to initialize client!\n");
			return 1;
		}
		
		clientObj->ConnectToRemoteCmd();
		return 0;
	}
	
	auto serviceObj = new CService();
	serviceObj->RunService(argc, argv);
	return 0;
}
