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

#elif PLATFORM(PLATFORM_LINUX)

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
#elif PLATFORM(PLATFORM_LINUX)
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
    if (*cursor++ != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b1 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b2 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '.') {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b3 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '\0') {
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
    int family = AF_INET;
    int type = SOCK_STREAM;
    int proto = 0;

    if (SbArgs_Parse("|i:family,i:type,i:proto", args, kwargs, &family, &type, &proto) < 0) {
        return NULL;
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
socketobj_getattr(socket_object *self, SbObject *args, SbObject *kwargs)
{
    const char *attr_str;
    SbObject *value;

    if (SbArgs_Parse("s:name", args, kwargs, &attr_str) < 0) {
        return NULL;
    }

    value = NULL;
    if (!Sb_StrCmp(attr_str, "family")) {
        return SbInt_FromNative(self->family);
    }
    if (!Sb_StrCmp(attr_str, "type")) {
        return SbInt_FromNative(self->type);
    }
    if (!Sb_StrCmp(attr_str, "proto")) {
        return SbInt_FromNative(self->proto);
    }
    return SbObject_DefaultGetAttr((SbObject *)self, args, kwargs);
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

    if (SbArgs_Parse("T:addr", args, kwargs, &o_address) < 0) {
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

    if (SbArgs_Parse("T:addr", args, kwargs, &o_address) < 0) {
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
    int how;
    int call_result;

    if (SbArgs_Parse("i:how", args, kwargs, &how) < 0) {
        return NULL;
    }

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
    int backlog = 0;
    int call_result;

    if (SbArgs_Parse("i:backlog", args, kwargs, &backlog) < 0) {
        return NULL;
    }

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
    SbObject *o_buffer;
    int bufsize;
    int flags = 0;
    int call_result;

    if (SbArgs_Parse("i:bufsize|i:flags", args, kwargs, &bufsize, &flags) < 0) {
        return NULL;
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
    SbObject *o_buffer;
    SbObject *o_address;
    int bufsize;
    int flags = 0;
    struct sockaddr sa;
    int sa_size = sizeof(sa);
    int call_result;

    if (SbArgs_Parse("i:bufsize|i:flags", args, kwargs, &bufsize, &flags) < 0) {
        return NULL;
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
    int flags = 0;
    int call_result;

    if (SbArgs_Parse("S:buffer|i:flags", args, kwargs, &o_buffer, &flags) < 0) {
        return NULL;
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
    int flags = 0;
    struct sockaddr sa;
    int call_result;

    if (SbArgs_Parse("S:buffer,T:address|i:flags", args, kwargs, &o_buffer, &o_address, &flags) < 0) {
        return NULL;
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

static SbObject *
socketobj_getsockname(socket_object *self, SbObject *args, SbObject *kwargs)
{
    struct sockaddr sa;
    int sa_size = sizeof(sa);
    int call_result;

    call_result = getsockname(self->s, &sa, &sa_size);
    if (call_result < 0) {
        socket_raise_error("getsockname");
        return NULL;
    }

    return sa2tuple(&sa, self->family);
}

static SbObject *
socketobj_getpeername(socket_object *self, SbObject *args, SbObject *kwargs)
{
    struct sockaddr sa;
    int sa_size = sizeof(sa);
    int call_result;

    call_result = getpeername(self->s, &sa, &sa_size);
    if (call_result < 0) {
        socket_raise_error("getpeername");
        return NULL;
    }

    return sa2tuple(&sa, self->family);
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
        { "__getattr__", (SbCFunction)socketobj_getattr },
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

        { "getsockname", (SbCFunction)socketobj_getsockname },
        { "getpeername", (SbCFunction)socketobj_getpeername },

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

    SbDict_SetItemString(dict, "AF_INET", SbInt_FromNative(AF_INET));
    SbDict_SetItemString(dict, "SOCK_STREAM", SbInt_FromNative(SOCK_STREAM));
    SbDict_SetItemString(dict, "SOCK_DGRAM", SbInt_FromNative(SOCK_DGRAM));
#if PLATFORM(PLATFORM_WINNT)
    SbDict_SetItemString(dict, "SHUT_RD", SbInt_FromNative(SD_RECEIVE));
    SbDict_SetItemString(dict, "SHUT_WR", SbInt_FromNative(SD_SEND));
    SbDict_SetItemString(dict, "SHUT_RDWR", SbInt_FromNative(SD_BOTH));
#elif PLATFORM(PLATFORM_LINUX)
    SbDict_SetItemString(dict, "SHUT_RD", SbInt_FromNative(SHUT_RD));
    SbDict_SetItemString(dict, "SHUT_WR", SbInt_FromNative(SHUT_WR));
    SbDict_SetItemString(dict, "SHUT_RDWR", SbInt_FromNative(SHUT_RDWR));
#endif

    Sb_ModuleSocket = m;
    return 0;
}

void
_Sb_ModuleFini_Socket()
{
    Sb_CLEAR(Sb_ModuleSocket);
}

#endif /* SUPPORTS(MODULE_SOCKET) */
