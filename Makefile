CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES = -Iinclude -Isrc/linux
LIBS = -pthread

SRCS = src/main.cpp \
       src/core/ap_manager.cpp \
       src/core/preset_manager.cpp \
       src/core/logger.cpp \
       src/core/i18n.cpp \
       src/network/interface.cpp \
       src/network/dhcp.cpp \
       src/network/dns.cpp \
       src/network/nat.cpp \
       src/portal/http_server.cpp \
       src/portal/captive_portal.cpp \
       src/ui/banner.cpp \
       src/linux/wifi_backend.cpp \
       src/linux/wifi_capabilities.cpp \
       src/linux/hostapd.cpp \
       src/linux/network.cpp \
       src/linux/firewall.cpp \
       src/linux/client_monitor.cpp

TEST_SRCS = tests/test_ap_generator.cpp \
            src/core/ap_manager.cpp \
            src/core/preset_manager.cpp \
            src/core/logger.cpp \
            src/core/i18n.cpp \
            src/network/interface.cpp \
            src/network/dhcp.cpp \
            src/network/dns.cpp \
            src/network/nat.cpp \
            src/portal/http_server.cpp \
            src/portal/captive_portal.cpp \
            src/ui/banner.cpp \
            src/linux/wifi_backend.cpp \
            src/linux/wifi_capabilities.cpp \
            src/linux/hostapd.cpp \
            src/linux/network.cpp \
            src/linux/firewall.cpp \
            src/linux/client_monitor.cpp

all: ap-generator run_tests

ap-generator: $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o ap-generator $(LIBS)

run_tests: $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) -o run_tests $(LIBS)

test: run_tests
	./run_tests

clean:
	rm -f ap-generator run_tests

.PHONY: all test clean
