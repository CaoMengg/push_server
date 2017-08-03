#ifndef OCRSERVER_H
#define OCRSERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ev++.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "uv.h"
#include "curl/curl.h"
#include "GLog.h"
#include "YamlConf.h"
#include "SocketConnection.h"

typedef std::map<int, SocketConnection*> connectionMap;
typedef std::pair<int, SocketConnection*> connectionPair;

class PushServer
{
    private:
        PushServer()
        {
            config = new YamlConf( "conf/push_server.yaml" );
            intListenPort = config->getInt( "listen" );

            uvLoop = uv_default_loop();
            uvServer = new uv_tcp_t();

            curlMultiTimer = new uv_timer_t();
            curlMultiTimer->data = this;
            uv_timer_init( uvLoop, curlMultiTimer );
        }
        ~PushServer()
        {
            delete config;
            uv_close( (uv_handle_t *)uvServer, NULL );
            delete uvServer;
            uv_timer_stop( curlMultiTimer );
            delete curlMultiTimer;

            if( multi ) {
                curl_multi_cleanup( multi );
            }
        }

        static PushServer *pInstance;
        YamlConf *config = NULL;
        int intListenPort = 0;
        connectionMap mapConnection;

    public:
        static PushServer *getInstance();
        uv_loop_t *uvLoop = NULL;
        uv_tcp_t *uvServer = NULL;

        CURLM *multi = NULL;
        uv_timer_t *curlMultiTimer = NULL;
        int intCurlRunning = 1;

        SocketConnection* getConnection( int intFd );
        void start();

        void acceptCB();
        void readCB( SocketConnection* pConnection, ssize_t nread, const uv_buf_t *buf );
        void writeCB( ev::io &watcher, int revents );
        //void curlSocketCB( ev::io &watcher, int revents );

        void parseQuery( SocketConnection *pConnection );
};

#endif
