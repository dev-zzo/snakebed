#include "snakebed.h"
#include "internal.h"

/* Ref: https://docs.python.org/2/library/socket.html */

#if PLATFORM(PLATFORM_WINNT)
#include <Winsock2.h>
#include <Windows.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#if SUPPORTS(MODULE_SOCKET)

SbObject *Sb_ModuleSocket = NULL;

static SbTypeObject *socket_error;

static void
socket_raise_error(const char *msg)
{
#if PLATFORM(PLATFORM_WINNT)
    SbErr_RaiseWithFormat(socket_error, "[errno %d]: %s", WSAGetLastError(), msg);
#else
    /* TODO */
#endif
}

/* Address conversions */

static int
tuple2sa_ipv4(SbObject *tuple, struct sockaddr *sa)
{
    SbObject *o_addr;
    SbObject *o_port;
    struct sockaddr_in *sa_ipv4 = (struct sockaddr_in *)sa;
    const char *cursor;
    unsigned long tmp;

    if (SbTuple_GetSizeUnsafe(tuple) != 2) {
        SbErr_RaiseWithFormat(SbExc_TypeError, "expected %d items in address tuple, found %d", 2, SbTuple_GetSizeUnsafe(tuple));
        return -1;
    }
    o_addr = SbTuple_GetItemUnsafe(tuple, 0);
    if (!SbStr_CheckExact(o_addr)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected str as IPv4 address");
        return -1;
    }
    o_port = SbTuple_GetItemUnsafe(tuple, 1);
    if (!SbInt_CheckExact(o_port)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as IPv4 port");
        return -1;
    }

    sa_ipv4->sin_family = AF_INET;
    sa_ipv4->sin_port = SbInt_AsNativeUnsafe(o_port);

    cursor = SbStr_AsStringUnsafe(o_addr);
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b1 = tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b2 = tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b3 = tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '\0') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b4 = tmp;

    return 0;

incorrect_addr:
    SbErr_RaiseWithFormat(SbExc_ValueError, "incorrect format of IPv4 address provided: '%s'", SbStr_AsStringUnsafe(o_addr));
    return -1;
}

static int
tuple2sa(SbObject *tuple, struct sockaddr *sa, int family)
{
    switch (family) {
    case AF_INET:
        return tuple2sa_ipv4(tuple, sa);
    default:
        SbErr_RaiseWithFormat(SbExc_ValueError, "unknown address family: %d", family);
        return -1;
    }
}

static SbObject *
sa2tuple_ipv4(struct sockaddr *sa)
{
    SbObject *o_addr;
    SbObject *o_port;
    struct sockaddr_in *sa_ipv4 = (struct sockaddr_in *)sa;

    o_addr = SbStr_FromFormat("%d.%d.%d.%d",
        (unsigned)sa_ipv4->sin_addr.S_un.S_un_b.s_b1,
        (unsigned)sa_ipv4->sin_addr.S_un.S_un_b.s_b2,
        (unsigned)sa_ipv4->sin_addr.S_un.S_un_b.s_b3,
        (unsigned)sa_ipv4->sin_addr.S_un.S_un_b.s_b4);
    if (!o_addr) {
        return NULL;
    }
    o_port = SbInt_FromNative(sa_ipv4->sin_port);
    if (!o_port) {
        return NULL;
    }

    return SbTuple_Pack(2, o_addr, o_port);
}

static SbObject *
sa2tuple(struct sockaddr *sa, int family)
{
    switch (family) {
    case AF_INET:
        return sa2tuple_ipv4(sa);
    default:
        SbErr_RaiseWithFormat(SbExc_ValueError, "unknown address family: %d", family);
        return NULL;
    }
}

/* Socket object */

typedef struct _socket_object {
    SbObject_HEAD;
#if PLATFORM(PLATFORM_WINNT)
    SOCKET s;
#else
    int s;
#endif
    int family;
    int type;
    int proto;
} socket_object;

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
        socket_raise_error("could not create a socket");
        return NULL;
    }
#else
    /* TODO */
