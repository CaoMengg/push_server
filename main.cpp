#include "main.h"

int main()
{
    initGLog( "push_server" );
    PushServer::getInstance()->start();
}
