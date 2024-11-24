#pragma once

#include "socket.h"

#include <cstdint>
#include <chrono>
#include <memory>

#include <sys/event.h>

namespace cppnet {

class EventPoller;

class Event {
    friend class EventPoller;

public:
    enum class Type : uint32_t {
        Socket,
        Timer,
    };

public:
    virtual Type GetType() const = 0;

    virtual ~Event() = default;

private:
    virtual struct kevent GetEvent(int flags) const = 0;
};

class SocketEvent : public Event {
public:
    enum class Type {
        Accept,
        ConnectionClosed,
        Receive,
        Send,
    };

public:
    SocketEvent(Socket socket, SocketEvent::Type type);

    Type GetSocketEventType() const;
    void SetSocketEventType(Type type);
    Event::Type GetType() const override;

    Socket& GetSocket();
    const Socket& GetSocket() const;

private:
    struct kevent GetEvent(int flags) const override;

private:
    Socket socket_;
    Type type_;
    bool is_closed_{false};
};

class TimerEvent : public Event {
public:
    TimerEvent(std::chrono::milliseconds duration);

    Event::Type GetType() const override;

private:
    struct kevent GetEvent(int flags) const override;

private:
    std::chrono::milliseconds::rep duration_;
};

std::shared_ptr<SocketEvent> CreateSocketEvent(Socket sock, SocketEvent::Type type);
std::shared_ptr<TimerEvent> CreateTimerEvent(std::chrono::milliseconds duration);

}  // namespace cppnet
