#include <chrono>
#include <iostream>
#include <thread>

#include <netinet/tcp.h>

#include <cppnet/internet_address.h>
#include <cppnet/ip_multicast.h>
#include <cppnet/socket.h>
#include <cppnet/time.h>

#include "common.h"
#include "integral.h"
#include "random_loss.h"

using namespace std::chrono_literals;

void Serve() {
    while (true) {
        cppnet::Socket broadcast_socket(cppnet::AddressFamily::IPv4, cppnet::Socket::Type::UDP);

        cppnet::InternetSocketAddress<cppnet::AddressFamily::IPv4> any_local_address(
            cppnet::AnyAddress<cppnet::AddressFamily::IPv4>::Value, defaults::BroadcastPort);

        broadcast_socket.SetOption(SOL_SOCKET, SO_REUSEADDR, 1);
        broadcast_socket.SetOption(SOL_SOCKET, SO_REUSEPORT, 1);

        broadcast_socket.Bind(any_local_address);

        std::cout << "Waiting for broacast\n";

        cppnet::InternetSocketAddress<cppnet::AddressFamily::IPv4> master_address;
        uint16_t port = broadcast_socket.ReceiveObjectFrom<uint16_t>(&master_address);
        
        if (ShouldBeLost()) {
            std::cout << "Skipped discovery\n";
            continue;
        }

        master_address.SetPort(port);

        std::cout << "Received address: " << master_address.GetAddress().AsString() << ':'
                  << master_address.GetPort() << '\n';

        cppnet::InternetSocketAddress worker_address(
            cppnet::AnyAddress<cppnet::AddressFamily::IPv4>::Value, port);
        cppnet::Socket worker_socket(cppnet::AddressFamily::IPv4, cppnet::Socket::Type::TCP);

        worker_socket.SetOption(SOL_SOCKET, SO_KEEPALIVE, 1);
        worker_socket.SetOption(IPPROTO_TCP, TCP_KEEPALIVE, 1);
        worker_socket.SetOption(IPPROTO_TCP, TCP_KEEPINTVL, 1);
        worker_socket.SetOption(IPPROTO_TCP, TCP_KEEPCNT, 2);

        worker_socket.Connect(master_address);

        std::cout << "Connected\n";

        while (true) {
            std::cout << "================================\n";
            std::cout << "Receiving task...\n";
            ResultType<Task> task = worker_socket.TryReceiveObject<Task>();
            if (ShouldBeLost() || !task) {
                std::cout << "Task wasn't received\n";
                break;
            }
            double result = NumericIntegration(task->left, task->right);
            // For testing
            // std::this_thread::sleep_for(300ms);
            TaskResponse response{.identifier = task->identifier, .result = result};
            std::cout << "Sending response...\n";
            if (ShouldBeLost() || worker_socket.TrySendObject(response)) {
                std::cout << "Response wasn't sent\n";
                break;
            }
            std::cout << "Successful\n";
        }
        std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 1 && argc != 2) {
        std::cout << "Invalid arguments\nUsage: " << argv[0]
                  << " [loss_probability]\n";
        return 1;
    }
    if (argc == 2) {
        EnableRandomLoss(std::stod(argv[1]));
    }
    Serve();
    return 0;
}
