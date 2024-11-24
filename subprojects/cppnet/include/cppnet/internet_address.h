#pragma once

#include <string>
#include <type_traits>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "socket.h"

namespace cppnet {

template <AddressFamilyType>
struct InternetAddress;

namespace detail {

template <AddressFamilyType>
struct POSIXInternetAddress {
    UNSUPPORTED_ADDRESS_FAMILY;
};

template <>
struct POSIXInternetAddress<AddressFamily::IPv4> {
    using Type = in_addr;
    using NumberType = uint32_t;
};

template <>
struct POSIXInternetAddress<AddressFamily::IPv6> {
    using Type = in6_addr;
    using NumberType = __uint128_t;
};

template <AddressFamilyType>
struct POSIXInetSockAddr {
    UNSUPPORTED_ADDRESS_FAMILY;
};

template <>
class POSIXInetSockAddr<AddressFamily::IPv4> : sockaddr_in {
public:
    POSIXInetSockAddr<AddressFamily::IPv4>();
    POSIXInetSockAddr<AddressFamily::IPv4>(in_port_t port, in_addr address);

    sa_family_t GetAdddressFamily() const;
    const in_addr& GetAddress() const;
    in_port_t GetPort() const;

    void SetAddressFamily(sa_family_t address_family);
    void SetPort(in_port_t port);
    void SetAddress(const in_addr& address);

private:
    const sockaddr_in* GetPOSIXStructure() const;
    sockaddr_in* GetPOSIXStructure();
};

template <>
class POSIXInetSockAddr<AddressFamily::IPv6> : sockaddr_in6 {
public:
    POSIXInetSockAddr();
    POSIXInetSockAddr(in_port_t port, in6_addr address);

    sa_family_t GetAdddressFamily() const;
    const in6_addr& GetAddress() const;
    in_port_t GetPort() const;

    void SetAddressFamily(sa_family_t address_family);
    void SetPort(in_port_t port);
    void SetAddress(const in6_addr& address);

private:
    const sockaddr_in6* GetPOSIXStructure() const;
    sockaddr_in6* GetPOSIXStructure();
};

template <AddressFamilyType AF>
InternetAddress<AF> AddressToNetworkFormat(const std::string& address) {
    InternetAddress<AF> result;
    int status = inet_pton(AF, address.c_str(), &result);
    if (status != 1) {
        if (status == 0) {
            throw std::runtime_error("Invalid network address: " + address);
        }
        throw std::runtime_error(std::string("Address to network format: ") + std::strerror(errno));
    }
    return result;
}

template <AddressFamilyType AF>
std::string NetworkFormatToAddress(const InternetAddress<AF>& internet_address) {
    char cstr_result[INET6_ADDRSTRLEN];
    if (inet_ntop(AF, &internet_address, cstr_result, INET6_ADDRSTRLEN) == nullptr) {
        throw std::runtime_error(std::string("Network format to address: ") + std::strerror(errno));
    }
    return std::string(cstr_result);
}

}  // namespace detail

template <AddressFamilyType AF>
struct InternetAddress : detail::POSIXInternetAddress<AF>::Type {
public:
    using NumberType = detail::POSIXInternetAddress<AF>::NumberType;

public:
    InternetAddress() {
        memset(this, 0, sizeof(*this));
    }

    template <typename T>
    InternetAddress(T address)
        requires std::is_convertible_v<T, std::string>
        : InternetAddress(detail::AddressToNetworkFormat<AF>(address)) {
    }

    InternetAddress(NumberType number) {
        memcpy(this, &number, sizeof(number));
    }

    std::string AsString() const {
        return detail::NetworkFormatToAddress<AF>(*this);
    }

    NumberType AsNumber() const {
        NumberType result;
        memcpy(&result, this, sizeof(result));
        return result;
    }
};

template <AddressFamilyType AF>
struct AnyAddress {
    static const InternetAddress<AF> Value;
};

template <AddressFamilyType AF>
const InternetAddress<AF> AnyAddress<AF>::Value = 0;

template <AddressFamilyType AF>
class InternetSocketAddress : public SocketAddress {
public:
    InternetSocketAddress() = default;
    InternetSocketAddress(const InternetAddress<AF> address, uint16_t port)
        : socket_address_(htons(port), address) {
    }

    const InternetAddress<AF>& GetAddress() const {
        return *reinterpret_cast<const InternetAddress<AF>*>(&socket_address_.GetAddress());
    }

    uint16_t GetPort() const {
        return ntohs(socket_address_.GetPort());
    }

    void SetAddress(const InternetAddress<AF>& address) {
        socket_address_.SetAddress(address);
    }

    void SetPort(uint16_t port) {
        socket_address_.SetPort(htons(port));
    }

protected:
    const sockaddr* SockAddrPtr() const override {
        return reinterpret_cast<const sockaddr*>(&socket_address_);
    }

    sockaddr* MutableSockAddrPtr() override {
        return reinterpret_cast<sockaddr*>(&socket_address_);
    }

    uint32_t Size() const override {
        return sizeof(socket_address_);
    }

private:
    detail::POSIXInetSockAddr<AF> socket_address_;
};

}  // namespace cppnet
