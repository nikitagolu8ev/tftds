#pragma once

#include "internet_address.h"

namespace cppnet {

enum class IPMulticastAction : uint8_t {
    Join = 1,
    Leave = 2,
};

namespace detail {

template <AddressFamilyType>
struct IPMReq {
    UNSUPPORTED_ADDRESS_FAMILY;
};

template <>
struct IPMReq<AddressFamilyType::IPv4> {
    using Type = ip_mreqn;

    static Type Construct(const in_addr& multicast_address, const in_addr& local_interface_address,
                          uint32_t interface_index) {
        Type result;
        result.imr_multiaddr = multicast_address;
        result.imr_address = local_interface_address;
        result.imr_ifindex = interface_index;
        return result;
    }
};

template <>
struct IPMReq<AddressFamilyType::IPv6> {
    using Type = ipv6_mreq;

    static Type Construct(const in6_addr& multicast_address, uint32_t interface_index) {
        Type result;
        result.ipv6mr_multiaddr = multicast_address;
        result.ipv6mr_interface = interface_index;
        return result;
    }
};

template <AddressFamilyType AF>
using IPMReqType = IPMReq<AF>::Type;

};  // namespace detail

template <AddressFamilyType AF>
class IPMulticastRequest : detail::IPMReqType<AF> {
    using IPMR = detail::IPMReq<AF>;
    using IPMRType = detail::IPMReqType<AF>;

public:
    IPMulticastRequest(const InternetAddress<AF>& multicast_address,
                       const InternetAddress<AF>& local_interface_address = AnyAddress<AF>::Value,
                       uint32_t interface_index = 0)
        requires(AF == AddressFamilyType::IPv4)
        : IPMRType(IPMR::Construct(multicast_address, local_interface_address, interface_index)) {
        static_assert(sizeof(IPMulticastRequest<AF>) == sizeof(detail::IPMReqType<AF>),
                      "Inheritance didn't work");
    }

    IPMulticastRequest(const InternetAddress<AF>& multicast_address, uint32_t interface_index = 0)
        requires(AF == AddressFamilyType::IPv6)
        : IPMRType(IPMR::Construct(multicast_address, interface_index)) {
    }
};

}  // namespace cppnet
