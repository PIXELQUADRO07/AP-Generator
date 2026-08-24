cmake_minimum_required(VERSION 3.16)
project(ap-generator LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()


#source multiplatform of the core...

set(APGEN_CORE_SOURCES
    src/main.cpp 
    src/core/ap_manager.cpp 
    src/core/preset_manager.cpp 
    src/network/dhcp.cpp 
    src/network/dns.cpp 
    src/network/nat.cpp 
    src/network/interface.cpp 
    src/portal/http_serve.cpp 
    src/portal/captive_portal.cpp 
)


# specific sources for Linux backend (only target available now)

if(UNIX AND NOT APPLE)
   list(APPEND APGEN_CORE_SOURCES
        src/linux/wifi_backend.cpp 
        src/linux/wifi_capabilities.cpp 
        src/linux/hostapd.cpp 
        src/linux/network.cpp 
        src/linux/firewall.cpp
   )
endif()

add_executable(ap-generator ${APGEN_CORE_SOURCES})

target_include_directories(ap-generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)



#wifi_capabilities.cpp query the kernel via nl80211 


if(UNIX AND NOT APPLE)
   find_pacakge(PkgConfig REQUIRED)
   pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

     target_include_directories(ap-generator PRIVATE ${LIBNL_INCLUDE_DIRS})
    target_link_libraries(ap-generator PRIVATE ${LIBNL_LIBRARIES})
    target_compile_options(ap-generator PRIVATE ${LIBNL_CFLAGS_OTHER})
endif()  


target_compile_options(ap-generator PRIVATE
    -Wall -Wextra
)