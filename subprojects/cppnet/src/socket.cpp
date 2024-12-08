#include "include/cppnet/socket.h"

#include <unistd.h>

namespace cppnet {

Socket::Socket(AddressFamilyType address_family, SocketType socket_type, int protocol) {
    using namespace std::literals::string_literals;
    socket_fd_ = socket(address_family, socket_type, protocol);
    if (socket_fd_ < 0) {
        ProcessError(GetLastError(), "Socket");
    }
}

Socket::Socket(int32_t socket_fd) : socket_fd_(socket_fd) {
}

Socket::Socket(Socket&& rhs) {
    socket_fd_ = std::exchange(rhs.socket_fd_, -1);
}

Socket& Socket::operator=(Socket&& rhs) {
    std::swap(socket_fd_, rhs.socket_fd_);
    return *this;
}

Socket Socket::Clone(const Socket& to_copy) {
    int new_fd = dup(to_copy.socket_fd_);
    if (new_fd < 0) {
        ProcessError(GetLastError(), "Socket::Clone");
    }
    return Socket(new_fd);
}

void Socket::Bind(const SocketAddress& address) {
    using namespace std::literals::string_literals;
    int res = bind(socket_fd_, address.SockAddrPtr(), address.Size());
    if (res < 0) {
        ProcessError(GetLastError(), "Bind");
    }
}

void Socket::Listen(uint32_t pending_queue_size) {
    if (listen(socket_fd_, pending_queue_size) < 0) {
        ProcessError(GetLastError(), "Listen");
    }
}

Socket Socket::Accept(SocketAddress* address) {
    sockaddr* address_ptr = nullptr;
    socklen_t address_size = 0;
    if (address != nullptr) {
        address_ptr = address->MutableSockAddrPtr();
        address_size = address->Size();
    }

    int32_t accepted_socket = accept(socket_fd_, address_ptr, &address_size);
    if (accepted_socket < 0) {
        ProcessError(GetLastError(), "Accept");
    }

    return Socket(accepted_socket);
}

ErrorType Socket::TryConnect(const SocketAddress& address) {
    if (connect(socket_fd_, address.SockAddrPtr(), address.Size()) < 0) {
        return GetLastError();
    }
    return {};
}

void Socket::Connect(const SocketAddress& address) {
    ProcessError(TryConnect(address), "Connect");
}

ErrorType Socket::TrySendStringTo(const std::string& message, const SocketAddress& address,
                                  int32_t flags) {
    size_t total_bytes_sent = 0;
    while (total_bytes_sent != message.size()) {
        ssize_t bytes_sent =
            sendto(socket_fd_, message.c_str() + total_bytes_sent,
                   message.size() - total_bytes_sent, flags, address.SockAddrPtr(), address.Size());
        if (bytes_sent < 0) {
            return GetLastError();
        }
        total_bytes_sent += bytes_sent;
    }
    return {};
}

ErrorType Socket::TrySendString(const std::string& message, int32_t flags) {
    return TrySendStringTo(message, detail::NullSocketAddress{}, flags);
}

void Socket::SendStringTo(const std::string& message, const SocketAddress& address, int32_t flags) {
    ProcessError(TrySendStringTo(message, address, flags), "Send");
}

void Socket::SendString(const std::string& message, int32_t flags) {
    ProcessError(TrySendString(message, flags), "Send");
}

std::string Socket::ReceiveAtMostNBytesFrom(size_t n_bytes, SocketAddress* address, int32_t flags) {
    std::string result(n_bytes + 1, '\0');

    sockaddr* address_ptr = nullptr;
    socklen_t address_size = 0;
    if (address != nullptr) {
        address_ptr = address->MutableSockAddrPtr();
        address_size = address->Size();
    }

    ssize_t bytes_received =
        recvfrom(socket_fd_, result.data(), n_bytes, flags, address_ptr, &address_size);
    if (bytes_received < 0) {
        ProcessError(GetLastError(), "Receive");
    }
    result.resize(bytes_received);
    return result;
}

std::string Socket::ReceiveAtMostNBytes(size_t n_bytes, int32_t flags) {
    return ReceiveAtMostNBytesFrom(n_bytes, nullptr, flags);
}

std::string Socket::ReceiveNBytesFrom(size_t n_bytes, SocketAddress* address, int32_t flags) {
    std::string result;
    result.reserve(n_bytes + 1);
    while (result.size() != n_bytes) {
        result += ReceiveAtMostNBytesFrom(n_bytes - result.size(), address, flags);
    }
    return result;
}

std::string Socket::ReceiveNBytes(size_t n_bytes, int32_t flags) {
    return ReceiveNBytesFrom(n_bytes, nullptr, flags);
}

void Socket::ShutDown(int flags) {
    shutdown(socket_fd_, flags);
}

void Socket::Close() {
    close(std::exchange(socket_fd_, -1));
}

Socket::~Socket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

}  // namespace cppnet
