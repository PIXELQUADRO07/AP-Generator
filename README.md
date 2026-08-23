# AP-Generator

**AP-Generator** is a C++ tool for creating, configuring, and managing Wi-Fi Access Points using an existing network connection.

The project is designed to provide a unified interface for configuring wireless networks without requiring the user to manually configure multiple networking components.

The first development target is **GNU/Linux**, with a **Windows backend planned for a future release**.

---

## Overview

AP-Generator is designed around a modular architecture that separates the application core from operating-system-specific networking functionality.

The main goal is to make Access Point creation as simple as:

```text
Discover → Configure → Create → Manage
```

The application can detect available Wi-Fi interfaces, inspect their capabilities, create an Access Point, optionally share an existing Internet connection, and provide additional services such as DHCP, DNS, NAT, and a captive portal.

---

## Main Features

### Wi-Fi Interface Discovery

AP-Generator can detect Wi-Fi interfaces available on the system and retrieve information about their capabilities.

The discovery system is designed to provide information such as:

* Interface name
* MAC address
* Interface state
* Connection status
* Current network
* IP configuration
* Access Point support
* Supported wireless bands
* Available channels
* WPA2 support
* WPA3 support
* Station/AP concurrency support

Example:

```text
Wi-Fi Interfaces
────────────────────────────────

wlan0
  MAC: XX:XX:XX:XX:XX:XX
  State: UP
  Connected: YES
  AP mode: YES
  WPA3: YES
  Bands: 2.4 GHz / 5 GHz

wlan1
  MAC: XX:XX:XX:XX:XX:XX
  State: DOWN
  Connected: NO
  AP mode: YES
  WPA3: NO
```

Capability detection is important because wireless adapters and drivers do not all support the same functionality.

---

# Access Point Creation

AP-Generator allows users to create a Wi-Fi Access Point using a compatible wireless interface.

The Access Point configuration can include:

* SSID
* Wireless interface
* Channel
* BSSID
* Security mode
* Password
* Internet sharing
* Captive portal

Example:

```text
Access Point Configuration

SSID:        MyNetwork
Interface:   wlan1
Channel:     6
BSSID:       Automatic

Security:    WPA2
Password:    ********

Internet sharing: YES
Captive portal:   NO
```

Once configured, AP-Generator handles the required networking components automatically.

---

# Wireless Security

AP-Generator is designed to support several wireless security modes.

## Open

An open network does not require a Wi-Fi password.

```text
Security: OPEN
```

This mode is useful for environments where authentication is handled by another mechanism, such as a captive portal.

---

## WPA2-Personal

Standard WPA2-Personal authentication using a pre-shared key.

```text
Security: WPA2
Password: ********
```

---

## WPA2/WPA3 Mixed

A mixed configuration allows compatible clients to use WPA3 while maintaining compatibility with clients that only support WPA2.

```text
Security: WPA2/WPA3
```

Availability depends on the wireless adapter, driver, and underlying networking stack.

---

## WPA3-Personal

WPA3-Personal can be used when supported by the hardware and software environment.

AP-Generator is designed to detect capabilities before enabling unsupported security configurations.

---

# BSSID

AP-Generator can support automatic or manually specified BSSID configuration.

```text
BSSID:
  Automatic
  Custom: XX:XX:XX:XX:XX:XX
```

Custom BSSID functionality depends on the capabilities of the wireless adapter and driver.

---

# Internet Sharing

One of the main features of AP-Generator is the ability to use an existing Internet connection as the upstream connection for the newly created Access Point.

For example:

```text
                    Internet
                       │
                       ▼
                ┌─────────────┐
                │ Wi-Fi Router│
                └──────┬──────┘
                       │
                       ▼
                  wlan0 / STA
                       │
                       │
                  AP-Generator
                       │
                       │ NAT
                       ▼
                  wlan1 / AP
                       │
              ┌────────┼────────┐
              ▼        ▼        ▼
           Laptop    Phone    Tablet
```

The upstream connection and the Access Point can potentially operate on the same physical wireless adapter when the hardware and driver support simultaneous Station/AP operation.

AP-Generator will detect this capability instead of assuming that every adapter supports it.

---

# DHCP and DNS

AP-Generator can provide the network services required by connected clients.

A typical AP network could use:

