cmake_minimum_required(VERSION 3.16)

project(ap-generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

set(APGEN_SOURCES
    src/main.cpp
    src/core/ap_manager.cpp
    src/core/preset_manager.cpp
    src/network/interface.cpp
    src/network/dhcp.cpp
    src/network/dns.cpp
    src/network/nat.cpp
    src/portal/http_server.cpp
    src/portal/captive_portal.cpp
)

if(UNIX AND NOT APPLE)
    list(APPEND APGEN_SOURCES
        src/linux/wifi_backend.cpp
        src/linux/wifi_capabilities.cpp
        src/linux/hostapd.cpp
        src/linux/network.cpp
        src/linux/firewall.cpp
    )
endif()

add_executable(ap-generator ${APGEN_SOURCES})

target_include_directories(ap-generator
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/linux
)

target_link_libraries(ap-generator
    PRIVATE
        Threads::Threads
)

if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBNL
            libnl-3.0
            libnl-genl-3.0
        )
        if(LIBNL_FOUND)
            target_include_directories(ap-generator PRIVATE ${LIBNL_INCLUDE_DIRS})
            target_link_libraries(ap-generator PRIVATE ${LIBNL_LIBRARIES})
            target_compile_options(ap-generator PRIVATE ${LIBNL_CFLAGS_OTHER})
            target_compile_definitions(ap-generator PRIVATE HAVE_LIBNL=1)
        endif()
    endif()
endif()

target_compile_options(ap-generator
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)