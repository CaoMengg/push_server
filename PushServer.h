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
#include <iostream>
#include <map>

#include "rapidjson/document.h"
//#include "rapidjson/writer.h"
//#include "rapidjson/stringbuffer.h"

#include "uv.h"
#include "curl/curl.h"
#include "GLog.h"
#include "YamlConf.h"
#include "SocketConnection.h"

typedef std::map<std::string, YamlConf*> confMap;
typedef std::map<std::string, YamlConf*>::iterator confIterator;

class PushServer
{
    private:
        PushServer()
        {
            uvLoop = uv_default_loop();
            uvServer = new uv_tcp_t();

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
        int intListenPort = 0;

    public:
        static PushServer *getInstance();
        uv_loop_t *uvLoop = NULL;
        uv_tcp_t *uvServer = NULL;

        CURLM *multi = NULL;
        uv_timer_t *curlMultiTimer = NULL;
        int intCurlRunning = 1;

        void start();
        void acceptCB();
        void readCB( SocketConnection* pConnection, ssize_t nread );
        void parseQuery( SocketConnection *pConnection );
};

#endif
