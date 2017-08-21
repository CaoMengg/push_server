#include "PushServer.h"

PushServer* PushServer::pInstance = NULL;

PushServer* PushServer::getInstance()
{
    if( pInstance == NULL )
    {
        pInstance = new PushServer();
    }
    return pInstance;
}

int PushServer::_LoadConf()
{
    mainConf = new YamlConf( "conf/push_server.yaml" );
    intListenPort = mainConf->getInt( "listen" );
    if( intListenPort <= 0 )
    {
        LOG(WARNING) << "listen port invalid";
        return 1;
    }

    if( ! mainConf->isSet( "apps" ) )
    {
        LOG(WARNING) << "apps not found";
        return 1;
    }
    YamlConf *appConf = NULL;
    for( YAML::const_iterator it=mainConf->config["apps"].begin(); it!=mainConf->config["apps"].end(); ++it)
    {
        std::string confFile = "conf/" + it->second.as<std::string>();
        appConf = new YamlConf( confFile );
        mapConf[ it->first.as<std::string>() ] = appConf;
    }

    return 0;
}

int PushServer::getConnectionConf( SocketConnection *pConnection )
{
    confIterator it = mapConf.find( pConnection->strAppName );
    if( it == mapConf.end() )
    {
        LOG(WARNING) << "app conf not found";
        return 1;
    }

    if( ! mapConf[ pConnection->strAppName ]->isSet( pConnection->strPushType ) )
    {
        LOG(WARNING) << "push type not found";
        return 1;
    }
    pConnection->conf = mapConf[ pConnection->strAppName ];
    return 0;
}

void clientTimeoutCB( uv_timer_t *timer )
{
    SocketConnection* pConnection = (SocketConnection *)(timer->data);
    delete pConnection;
    LOG(WARNING) << "client timeout";
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
    YAML::Node conf = pConnection->conf->config[ pConnection->strPushType ];

    pConnection->upstreamHandle = curl_easy_init();
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POST, 1L);
    curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_POSTFIELDS, (*( pConnection->reqData ))["payload"].GetString());
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

        std::string authHeader = "Authorization: key=";
        authHeader += conf["key"].as<std::string>();

        struct curl_slist *curlHeader = NULL;
        curlHeader = curl_slist_append( curlHeader, authHeader.c_str() );
        curl_easy_setopt(pConnection->upstreamHandle, CURLOPT_HTTPHEADER, curlHeader);
    } else {
        LOG(WARNING) << "unsurpported push type";
        return 1;
    }
    return 0;
}

void PushServer::parseQuery( SocketConnection *pConnection )
{
    LOG(INFO) << "recv query: " << pConnection->inBuf->data;

    pConnection->reqData->Parse( (const char*)(pConnection->inBuf->data) );
    if( ! pConnection->reqData->IsObject() )
    {
        LOG(WARNING) << "request data is not json";
        delete pConnection;
        return;
    }

    if( ! pConnection->reqData->HasMember("app_name") || ! (*(pConnection->reqData))["app_name"].IsString() ||
            ! pConnection->reqData->HasMember("push_type") || ! (*(pConnection->reqData))["push_type"].IsString() )
    {
        LOG(WARNING) << "request data is invalid";
        delete pConnection;
        return;
    }
    pConnection->strAppName = (*(pConnection->reqData))["app_name"].GetString();
    pConnection->strPushType = (*(pConnection->reqData))["push_type"].GetString();

    if( getConnectionConf( pConnection ) )
    {
        LOG(WARNING) << "get connection conf fail";
        delete pConnection;
        return;
    }

    if( getConnectionHandle( pConnection ) )
    {
        LOG(WARNING) << "get connection handle fail";
        delete pConnection;
        return;
    }

    if( curl_multi_add_handle( multi, pConnection->upstreamHandle ) != CURLM_OK )
    {
        LOG(WARNING) << "add connection handle fail";
        curl_easy_cleanup( pConnection->upstreamHandle );
        delete pConnection;
        return;
    }
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
            // client close normally
            delete pConnection;
        }
    } else if ( nread > 0 ) {
        PushServer::getInstance()->readCB( pConnection, nread );
    }
}

void writeCallback( uv_write_t *req, int status ) {
    SocketConnection* pConnection = (SocketConnection *)(req->data);

    if( status < 0 ) {
        LOG(WARNING) << "write fail, error:" << uv_strerror( status );
        delete pConnection;
    } else {
        uv_timer_stop( pConnection->clientTimer );
    }
}