```text
Gateway:
192.168.50.1

Network:
192.168.50.0/24

Clients:
192.168.50.2
192.168.50.3
192.168.50.4
...
```

DHCP can provide clients with:

* IP address
* Subnet mask
* Default gateway
* DNS server
* Lease information

DNS functionality can also be used as part of the captive portal system.

---

# NAT and Routing

When Internet sharing is enabled, AP-Generator can configure routing and Network Address Translation between the upstream interface and the Access Point.

Example:

```text
Internet
   │
   ▼
Upstream Interface
   │
   ▼
  NAT
   │
   ▼
AP Interface
   │
   ▼
192.168.50.0/24
```

The Linux implementation is intended to integrate with the native Linux networking and firewall infrastructure.

---

# Captive Portal

AP-Generator is designed to provide an optional captive portal for Open Access Points.

A captive portal can present a local web page to clients before granting normal network access.

Example:

```text
Client
  │
  │ Connects to AP
  ▼
DHCP
  │
  ▼
DNS
  │
  ▼
Captive Portal
  │
  ▼
Login / Welcome Page
  │
  ▼
Authorized Client
  │
  ▼
Internet
```

The portal can be customized using standard web technologies.

Example:

```text
portal/
└── myportal/
    ├── index.html
    ├── style.css
    └── script.js
```

Users can create their own portal interface without modifying the AP-Generator source code.

---

# Custom Captive Portals

AP-Generator is intended to support custom HTML-based captive portals.

A portal may contain:

* HTML
* CSS
* JavaScript
* Images
* Other static web resources

Example:

```bash
ap-generator portal load ./myportal
```

This makes it possible to create custom interfaces for:

* Guest networks
* Private laboratories
* Events
* Educational environments
* IoT networks
* Development environments
* Network testing

AP-Generator does not require or intend to intercept HTTPS traffic. Captive portal functionality is designed to operate using standard captive-network mechanisms.

---

# Client Management

AP-Generator is planned to provide information about clients connected to the Access Point.

Potential information includes:

* Client MAC address
* Assigned IP address
* Connection state
* Connection time
* Traffic statistics
* DHCP lease information

Example:

```text
Connected Clients
────────────────────────────────────

MAC Address          IP Address
XX:XX:XX:XX:XX:01    192.168.50.10
XX:XX:XX:XX:XX:02    192.168.50.11
XX:XX:XX:XX:XX:03    192.168.50.12
```

---

# Presets

AP-Generator can store Access Point configurations as reusable presets.

Example:

```json
{
    "name": "Lab",
    "ssid": "MyLab",
    "channel": 6,
    "security": "wpa2",
    "interface": "wlan1",
    "internet_sharing": true,
    "captive_portal": false
}
```

Presets can be used to quickly recreate previously configured networks.

Example commands:

```bash
ap-generator preset list
ap-generator preset save lab
ap-generator preset load lab
ap-generator preset delete lab
```

Presets can also be exported and imported for use on other systems.

---

# Command Line Interface

The initial version of AP-Generator is designed around a CLI.

## List Wi-Fi Interfaces

```bash
ap-generator interfaces
```

Displays the Wi-Fi interfaces detected by AP-Generator.

---

## Show Interface Capabilities

```bash
ap-generator capabilities wlan0
```

Displays the capabilities detected for a specific interface.

---

## Start an Access Point

```bash
ap-generator start
```

Starts the configured Access Point.

---

## Stop an Access Point

```bash
ap-generator stop
```

Stops the currently running Access Point and cleans up the networking configuration.

---

## Show Status

```bash
ap-generator status
```

Displays the current state of the Access Point and connected clients.

---

## Presets

```bash
ap-generator preset list
ap-generator preset save lab
ap-generator preset load lab
```

---

## Captive Portal

```bash
ap-generator portal start
ap-generator portal stop
ap-generator portal load ./myportal
```

---

# Architecture

AP-Generator uses a platform-independent core with operating-system-specific backends.

```text
                    AP-Generator
                         │
                ┌────────▼────────┐
                │   Core Engine   │
                └────────┬────────┘
                         │
          ┌──────────────┴──────────────┐
          │                             │
   ┌──────▼──────┐               ┌──────▼──────┐
   │    Linux    │               │   Windows   │
   │   Backend   │               │   Backend   │
   └──────┬──────┘               └──────┬──────┘
          │                             │
   Linux networking              Windows networking
```

