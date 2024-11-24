#pragma once

#include <optional>
#include <vector>

#include <ifaddrs.h>

#include "internet_address.h"

namespace cppnet {

template <AddressFamilyType AF>
struct InterfaceAddresses {
    InternetAddress<AF> address;
    std::optional<InternetAddress<AF>> netmask;
    std::optional<InternetAddress<AF>> broadcast_address;
    std::optional<InternetAddress<AF>> destination_address;
};

class InterfaceInfo {
public:
    InterfaceInfo(const std::string& name) : name_(name) {
    }

    std::string GetName() const;
    std::optional<InterfaceAddresses<AddressFamily::IPv4>> GetIPv4Addresses();
    std::optional<InterfaceAddresses<AddressFamily::IPv6>> GetIPv6Addresses();

    void SetIPv4Addresses(const InterfaceAddresses<AddressFamily::IPv4>& ipv4_addresses);
    void SetIPv6Addresses(const InterfaceAddresses<AddressFamily::IPv6>& ipv6_addresses);

private:
    std::string name_;
    std::optional<InterfaceAddresses<AddressFamily::IPv4>> ipv4_addresses_;
    std::optional<InterfaceAddresses<AddressFamily::IPv6>> ipv6_addresses_;
};

std::vector<InterfaceInfo> GetInterfaceInfos();
std::optional<InterfaceInfo> GetInterfaceInfo(const std::string& name);

}  // namespace cppnet
