CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES = -Iinclude -Isrc/linux
LIBS = -pthread
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

# Colors for terminal output
GREEN  = \033[1;32m
YELLOW = \033[1;33m
CYAN   = \033[1;36m
RESET  = \033[0m

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
	@echo ""
	@echo "========================================================================="
	@echo "$(GREEN)✓ BUILD COMPLETATA CON SUCCESSO!$(RESET)"
	@echo "========================================================================="
	@echo "File eseguibili generati nella cartella del progetto:"
	@echo "  $(CYAN)• ./ap-generator$(RESET)   (Programma principale e Shell interattiva)"
	@echo "  $(CYAN)• ./run_tests$(RESET)      (Suite di test di integrazione)"
	@echo ""
	@echo "Per avviare AP-Generator:"
	@echo "  $(YELLOW)sudo ./ap-generator$(RESET)         (Avvia la Shell Interattiva REPL)"
	@echo "  $(YELLOW)./ap-generator interfaces$(RESET)   (Visualizza le schede Wi-Fi)"
	@echo "  $(YELLOW)make test$(RESET)                   (Esegue la suite di test)"
	@echo "========================================================================="
	@echo ""

ap-generator: $(SRCS)
	@echo "$(CYAN)[1/2] Compilazione eseguibile principale AP-Generator...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o ap-generator $(LIBS)
	@mkdir -p bin && cp ap-generator bin/
	@echo "$(GREEN)  -> Creato binario: ./ap-generator (e bin/ap-generator)$(RESET)"

run_tests: $(TEST_SRCS)
	@echo "$(CYAN)[2/2] Compilazione suite di test di integrazione...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) -o run_tests $(LIBS)
	@mkdir -p bin && cp run_tests bin/
	@echo "$(GREEN)  -> Creato binario: ./run_tests (e bin/run_tests)$(RESET)"

test: run_tests
	@echo ""
	@echo "$(CYAN)Esecuzione test automatici...$(RESET)"
	@./run_tests

install: ap-generator
	@echo "Installazione in $(BINDIR)..."
	@install -d $(BINDIR)
	@install -m 755 ap-generator $(BINDIR)/ap-generator
	@echo "$(GREEN)Installato con successo! Ora puoi digitare direttamente 'ap-generator' da qualsiasi percorso.$(RESET)"

uninstall:
	@rm -f $(BINDIR)/ap-generator
	@echo "Disinstallato."

clean:
	@rm -f ap-generator run_tests
	@rm -rf bin
	@echo "File binari rimossi."

.PHONY: all test clean install uninstall
