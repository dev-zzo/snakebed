#include "snakebed.h"
#include "internal.h"

#if SUPPORTS(MODULE_SOCKET)

/* Ref: https://docs.python.org/2/library/socket.html */

#if PLATFORM(PLATFORM_WINNT)
#include <Winsock2.h>
#include <Windows.h>
#pragma comment(lib, "Ws2_32.lib")

typedef SOCKET OSSocket_t;

#define IS_SOCKET_VALID(x) ((x) != INVALID_SOCKET)

#endif

#if PLATFORM(LINUX)

typedef int OSSocket_t;

#define IS_SOCKET_VALID(x) ((x) >= 0)

#endif

SbObject *Sb_ModuleSocket = NULL;
SbTypeObject *Sb_SocketType = NULL;

static SbTypeObject *socket_error;

static void
socket_raise_error(const char *func)
{
#if PLATFORM(PLATFORM_WINNT)
    SbErr_RaiseWithFormat(socket_error, "%s: [errno %d]", func, WSAGetLastError());
#elif PLATFORM(LINUX)
    SbErr_RaiseWithFormat(socket_error, "%s: [errno %d]", func, errno);
#else
    /* FAIL */
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

    /* NOTE: none of this code verifies range of the integers provided... */
    sa_ipv4->sin_family = AF_INET;
    sa_ipv4->sin_port = (unsigned short)SbInt_AsNativeUnsafe(o_port);

    cursor = SbStr_AsStringUnsafe(o_addr);
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b1 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b2 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b3 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor != '\0') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b4 = (unsigned char)tmp;

    return 0;

incorrect_addr:
    SbErr_RaiseWithFormat(SbExc_ValueError, "incorrect format of IPv4 address provided: '%s'", SbStr_AsStringUnsafe(o_addr));
    return -1;
}

static int
tuple2sa(SbObject *tuple, struct sockaddr *sa, int family)
{
    if (!SbTuple_CheckExact(tuple)) {
        SbErr_RaiseWithString(SbExc_TypeError, "address is not a tuple");
        return -1;
    }

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
    OSSocket_t s;
    int family;
    int type;
    int proto;
} socket_object;

static SbObject *
socketobj_new(int s, int family, int type, int proto)
{
    SbObject *o;

    o = SbObject_New(Sb_SocketType);
    if (o) {
        socket_object *self = (socket_object *)o;

        self->s = s;
        self->family = family;
        self->type = type;
        self->proto = proto;
    }

    return o;
}

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
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as family");
        return NULL;
    }
    else {
        family = AF_INET;
    }
    if (o_type) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as type");
        return NULL;
    }
    else {
        type = SOCK_STREAM;
    }
    if (o_proto) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as proto");
        return NULL;
    }
    else {
        proto = 0;
    }

    self->family = family;
    self->type = type;
    self->proto = proto;
    self->s = socket(family, type, proto);

