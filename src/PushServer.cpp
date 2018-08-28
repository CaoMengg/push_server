#include "PushServer.h"

PushServer* PushServer::pInstance = NULL;


// 单例模式
PushServer* PushServer::getInstance()
{
    if( pInstance == NULL )
    {
        pInstance = new PushServer();
    }
    return pInstance;
}


// 加载服务配置
int PushServer::_LoadConf()
{
    mainConf = new YamlConf( "conf/push_server.yaml" );
    intListenPort = mainConf->getInt( "listen" );
    if( intListenPort <= 0 )
    {
        LOG(ERROR) << "load conf fail, listen port invalid";
        return 1;
    }

    if( ! mainConf->isSet( "apps" ) )
    {
        LOG(ERROR) << "load conf fail, apps not found";
        return 1;
    }

    YamlConf *appConf = NULL;
    for( YAML::const_iterator it=mainConf->config["apps"].begin(); it!=mainConf->config["apps"].end(); ++it)
    {
        std::string confFile = "conf/" + it->second.as<std::string>();
        appConf = new YamlConf( confFile );
        mapConf[ it->first.as<std::string>() ] = appConf;
        DLOG(INFO) << "DEBUG: load app conf:" << confFile;
    }

    return 0;
}


// 获取链接配置
int PushServer::getConnectionConf( SocketConnection *pConnection )
{
    confIterator it = mapConf.find( pConnection->strAppName );
    if( it == mapConf.end() )
    {
        std::string strInfo( "app conf not found" );
        pConnection->logWarning( strInfo );
        return 1;
    }

    if( ! mapConf[ pConnection->strAppName ]->isSet( pConnection->strPushType ) )
    {
        std::string strInfo( "push type not found" );
        pConnection->logWarning( strInfo );
        return 1;
    }

    pConnection->conf = mapConf[ pConnection->strAppName ];
    return 0;
}


int PushServer::getUpstreamWatcher( SocketConnection *pConnection )
{
    if( pConnection->upstreamFd <= 0 ) {
        return 1;
    }

    upstreamWatcherIterator it = mapUpstreamWatcher.find( pConnection->upstreamFd );
    if( it == mapUpstreamWatcher.end() )
    {
        uv_poll_t *pUvPoll = new uv_poll_t();
        int intRet = uv_poll_init( pConnection->pLoop, pUvPoll, pConnection->upstreamFd );
        if( intRet < 0 )
        {
            std::string strInfo( "uv_poll_init error:" );
            strInfo += uv_strerror( intRet );
            pConnection->logWarning( strInfo );
            return 2;
        }

        mapUpstreamWatcher[ pConnection->upstreamFd ] = pUvPoll;
        pConnection->upstreamWatcher = pUvPoll;
    } else {
        pConnection->upstreamWatcher = it->second;
    }

    pConnection->upstreamWatcher->data = pConnection;
    return 0;
}


void clientTimeoutCB( uv_timer_t *timer )
{
    SocketConnection* pConnection = (SocketConnection *)(timer->data);
    std::string strInfo( "client timeout" );
    pConnection->logWarning( strInfo );

    if( pConnection->clientWatcher != NULL )
    {
        uv_close( (uv_handle_t *)(pConnection->clientWatcher), uvCloseCB );
        pConnection->clientWatcher = NULL;
    }
    if( pConnection->clientTimer != NULL )
    {
        uv_close( (uv_handle_t *)(pConnection->clientTimer), uvCloseCB );
        pConnection->clientTimer = NULL;
    }

    pConnection->tryDestroy();
}


static size_t curlWriteCB( void *ptr, size_t size, size_t nmemb, void *data )
{
    size_t realsize = size * nmemb;
    if( realsize <= 0 )
    {
        return realsize;
    }

    SocketConnection * pConnection = (SocketConnection *)data;
    while( int(pConnection->upstreamBuf->intLen+realsize) >= pConnection->upstreamBuf->intSize ) {
        pConnection->upstreamBuf->enlarge();
    }
    memcpy( pConnection->upstreamBuf->data + pConnection->upstreamBuf->intLen, ptr, realsize );
    pConnection->upstreamBuf->intLen += realsize;
    return realsize;
}


