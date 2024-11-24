#include "include/cppnet/event_poller.h"
#include "include/cppnet/error.h"

#include <sys/event.h>

namespace cppnet {

EventPoller::EventPoller() : kq_(kqueue()) {
    if (kq_ < 0) {
        ProcessError(GetLastError(), "Event poller");
    }
}

}
