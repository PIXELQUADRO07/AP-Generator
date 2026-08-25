CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES = -Iinclude -Isrc/linux
LIBS = -pthread
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

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
	@printf "\n"
	@printf "\033[1;32m=========================================================================\033[0m\n"
	@printf "\033[1;32m  ✓ BUILD COMPLETATA CON SUCCESSO!\033[0m\n"
	@printf "\033[1;32m=========================================================================\033[0m\n"
	@printf "File eseguibili generati nella cartella del progetto:\n"
	@printf "  \033[1;36m• ./ap-generator\033[0m   (Programma principale e Shell interattiva)\n"
	@printf "  \033[1;36m• ./run_tests\033[0m      (Suite di test di integrazione)\n"
	@printf "\n"
	@printf "Per avviare AP-Generator:\n"
	@printf "  \033[1;33msudo ./ap-generator\033[0m         (Avvia la Shell Interattiva REPL)\n"
	@printf "  \033[1;33m./ap-generator interfaces\033[0m   (Visualizza le schede Wi-Fi)\n"
	@printf "  \033[1;33mmake test\033[0m                   (Esegue la suite di test)\n"
	@printf "\033[1;32m=========================================================================\033[0m\n"
	@printf "\n"

ap-generator: $(SRCS)
	@printf "\033[1;36m[1/2] Compilazione eseguibile principale AP-Generator...\033[0m\n"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o ap-generator $(LIBS)
	@mkdir -p bin && cp ap-generator bin/
	@printf "\033[1;32m  -> Creato binario: ./ap-generator (e bin/ap-generator)\033[0m\n"

run_tests: $(TEST_SRCS)
	@printf "\033[1;36m[2/2] Compilazione suite di test di integrazione...\033[0m\n"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) -o run_tests $(LIBS)
	@mkdir -p bin && cp run_tests bin/
	@printf "\033[1;32m  -> Creato binario: ./run_tests (e bin/run_tests)\033[0m\n"

test: run_tests
	@printf "\n"
	@printf "\033[1;36mEsecuzione test automatici...\033[0m\n"
	@./run_tests

install: ap-generator
	@printf "Installazione in $(BINDIR)...\n"
	@install -d $(BINDIR)
	@install -m 755 ap-generator $(BINDIR)/ap-generator
	@printf "\033[1;32mInstallato con successo! Ora puoi digitare direttamente 'ap-generator' da qualsiasi percorso.\033[0m\n"

uninstall:
	@rm -f $(BINDIR)/ap-generator
	@printf "Disinstallato.\n"

clean:
	@rm -f ap-generator run_tests
	@rm -rf bin
	@printf "File binari rimossi.\n"

.PHONY: all test clean install uninstall

