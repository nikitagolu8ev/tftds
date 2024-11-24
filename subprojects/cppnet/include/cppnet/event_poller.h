#pragma once

#include "event.h"

#include <map>

#include <unistd.h>

namespace cppnet {

class EventPoller {
    using IteratorType = std::map<std::shared_ptr<Event>, size_t>::iterator;

public:
    EventPoller();

    void Add(std::shared_ptr<Event> event) {
        struct kevent kev = event->GetEvent(EV_ADD | EV_DISABLE);
        auto [iterator, inserted] = storage_.emplace(std::move(event), 1);
        if (!inserted) {
            ++iterator->second;
        }
        kev.udata = IteratorToPointer(iterator);

        if (kevent(kq_, &kev, 1, nullptr, 0, nullptr) < 0) {
            ProcessError(GetLastError(), "EventPoller::Add");
        }
    }

    void AddAndEnable(std::shared_ptr<Event> event) {
        struct kevent kev = event->GetEvent(EV_ADD | EV_ENABLE);
        auto [iterator, inserted] = storage_.emplace(std::move(event), 1);
        if (!inserted) {
            ++iterator->second;
        }
        kev.udata = IteratorToPointer(iterator);

        if (kevent(kq_, &kev, 1, nullptr, 0, nullptr) < 0) {
            ProcessError(GetLastError(), "EventPoller::AddAndEnable");
        }
    }

    void Enable(std::shared_ptr<Event> event) {
        struct kevent kev = event->GetEvent(EV_ENABLE);
        IteratorType iterator = storage_.find(std::move(event));
        if (iterator == storage_.end()) {
            throw std::runtime_error("EventPoller::Enable: Event wasn't added");
        }

        kev.udata = IteratorToPointer(iterator);

        if (kevent(kq_, &kev, 1, nullptr, 0, nullptr) < 0) {
            ProcessError(GetLastError(), "EventPoller::Enable");
        }
    }

    void Disable(std::shared_ptr<Event> event) {
        IteratorType iterator = storage_.find(event);
        if (iterator == storage_.end()) {
            throw std::runtime_error("EventPoller::Disable: Event wasn't added");
        }

        struct kevent kev = event->GetEvent(EV_DISABLE);
        kev.udata = IteratorToPointer(iterator);

        if (kevent(kq_, &kev, 1, nullptr, 0, nullptr) < 0) {
            ProcessError(GetLastError(), "EventPoller::Enable");
        }
    }

    void Delete(std::shared_ptr<Event> event) {
        struct kevent kev = event->GetEvent(EV_DELETE | EV_DISABLE);

        IteratorType iterator = storage_.find(event);
        if (iterator == storage_.end()) {
            throw std::runtime_error("EventPoller::Delete: Event wasn't added");
        }
        
        if (--iterator->second == 0) {
            storage_.erase(iterator);
        }
        
        kev.udata = IteratorToPointer(iterator);

        if (kevent(kq_, &kev, 1, nullptr, 0, nullptr) < 0) {
            ProcessError(GetLastError(), "EventPoller::Delete");
        }
    }

    std::shared_ptr<Event> Poll() {
        struct kevent result;
        int status;
        do {
            if (status = kevent(kq_, nullptr, 0, &result, 1, nullptr); status < 0) {
                ProcessError(GetLastError(), "EventPoller::Poll");
            }
        } while (status != 1);

        std::shared_ptr event = PointerToIterator(result.udata)->first;

        if (event->GetType() == Event::Type::Socket) {
            if (result.flags & EV_EOF) {
                dynamic_cast<SocketEvent*>(event.get())
                    ->SetSocketEventType(SocketEvent::Type::ConnectionClosed);
            }
        }

        return event;
    }

    ~EventPoller() {
        close(kq_);
    }

private:
    void* IteratorToPointer(IteratorType iterator) {
        static_assert(sizeof(iterator) == sizeof(void*));
        return *reinterpret_cast<void**>(&iterator);
    }

    IteratorType PointerToIterator(void* pointer) {
        static_assert(sizeof(pointer) == sizeof(IteratorType));
        return *reinterpret_cast<IteratorType*>(&pointer);
    }

private:
    std::map<std::shared_ptr<Event>, size_t> storage_;
    int kq_;
};

}  // namespace cppnet
