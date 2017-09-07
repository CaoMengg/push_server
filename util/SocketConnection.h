#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <list>
#include <stdlib.h>
#include <unistd.h>

#include "uv.h"
#include "curl/curl.h"
#include "GLog.h"
#include "YamlConf.h"
#include "SocketBuffer.h"
#include "rapidjson/document.h"

enum enumConnectionStatus
{
    csInit,
    csAccepted,
    csConnected,
    csClosing,
};

void uvCloseCB( uv_handle_t* handle );

class SocketConnection
{
    public:
        void logNotice()
        {
            LOG(INFO) << "NOTICE: app_name:" << strAppName << " push_type:" << strPushType << " is_succ:" << isSucc;
        }

        void logWarning( std::string strInfo )
        {
            LOG(WARNING) << "WARNING: app_name:" << strAppName << " push_type:" << strPushType << " is_succ:" << isSucc << " " << strInfo;
        }

        SocketConnection( uv_loop_t *loop )
        {
            pLoop = loop;
            inBuf = new SocketBuffer( 4096 );
            upstreamBuf = new SocketBuffer( 4096 );

            clientWatcher = new uv_tcp_t();
            clientWatcher->data = this;

            clientTimer = new uv_timer_t();
            clientTimer->data = this;
            uv_timer_init( pLoop, clientTimer );

            upstreamWatcher = new uv_poll_t();
            upstreamWatcher->data = this;

            writeReq = new uv_write_t();
            writeReq->data = this;

            shutdownReq = new uv_shutdown_t();
            shutdownReq->data = this;

            reqData = new rapidjson::Document();
            resData = new rapidjson::Document();
        }

        ~SocketConnection()
        {
            logNotice();
            delete inBuf;
            delete upstreamBuf;

            uv_close( (uv_handle_t *)clientWatcher, uvCloseCB );
            uv_close( (uv_handle_t *)clientTimer, uvCloseCB );

            if( upstreamFd > 0 )
            {
                //uv_close( (uv_handle_t *)upstreamWatcher, uvCloseCB );
            }

            delete writeReq;
            delete shutdownReq;
            delete reqData;
            delete resData;
        }

        enumConnectionStatus status = csInit;
        int upstreamFd = 0;

        uv_loop_t *pLoop = NULL;
        SocketBuffer *inBuf = NULL;
        SocketBuffer *upstreamBuf = NULL;

        uv_tcp_t *clientWatcher = NULL;
        uv_timer_t *clientTimer = NULL;

        CURL *upstreamHandle = NULL;
        uv_poll_t *upstreamWatcher = NULL;

        uv_write_t *writeReq = NULL;
        uv_buf_t uvOutBuf;
        uv_shutdown_t *shutdownReq;

        rapidjson::Document *reqData = NULL;
        rapidjson::Document *resData = NULL;
        std::string strAppName = "";
        std::string strPushType = "";
        std::string strReqSucc = "{\"errno\":0}";
        bool isSucc = false;
        YamlConf *conf = NULL;

        long readTimeout = 100;
        long writeTimeout = 100;
};

#endif
