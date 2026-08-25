# AP-Generator

**AP-Generator** è un software avanzato in C++17 per la creazione, configurazione e gestione automatizzata di Access Point Wi-Fi con condivisione di rete (NAT), servizi DHCP/DNS, gestione preset, Captive Portal personalizzabile e monitoraggio avanzato dei client.

Offre sia una **Shell Interattiva (REPL)** da terminale con prompt dedicato, banner e **Procedura Guidata (Wizard)**, sia un'interfaccia a riga di comando tradizionale per script e automazioni, con supporto **multilingua nativo (Italiano / Inglese)**.

📖 **Per la documentazione approfondita e la guida a ciascuna azione, consulta il [Manuale Utente e Tecnico (docs/DOCUMENTATION.md)](docs/DOCUMENTATION.md).**

---

## Stato Attuale del Progetto (v1.1-alpha / Linux Target)

* [x] **Shell Interattiva (REPL) & Wizard**: Avvio automatico con prompt dedicato (`ap-generator: `), gestione comandi dinamica e configurazione guidata (`wizard`).
* [x] **Supporto Multi-lingua (i18n)**: Dizionari completi in **Italiano (`it`)** e **Inglese (`en`)**, rilevamento automatico `LANG` e switch a runtime (`language it|en`).
* [x] **Discovery & Capabilities**: Rilevamento automatico schede Wi-Fi (`/sys/class/net`, `iw phy`), canali 2.4 GHz / 5 GHz, supporto AP, WPA2, WPA3 (SAE) e AP+STA concurrency.
* [x] **Motore Access Point**: Integrazione e generazione configurazioni dinamiche per `hostapd` (Open, WPA2, WPA3, WPA2/WPA3 Mixed, canali e BSSID custom).
* [x] **Servizi di Rete & RFC 8908**: Gestione IP, orchestrazione di `dnsmasq` per DHCP autoritativo e server DNS, opzione DHCP 114 (Captive Portal Architecture) per popup istantaneo su smartphone.
* [x] **Condivisione Internet & NAT**: Abilitazione IP forwarding kernel (`sysctl`), routing e masquerade `iptables` / `nftables` con upstream interface dedicata (es. `eth0`, `wlan0`).
* [x] **Captive Portal Nativo**: Server HTTP asincrono non-bloccante C++, intercettazione e redirect DNS/Firewall, endpoint di captive detection per dispositivi Android (`/generate_204`), Windows (`/ncsi.txt`) e Apple (`/hotspot-detect.html`), autorizzazione dinamica client.
* [x] **13 Template Portal Inclusi**: Template HTML/CSS pronti all'uso (Google, Amazon, Microsoft, TP-Link, Starlink, Facebook, Apple, Starbucks, Twitch, Twitter).
* [x] **Preset Manager**: Salvataggio, caricamento, esportazione, importazione e validazione di configurazioni in formato JSON.
* [x] **Monitoraggio Avanzato & Client**: Tracciamento stato AP, uptime, lettura leases DHCP e statistiche stazioni via `iw station dump` (MAC, IP, Hostname, Segnale dBm, Byte RX/TX).
* [x] **Client Kick & MAC Blacklist**: Disconnessione forzata stazioni (`kick <mac>`) e gestione lista nera MAC address (`blacklist <add|del|list>`).
* [x] **Suite di Test (6/6 Passati)**: Test di integrazione nativi C++ per preset, hostapd, DNS, HTTP captive portal, i18n e blacklist.

---

## Architettura del Sistema

```text
                           AP-Generator
                                │
                   ┌────────────▼────────────┐
                   │  Interactive REPL Shell │ (Wizard, i18n IT/EN)
                   └────────────┬────────────┘
                                │
                        ┌───────▼───────┐
                        │   ApManager   │ (Core Engine & Validation)
                        └───────┬───────┘
                                │
          ┌─────────────────────┴─────────────────────┐
          │                                           │
   ┌──────▼──────┐                             ┌──────▼──────┐
   │    Linux    │ (Attivo)                    │   Windows   │ (Pianificato)
   │   Backend   │                             │   Backend   │
   └──────┬──────┘                             └──────┬──────┘
          │
  ┌───────┼───────────────────┬───────────────────┐
  ▼       ▼                   ▼                   ▼
nl80211 hostapd            dnsmasq             iptables /
 / iw   (Wi-Fi AP)      (DHCP & DNS)           nftables (NAT)
```

---

## Requisiti di Sistema (Linux)

### Strumenti e Servizi di Rete Richiesti a Runtime:
* **Linux Kernel** con supporto a mac80211 / nl80211
* **hostapd**: Per la gestione della modalità Master/Access Point
* **dnsmasq**: Per il server DHCP e DNS locale
* **iptables** / **nftables**: Per la gestione NAT Masquerade e redirect porta 80
* **iproute2** (`ip` command): Per la configurazione di link e indirizzi IP
* **iw**: Per l'ispezione avanzata delle frequenze, canali e stazioni connesse

