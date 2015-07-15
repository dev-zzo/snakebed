#include "snakebed.h"
#include "internal.h"

#if SUPPORTS(MODULE_SOCKET)

/* Ref: https://docs.python.org/2/library/socket.html */

/* NOTE:
   This implementation DEPARTS from the standard Python library in the following:

 * The basic error class name is `SocketError`, not `error`.
 * socket.settimeout() accepts None or int, instead of float.
 */

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

static SbTypeObject *SbExc_SocketError;

static SbObject *
socket_raise_error(void)
{
    int error_id;
    SbObject *o_errno;
    SbObject *o_text;
    
#if PLATFORM(PLATFORM_WINNT)
    error_id = WSAGetLastError();
#elif PLATFORM(PLATFORM_LINUX)
    error_id = errno;
#endif
    o_errno = SbInt_FromNative(error_id);
    o_text = SbStr_FromString("<strerror not available>");
    SbErr_RaiseWithObject(SbExc_SocketError, SbTuple_Pack(2, o_errno, o_text));
    Sb_DECREF(o_errno);
    Sb_DECREF(o_text);
    return NULL;
}

/* Address conversions */

static int
addr2sa_ipv4(SbObject *tuple, struct sockaddr *sa)
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
    sa_ipv4->sin_port = htons((unsigned short)SbInt_AsNativeUnsafe(o_port));

    cursor = SbStr_AsStringUnsafe(o_addr);
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '.' || tmp > 255) {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b1 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '.' || tmp > 255) {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b2 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '.' || tmp > 255) {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b3 = (unsigned char)tmp;
    Sb_AtoUL(cursor, &cursor, 10, &tmp);
    if (*cursor++ != '\0' || tmp > 255) {
        goto incorrect_addr;
    }
    sa_ipv4->sin_addr.S_un.S_un_b.s_b4 = (unsigned char)tmp;

    return 0;

incorrect_addr:
    SbErr_RaiseWithFormat(SbExc_ValueError, "incorrect format of IPv4 address provided: '%s'", SbStr_AsStringUnsafe(o_addr));
    return -1;
}

static int
addr2sa(SbObject *tuple, struct sockaddr *sa, int family)
{
    if (!SbTuple_CheckExact(tuple)) {
        SbErr_RaiseWithString(SbExc_TypeError, "address is not a tuple");
        return -1;
    }

    switch (family) {
    case AF_INET:
        return addr2sa_ipv4(tuple, sa);
    default:
        SbErr_RaiseWithFormat(SbExc_ValueError, "unknown address family: %d", family);
        return -1;
    }
}

static SbObject *
sa2addr_ipv4(struct sockaddr *sa)
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
    o_port = SbInt_FromNative(ntohs(sa_ipv4->sin_port));
    if (!o_port) {
        return NULL;
    }

    return SbTuple_Pack(2, o_addr, o_port);
}

static SbObject *
sa2addr(struct sockaddr *sa, int family)
{
    switch (family) {
    case AF_INET:
        return sa2addr_ipv4(sa);
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
    int timeout; /* -1: blocking; 0: non-blocking; others: timeout */
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
    if (!IS_SOCKET_VALID(self->s)) {
        return socket_raise_error();
    }

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
        return socket_raise_error();
    }
    o_socket = socketobj_new(new_socket, self->family, self->type, self->proto);
    if (!o_socket) {
        return NULL;
    }

    o_address = sa2addr(&sa, self->family);
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

    if (addr2sa(o_address, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = bind(self->s, &sa, sizeof(sa));

    if (call_result != 0) {
        return socket_raise_error();
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

    if (addr2sa(o_address, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = connect(self->s, &sa, sizeof(sa));

    if (call_result != 0) {
        return socket_raise_error();
    }

    Sb_RETURN_NONE;
}

static SbObject *
socketobj_close(socket_object *self, SbObject *args, SbObject *kwargs)
{
    int call_result;

    call_result = closesocket(self->s);
    if (call_result != 0) {
        return socket_raise_error();
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
        return socket_raise_error();
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
        return socket_raise_error();
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
        Sb_DECREF(o_buffer);
        return socket_raise_error();
    }
    SbStr_Truncate(o_buffer, call_result);

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
        Sb_DECREF(o_buffer);
        return socket_raise_error();
    }
    SbStr_Truncate(o_buffer, call_result);
    o_address = sa2addr(&sa, self->family);
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
        return socket_raise_error();
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

    if (addr2sa(o_address, &sa, self->family) < 0) {
        return NULL;
    }

    call_result = sendto(self->s, SbStr_AsStringUnsafe(o_buffer), SbStr_GetSizeUnsafe(o_buffer), flags, &sa, sizeof(sa));
    if (call_result < 0) {
        return socket_raise_error();
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
        return socket_raise_error();
    }

    return sa2addr(&sa, self->family);
}

static SbObject *
socketobj_getpeername(socket_object *self, SbObject *args, SbObject *kwargs)
{
    struct sockaddr sa;
    int sa_size = sizeof(sa);
    int call_result;

    call_result = getpeername(self->s, &sa, &sa_size);
    if (call_result < 0) {
        return socket_raise_error();
    }

    return sa2addr(&sa, self->family);
}

static int
socketobj_apply_timeout(socket_object *self, int timeout)
{
    self->timeout = timeout;
    return 0;
}

static SbObject *
socketobj_settimeout(socket_object *self, SbObject *args, SbObject *kwargs)
{
    SbObject *o_timeout;
    int timeout;

    if (SbArgs_Parse("O:timeout", args, kwargs, &o_timeout) < 0) {
        return NULL;
    }

    if (o_timeout == Sb_None) {
        /* no timeout, blocking mode */
        timeout = -1;
    }
    else if (SbInt_Check(o_timeout)) {
        timeout = SbInt_AsNativeUnsafe(o_timeout);
        if (timeout < 0) {
            /* raise */
            SbErr_RaiseWithString(SbExc_ValueError, "timeout cannot be negative");
            return NULL;
        }
    }
    else {
        SbErr_RaiseWithFormat(SbExc_TypeError, "expected arg '%s' to be %s, got %s",
            "timeout", "int or None", Sb_TYPE(o_timeout)->tp_name);
        return NULL;
    }

    socketobj_apply_timeout(self, timeout);
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

        { "settimeout", (SbCFunction)socketobj_settimeout },

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

    SbExc_SocketError = SbExc_NewException("socket.SocketError", SbExc_IOError);
    SbDict_SetItemString(dict, "SocketError", (SbObject *)SbExc_SocketError);

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
