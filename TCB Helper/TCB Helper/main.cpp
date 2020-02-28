#include "pch.h"
#include "Service.h"

#include <iostream>

int _tmain( int argc, char* argv[] )
{
	auto serviceObj = new Service();
	serviceObj->RunService(argc, argv);
	
	return 0;
}
 