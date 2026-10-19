#pragma once

// Capa mínima de sockets UDP: POSIX en Linux, macOS y Android; Winsock 2 en Windows.
// El resto del código de red usa solo estas funciones y tipos, nunca la API del sistema directamente.

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace OpenCraft::Net {

#if defined(_WIN32)
using SocketHandle = SOCKET;
inline const SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
using SockLen = int;
#else
using SocketHandle = int;
inline constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
using SockLen = socklen_t;
#endif

inline bool isValidSocket(SocketHandle s) noexcept { return s != INVALID_SOCKET_HANDLE; }

// Inicializa Winsock una sola vez por proceso (no hace nada en POSIX)
inline bool initSockets() noexcept {
#if defined(_WIN32)
    struct WinsockGuard {
        bool ok = false;
        WinsockGuard() {
            WSADATA data{};
            ok = WSAStartup(MAKEWORD(2, 2), &data) == 0;
        }
        ~WinsockGuard() {
            if (ok) WSACleanup();
        }
    };
    static WinsockGuard guard;
    return guard.ok;
#else
    return true;
#endif
}

inline void closeSocket(SocketHandle s) noexcept {
    if (!isValidSocket(s)) return;
#if defined(_WIN32)
    ::closesocket(s);
#else
    ::close(s);
#endif
}

inline bool setNonBlocking(SocketHandle s) noexcept {
#if defined(_WIN32)
    u_long mode = 1;
    return ::ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = ::fcntl(s, F_GETFL, 0);
    if (flags == -1) return false;
    return ::fcntl(s, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

inline bool setSocketOption(SocketHandle s, int level, int option, int value) noexcept {
    return ::setsockopt(s, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}

// Socket UDP IPv4 no bloqueante
inline SocketHandle openUdpSocket() noexcept {
    if (!initSockets()) return INVALID_SOCKET_HANDLE;
    SocketHandle s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!isValidSocket(s)) return s;
#if defined(_WIN32)
    // Sin esto, un ICMP «puerto inalcanzable» hace que el siguiente recvfrom falle con WSAECONNRESET
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
    BOOL reportReset = FALSE;
    DWORD bytesReturned = 0;
    ::WSAIoctl(s, SIO_UDP_CONNRESET, &reportReset, sizeof(reportReset), nullptr, 0, &bytesReturned, nullptr,
               nullptr);
#endif
    setNonBlocking(s);
    return s;
}

inline bool bindSocket(SocketHandle s, const sockaddr_in& addr) noexcept {
    return ::bind(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0;
}

// Devuelve los bytes enviados o -1
inline long long sendTo(SocketHandle s, const void* data, size_t size, const sockaddr_in& dest) noexcept {
#if defined(_WIN32)
    return ::sendto(s, static_cast<const char*>(data), static_cast<int>(size), 0,
                    reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
#else
    return ::sendto(s, data, size, 0, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
#endif
}

// Devuelve los bytes recibidos, 0 o -1 si no hay datos (el socket es no bloqueante)
inline long long recvFrom(SocketHandle s, void* buffer, size_t size, sockaddr_in& sender) noexcept {
    SockLen len = sizeof(sender);
#if defined(_WIN32)
    return ::recvfrom(s, static_cast<char*>(buffer), static_cast<int>(size), 0, reinterpret_cast<sockaddr*>(&sender),
                      &len);
#else
    return ::recvfrom(s, buffer, size, 0, reinterpret_cast<sockaddr*>(&sender), &len);
#endif
}

} // namespace OpenCraft::Net