void PushServer::parseResponse( SocketConnection *pConnection )
{
    pConnection->upstreamBuf->data[ pConnection->upstreamBuf->intLen ] = '\0';

    if( pConnection->strPushType == "apns" )
    {
        long httpStatus = 0;
        curl_easy_getinfo(pConnection->upstreamHandle, CURLINFO_RESPONSE_CODE, &httpStatus);
        if( httpStatus != 200 )
        {
            LOG(WARNING) << "apns return fail: " << pConnection->upstreamBuf->data;
            delete pConnection;
            return;
        }
    } else if( pConnection->strPushType == "xiaomi" ) {
        pConnection->resData->Parse( (const char*)(pConnection->upstreamBuf->data) );
        if( ! pConnection->resData->IsObject() )
        {
            LOG(WARNING) << "xiaomi result is not json";
            delete pConnection;
            return;
        }

        if( ! pConnection->resData->HasMember("result") || (*(pConnection->resData))["result"]!="ok" )
        {
            LOG(WARNING) << "xiaomi return fail: " << (*(pConnection->resData))["info"].GetString();
            delete pConnection;
            return;
        }
    }

    //pConnection->uvOutBuf = uv_buf_init( (char*)(pConnection->upstreamBuf->data), pConnection->upstreamBuf->intLen );
    pConnection->uvOutBuf = uv_buf_init( (char*)(pConnection->strReqSucc.c_str()), pConnection->strReqSucc.length() );
    uv_write( pConnection->writeReq, (uv_stream_t*)(pConnection->clientWatcher), &(pConnection->uvOutBuf), 1, writeCallback );
    uv_timer_start( pConnection->clientTimer, clientTimeoutCB, pConnection->writeTimeout, 0 );
}

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

            if( msg->data.result == 0 ) {
                PushServer* pPushServer = PushServer::getInstance();
                pPushServer->parseResponse( pConnection );
            } else {
                delete pConnection;
            }

            curl_multi_remove_handle( PushServer::getInstance()->multi, easy );
            curl_easy_cleanup( easy );
        }
    }
}

void curlSocketCB( uv_poll_t *watcher, int status, int revents )
{
    if( status < 0 ) {
        LOG(WARNING) << "curlSocketCB fail, error:" << uv_strerror( status );
        return;
    }

    int action = (revents&UV_READABLE ? CURL_POLL_IN : 0) | (revents&UV_WRITABLE ? CURL_POLL_OUT : 0);
    SocketConnection *pConnection = (SocketConnection *)(watcher->data);
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

    if( pConnection->upstreamFd <= 0 )
    {
        pConnection->upstreamFd = s;
        uv_poll_init_socket( pConnection->pLoop, pConnection->upstreamWatcher, s );
    }

    if( action == CURL_POLL_REMOVE )
    {
        uv_poll_stop( pConnection->upstreamWatcher );
    }
    else
    {
        int kind = (action&CURL_POLL_IN ? UV_READABLE : 0) | (action&CURL_POLL_OUT ? UV_WRITABLE : 0);
        uv_poll_start( pConnection->upstreamWatcher, kind, curlSocketCB );
    }
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
    SocketConnection* pConnection = new SocketConnection( uvLoop );
    uv_tcp_init( uvLoop, pConnection->clientWatcher );

    if( uv_accept( (uv_stream_t*)uvServer, (uv_stream_t*)(pConnection->clientWatcher) ) == 0 ) {
        pConnection->status = csAccepted;
        uv_read_start( (uv_stream_t*)(pConnection->clientWatcher), uvAllocBuffer, readCallBack );
        uv_timer_start( pConnection->clientTimer, clientTimeoutCB, pConnection->readTimeout, 0 );
    } else {
        delete pConnection;
    }
}

void acceptCallback( uv_stream_t *server, int status ) {
    (void)server;
    if( status < 0 ) {
        LOG(WARNING) << "accept fail, error:" << uv_strerror( status );
        return;
    }
    PushServer::getInstance()->acceptCB();
}

void PushServer::start()
{
    int intRet;
    intRet = _LoadConf();
    if( intRet != 0 )
    {
        LOG(WARNING) << "load conf fail";
        return;
    }

    multi = curl_multi_init();
    if( multi == NULL ) {
        LOG(WARNING) << "curl_multi_init fail";
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
        LOG(WARNING) << "bind fail";
        return;
    }

    intRet = uv_listen( (uv_stream_t*)uvServer, 255, acceptCallback );
    if( intRet != 0 ) {
        LOG(WARNING) << "listen fail";
        return;
    }

    LOG(INFO) << "server start, version=0.0.1, listen port=" << intListenPort;
    uv_run( uvLoop, UV_RUN_DEFAULT );

    curl_global_cleanup();
}
