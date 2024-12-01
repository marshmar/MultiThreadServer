#include "Client.h"


INT _tmain(INT argc, TCHAR* argv[])
{
	Client client;
	client.Initialize();
	client.Communicate();

	return 0;
}