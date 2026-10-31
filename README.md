# AP-Generator

**AP-Generator** is an advanced C++17 software for the creation, configuration, and automated management of Wi-Fi Access Points with network sharing (NAT), DHCP/DNS services, preset management, and more.

It offers both an Interactive Shell (REPL) in the terminal with a dedicated prompt, banner, and Wizard, as well as a traditional command-line interface for scripts and automation.

📖 For the in-depth documentation and guidance on each action, see the [User and Technical Manual (docs/DOCUMENTATION.md)](docs/DOCUMENTATION.md).

---

## Current Project Status (v1.1-alpha / Linux Target)

* [x] **Interactive Shell (REPL) & Wizard**: Automatic startup with a dedicated prompt (`ap-generator: `), dynamic command management, and guided setup (`wizard`).
* [x] **Multi-language Support (i18n)**: Complete dictionaries in **Italian (`it`)** and **English (`en`)**, automatic `LANG` detection, and runtime switching (`language it|en`).
* [x] **Discovery & Capabilities**: Automatic detection of Wi-Fi interfaces (`/sys/class/net`, `iw phy`), 2.4 GHz / 5 GHz channels, AP support, WPA2, WPA3 (SAE), and AP+STA concurrency.
* [x] **Access Point Engine**: Integration and dynamic generation of `hostapd` configurations (Open, WPA2, WPA3, WPA2/WPA3 Mixed, custom channels, and BSSID).
* [x] **Network Services & RFC 8908**: IP management, `dnsmasq` orchestration for authoritative DHCP and DNS, DHCP option 114 (Captive Portal Architecture) for instant popup on smartphones and devices.
* [x] **Internet Sharing & NAT**: Kernel IP forwarding (`sysctl`), routing, and masquerade with `iptables` / `nftables` using a dedicated upstream interface (e.g. `eth0`, `wlan0`).
* [x] **Native Captive Portal**: Asynchronous non-blocking C++ HTTP server, DNS/firewall interception and redirect, captive detection endpoints for Android devices (`/generate_204`), Windows, and iOS.
* [x] **13 Included Portal Templates**: Ready-to-use HTML/CSS templates (Google, Amazon, Microsoft, TP-Link, Starlink, Facebook, Apple, Starbucks, Twitch, Twitter).
* [x] **Preset Manager**: Save, load, export, import, and validate configurations in JSON format.
* [x] **Advanced Monitoring & Clients**: AP status tracking, uptime, DHCP lease reading, and station statistics via `iw station dump` (MAC, IP, Hostname, Signal dBm, RX/TX bytes).
* [x] **Client Kick & MAC Blacklist**: Forced disconnection of stations (`kick <mac>`) and management of a MAC blacklist (`blacklist <add|del|list>`).
* [x] **Test Suite (6/6 Passed)**: Native C++ integration tests for preset, hostapd, DNS, HTTP captive portal, i18n, and blacklist.

---

## System Architecture

```text
                           AP-Generator
                                │
                   ┌────────────▼────────────┐
                   │  Interactive REPL Shell │ (Wizard, i18n EN/IT)
                   └────────────┬────────────┘
                                │
                        ┌───────▼───────┐
                        │   ApManager   │ (Core Engine & Validation)
                        └───────┬───────┘
                                │
          ┌─────────────────────┴─────────────────────┐
          │                                           │
   ┌──────▼──────┐                             ┌──────▼──────┐
   │    Linux    │ (Active)                   │   Windows   │ (Planned)
   │   Backend   │                             │   Backend   │
   └──────┬──────┘                             └──────┬──────┘
          │
  ┌───────┼───────────────────┬───────────────────┐
  ▼       ▼                   ▼                   ▼
nl80211 hostapd            dnsmasq             iptables /
  / iw   (Wi-Fi AP)      (DHCP & DNS)           nftables (NAT)
```

---

## System Requirements (Linux)

### Runtime Tools and Network Services Required:
* **Linux Kernel** with support for mac80211 / nl80211
* **hostapd**: For managing Master/Access Point mode
* **dnsmasq**: For the local DHCP and DNS server
* **iptables** / **nftables**: For NAT masquerade management and port 80 redirection
* **iproute2** (`ip` command): For configuring link and IP addresses
* **iw**: For advanced inspection of frequencies, channels, and connected stations

