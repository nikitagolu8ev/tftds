project('cppnet', 'cpp', default_options: ['cpp_std=c++23'])

srcs = files(
    'src/error.cpp',
    'src/event.cpp',
    'src/event_poller.cpp',
    'src/interface_info.cpp',
    'src/internet_address.cpp',
    'src/ip_multicast.cpp',
    'src/socket.cpp',
    'src/time.cpp',
)

cppnet_inc = include_directories('include')
cppnet_lib = static_library('cppnet_lib', srcs)

cppnet_dep = declare_dependency(
    link_with: cppnet_lib,
    include_directories: cppnet_inc,
)
