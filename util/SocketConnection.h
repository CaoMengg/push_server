#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <list>
#include <stdlib.h>
#include <unistd.h>
#include "uv.h"
#include "GLog.h"
#include "SocketBuffer.h"

enum enumConnectionStatus
{
    csInit,
    csAccepted,
    csConnected,
    csClosing,
};

class SocketConnection
{
    public:
        SocketConnection( uv_loop_t *loop ) {
            pLoop = loop;
            inBuf = new SocketBuffer( 1024 );
            upstreamBuf = new SocketBuffer( 1024 );

            clientWatcher = new uv_tcp_t();
            clientWatcher->data = this;

            clientTimer = new uv_timer_t();
            clientTimer->data = this;
            uv_timer_init( pLoop, clientTimer );

            upstreamWatcher = new uv_poll_t();
            upstreamWatcher->data = this;

            writeReq = new uv_write_t();
            writeReq->data = this;
        }
        ~SocketConnection() {
            delete inBuf;
            delete upstreamBuf;

            uv_close( (uv_handle_t *)clientWatcher, NULL );
            delete clientWatcher;
            uv_timer_stop( clientTimer );
            delete clientTimer;

            if( upstreamFd > 0 )
            {
                uv_close( (uv_handle_t *)upstreamWatcher, NULL );
                delete upstreamWatcher;
            }

            delete writeReq;
        }

        enumConnectionStatus status = csInit;
        int upstreamFd = 0;

        uv_loop_t *pLoop = NULL;
        SocketBuffer *inBuf = NULL;
        SocketBuffer *upstreamBuf = NULL;

        uv_tcp_t *clientWatcher = NULL;
        uv_timer_t *clientTimer = NULL;

        uv_poll_t *upstreamWatcher = NULL;

        uv_write_t *writeReq = NULL;
        uv_buf_t uvOutBuf;

        long readTimeout = 1000;
        long writeTimeout = 1000;
};

#endif