### Build Dependencies:
* **C++17 compiler** (GCC 9+ or Clang 10+)
* **Make** or **CMake** 3.16+

---

## Build and Test Execution

1. **Quick build with Makefile:**
   ```bash
   make
   ```

2. **Run the test suite (6 integration tests):**
   ```bash
   make test
   # or ./run_tests
   ```

---

## Usage

### Mode 1: Interactive Shell (REPL)

Launching `sudo ./ap-generator` without arguments enters the **Interactive Shell**:

```text
ap-generator: 
```

From the shell, you can type commands continuously:

```bash
ap-generator: wizard              # Starts the interactive guided setup
ap-generator: interfaces          # Shows Wi-Fi interfaces
ap-generator: capabilities wlan0  # Shows interface details and channels
ap-generator: preset list         # Lists saved presets
ap-generator: start LabHotspot    # Starts the Access Point
ap-generator: status              # Shows status, uptime, and active clients
ap-generator: clients             # Shows the client table with dBm signal and traffic
ap-generator: kick aa:bb:cc:dd:ee # Disconnects a client
ap-generator: blacklist add <mac> # Adds a MAC to the blacklist
ap-generator: portal list         # Lists captive portal templates
ap-generator: portal test Google_Modern # Previews a web template
ap-generator: language en         # Switches language to English (or 'language it')
ap-generator: stop                # Stops the Access Point
ap-generator: exit                # Exits the application
```

---

### Mode 2: Traditional Command-Line (One-Shot CLI)

For scripts, automation, or direct execution:

```bash
# Show Wi-Fi interfaces
./ap-generator interfaces

# Guided setup or quick startup from a preset
sudo ./ap-generator start LabHotspot

# Client and status checks
./ap-generator status
./ap-generator clients

# Forced client disconnection
sudo ./ap-generator kick 00:11:22:33:44:55

# Language management from CLI
./ap-generator --lang en help
```

---

### Guided Setup (`wizard`)

The `wizard` command guides you step by step through configuration:
1. Numeric selection of the Wi-Fi interface.
2. Wi-Fi network name (SSID).
3. Radio channel selection (2.4 GHz / 5 GHz).
4. Security mode: Open, WPA2-Personal, WPA3-Personal.
5. Internet sharing (NAT) and upstream interface.
6. Captive portal enablement and guest portal template selection.
7. Optional save as a reusable preset.
8. Immediate start of the Access Point.

---

### Included Captive Portal Templates (`portal/presets/`)

AP-Generator includes 13 ready-to-use templates:
* `Google_Modern`, `Better_Google_Mobile`
* `Amazon`, `Apple`, `Microsoft`, `Facebook`, `Twitter`, `Twitch`
* `Starlink`, `Starbucks` (before/after)
* `TP-LINK-Var1`, `TP-LINK-Var2-OLD`

Local preview:
```bash
./ap-generator portal test Starlink --port 8080
```

---

## Development Roadmap

```text
[==================================>   ] 85% Complete
```

### ✅ Completed
- [x] Modular C++17 core engine and Logging system
- [x] Interactive REPL shell with dedicated prompt and built-in commands
- [x] Interactive step-by-step guided setup (`wizard`)
- [x] Full multi-language support (Italian / English)
- [x] Wi-Fi interface discovery and inspection (channels, bands, WPA3, concurrency)
- [x] Dynamic `hostapd.conf` generation (Open, WPA2, WPA3 SAE, Mixed)
- [x] DHCP/DNS orchestration (`dnsmasq`) with RFC 8908 DHCP option 114
- [x] NAT masquerade routing (`iptables` / `nftables`)
- [x] Native non-blocking HTTP captive portal with 13 included templates
- [x] Station monitoring (`iw station dump` for RSSI and RX/TX traffic)
- [x] Client kick (`hostapd_cli deauthenticate`) and MAC blacklist
- [x] JSON preset manager (save, load, delete, validate, export)
- [x] Automated integration test suite (6 tests)

### 🔜 Next Steps
- [ ] Daemon Watchdog / Process Supervisor mode (auto-recovery in case of crashes)
- [ ] Systemd Unit File creation (`systemctl start ap-generator@preset`)
- [ ] Graphical User Interface (GUI with Qt or a local Web Dashboard)
- [ ] Windows backend implementation (`WindowsWifiBackend` with Native Wifi API)

---

## License

This project is distributed under the **MIT** license. See the `LICENSE` file for more details.
