#ifndef OCRSERVER_H
#define OCRSERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "openssl/sha.h"
#include "uv.h"
#include "curl/curl.h"
#include "GLog.h"
#include "YamlConf.h"
#include "SocketConnection.h"

typedef std::map<std::string, YamlConf*> confMap;
typedef std::map<std::string, YamlConf*>::iterator confIterator;

typedef std::map<int, uv_poll_t*> upstreamWatcherMap;
typedef std::map<int, uv_poll_t*>::iterator upstreamWatcherIterator;

class PushServer
{
    private:
        PushServer()
        {
            uvLoop = uv_default_loop();
            uvServer = new uv_tcp_t();

            maintainTimer = new uv_timer_t();
            maintainTimer->data = this;
            uv_timer_init( uvLoop, maintainTimer );

            curlMultiTimer = new uv_timer_t();
            curlMultiTimer->data = this;
            uv_timer_init( uvLoop, curlMultiTimer );
        }
        ~PushServer()
        {
            if( mainConf )
            {
                delete mainConf;
            }

            uv_close( (uv_handle_t *)uvServer, NULL );
            delete uvServer;
            uv_timer_stop( maintainTimer );
            delete maintainTimer;
            uv_timer_stop( curlMultiTimer );
            delete curlMultiTimer;

            if( multi ) {
                curl_multi_cleanup( multi );
            }
        }
        int _LoadConf();
        int getConnectionConf( SocketConnection *pConnection );
        int getConnectionHandle( SocketConnection *pConnection );

        static PushServer *pInstance;
        YamlConf *mainConf = NULL;
        confMap mapConf;
        upstreamWatcherMap mapUpstreamWatcher;
        int intListenPort = 0;

    public:
        static PushServer *getInstance();
        uv_loop_t *uvLoop = NULL;
        uv_tcp_t *uvServer = NULL;

        uv_timer_t *maintainTimer = NULL;

        CURLM *multi = NULL;
        uv_timer_t *curlMultiTimer = NULL;
        int intCurlRunning = 1;

        void start();

        void refreshGetuiToken( std::string appName, YamlConf* appConf );
        void maintainCB();

        void acceptCB();
        void readCB( SocketConnection* pConnection, ssize_t nread );
        void parseQuery( SocketConnection *pConnection );
        void parseResponse( SocketConnection *pConnection );
        int getUpstreamWatcher( SocketConnection *pConnection );
};

#endif
