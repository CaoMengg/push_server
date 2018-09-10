#include "main.h"

int main()
{
    initGLog("push_server");

    CURLcode res;
    res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK)
    {
        LOG(ERROR) << "curl_global_init fail";
        return 1;
    }
    LOG(INFO) << "libcurl version " << curl_version();
    LOG(INFO) << "libuv version " << uv_version_string();
    LOG(INFO) << "server version 1.0.1";

    PushServer::getInstance()->start();
    return 0;
}
