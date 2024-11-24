#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <netinet/tcp.h>

#include <cppnet/error.h>
#include <cppnet/event_poller.h>
#include <cppnet/interface_info.h>
#include <cppnet/internet_address.h>
#include <cppnet/socket.h>

#include "common.h"
#include "random_loss.h"

using namespace cppnet;
using namespace std::chrono_literals;

class Client {
public:
    Client(uint16_t port) : Client(AnyAddress<AddressFamily::IPv4>::Value, port) {
    }

    Client(const InternetAddress<AddressFamily::IPv4>& address, uint16_t port)
        : client_address_(address, port),
          broadcast_address_(*GetInterfaceInfo("en0")->GetIPv4Addresses()->broadcast_address,
                             defaults::BroadcastPort) {
        client_socket_.SetOption(SOL_SOCKET, SO_REUSEADDR, 1);
        client_socket_.Bind(client_address_);
        broadcast_socket_.SetOption(SOL_SOCKET, SO_BROADCAST, 1);
    }

    double Integrate(double left, double right, double delta) && {
        InitTasks(left, right, delta);
        client_socket_.Listen();
        poller_.AddAndEnable(CreateTimerEvent(1s));
        poller_.AddAndEnable(
            CreateSocketEvent(std::move(client_socket_), SocketEvent::Type::Accept));

        double result = 0;
        size_t finished_tasks = 0;

        DiscoverWorkers();

        while (finished_tasks != tasks_.size()) {
            auto event = poller_.Poll();
            if (event->GetType() == Event::Type::Socket) {
                SocketEvent* socket_event = dynamic_cast<SocketEvent*>(event.get());

                if (socket_event->GetSocketEventType() == SocketEvent::Type::Accept) {
                    std::cout << "New worker discovered\n";
                    auto socket = socket_event->GetSocket().Accept();
                    socket.SetOption(SOL_SOCKET, SO_KEEPALIVE, 1);
                    socket.SetOption(IPPROTO_TCP, TCP_KEEPALIVE, 1);
                    socket.SetOption(IPPROTO_TCP, TCP_KEEPINTVL, 1);
                    socket.SetOption(IPPROTO_TCP, TCP_KEEPCNT, 2);

                    auto event = CreateSocketEvent(std::move(socket), SocketEvent::Type::Receive);
                    poller_.Add(event);
                    event->SetSocketEventType(SocketEvent::Type::Send);
                    poller_.AddAndEnable(event);

                } else if (socket_event->GetSocketEventType() == SocketEvent::Type::Send) {
                    poller_.Disable(event);
                    socket_event->SetSocketEventType(SocketEvent::Type::Receive);
                    poller_.Enable(event);
                    ProcessReadySocket(event);

                } else if (socket_event->GetSocketEventType() == SocketEvent::Type::Receive) {
                    poller_.Disable(event);
                    socket_event->SetSocketEventType(SocketEvent::Type::Send);
                    poller_.Enable(event);
                    ResultType<TaskResponse> response =
                        socket_event->GetSocket().TryReceiveObject<TaskResponse>(MSG_DONTWAIT);
                    if (!response || ShouldBeLost()) {
                        ProcessBadSocket(event);
                        continue;
                    }
                    if (!task_is_finished_[response->identifier - 1]) {
                        task_is_finished_[response->identifier - 1] = true;
                        result += response->result;
                        ++finished_tasks;
                    }

                } else if (socket_event->GetSocketEventType() ==
                           SocketEvent::Type::ConnectionClosed) {
                    ProcessBadSocket(event);
                }

            } else if (event->GetType() == Event::Type::Timer) {
                DiscoverWorkers();
            }
        }
        return result;
    }

private:
    void InitTasks(double left, double right, double delta) {
        uint32_t current_task_id = 0;
        while (left < right) {
            tasks_.emplace_back(++current_task_id, left, std::min(left + delta, right));
            left += delta;
        }
        pending_tasks_ = tasks_;
        task_is_finished_.assign(tasks_.size(), false);
    }

    void DiscoverWorkers() {
        std::cout << "Discovering\n";
        if (!ShouldBeLost()) {
            broadcast_socket_.SendObjectTo<uint16_t>(client_address_.GetPort(), broadcast_address_);
        }
    }

    void ProcessBadSocket(std::shared_ptr<Event> socket_event) {
        std::cout << "Bad socket\n";
        SocketEvent* socket_event_ptr = dynamic_cast<SocketEvent*>(socket_event.get());
        socket_event_ptr->SetSocketEventType(SocketEvent::Type::Send);
        poller_.Delete(socket_event);
        socket_event_ptr->SetSocketEventType(SocketEvent::Type::Receive);
        poller_.Delete(socket_event);

        if (uint32_t task_id = task_of_socket_[socket_event];
            task_id != 0 && !task_is_finished_[task_id - 1]) {
            ProcessNewTask(tasks_[task_id - 1]);
        }
        task_of_socket_.erase(socket_event);
    }

    void ProcessNewTask(Task task) {
        if (ready_sockets_.empty()) {
            pending_tasks_.emplace_back(task);
            return;
        }
        std::shared_ptr<Event> event = ready_sockets_.back();
        ready_sockets_.pop_back();
        ProcessSocketWithTask(event, task);
    }

    void ProcessReadySocket(std::shared_ptr<Event> socket_event) {
        if (pending_tasks_.empty()) {
            ready_sockets_.emplace_back(socket_event);
            return;
        }
        Task task = pending_tasks_.back();
        pending_tasks_.pop_back();
        ProcessSocketWithTask(socket_event, task);
    }

    void ProcessSocketWithTask(std::shared_ptr<Event> socket_event, Task task) {
        SocketEvent* socket_event_ptr = dynamic_cast<SocketEvent*>(socket_event.get());
        task_of_socket_[socket_event] = task.identifier;
        if (ShouldBeLost() || socket_event_ptr->GetSocket().TrySendObject<Task>(task)) {
            ProcessBadSocket(socket_event);
        }
    }

private:
    InternetSocketAddress<AddressFamily::IPv4> client_address_;
    InternetSocketAddress<AddressFamily::IPv4> broadcast_address_;

    Socket client_socket_{AddressFamily::IPv4, Socket::Type::TCP};
    Socket broadcast_socket_{AddressFamily::IPv4, Socket::Type::UDP};

    std::vector<Task> tasks_;
    std::vector<Task> pending_tasks_;
    std::vector<bool> task_is_finished_;
    std::unordered_map<std::shared_ptr<Event>, uint32_t> task_of_socket_;

    std::vector<std::shared_ptr<Event>> ready_sockets_;

    EventPoller poller_;
};

int main(int argc, char* argv[]) {
    if (argc != 4 && argc != 5) {
        std::cout << "Invalid arguments\nUsage: " << argv[0]
                  << " left_border right_border task_size [loss_probability]\n";
        return 1;
    }

    double left = std::stod(argv[1]);
    double right = std::stod(argv[2]);
    double delta = std::stod(argv[3]);

    if (argc == 5) {
        EnableRandomLoss(std::stod(argv[4]));
    }

    Client client(defaults::ConnectionPort);
    std::cout << std::fixed << std::setprecision(15)
              << std::move(client).Integrate(left, right, delta) << '\n';
    return 0;
}