int PushServer::getConnectionHandle( SocketConnection *pConnection )
{
    YAML::Node conf;
    if( pConnection->strPushType == "getui_auth" )
    {
        conf = pConnection->conf->config[ "getui" ];
    }
    else
    {
        conf = pConnection->conf->config[ pConnection->strPushType ];
    }

    pConnection->upstreamHandle = curl_easy_init();
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POST, 1L);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_WRITEFUNCTION, curlWriteCB);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_WRITEDATA, pConnection);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_PRIVATE, pConnection);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_TIMEOUT_MS, conf["timeout"].as<long>());
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSL_VERIFYHOST, 0L);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_PIPEWAIT, 1L);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_DEBUGFUNCTION, my_trace);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_DEBUGDATA, &debug_data);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_NOPROGRESS, 0L);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_LOW_SPEED_TIME, 3L);
    //curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_LOW_SPEED_LIMIT, 10L);

    if( pConnection->strPushType == "apns" )
    {
        std::string apiUrl = conf["api"].as<std::string>() + (*( pConnection->reqData ))["token"].GetString();
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POSTFIELDS, (*( pConnection->reqData ))["payload"].GetString());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSLCERTTYPE, "PEM");
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSLCERT, conf["cert_file"].as<std::string>().c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSLCERTPASSWD, conf["cert_password"].as<std::string>().c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSLKEYTYPE, "PEM");
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSLKEY, conf["key_file"].as<std::string>().c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_SSLKEYPASSWD, conf["cert_password"].as<std::string>().c_str());

        std::string topicHeader = "apns-topic: ";
        topicHeader += conf["topic"].as<std::string>();

        struct curl_slist *curlHeader = NULL;
        curlHeader = curl_slist_append( curlHeader, topicHeader.c_str() );
        curlHeader = curl_slist_append( curlHeader, "apns-priority: 10" );
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTPHEADER, curlHeader);
    } else if( pConnection->strPushType == "xiaomi" ) {
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_URL, conf["api"].as<std::string>().c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POSTFIELDS, (*( pConnection->reqData ))["payload"].GetString());

        std::string authHeader = "Authorization: key=";
        authHeader += conf["key"].as<std::string>();

        struct curl_slist *curlHeader = NULL;
        curlHeader = curl_slist_append( curlHeader, authHeader.c_str() );
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTPHEADER, curlHeader);
    } else if( pConnection->strPushType == "getui_auth" ) {
        rapidjson::Document jsonDom;
        jsonDom.SetObject();

        std::string strAppKey = conf["app_key"].as<std::string>();
        rapidjson::Value appkey;
        appkey = rapidjson::StringRef( strAppKey.c_str() );
        jsonDom.AddMember( "appkey", appkey, jsonDom.GetAllocator() );

        std::string strTimestamp;
        struct timeval tv;
        gettimeofday( &tv, NULL );
        strTimestamp = std::to_string( tv.tv_sec * 1000 );
        rapidjson::Value timestamp;
        timestamp = rapidjson::StringRef( strTimestamp.c_str() );
        jsonDom.AddMember( "timestamp", timestamp, jsonDom.GetAllocator() );

        std::string strSignStr = strAppKey + strTimestamp + conf["master_secret"].as<std::string>();
        char buf[2];
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init( &sha256 );
        SHA256_Update( &sha256, strSignStr.c_str(), strSignStr.size() );
        SHA256_Final( hash, &sha256 );
        std::string strSign;
        for( int i=0; i<SHA256_DIGEST_LENGTH; i++)
        {
            sprintf( buf, "%02x", hash[i] );
            strSign += buf;
        }
        rapidjson::Value sign;
        sign = rapidjson::StringRef( strSign.c_str() );
        jsonDom.AddMember( "sign", sign, jsonDom.GetAllocator() );

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer( buffer );
        jsonDom.Accept( writer );

        std::string typeHeader = "Content-Type: application/json";
        struct curl_slist *curlHeader = NULL;
        curlHeader = curl_slist_append( curlHeader, typeHeader.c_str() );

        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_URL, conf["api_auth"].as<std::string>().c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTPHEADER, curlHeader);
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_COPYPOSTFIELDS, buffer.GetString());
    } else if( pConnection->strPushType == "getui" ) {
        if( ! conf["auth_token"] ) {
            std::string strInfo( "no getui auth token, wait for refresh" );
            pConnection->logWarning( strInfo );
            return 1;
        }

        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_URL, conf["api"].as<std::string>().c_str());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POSTFIELDS, (*( pConnection->reqData ))["payload"].GetString());

        std::string typeHeader = "Content-Type: application/json";
        std::string authHeader = "authtoken:";
        authHeader += conf["auth_token"].as<std::string>();

        struct curl_slist *curlHeader = NULL;
        curlHeader = curl_slist_append( curlHeader, typeHeader.c_str() );
        curlHeader = curl_slist_append( curlHeader, authHeader.c_str() );
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTPHEADER, curlHeader);
    } else if( pConnection->strPushType == "bbserver" ) {
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_URL, (*( pConnection->reqData ))["token"].GetString());

        std::string strDecode;
        Base64::Decode((*( pConnection->reqData ))["payload"].GetString(), &strDecode);

        // 解析msgpack
        /* msgpack::object_handle ohMsgpack = msgpack::unpack((const char*)(strDecode.data()), strDecode.size());
        msgpack::object objMsgpack = ohMsgpack.get();
        std::ostringstream osMsgpack;
        osMsgpack << objMsgpack;
        std::string strMsgpack = osMsgpack.str();
        DLOG(INFO) << "recv msgpack: " << strMsgpack; */

        std::string typeHeader = "Content-Type: text/xml";
        std::string lenHeader = "Content-Length: " + strDecode.size();
        struct curl_slist *curlHeader = NULL;
        curlHeader = curl_slist_append( curlHeader, typeHeader.c_str() );
        curlHeader = curl_slist_append( curlHeader, lenHeader.c_str() );
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTPHEADER, curlHeader);

        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POSTFIELDS, strDecode.data());
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POSTFIELDSIZE, strDecode.size());
    } else {
        std::string strInfo( "unsurpported push type" );
        pConnection->logWarning( strInfo );
        return 1;
    }
    DLOG(INFO) << "DEBUG: getConnectionHandle app_name:" << pConnection->strAppName << " push_type:" << pConnection->strPushType;
    return 0;
}