### Dipendenze di Build:
* **Compilatore C++17** (GCC 9+ o Clang 10+)
* **Make** o **CMake** 3.16+

---

## Compilazione ed Esecuzione Test

1. **Compilazione rapida con Makefile:**
   ```bash
   make
   ```

2. **Esecuzione della suite di test (6 test di integrazione):**
   ```bash
   make test
   # oppure ./run_tests
   ```

---

## Utilizzo

### Modalità 1: Shell Interattiva (REPL)

Avviando `sudo ./ap-generator` senza argomenti, si entra nella **Shell Interattiva**:

```text
ap-generator: 
```

Dalla shell è possibile digitare i comandi in modo continuo:

```bash
ap-generator: wizard              # Avvia la procedura guidata interattiva
ap-generator: interfaces          # Mostra le schede Wi-Fi
ap-generator: capabilities wlan0  # Mostra dettagli e canali della scheda
ap-generator: preset list         # Elenca i preset salvati
ap-generator: start LabHotspot    # Avvia l'Access Point
ap-generator: status              # Mostra stato, uptime e client attivi
ap-generator: clients             # Mostra tabella client con segnale dBm e traffico
ap-generator: kick aa:bb:cc:dd:ee # Disconnette un client
ap-generator: blacklist add <mac> # Aggiunge un MAC alla lista nera
ap-generator: portal list         # Elenca i template captive portal
ap-generator: portal test Google_Modern # Anteprima web template
ap-generator: language en         # Cambia lingua in Inglese (o 'language it')
ap-generator: stop                # Arresta l'Access Point
ap-generator: exit                # Esci dall'applicazione
```

---

### Modalità 2: Riga di Comando Tradizionale (One-Shot CLI)

Per script, automazioni o esecuzione diretta:

```bash
# Mostra le interfacce Wi-Fi
./ap-generator interfaces

# Procedura guidata o avvio rapido da preset
sudo ./ap-generator start LabHotspot

# Controllo client e stato
./ap-generator status
./ap-generator clients

# Disconnessione forzata client
sudo ./ap-generator kick 00:11:22:33:44:55

# Gestione lingua da CLI
./ap-generator --lang en help
```

---

### Procedura Guidata (`wizard`)

Il comando `wizard` guida passo-passo nella configurazione:
1. Selezione numerica dell'interfaccia Wi-Fi.
2. Nome della rete Wi-Fi (SSID).
3. Selezione canale radio (2.4 GHz / 5 GHz).
4. Modalità di sicurezza: Open, WPA2-Personal, WPA3-Personal.
5. Condivisione Internet (NAT) e interfaccia upstream.
6. Abilitazione Captive Portal e selezione template grafico.
7. Salvataggio opzionale come Preset riutilizzabile.
8. Avvio immediato dell'Access Point.

---

### Template Captive Portal Inclusi (`portal/presets/`)

AP-Generator include 13 template pronti:
* `Google_Modern`, `Better_Google_Mobile`
* `Amazon`, `Apple`, `Microsoft`, `Facebook`, `Twitter`, `Twitch`
* `Starlink`, `Starbucks` (before/after)
* `TP-LINK-Var1`, `TP-LINK-Var2-OLD`

Anteprima locale:
```bash
./ap-generator portal test Starlink --port 8080
```

---

## Roadmap di Sviluppo

```text
[==================================>   ] 85% Completato
```

### ✅ Completato
- [x] Motore Core C++17 modulare e sistema di Logging
- [x] Shell Interattiva (REPL) con prompt dedicato e comandi integrati
- [x] Procedura guidata interattiva passo-passo (`wizard`)
- [x] Supporto Multi-lingua completo (Italiano / Inglese)
- [x] Ispezione e discovery interfacce Wi-Fi (canali, bande, WPA3, concurrency)
- [x] Generazione dinamica `hostapd.conf` (Open, WPA2, WPA3 SAE, Mixed)
- [x] Orchestrazione DHCP/DNS (`dnsmasq`) con RFC 8908 DHCP option 114
- [x] Routing NAT Masquerade (`iptables` / `nftables`)
- [x] Captive Portal HTTP nativo non-bloccante con 13 template inclusi
- [x] Monitoraggio stazioni (`iw station dump` per segnale RSSI e traffico RX/TX)
- [x] Client kick (`hostapd_cli deauthenticate`) e MAC Blacklist
- [x] Preset Manager JSON (salva, carica, elimina, valida, esporta)
- [x] Suite di test di integrazione automatizzata (6 test)

### 🔜 Prossimi Passi
- [ ] Modalità Demone Watchdog / Process Supervisor (auto-ripristino in caso di crash)
- [ ] Creazione Unit File Systemd (`systemctl start ap-generator@preset`)
- [ ] Interfaccia Grafica Utente (GUI con Qt o Web Dashboard locale)
- [ ] Implementazione Backend Windows (`WindowsWifiBackend` con Native Wifi API)

---

## Licenza

Questo progetto è distribuito sotto licenza **MIT**. Consulta il file `LICENSE` per ulteriori dettagli.