The core should not directly depend on operating-system-specific networking APIs.

Instead, platform-specific implementations communicate with the core through abstract interfaces.

---

# Linux Backend

GNU/Linux is the initial development target.

The Linux backend is expected to integrate with existing Linux networking components.

The planned architecture is:

```text
                  AP-Generator
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
     nl80211        hostapd         Network
        │                             │
        │                          dnsmasq
        │                             │
        └──────────────┬──────────────┘
                       ▼
                    nftables
```

## nl80211

The Linux wireless subsystem can be used to obtain wireless interface information and capabilities.

## hostapd

`hostapd` is intended to manage Access Point functionality such as:

* SSID
* Channel
* Authentication
* WPA2
* WPA3
* Client association
* AP operation

## dnsmasq

`dnsmasq` can provide:

* DHCP
* DNS

for the Access Point network.

## nftables

`nftables` can be used for:

* NAT
* Forwarding
* Firewall rules
* Network isolation
* Captive portal traffic policies

The exact implementation may evolve during development.

---

# Project Structure

The project is organized to keep platform-independent functionality separate from operating-system-specific code.

```text
AP-Generator/
├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── include/
│   └── apgen/
│       ├── core/
│       │   ├── types.hpp
│       │   ├── wifi_backend.hpp
│       │   ├── ap_manager.hpp
│       │   └── preset_manager.hpp
│       │
│       ├── network/
│       │   ├── dhcp.hpp
│       │   ├── dns.hpp
│       │   ├── nat.hpp
│       │   └── interface.hpp
│       │
│       └── portal/
│           ├── captive_portal.hpp
│           └── http_server.hpp
│
├── src/
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── ap_manager.cpp
│   │   └── preset_manager.cpp
│   │
│   ├── linux/
│   │   ├── wifi_backend.cpp
│   │   ├── hostapd.cpp
│   │   ├── network.cpp
│   │   └── firewall.cpp
│   │
│   ├── network/
│   │   ├── dhcp.cpp
│   │   ├── dns.cpp
│   │   ├── nat.cpp
│   │   └── interface.cpp
│   │
│   └── portal/
│       ├── captive_portal.cpp
│       └── http_server.cpp
│
├── config/
│   └── presets/
│
├── portal/
│   └── default/
│       ├── index.html
│       ├── style.css
│       └── script.js
│
├── tests/
│
└── docs/
```

---

# C++ API

The core is designed around abstract interfaces.

For example:

```cpp
class WifiBackend {
public:
    virtual ~WifiBackend() = default;

    virtual std::vector<WifiInterface>
    discover_interfaces() = 0;

    virtual bool create_ap(
        const AccessPointConfig& config
    ) = 0;

    virtual bool stop_ap() = 0;
};
```

The Linux implementation can provide:

```text
LinuxWifiBackend
LinuxAPBackend
LinuxNetworkBackend
```

A future Windows implementation can provide:

```text
WindowsWifiBackend
WindowsAPBackend
WindowsNetworkBackend
```

This allows the core application to remain independent of the operating system.

---

# Project Design Goals

AP-Generator follows several design principles.

## Platform Independence

The application core should not depend on a specific operating system.

## Capability Detection

The application should detect hardware and driver capabilities rather than assuming that every adapter supports every feature.

## Modular Architecture

Wi-Fi management, networking, DHCP, DNS, NAT, firewall management, and captive portal functionality should remain separate components.

## Reuse Existing Infrastructure

Where appropriate, AP-Generator should orchestrate existing and well-tested networking components instead of unnecessarily reimplementing mature networking functionality.

## Configuration Driven

Network configurations should be stored in structured configuration files and reusable presets.

## CLI First

The initial development will focus on a command-line interface.

A graphical interface can be added later without requiring a redesign of the core.

---

# Requirements

The Linux version is expected to require:

* GNU/Linux
* A compatible Wi-Fi adapter
* A driver supporting Access Point mode
* C++ compiler
* CMake
* Appropriate privileges for network configuration

Additional dependencies may be required depending on the selected implementation.

---

# Building

AP-Generator uses CMake.

Clone the repository:

```bash
git clone https://github.com/<username>/AP-Generator.git
cd AP-Generator
```

Create a build directory:

