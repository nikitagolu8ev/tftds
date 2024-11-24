#pragma once

#include <cerrno>
#include <expected>
#include <string>

#include <sys/socket.h>

#include "error.h"

namespace cppnet {

class Socket;
class SocketEvent;

#define UNSUPPORTED_ADDRESS_FAMILY static_assert(false, "Unsupported address family")

struct AddressFamily {
    enum _ : sa_family_t {
        Unspecified = AF_UNSPEC,
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        LinkLayer = AF_LINK,
    };
};

using AddressFamilyType = AddressFamily::_;

class SocketAddress {
    friend class Socket;

protected:
    virtual const sockaddr* SockAddrPtr() const = 0;
    virtual sockaddr* MutableSockAddrPtr() = 0;
    virtual uint32_t Size() const = 0;
};

namespace detail {

class NullSocketAddress : public SocketAddress {
protected:
    const sockaddr* SockAddrPtr() const override {
        return nullptr;
    }

    sockaddr* MutableSockAddrPtr() override {
        return nullptr;
    }

    uint32_t Size() const override {
        return 0;
    }
};

}  // namespace detail

class Socket {
    friend class SocketEvent;

public:
    struct Type {
        enum _ : uint8_t {
            TCP = SOCK_STREAM,
            UDP = SOCK_DGRAM,
        };
    };

    using SocketType = Type::_;

public:
    Socket(AddressFamilyType address_family, SocketType socket_type, int protocol = 0);

private:
    Socket(int32_t socket_fd);

public:
    // Non-copyable
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Moveable
    Socket(Socket&&);
    Socket& operator=(Socket&&);

    static Socket Clone(const Socket& other);

    template <typename T>
    void SetOption(int level, int option_name, const T& option_value) {
        using namespace std::literals::string_literals;
        if (setsockopt(socket_fd_, level, option_name, &option_value, sizeof(option_value)) < 0) {
            ProcessError(GetLastError(), "Socket::SetOption");
        }
    }

    void Bind(const SocketAddress& address);
    void Listen(uint32_t pending_queue_size = DefaultListenPendingQueueSize);
    Socket Accept(SocketAddress* address = nullptr);

    ErrorType TryConnect(const SocketAddress& address);
    void Connect(const SocketAddress& address);

    template <typename T>
    T GetSocketAddress() const
        requires std::is_convertible_v<T&, SocketAddress&>
    {
        T result;
        SocketAddress& socket_address = result;
        socklen_t address_size = socket_address.Size();
        if (getsockname(socket_fd_, socket_address.MutableSockAddrPtr(), &address_size) < 0) {
            ProcessError(GetLastError(), "GetSocketAddress");
        }
        return result;
    }

    // Sending methods
    ErrorType TrySendStringTo(const std::string& message, const SocketAddress& address,
                              int32_t flags = 0);
    ErrorType TrySendString(const std::string& message, int32_t flags = 0);
    void SendStringTo(const std::string& message, const SocketAddress& address, int32_t flags = 0);
    void SendString(const std::string& message, int32_t flags = 0);

    template <typename T>
    ErrorType TrySendObjectTo(const T& object, const SocketAddress& address, int32_t flags = 0) {
        size_t total_bytes_sent = 0;
        const std::byte* obj_addr = reinterpret_cast<const std::byte*>(&object);
        while (total_bytes_sent != sizeof(object)) {
            ssize_t bytes_sent =
                sendto(socket_fd_, obj_addr + total_bytes_sent, sizeof(object) - total_bytes_sent,
                       flags, address.SockAddrPtr(), address.Size());
            if (bytes_sent < 0) {
                return GetLastError();
            }
            total_bytes_sent += bytes_sent;
        }
        return {};
    }

    template <typename T>
    ErrorType TrySendObject(const T& object, int32_t flags = 0) {
        return TrySendObjectTo(object, detail::NullSocketAddress{}, flags);
    }

    template <typename T>
    void SendObjectTo(const T& object, const SocketAddress& address, int32_t flags = 0) {
        if (TrySendObjectTo(object, address, flags)) {
            ProcessError(GetLastError(), "Send");
        }
    }

    template <typename T>
    void SendObject(const T& object, int32_t flags = 0) {
        SendObjectTo(object, detail::NullSocketAddress{}, flags);
    }

    // Receiving methods
    std::string ReceiveAtMostNBytesFrom(size_t n_bytes, SocketAddress* address, int32_t flags = 0);
    std::string ReceiveAtMostNBytes(size_t n_bytes, int32_t flags = 0);

    std::string ReceiveNBytesFrom(size_t n_bytes, SocketAddress* address, int32_t flags = 0);
    std::string ReceiveNBytes(size_t n_bytes, int32_t flags = 0);

    template <typename T>
    ResultType<T> TryReceiveObjectFrom(SocketAddress* address, int32_t flags = 0) {
        size_t total_bytes_received = 0;
        alignas(alignof(T)) std::byte object_buffer[sizeof(T)];

        sockaddr* address_ptr = nullptr;
        socklen_t address_size = 0;
        if (address != nullptr) {
            address_ptr = address->MutableSockAddrPtr();
            address_size = address->Size();
        }

        while (total_bytes_received != sizeof(object_buffer)) {
            ssize_t bytes_received = recvfrom(socket_fd_, &object_buffer + total_bytes_received,
                                              sizeof(object_buffer) - total_bytes_received, flags,
                                              address_ptr, &address_size);
            if (bytes_received < 0) {
                return std::unexpected(GetLastError());
            }
            total_bytes_received += bytes_received;
        }
        return *reinterpret_cast<T*>(&object_buffer);
    }

    template <typename T>
    ResultType<T> TryReceiveObject(int32_t flags = 0) {
        return TryReceiveObjectFrom<T>(nullptr, flags);
    }

    template <typename T>
    T ReceiveObjectFrom(SocketAddress* address, int32_t flags = 0) {
        ResultType<T> result = TryReceiveObjectFrom<T>(address, flags);
        ProcessResultError(result, "Receive");
        return *result;
    }

    template <typename T>
    T ReceiveObject(int32_t flags = 0) {
        return ReceiveObjectFrom<T>(nullptr, flags);
    }

    void ShutDown(int32_t flags = SHUT_RDWR);
    void Close();

    ~Socket();

    int GetFileDescriptor() {
        return socket_fd_;
    }

public:
    static constexpr uint32_t DefaultListenPendingQueueSize = 10;

private:
    int32_t socket_fd_ = -1;
};

}  // namespace cppnet
