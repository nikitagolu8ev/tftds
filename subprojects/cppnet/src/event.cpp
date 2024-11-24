#include "include/cppnet/event.h"

namespace cppnet {

SocketEvent::SocketEvent(Socket socket, SocketEvent::Type type)
    : socket_(std::move(socket)), type_(type) {
}

SocketEvent::Type SocketEvent::GetSocketEventType() const {
    if (is_closed_) {
        return SocketEvent::Type::ConnectionClosed;
    }
    return type_;
}

void SocketEvent::SetSocketEventType(Type type) {
    if (type == SocketEvent::Type::ConnectionClosed) {
        is_closed_ = true;
    } else {
        type_ = type;
    }
}

Event::Type SocketEvent::GetType() const {
    return Event::Type::Socket;
}

Socket& SocketEvent::GetSocket() {
    return socket_;
}

const Socket& SocketEvent::GetSocket() const {
    return socket_;
}

struct kevent SocketEvent::GetEvent(int flags) const {
    struct kevent result;
    if (type_ == SocketEvent::Type::Accept || type_ == SocketEvent::Type::Receive ||
        type_ == SocketEvent::Type::ConnectionClosed) {
        EV_SET(&result, socket_.socket_fd_, EVFILT_READ, flags, 0, 0, nullptr);
    } else if (type_ == SocketEvent::Type::Send) {
        EV_SET(&result, socket_.socket_fd_, EVFILT_WRITE, flags, 0, 0, nullptr);
    }
    return result;
}

TimerEvent::TimerEvent(std::chrono::milliseconds duration) : duration_(duration.count()) {
}

Event::Type TimerEvent::GetType() const {
    return Event::Type::Timer;
}

struct kevent TimerEvent::GetEvent(int flags) const {
    struct kevent result;
    EV_SET(&result, 1, EVFILT_TIMER, flags, 0, duration_, nullptr);
    return result;
}

std::shared_ptr<SocketEvent> CreateSocketEvent(Socket socket, SocketEvent::Type type) {
    return std::make_shared<SocketEvent>(std::move(socket), type);
}

std::shared_ptr<TimerEvent> CreateTimerEvent(std::chrono::milliseconds duration) {
    return std::make_shared<TimerEvent>(duration);
}

}  // namespace cppnet