#if PLATFORM(PLATFORM_WINNT)
    if (!IS_SOCKET_VALID(self->s)) {
        socket_raise_error("socket");
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
socketobj_accept(socket_object *self, SbObject *args, SbObject *kwargs)
{
    struct sockaddr sa;
    int sa_size = sizeof(sa);
    OSSocket_t new_socket;
    SbObject *o_socket;
    SbObject *o_address;

    new_socket = accept(self->s, &sa, &sa_size);
    if (!IS_SOCKET_VALID(new_socket)) {
        socket_raise_error("accept");
        return NULL;
    }
    o_socket = socketobj_new(new_socket, self->family, self->type, self->proto);
    if (!o_socket) {
        return NULL;
    }

    o_address = sa2tuple(&sa, self->family);
    if (!o_address) {
        Sb_DECREF(o_socket);
        return NULL;
    }

    return SbTuple_Pack(2, o_socket, o_address);
}

static SbObject *
socketobj_bind(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_address;
    struct sockaddr sa;
    int call_result;

    if (SbArgs_Unpack(args, 1, 1, &o_address) < 0) {
        return NULL;
    }
    if (!SbTuple_CheckExact(o_address)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected tuple as address");
        return NULL;
    }

    if (tuple2sa(o_address, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = bind(self->s, &sa, sizeof(sa));

    if (call_result != 0) {
        socket_raise_error("bind");
        return NULL;
    }

    Sb_RETURN_NONE;
}

static SbObject *
socketobj_connect(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_address;
    struct sockaddr sa;
    int call_result;

    if (SbArgs_Unpack(args, 1, 1, &o_address) < 0) {
        return NULL;
    }
    if (!SbTuple_CheckExact(o_address)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected tuple as address");
        return NULL;
    }

    if (tuple2sa(o_address, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = connect(self->s, &sa, sizeof(sa));

    if (call_result != 0) {
        socket_raise_error("connect");
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
        socket_raise_error("close");
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
        socket_raise_error("shutdown");
        return NULL;
    }

    Sb_RETURN_NONE;
}

static SbObject *
socketobj_listen(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_backlog;
    int backlog = 0;
    int call_result;

    if (SbArgs_Unpack(args, 1, 1, &o_backlog) < 0) {
        return NULL;
    }
    if (!SbInt_CheckExact(o_backlog)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected int as backlog");
        return NULL;
    }
    backlog = SbInt_AsNativeUnsafe(o_backlog);

    call_result = listen(self->s, backlog);
    if (call_result < 0) {
        socket_raise_error("listen");
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
    if (call_result < 0) {
        socket_raise_error("recv");
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
    struct sockaddr sa;
    int sa_size = sizeof(sa);
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

    call_result = recvfrom(self->s, SbStr_AsStringUnsafe(o_buffer), bufsize, flags, &sa, &sa_size);
    if (call_result < 0) {
        socket_raise_error("recvfrom");
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

static SbObject *
socketobj_send(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_buffer;
    SbObject *o_flags = NULL;
    int flags = 0;
    int call_result;

    if (SbArgs_Unpack(args, 1, 2, &o_buffer, &o_flags) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(o_buffer)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected str as buffer");
        return NULL;
    }
    if (o_flags) {
        if (!SbInt_CheckExact(o_flags)) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected int as flags");
            return NULL;
        }
        flags = SbInt_AsNativeUnsafe(o_flags);
    }

    call_result = send(self->s, SbStr_AsStringUnsafe(o_buffer), SbStr_GetSizeUnsafe(o_buffer), flags);
    if (call_result < 0) {
        socket_raise_error("send");
        return NULL;
    }

    return SbInt_FromNative(call_result);
}

static SbObject *
socketobj_sendto(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_buffer;
    SbObject *o_address;
    SbObject *o_flags = NULL;
    int flags = 0;
    struct sockaddr sa;
    int call_result;

    if (SbArgs_Unpack(args, 2, 3, &o_buffer, &o_address, &o_flags) < 0) {
        return NULL;
    }
    if (!SbStr_CheckExact(o_buffer)) {
        SbErr_RaiseWithString(SbExc_TypeError, "expected str as buffer");
        return NULL;
    }
    if (o_flags) {
        if (!SbInt_CheckExact(o_flags)) {
            SbErr_RaiseWithString(SbExc_TypeError, "expected int as flags");
            return NULL;
        }
        flags = SbInt_AsNativeUnsafe(o_flags);
    }
    if (tuple2sa(o_address, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = sendto(self->s, SbStr_AsStringUnsafe(o_buffer), SbStr_GetSizeUnsafe(o_buffer), flags, &sa, sizeof(sa));
    if (call_result < 0) {
        socket_raise_error("sendto");
        return NULL;
    }

    return SbInt_FromNative(call_result);
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

static SbTypeObject *
_Sb_TypeInit_Socket(SbObject *m)
{
    SbTypeObject *tp;
    static const SbCMethodDef methods[] = {
        { "__init__", (SbCFunction)socketobj_init, },
        { "__del__", (SbCFunction)socketobj_del, },
        { "connect", (SbCFunction)socketobj_connect },
        { "close", (SbCFunction)socketobj_close },
        { "shutdown", (SbCFunction)socketobj_shutdown },
        { "recv", (SbCFunction)socketobj_recv },
        { "recvfrom", (SbCFunction)socketobj_recvfrom },
        { "send", (SbCFunction)socketobj_send },
        { "sendto", (SbCFunction)socketobj_sendto },
        { "bind", (SbCFunction)socketobj_bind },
        { "listen", (SbCFunction)socketobj_listen },
        { "accept", (SbCFunction)socketobj_accept },
        /* Sentinel */
        { NULL, NULL },
    };

    if (socket_platform_init() < 0) {
        SbErr_RaiseWithString(SbExc_SystemError, "socket: platform init failed");
    }

    tp = _SbType_FromCDefs("socket.socket", SbObject_Type, methods, sizeof(socket_object));
    if (!tp) {
        return NULL;
    }

    return tp;
}

/* Socket module */

int
_Sb_ModuleInit_Socket()
{
    SbObject *m;
    SbObject *dict;
    SbTypeObject *tp;

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
    Sb_SocketType = tp;
    SbDict_SetItemString(dict, "socket", (SbObject *)tp);

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