```bash
mkdir build
cd build
```

Configure the project:

```bash
cmake ..
```

Build:

```bash
cmake --build . -j$(nproc)
```

Installation instructions will be added as the project develops.

---

# Typical Workflow

A typical AP-Generator workflow is:

```text
┌─────────────────────┐
│ Discover Interfaces │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Detect Capabilities │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Select Wi-Fi Device │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Configure AP        │
│ SSID / Channel /    │
│ Security / BSSID    │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Start Access Point  │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Configure DHCP/DNS  │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Optional NAT        │
│ / Internet Sharing  │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Optional Captive    │
│ Portal              │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Monitor Clients     │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Stop & Clean Up     │
└─────────────────────┘
```

---

# Development Roadmap

## Phase 1 — Core

* [ ] C++ project structure
* [ ] CMake build system
* [ ] Logging
* [ ] Configuration system
* [ ] CLI
* [ ] Wi-Fi interface discovery

## Phase 2 — Access Point

* [ ] Open Access Point
* [ ] WPA2-Personal
* [ ] WPA2/WPA3 mixed mode
* [ ] WPA3-Personal
* [ ] SSID configuration
* [ ] Channel configuration
* [ ] BSSID configuration
* [ ] AP start/stop management

## Phase 3 — Network Services

* [ ] DHCP
* [ ] DNS
* [ ] IP forwarding
* [ ] NAT
* [ ] Firewall management
* [ ] Network cleanup

## Phase 4 — Internet Sharing

* [ ] Detect upstream interface
* [ ] Configure routing
* [ ] Configure NAT
* [ ] Connection status
* [ ] Automatic cleanup

## Phase 5 — Captive Portal

* [ ] Embedded HTTP server
* [ ] Custom HTML portals
* [ ] DNS integration
* [ ] Captive portal detection handling
* [ ] Client authorization
* [ ] Portal templates

## Phase 6 — Presets

* [ ] Save configurations
* [ ] Load configurations
* [ ] Delete configurations
* [ ] Import configurations
* [ ] Export configurations

## Phase 7 — Client Management

* [ ] Connected client detection
* [ ] DHCP lease information
* [ ] Connection statistics
* [ ] Traffic statistics
* [ ] Client management

## Phase 8 — Graphical Interface

* [ ] Cross-platform GUI
* [ ] Interface selection
* [ ] AP configuration
* [ ] Client monitoring
* [ ] Captive portal management
* [ ] Preset management

## Phase 9 — Windows Backend

* [ ] Windows Wi-Fi discovery
* [ ] Windows capability detection
* [ ] Windows AP implementation
* [ ] Windows networking
* [ ] Windows Internet sharing
* [ ] Cross-platform testing

---

# Security and Intended Use

AP-Generator is intended for networks, devices, and infrastructure that the user owns or is explicitly authorized to administer.

Examples of intended use include:

* Personal Wi-Fi hotspots
* Guest networks
* Private laboratories
* Development environments
* Educational demonstrations
* IoT testing
* Local network experiments
* Controlled networking environments

AP-Generator does not require or intend to intercept encrypted HTTPS traffic.

Users are responsible for ensuring that their network configuration complies with applicable laws, regulations, and network policies.

---

# Long-Term Vision

The long-term goal of AP-Generator is to provide a unified and easy-to-use interface for Access Point management across GNU/Linux and Windows.

The intended workflow is:

```text
                 AP-Generator

                      │
                      ▼
                 DISCOVER
                      │
                      ▼
                CONFIGURE
                      │
                      ▼
                  CREATE
                      │
             ┌────────┴────────┐
             ▼                 ▼
        INTERNET           LOCAL ONLY
        SHARING
             │
             ▼
       CAPTIVE PORTAL
             │
             ▼
       MANAGE CLIENTS
             │
             ▼
          SAVE AS
          PRESET
```

The Linux implementation will serve as the foundation of the project. Once the Linux backend is stable, additional platform backends can be implemented while keeping the same core architecture.

---

# Status

**Current development target: GNU/Linux**

AP-Generator is currently in development. Features listed in the roadmap may not yet be implemented.

The project is being developed incrementally, starting with Wi-Fi interface discovery and Access Point creation before moving toward networking services, Internet sharing, captive portals, presets, and eventually Windows support.

---

# License

The project license has not yet been selected.
