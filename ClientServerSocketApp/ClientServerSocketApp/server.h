#pragma once

#pragma comment (lib, "Ws2_32.lib")

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class server {
public:
	int run_server();
};
