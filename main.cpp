#include "main.h"

int main()
{
    initGLog( "push_server" );

    CURLcode res;
    res = curl_global_init( CURL_GLOBAL_ALL );
    if( res != CURLE_OK )
    {
        LOG(ERROR) << "curl_global_init fail";
        return 1;
    }
    LOG(INFO) << "curl_global_init succ, version: " << curl_version();

    PushServer::getInstance()->start();
    return 0;
}
