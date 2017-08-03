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

SocketConnection* PushServer::getConnection( int intFd )
{
    connectionMap::iterator it;
    it = mapConnection.find( intFd );
    if( it == mapConnection.end() )
    {
        LOG(WARNING) << "connection not found, fd=" << intFd;
        return NULL;
    }
    return it->second;
}

void uvAllocBuffer( uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf )
{
    (void)handle;
    *buf = uv_buf_init( (char*)malloc(suggested_size), suggested_size );
}

void clientTimeoutCB( uv_timer_t *timer )
{
    SocketConnection* pConnection = (SocketConnection *)(timer->data);
    delete pConnection;
    LOG(WARNING) << "client timeout";
}

/*static size_t curlWriteCB(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    if( realsize <= 0 )
    {
        return realsize;
    }

    SocketConnection * pConnection = (SocketConnection *)data;
    while( int(pConnection->upstreamBuf->intLen+realsize) > pConnection->upstreamBuf->intSize ) {
        pConnection->upstreamBuf->enlarge();
    }
    memcpy( pConnection->upstreamBuf->data + pConnection->upstreamBuf->intLen, ptr, realsize );
    pConnection->upstreamBuf->intLen += realsize;
    return realsize;
}*/

void writeCallback( uv_write_t *req, int status ) {
    SocketConnection* pConnection = (SocketConnection *)(req->data);

    if( status < 0 ) {
        LOG(WARNING) << "write fail, error:" << uv_strerror( status );
        delete pConnection;
    } else {
        uv_timer_stop( pConnection->clientTimer );
    }
}

void PushServer::parseQuery( SocketConnection *pConnection )
{
    rapidjson::Document docJson;
    docJson.Parse( (const char*)pConnection->inBuf->data );

    int size = 5;
    pConnection->uvOutBuf = uv_buf_init( (char*)malloc(size), size );
    memcpy(pConnection->uvOutBuf.base, "abcd", size);
    uv_write( pConnection->writeReq, (uv_stream_t*)(pConnection->clientWatcher), &(pConnection->uvOutBuf), 1, writeCallback );
    uv_timer_start( pConnection->clientTimer, clientTimeoutCB, pConnection->writeTimeout, 0 );

    /*CURL *easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_URL, docJson[0]["push_url"].GetString());
    curl_easy_setopt(easy, CURLOPT_POSTFIELDS, docJson[0]["push_data"].GetString());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curlWriteCB);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, pConnection);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, pConnection);
    //curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    //curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, 3L);
    //curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 10L);
    curl_multi_add_handle(multi, easy);*/
}

void PushServer::readCB( SocketConnection* pConnection, ssize_t nread, const uv_buf_t *buf )
{
    while( pConnection->inBuf->intSize < pConnection->inBuf->intLen+nread )
    {
        pConnection->inBuf->enlarge();
    }
    memcpy( pConnection->inBuf->data+pConnection->inBuf->intLen, buf->base, nread );
    pConnection->inBuf->intLen += nread;

    if( pConnection->inBuf->data[pConnection->inBuf->intLen-1] == '\0' )
    {
        LOG(INFO) << "recv query";
        uv_timer_stop( pConnection->clientTimer );
        parseQuery( pConnection );
    }
}

void readCallBack( uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf )
{
    SocketConnection* pConnection = (SocketConnection *)(stream->data);
    pConnection->status = csConnected;

    if( nread < 0 ) {
        if( nread == UV_EOF ) {
            // client close normally
            delete pConnection;
        }
    } else if ( nread > 0 ) {
        PushServer::getInstance()->readCB( pConnection, nread, buf );
    }

    if( buf->base )
    {
        free(buf->base);
    }
}

/*static void checkMultiInfo()
{
    CURLMsg *msg;
    int msgs_left;
    CURL *easy;
    CURLcode res;

    while( (msg = curl_multi_info_read(PushServer::getInstance()->multi, &msgs_left)) ) {
        if( msg->msg == CURLMSG_DONE ) {
            easy = msg->easy_handle;
            SocketConnection *pConnection;
            curl_easy_getinfo( easy, CURLINFO_PRIVATE, &pConnection );

            res = msg->data.result;
            if( res == 0 ) {
                SocketBuffer* outBuf;
                outBuf = new SocketBuffer( pConnection->upstreamBuf->intLen );
                memcpy( outBuf->data, pConnection->upstreamBuf->data, pConnection->upstreamBuf->intLen );
                outBuf->intLen = pConnection->upstreamBuf->intLen;
                pConnection->outBufList.push_back( outBuf );

                pConnection->inBuf->intLen = 0;
                pConnection->inBuf->intExpectLen = 0;

                //pConnection->writeWatcher->start();
                //pConnection->writeTimer->start();
            }
            else {
                delete pConnection;
            }

            curl_multi_remove_handle( PushServer::getInstance()->multi, easy );
            curl_easy_cleanup( easy );
        }
    }
}

void curlSocketCB( uv_poll_t *watcher, int status, int revents )
{
    std::cout << 4 << std::endl;
    int action = (revents&UV_READABLE ? CURL_POLL_IN : 0) | (revents&UV_WRITABLE ? CURL_POLL_OUT : 0);
    SocketConnection *pConnection = (SocketConnection *)(watcher->data);
    PushServer* pPushServer = PushServer::getInstance();
    curl_multi_socket_action( pPushServer->multi, pConnection->upstreamFd, action, &(pPushServer->intCurlRunning));
    checkMultiInfo();
}

static int curlSocketCallback( CURL *e, curl_socket_t s, int action, void *cbp, void *sockp )
{
    std::cout << 3 << std::endl;
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
    std::cout << 2 << std::endl;
    (void)timer;
    PushServer* pPushServer = PushServer::getInstance();
    curl_multi_socket_action( pPushServer->multi, CURL_SOCKET_TIMEOUT, 0, &(pPushServer->intCurlRunning) );
    checkMultiInfo();
}

static int curlTimerCallback( CURLM *multi, long timeout_ms, void *g )
{
    std::cout << 1 << std::endl;
    (void)multi;
    (void)g;
    PushServer* pPushServer = PushServer::getInstance();
    //uv_timer_stop( pPushServer->curlMultiTimer );
    uv_timer_start( pPushServer->curlMultiTimer, curlTimeoutCB, timeout_ms, 0 );

    return 0;
}*/

void PushServer::acceptCB()
{
    SocketConnection* pConnection = new SocketConnection( uvLoop );
    uv_tcp_init( uvLoop, pConnection->clientWatcher );

    if( uv_accept( (uv_stream_t*)uvServer, (uv_stream_t*)(pConnection->clientWatcher) ) == 0 ) {
        pConnection->status = csAccepted;
        uv_read_start( (uv_stream_t*)(pConnection->clientWatcher), uvAllocBuffer, readCallBack );
        uv_timer_start( pConnection->clientTimer, clientTimeoutCB, pConnection->readTimeout, 0 );
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
    /*CURLcode res;
    res = curl_global_init( CURL_GLOBAL_ALL );
    if( res != CURLE_OK )
    {
        LOG(WARNING) << "curl_global_init fail";
        return;
    }

    multi = curl_multi_init();
    if( multi == NULL ) {
        LOG(WARNING) << "curl_multi_init fail";
        return;
    }
    curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, curlTimerCallback);
    curl_multi_setopt(multi, CURLMOPT_TIMERDATA, NULL);
    curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, curlSocketCallback);
    curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, NULL);*/

    int intRet;
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

    LOG(INFO) << "server start, listen port=" << intListenPort;
    uv_run( uvLoop, UV_RUN_DEFAULT );

    curl_global_cleanup();
}