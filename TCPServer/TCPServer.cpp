#include "Server.h"

int main()
{
    Server server = Server();
    
    if (server.Start()) {

        server.Stop();
    }

    return 0;
}