void writeCallback( uv_write_t *req, int status ) {
    DLOG(INFO) << "DEBUG: write to client status:" << status;
    if( status < 0 ) {
        SocketConnection* pConnection = (SocketConnection *)(req->data);
        std::string strInfo( "write fail, error:" );
        strInfo += uv_strerror( status );
        pConnection->logWarning( strInfo );
    }
}


void shutdownCallback( uv_shutdown_t* req, int status ) {
    DLOG(INFO) << "DEBUG: shutdown client connect status:" << status;
    SocketConnection* pConnection = (SocketConnection *)(req->data);

    /* if( status < 0 ) {
        std::string strInfo( "shutdown fail, error:" );
        strInfo += uv_strerror( status );
        pConnection->logWarning( strInfo );
    } */

    if( pConnection->clientWatcher != NULL )
    {
        uv_close( (uv_handle_t *)(pConnection->clientWatcher), uvCloseCB );
        pConnection->clientWatcher = NULL;
    }
    if( pConnection->clientTimer != NULL )
    {
        uv_timer_stop( pConnection->clientTimer );
        uv_close( (uv_handle_t *)(pConnection->clientTimer), uvCloseCB );
        pConnection->clientTimer = NULL;
    }

    pConnection->tryDestroy();
}


// 解析客户端query
void PushServer::parseQuery( SocketConnection *pConnection )
{
    DLOG(INFO) << "DEBUG: recv query from client: " << pConnection->inBuf->data;

    pConnection->reqData->Parse( (const char*)(pConnection->inBuf->data) );
    if( ! pConnection->reqData->IsObject() )
    {
        std::string strInfo( "request data is not json" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
        return;
    }

    if( ! pConnection->reqData->HasMember("app_name") || ! (*(pConnection->reqData))["app_name"].IsString() ||
            ! pConnection->reqData->HasMember("push_type") || ! (*(pConnection->reqData))["push_type"].IsString() )
    {
        std::string strInfo( "request data is invalid" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
        return;
    }
    pConnection->strAppName = (*(pConnection->reqData))["app_name"].GetString();
    pConnection->strPushType = (*(pConnection->reqData))["push_type"].GetString();

    if( getConnectionConf( pConnection ) )
    {
        std::string strInfo( "get connection conf fail" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
        return;
    }

    if( getConnectionHandle( pConnection ) )
    {
        std::string strInfo( "get connection handle fail" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
        return;
    }

    if( curl_multi_add_handle( multi, pConnection->upstreamHandle ) != CURLM_OK )
    {
        std::string strInfo( "add connection handle fail" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
        return;
    }
    ++pConnection->refcount;

    // return to client
    pConnection->uvOutBuf = uv_buf_init( (char*)(pConnection->strReqSucc.c_str()), pConnection->strReqSucc.length() );
    uv_write( pConnection->writeReq, (uv_stream_t*)(pConnection->clientWatcher), &(pConnection->uvOutBuf), 1, writeCallback );
    uv_timer_start( pConnection->clientTimer, clientTimeoutCB, pConnection->writeTimeout, 0 );
    uv_shutdown( pConnection->shutdownReq, (uv_stream_t*)(pConnection->clientWatcher), shutdownCallback );
}


void PushServer::readCB( SocketConnection* pConnection, ssize_t nread )
{
    pConnection->inBuf->intLen += nread;

    if( pConnection->inBuf->data[pConnection->inBuf->intLen-1] == '\0' )
    {
        uv_read_stop( (uv_stream_t*)(pConnection->clientWatcher) );
        uv_timer_stop( pConnection->clientTimer );
        parseQuery( pConnection );
    }
}


void readCallBack( uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf )
{
    (void)buf;
    SocketConnection* pConnection = (SocketConnection *)(stream->data);
    pConnection->status = csConnected;

    if( nread < 0 ) {
        if( nread == UV_EOF ) {
            std::string strInfo( "client close connection" );
            pConnection->logWarning( strInfo );
        }
        pConnection->tryDestroy();
    } else if ( nread > 0 ) {
        PushServer::getInstance()->readCB( pConnection, nread );
    }
}


// 解析上游response
void PushServer::parseResponse( SocketConnection *pConnection )
{
    if( pConnection->strPushType == "apns" )
    {
        long httpCode = 0;
        curl_easy_getinfo(pConnection->upstreamHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        if( httpCode!=200 && httpCode!=410 )
        {
            std::string strInfo( "apns return fail http_code:" );
            strInfo += httpCode + " ";
            strInfo += (char*)(pConnection->upstreamBuf->data);
            strInfo += '\0';
            pConnection->logWarning( strInfo );
        } else {
            pConnection->isSucc = true;
        }
    } else if( pConnection->strPushType == "xiaomi" ) {
        pConnection->upstreamBuf->data[ pConnection->upstreamBuf->intLen ] = '\0';
        DLOG(INFO) << "DEBUG: recv response from " << pConnection->strPushType << ":" << pConnection->upstreamBuf->data;
        pConnection->resData->Parse( (const char*)(pConnection->upstreamBuf->data) );
        if( ! pConnection->resData->IsObject() )
        {
            std::string strInfo( "xiaomi result is not json" );
            pConnection->logWarning( strInfo );
        } else if( ! pConnection->resData->HasMember("result") || (*(pConnection->resData))["result"]!="ok" ) {
            std::string strInfo( "xiaomi return fail: " );
            strInfo += (char*)(pConnection->upstreamBuf->data);
            pConnection->logWarning( strInfo );
        } else {
            pConnection->isSucc = true;
        }
    } else if( pConnection->strPushType == "getui" ) {
        pConnection->upstreamBuf->data[ pConnection->upstreamBuf->intLen ] = '\0';
        DLOG(INFO) << "DEBUG: recv response from " << pConnection->strPushType << ":" << pConnection->upstreamBuf->data;
        pConnection->resData->Parse( (const char*)(pConnection->upstreamBuf->data) );
        if( ! pConnection->resData->IsObject() )
        {
            std::string strInfo( "getui result is not json" );
            pConnection->logWarning( strInfo );
        } else if( !pConnection->resData->HasMember("result") || (*(pConnection->resData))["result"]!="ok" ) {
            std::string strInfo( "getui return fail: " );
            strInfo += (char*)(pConnection->upstreamBuf->data);
            pConnection->logWarning( strInfo );
        } else {
            pConnection->isSucc = true;
        }
    } else if( pConnection->strPushType == "getui_auth" ) {
        pConnection->upstreamBuf->data[ pConnection->upstreamBuf->intLen ] = '\0';
        DLOG(INFO) << "DEBUG: recv response from " << pConnection->strPushType << ":" << pConnection->upstreamBuf->data;
        pConnection->resData->Parse( (const char*)(pConnection->upstreamBuf->data) );
        if( ! pConnection->resData->IsObject() )
        {
            std::string strInfo( "getui auth result is not json" );
            pConnection->logWarning( strInfo );
        } else if( !pConnection->resData->HasMember("result") || (*(pConnection->resData))["result"]!="ok" || !pConnection->resData->HasMember("auth_token") ) {
            std::string strInfo( "getui auth return fail: " );
            strInfo += (char*)(pConnection->upstreamBuf->data);
            pConnection->logWarning( strInfo );
        } else {
            pConnection->conf->config["getui"]["auth_token"] = (*(pConnection->resData))["auth_token"].GetString();
            pConnection->isSucc = true;
        }
    } else if( pConnection->strPushType == "bbserver" ) {
        msgpack::object_handle ohMsgpack = msgpack::unpack((const char*)(pConnection->upstreamBuf->data), pConnection->upstreamBuf->intLen);
        msgpack::object objMsgpack = ohMsgpack.get();
        std::ostringstream osMsgpack;
        osMsgpack << objMsgpack;
        std::string strMsgpack = osMsgpack.str();
        DLOG(INFO) << "DEBUG: recv response from " << pConnection->strPushType << ":" << strMsgpack;
        pConnection->resData->Parse( (const char*)(strMsgpack.c_str()) );
        if( ! pConnection->resData->IsObject() )
        {
            std::string strInfo( "bbserver result is not json" );
            pConnection->logWarning( strInfo );
        } else if( !pConnection->resData->HasMember("error") || (*(pConnection->resData))["error"]["errno"]!=200 ) {
            std::string strInfo( "bbserver return fail: " );
            strInfo += strMsgpack;
            pConnection->logWarning( strInfo );
        } else {
            pConnection->isSucc = true;
        }
    }

    pConnection->tryDestroy();
    return;
}


// 处理libcurl msg
static void checkMultiInfo()
{
    CURLMsg *msg;
    int msgs_left;
    CURL *easy;

    while( (msg = curl_multi_info_read(PushServer::getInstance()->multi, &msgs_left)) ) {
        if( msg->msg == CURLMSG_DONE ) {
            easy = msg->easy_handle;

            SocketConnection *pConnection;
            curl_easy_getinfo( easy, CURLINFO_PRIVATE, &pConnection );

            curl_multi_remove_handle( PushServer::getInstance()->multi, easy );
            if( pConnection->upstreamFd > 0 ) {
                DLOG(INFO) << "DEBUG: uv_poll_stop in checkMultiInfo fd=" << pConnection->upstreamFd;
                uv_poll_stop( pConnection->upstreamWatcher );
                pConnection->upstreamFd = 0;
            }

            if( msg->data.result == 0 ) {
                PushServer* pPushServer = PushServer::getInstance();
                pPushServer->parseResponse( pConnection );
            } else {
                pConnection->tryDestroy();
            }
        }
    }
}


void curlSocketCB( uv_poll_t *watcher, int status, int revents )
{
    SocketConnection *pConnection = (SocketConnection *)(watcher->data);
    if( status < 0 ) {
        std::string strInfo( "curlSocketCB fail, error:" );
        strInfo += uv_strerror( status );
        pConnection->logWarning( strInfo );
        return;
    }

    int action = (revents&UV_READABLE ? CURL_POLL_IN : 0) | (revents&UV_WRITABLE ? CURL_POLL_OUT : 0);
    PushServer* pPushServer = PushServer::getInstance();
    curl_multi_socket_action( pPushServer->multi, pConnection->upstreamFd, action, &(pPushServer->intCurlRunning));
    checkMultiInfo();
}


static int curlSocketCallback( CURL *e, curl_socket_t s, int action, void *cbp, void *sockp )
{
    (void)cbp;
    (void)sockp;
    SocketConnection *pConnection;
    curl_easy_getinfo( e, CURLINFO_PRIVATE, &pConnection );

    if( action == CURL_POLL_REMOVE )
    {
        //if( uv_is_active( (const uv_handle_t*)(pConnection->upstreamWatcher) ) ) {
        if( pConnection->upstreamFd > 0 ) {
            DLOG(INFO) << "DEBUG: uv_poll_stop in curlSocketCallback fd=" << s;
            uv_poll_stop( pConnection->upstreamWatcher );
            pConnection->upstreamFd = 0;
        }
        return 0;
    }

    if( pConnection->upstreamFd <= 0 )
    {
        /*int intRet = uv_poll_init( pConnection->pLoop, pConnection->upstreamWatcher, s );
          if( intRet < 0 )
          {
          std::string strInfo( "uv_poll_init, error:" );
          strInfo += uv_strerror( intRet );
          pConnection->logWarning( strInfo );
          return 0;
          }
          DLOG(INFO) << "DEBUG: uv_poll_init fd=" << s;
          pConnection->upstreamFd = s;*/

        PushServer* pPushServer = PushServer::getInstance();
        pConnection->upstreamFd = s;
        int intRet = pPushServer->getUpstreamWatcher( pConnection );
        if( intRet != 0 )
        {
            return 0;
        }
        DLOG(INFO) << "DEBUG: getUpstreamWatcher fd=" << s;
    }

    int kind = (action&CURL_POLL_IN ? UV_READABLE : 0) | (action&CURL_POLL_OUT ? UV_WRITABLE : 0);
    uv_poll_start( pConnection->upstreamWatcher, kind, curlSocketCB );
    return 0;
}


void curlTimeoutCB( uv_timer_t *timer )
{
    (void)timer;
    PushServer* pPushServer = PushServer::getInstance();
    curl_multi_socket_action( pPushServer->multi, CURL_SOCKET_TIMEOUT, 0, &(pPushServer->intCurlRunning) );
    checkMultiInfo();
}


static int curlTimerCallback( CURLM *multi, long timeout_ms, void *g )
{
    (void)multi;
    (void)g;
    PushServer* pPushServer = PushServer::getInstance();

    if( timeout_ms < 0 ) {
        uv_timer_stop( pPushServer->curlMultiTimer );
        return 0;
    }

    if( timeout_ms == 0 ) {
        timeout_ms = 1;
    }
    uv_timer_start( pPushServer->curlMultiTimer, curlTimeoutCB, timeout_ms, 0 );
    return 0;
}


void uvAllocBuffer( uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf )
{
    (void)suggested_size;
    SocketConnection* pConnection = (SocketConnection *)(handle->data);
    if( pConnection->inBuf->intLen >= pConnection->inBuf->intSize )
    {
        pConnection->inBuf->enlarge();
    }
    *buf = uv_buf_init( (char *)(pConnection->inBuf->data) + pConnection->inBuf->intLen, pConnection->inBuf->intSize - pConnection->inBuf->intLen );
}


void PushServer::acceptCB()
{
    SocketConnection* pConnection = new SocketConnection( uvLoop, multi );

    if( uv_accept( (uv_stream_t*)uvServer, (uv_stream_t*)(pConnection->clientWatcher) ) == 0 ) {
        pConnection->status = csAccepted;
        uv_read_start( (uv_stream_t*)(pConnection->clientWatcher), uvAllocBuffer, readCallBack );
        uv_timer_start( pConnection->clientTimer, clientTimeoutCB, pConnection->readTimeout, 0 );
    } else {
        pConnection->tryDestroy();
    }
}


void acceptCallback( uv_stream_t *server, int status )
{
    (void)server;
    if( status < 0 ) {
        LOG(WARNING) << "accept fail, error:" << uv_strerror( status );
        return;
    }
    PushServer::getInstance()->acceptCB();
}


void PushServer::refreshGetuiToken( std::string appName, YamlConf* appConf )
{
    SocketConnection* pConnection = new SocketConnection( uvLoop, multi );
    pConnection->strAppName = appName;
    pConnection->strPushType = "getui_auth";
    pConnection->conf = appConf;

    if( getConnectionHandle( pConnection ) )
    {
        std::string strInfo( "get connection handle fail" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
        return;
    }

    if( curl_multi_add_handle( multi, pConnection->upstreamHandle ) != CURLM_OK )
    {
        std::string strInfo( "add connection handle fail" );
        pConnection->logWarning( strInfo );
        pConnection->tryDestroy();
    }
    return;
}


void PushServer::maintainCB()
{
    LOG(INFO) << "maintain in progress";

    YamlConf *appConf = NULL;
    for( confIterator it=mapConf.begin(); it!=mapConf.end(); ++it )
    {
        appConf = it->second;
        if( appConf->isSet( "getui" ) )
        {
            refreshGetuiToken( it->first, appConf );
        }
    }
}


void maintainCallback( uv_timer_t *timer )
{
    (void)timer;
    PushServer::getInstance()->maintainCB();
}


void PushServer::start()
{
    int intRet;
    intRet = _LoadConf();
    if( intRet != 0 )
    {
        LOG(ERROR) << "load conf fail";
        return;
    }

    multi = curl_multi_init();
    if( multi == NULL ) {
        LOG(ERROR) << "curl_multi_init fail";
        return;
    }
    // pipelining & multiplexing, re-use connection
    curl_multi_setopt(multi, CURLMOPT_PIPELINING, CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX);
    curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, curlTimerCallback);
    curl_multi_setopt(multi, CURLMOPT_TIMERDATA, NULL);
    curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, curlSocketCallback);
    curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, NULL);

    struct sockaddr_in addr;
    uv_tcp_init( uvLoop, uvServer );
    uv_ip4_addr( "0.0.0.0", intListenPort, &addr );

    intRet = uv_tcp_bind( uvServer, (const struct sockaddr*)&addr, 0 );
    if( intRet != 0 ) {
        LOG(ERROR) << "bind fail";
        return;
    }

    intRet = uv_listen( (uv_stream_t*)uvServer, 255, acceptCallback );
    if( intRet != 0 ) {
        LOG(ERROR) << "listen fail";
        return;
    }

    uv_timer_start( maintainTimer, maintainCallback, 0, 10000 );

    LOG(INFO) << "server start, listen port " << intListenPort;
    uv_run( uvLoop, UV_RUN_DEFAULT );

    curl_multi_cleanup( multi );
    curl_global_cleanup();
}