#endif
    self->family = family;
    self->type = type;
    self->proto = proto;
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
    SbObject *o_addrtuple;
    struct sockaddr sa;
    int call_result;

    if (SbArgs_Unpack(args, 1, 1, &o_addrtuple) < 0) {
        return NULL;
    }
    if (!SbTuple_CheckExact(o_addrtuple)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected tuple as address");
        return NULL;
    }

    if (tuple2sa(o_addrtuple, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = connect(self->s, &sa, sizeof(sa));

    if (call_result != 0) {
        socket_raise_error("could not connect a socket");
        return NULL;
    }

    Sb_RETURN_NONE;
}

static SbObject *
socketobj_close(socket_object *self, SbObject *args, SbObject *kwargs)
{
    int call_result;

    call_result = closesocket(self->s);
    if (call_result != 0) {
        socket_raise_error("could not close a socket");
        return NULL;
    }

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
    if (!SbInt_CheckExact(o_how)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as how");
        return NULL;
    }
    how = SbInt_AsNativeUnsafe(o_how);

    call_result = shutdown(self->s, how);
    if (call_result != 0) {
        socket_raise_error("could not shutdown a socket");
        return NULL;
    }

    Sb_RETURN_NONE;
}

static SbObject *
socketobj_recv(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_bufsize;
    SbObject *o_flags = NULL;
    SbObject *o_buffer;
    int bufsize;
    int flags = 0;
    int call_result;

    if (SbArgs_Unpack(args, 1, 2, &o_bufsize, &o_flags) < 0) {
        return NULL;
    }
    if (!SbInt_CheckExact(o_bufsize)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as bufsize");
        return NULL;
    }
    bufsize = SbInt_AsNativeUnsafe(o_bufsize);
    if (o_flags) {
        if (!SbInt_CheckExact(o_flags)) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected int as flags");
            return NULL;
        }
        flags = SbInt_AsNativeUnsafe(o_flags);
    }

    o_buffer = SbStr_FromStringAndSize(NULL, bufsize);
    if (!o_buffer) {
        return NULL;
    }

    call_result = recv(self->s, SbStr_AsStringUnsafe(o_buffer), bufsize, flags);
    if (call_result != 0) {
        socket_raise_error("could not receive from a socket");
        return NULL;
    }

    return o_buffer;
}

static SbObject *
socketobj_recvfrom(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_bufsize;
    SbObject *o_flags = NULL;
    SbObject *o_buffer;
    SbObject *o_address;
    int bufsize;
    int flags = 0;
    int call_result;
    struct sockaddr sa;
    int sa_size = sizeof(sa);

    if (SbArgs_Unpack(args, 1, 2, &o_bufsize, &o_flags) < 0) {
        return NULL;
    }
    if (!SbInt_CheckExact(o_bufsize)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as bufsize");
        return NULL;
    }
    bufsize = SbInt_AsNativeUnsafe(o_bufsize);
    if (o_flags) {
        if (!SbInt_CheckExact(o_flags)) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected int as flags");
            return NULL;
        }
        flags = SbInt_AsNativeUnsafe(o_flags);
    }

    o_buffer = SbStr_FromStringAndSize(NULL, bufsize);
    if (!o_buffer) {
        return NULL;
    }

    call_result = recvfrom(self->s, SbStr_AsStringUnsafe(o_buffer), bufsize, flags, &sa, &sa_size);
    if (call_result != 0) {
        socket_raise_error("could not receive from a socket");
        Sb_DECREF(o_buffer);
        return NULL;
    }
    o_address = sa2tuple(&sa, self->family);
    if (!o_address) {
        Sb_DECREF(o_buffer);
        return NULL;
    }

    return SbTuple_Pack(2, o_buffer, o_address);
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
        { "recv", (SbCFunction)socketobj_recv },
        { "recvfrom", (SbCFunction)socketobj_recvfrom },
#if 0
        { "send", (SbCFunction)socketobj_send },
        { "sendto", (SbCFunction)socketobj_sendto },
#endif
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
