cmake_minimum_required(VERSION 3.16)
project(ap-generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


set(APGEN_CORE_SOURCES
    src/main.cpp
    src/core/ap_manager.cpp
    src/core/preset_manager.cpp
    src/network/dhcp.cpp
    src/network/dns.cpp
    src/network/nat.cpp
    src/network/interface.cpp
    src/portal/http_server.cpp
    src/portal/captive_portal.cpp
)


if(UNIX AND NOT APPLE)
    list(APPEND APGEN_CORE_SOURCES
        src/linux/wifi_backend.cpp
        src/linux/hostapd.cpp
        src/linux/network.cpp
        src/linux/firewall.cpp
    )
endif()

add_executable(ap-generator ${APGEN_CORE_SOURCES})

target_include_directories(ap-generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(ap-generator PRIVATE
    -Wall -Wextra
)