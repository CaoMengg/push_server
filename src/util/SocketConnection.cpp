#include "SocketConnection.h"

void uvCloseCB(uv_handle_t *handle)
{
    DLOG(INFO) << "DEBUG: close uv handle";
    delete handle;
}
