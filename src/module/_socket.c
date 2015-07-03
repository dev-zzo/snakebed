#include "snakebed.h"
#include "internal.h"

#if PLATFORM(PLATFORM_WINNT)
#include <Winsock2.h>
#include <Windows.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#if SUPPORTS(MODULE_SOCKET)

SbObject *Sb_ModuleSocket = NULL;

typedef struct _socket_object {
    SbObject_HEAD;
#if PLATFORM(PLATFORM_WINNT)
    SOCKET s;
#else
    int s;
#endif
} socket_object;

static SbTypeObject *socket_error;

static SbObject *
socketobj_init(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_family = NULL;
    SbObject *o_type = NULL;
    SbObject *o_proto = NULL;
    int family, type, proto;

    if (SbArgs_Unpack(args, 0, 3, &o_family, &o_type, &o_proto) < 0) {
        return NULL;
    }
    if (o_family) {
    }
    else {
        family = AF_INET;
    }
    if (o_type) {
    }
    else {
        type = SOCK_STREAM;
    }
    if (o_proto) {
    }
    else {
        proto = 0;
    }

    self->s = socket(family, type, proto);

#if PLATFORM(PLATFORM_WINNT)
    if (self->s == INVALID_SOCKET) {
        SbErr_RaiseWithFormat(socket_error, "[errno %d]: can't create a socket", WSAGetLastError());
        return NULL;
    }
#else
    /* TODO */
#endif

    Sb_RETURN_NONE;
}

static SbObject *
socketobj_del(socket_object *self, SbObject *args, SbObject *kwargs)
{
    shutdown(self->s, SD_SEND);
    closesocket(self->s);
    Sb_RETURN_NONE;
}

static SbObject *
socketobj_connect(socket_object *self, SbObject *args, SbObject *kwargs)
{
    Sb_RETURN_NONE;
}

static SbObject *
socketobj_close(socket_object *self, SbObject *args, SbObject *kwargs)
{
    int call_result;

    call_result = closesocket(self->s);
    Sb_RETURN_NONE;
}

static SbObject *
socketobj_shutdown(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_how;
    int how;
    int call_result;

    if (SbArgs_Unpack(args, 1, 1, &o_how) < 0) {
        return NULL;
    }

    Sb_RETURN_NONE;
}

static int
socket_platform_init()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        return -1;
    }

    return 0;
}

static SbObject *
_Sb_TypeInit_Socket(SbObject *m)
{
    SbTypeObject *tp;
    static const SbCMethodDef socket_methods[] = {
        { "__init__", (SbCFunction)socketobj_init, },
        { "__del__", (SbCFunction)socketobj_del, },
        { "connect", (SbCFunction)socketobj_connect },
        { "close", (SbCFunction)socketobj_close },
        { "shutdown", (SbCFunction)socketobj_shutdown },
        /* Sentinel */
        { NULL, NULL },
    };

    if (socket_platform_init() < 0) {
        SbErr_RaiseWithString(SbExc_SystemError, "socket: platform init failed");
    }

    tp = _SbType_FromCDefs("socket.socket", SbObject_Type, socket_methods, sizeof(socket_object));
    if (!tp) {
        return NULL;
    }

    return (SbObject *)tp;
}

int
_Sb_ModuleInit_Socket()
{
    SbObject *m;
    SbObject *dict;
    SbObject *tp;

    m = Sb_InitModule("socket");
    if (!m) {
        return -1;
    }

    dict = SbModule_GetDict(m);
    if (!dict) {
        return -1;
    }

    tp = _Sb_TypeInit_Socket(m);
    if (!tp) {
        Sb_DECREF(m);
        return -1;
    }
    SbDict_SetItemString(dict, "socket", tp);

    socket_error = SbExc_NewException("socket.error", SbExc_Exception);
    SbDict_SetItemString(dict, "error", (SbObject *)socket_error);

    Sb_ModuleSocket = m;
    return 0;
}

void
_Sb_ModuleFini_Socket()
{
    Sb_CLEAR(Sb_ModuleSocket);
}

#endif /* SUPPORTS(MODULE_SOCKET) */
