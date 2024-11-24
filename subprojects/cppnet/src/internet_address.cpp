#include "include/cppnet/internet_address.h"

namespace cppnet::detail {

POSIXInetSockAddr<AddressFamily::IPv4>::POSIXInetSockAddr() {
    memset(this, 0, sizeof(*this));
}
POSIXInetSockAddr<AddressFamily::IPv4>::POSIXInetSockAddr(in_port_t port, in_addr address) {
    memset(this, 0, sizeof(*this));
    GetPOSIXStructure()->sin_family = AddressFamily::IPv4;
    GetPOSIXStructure()->sin_port = port;
    GetPOSIXStructure()->sin_addr = address;
}

sa_family_t POSIXInetSockAddr<AddressFamily::IPv4>::GetAdddressFamily() const {
    return GetPOSIXStructure()->sin_family;
}

const in_addr& POSIXInetSockAddr<AddressFamily::IPv4>::GetAddress() const {
    return GetPOSIXStructure()->sin_addr;
}

in_port_t POSIXInetSockAddr<AddressFamily::IPv4>::GetPort() const {
    return GetPOSIXStructure()->sin_port;
}

void POSIXInetSockAddr<AddressFamily::IPv4>::SetAddressFamily(sa_family_t address_family) {
    GetPOSIXStructure()->sin_family = address_family;
}

void POSIXInetSockAddr<AddressFamily::IPv4>::SetPort(in_port_t port) {
    GetPOSIXStructure()->sin_port = port;
}

void POSIXInetSockAddr<AddressFamily::IPv4>::SetAddress(const in_addr& address) {
    GetPOSIXStructure()->sin_addr = address;
}

const sockaddr_in* POSIXInetSockAddr<AddressFamily::IPv4>::GetPOSIXStructure() const {
    return reinterpret_cast<const sockaddr_in*>(this);
}

sockaddr_in* POSIXInetSockAddr<AddressFamily::IPv4>::GetPOSIXStructure() {
    return reinterpret_cast<sockaddr_in*>(this);
}

POSIXInetSockAddr<AddressFamily::IPv6>::POSIXInetSockAddr() {
    memset(this, 0, sizeof(*this));
}

POSIXInetSockAddr<AddressFamily::IPv6>::POSIXInetSockAddr(in_port_t port, in6_addr address) {
    memset(this, 0, sizeof(*this));
    GetPOSIXStructure()->sin6_family = AddressFamily::IPv6;
    GetPOSIXStructure()->sin6_port = port;
    GetPOSIXStructure()->sin6_addr = address;
}

sa_family_t POSIXInetSockAddr<AddressFamily::IPv6>::GetAdddressFamily() const {
    return GetPOSIXStructure()->sin6_family;
}

const in6_addr& POSIXInetSockAddr<AddressFamily::IPv6>::GetAddress() const {
    return GetPOSIXStructure()->sin6_addr;
}

in_port_t POSIXInetSockAddr<AddressFamily::IPv6>::GetPort() const {
    return GetPOSIXStructure()->sin6_port;
}

void POSIXInetSockAddr<AddressFamily::IPv6>::SetAddressFamily(sa_family_t address_family) {
    GetPOSIXStructure()->sin6_family = address_family;
}

void POSIXInetSockAddr<AddressFamily::IPv6>::SetPort(in_port_t port) {
    GetPOSIXStructure()->sin6_port = port;
}

void POSIXInetSockAddr<AddressFamily::IPv6>::SetAddress(const in6_addr& address) {
    GetPOSIXStructure()->sin6_addr = address;
}

const sockaddr_in6* POSIXInetSockAddr<AddressFamily::IPv6>::GetPOSIXStructure() const {
    return reinterpret_cast<const sockaddr_in6*>(this);
}

sockaddr_in6* POSIXInetSockAddr<AddressFamily::IPv6>::GetPOSIXStructure() {
    return reinterpret_cast<sockaddr_in6*>(this);
}

}  // namespace cppnet::detail
