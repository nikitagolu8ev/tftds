#include <algorithm>
#include <array>

#include "include/cppnet/error.h"
#include "include/cppnet/interface_info.h"

#include <net/if.h>

namespace cppnet {

std::string InterfaceInfo::GetName() const {
    return name_;
}

std::optional<InterfaceAddresses<AddressFamily::IPv4>> InterfaceInfo::GetIPv4Addresses() {
    return ipv4_addresses_;
}

std::optional<InterfaceAddresses<AddressFamily::IPv6>> InterfaceInfo::GetIPv6Addresses() {
    return ipv6_addresses_;
}

void InterfaceInfo::SetIPv4Addresses(
    const InterfaceAddresses<AddressFamily::IPv4>& ipv4_addresses) {
    ipv4_addresses_ = ipv4_addresses;
}

void InterfaceInfo::SetIPv6Addresses(
    const InterfaceAddresses<AddressFamily::IPv6>& ipv6_addresses) {
    ipv6_addresses_ = ipv6_addresses;
}

std::vector<InterfaceInfo> GetInterfaceInfos() {
    static constexpr std::array<AddressFamilyType, 2> SupportedAddressFamilies{AddressFamily::IPv4,
                                                                               AddressFamily::IPv6};

    ifaddrs* interface_addresses;
    if (getifaddrs(&interface_addresses) == -1) {
        ProcessError(GetLastError(), "Interfaces info");
    }

    std::vector<InterfaceInfo> interface_infos;

    for (ifaddrs* current_interface = interface_addresses; current_interface != nullptr;
         current_interface = current_interface->ifa_next) {
        if (current_interface->ifa_name == nullptr || current_interface->ifa_addr == nullptr ||
            !std::ranges::contains(SupportedAddressFamilies,
                                   current_interface->ifa_addr->sa_family)) {
            continue;
        }

        std::vector<InterfaceInfo>::iterator info_it =
            std::ranges::find_if(interface_infos, [&](const InterfaceInfo& info) {
                return info.GetName() == current_interface->ifa_name;
            });

        if (info_it == interface_infos.end()) {
            interface_infos.emplace_back(current_interface->ifa_name);
            info_it = std::prev(interface_infos.end());
        }

        if (current_interface->ifa_addr->sa_family == AddressFamily::IPv4) {
            InterfaceAddresses<AddressFamily::IPv4> addresses;
            addresses.address =
                reinterpret_cast<sockaddr_in*>(current_interface->ifa_addr)->sin_addr.s_addr;

            if (current_interface->ifa_netmask) {
                addresses.netmask =
                    reinterpret_cast<sockaddr_in*>(current_interface->ifa_netmask)->sin_addr.s_addr;
            }

            if (current_interface->ifa_flags & IFF_BROADCAST) {
                addresses.broadcast_address =
                    reinterpret_cast<sockaddr_in*>(current_interface->ifa_broadaddr)
                        ->sin_addr.s_addr;
            }

            if (current_interface->ifa_flags & IFF_POINTOPOINT) {
                addresses.destination_address =
                    reinterpret_cast<sockaddr_in*>(current_interface->ifa_dstaddr)->sin_addr.s_addr;
            }

            info_it->SetIPv4Addresses(addresses);

        } else if (current_interface->ifa_addr->sa_family == AddressFamily::IPv6) {
            InterfaceAddresses<AddressFamily::IPv6> addresses;
            addresses.address = reinterpret_cast<__uint128_t>(
                reinterpret_cast<sockaddr_in6*>(current_interface->ifa_addr)->sin6_addr.s6_addr);

            if (current_interface->ifa_netmask) {
                addresses.netmask = reinterpret_cast<__uint128_t>(
                    reinterpret_cast<sockaddr_in6*>(current_interface->ifa_netmask)
                        ->sin6_addr.s6_addr);
            }

            if (current_interface->ifa_flags & IFF_BROADCAST) {
                addresses.broadcast_address = reinterpret_cast<__uint128_t>(
                    reinterpret_cast<sockaddr_in6*>(current_interface->ifa_broadaddr)
                        ->sin6_addr.s6_addr);
            }

            if (current_interface->ifa_flags & IFF_POINTOPOINT) {
                addresses.destination_address = reinterpret_cast<__uint128_t>(
                    reinterpret_cast<sockaddr_in6*>(current_interface->ifa_dstaddr)
                        ->sin6_addr.s6_addr);
            }

            info_it->SetIPv6Addresses(addresses);
        }
    }

    return interface_infos;
}

std::optional<InterfaceInfo> GetInterfaceInfo(const std::string& name) {
    std::vector<InterfaceInfo> interface_infos = GetInterfaceInfos();
    std::vector<InterfaceInfo>::iterator info_it = std::ranges::find_if(
        interface_infos, [&](const InterfaceInfo& info) { return info.GetName() == name; });

    if (info_it == interface_infos.end()) {
        return std::nullopt;
    }
    return *info_it;
}

}  // namespace cppnet
