# Guida Completa e Documentazione Tecnica di AP-Generator

Benvenuto nella documentazione ufficiale di **AP-Generator**. Questo documento spiega nel dettaglio l'architettura del software, come interagisce con il kernel e i demoni di rete Linux, e cosa fa ciascun comando e azione disponibile nel tool.

---

## Indice

1. [Panoramica e Architettura del Sistema](#1-panoramica-e-architettura-del-sistema)
2. [Come Funziona AP-Generator (Sotto il Cofano)](#2-come-funziona-ap-generator-sotto-il-cofano)
   * [2.1 Rilevamento Hardware e Capacità Wi-Fi](#21-rilevamento-hardware-e-capacità-wi-fi)
   * [2.2 Creazione dell'Access Point (hostapd)](#22-creazione-dellaccess-point-hostapd)
   * [2.3 Assegnazione IP e Servizi DHCP/DNS (dnsmasq)](#23-assegnazione-ip-e-servizi-dhcpdns-dnsmasq)
   * [2.4 Condivisione Internet e NAT (iptables / nftables)](#24-condivisione-internet-e-nat-iptables--nftables)
   * [2.5 Captive Portal e Architettura RFC 8908](#25-captive-portal-e-architettura-rfc-8908)
   * [2.6 Monitoraggio Client in Tempo Reale e Blacklist](#26-monitoraggio-client-in-tempo-reale-e-blacklist)
   * [2.7 Sistema di Internazionalizzazione (i18n)](#27-sistema-di-internazionalizzazione-i18n)
3. [Riferimento Completo dei Comandi](#3-riferimento-completo-dei-comandi)
   * [`interfaces` / `ifaces`](#interfaces--ifaces)
   * [`capabilities <interfaccia>`](#capabilities-interfaccia)
   * [`wizard`](#wizard)
   * [`start <preset>`](#start-preset)
   * [`stop`](#stop)
   * [`status`](#status)
   * [`clients`](#clients)
   * [`kick <mac>`](#kick-mac)
   * [`blacklist <list|add|del> [mac]`](#blacklist-listadddel-mac)
   * [`preset list`](#preset-list)
   * [`preset show <nome>`](#preset-show-nome)
   * [`preset save <nome> [opzioni]`](#preset-save-nome-opzioni)
   * [`preset delete <nome>`](#preset-delete-nome)
   * [`validate <nome|file>`](#validate-nomefile)
   * [`portal list`](#portal-list)
   * [`portal test <template> [--port <p>]`](#portal-test-template---port-p)
   * [`language <it|en>`](#language-iten)
   * [`clear`](#clear)
   * [`help` / `?`](#help--)
   * [`exit` / `quit` / `q`](#exit--quit--q)
4. [Risoluzione dei Problemi e FAQ](#4-risoluzione-dei-problemi-e-faq)

---

## 1. Panoramica e Architettura del Sistema

AP-Generator è sviluppato in **C++17** con un approccio ad alta modularità basato su interfacce astratte. Il nucleo centrale (`ApManager`) non dipende dalle API specifiche di un singolo sistema operativo, ma dialoga con un backend intercambiabile (`WifiBackend`):

```text
                        ┌───────────────────────────────┐
                        │   Interfaccia Utente (UI)     │
                        │  Shell REPL / CLI / Wizard    │
                        └───────────────┬───────────────┘
                                        │
                        ┌───────────────▼───────────────┐
                        │          ApManager            │ (Validazione, Preset, i18n)
                        └───────────────┬───────────────┘
                                        │ (Interfaccia Astratta)
                ┌───────────────────────┴───────────────────────┐
                │                                               │
        ┌───────▼───────┐                               ┌───────▼───────┐
        │ Linux Backend │                               │Windows Backend│ (Pianificato)
        └───────┬───────┘                               └───────────────┘
                │
 ┌──────────────┼──────────────┬──────────────┬──────────────┐
 ▼              ▼              ▼              ▼              ▼
Hardware      hostapd       dnsmasq       iptables /     HttpServer
(sysfs/iw)    (Wi-Fi AP)    (DHCP/DNS)    nftables (NAT) (Captive Portal)
```

---

## 2. Come Funziona AP-Generator (Sotto il Cofano)

### 2.1 Rilevamento Hardware e Capacità Wi-Fi
* Esamina `/sys/class/net/` per individuare le interfacce dotate dei descrittori `wireless` o `phy80211`.
* Interroga il sottosistema wireless Linux (`iw phy <phy> info`) per estrarre:
  * Modalità operative supportate (in particolare la modalità `AP`).
  * Supporto a **WPA2** e **WPA3-SAE**.
  * Bande radio e canali consentiti (2.4 GHz e 5 GHz).
  * Supporto alla concorrenza simultanea **AP + Station** (se la stessa scheda può essere contemporaneamente connessa a un router Wi-Fi e fare da Hotspot).

### 2.2 Creazione dell'Access Point (hostapd)
* Quando viene richiesto l'avvio, AP-Generator genera dinamicamente il file di configurazione `config/run/hostapd.conf`.
* Configura i parametri 802.11n (`ieee80211n=1`), 802.11ac (`ieee80211ac=1` se canale >= 36), WMM QoS e le chiavi di sicurezza (`WPA-PSK`, `SAE` o aperta).
* Se è presente una blacklist MAC, include `macaddr_acl=0` e `deny_mac_file=config/mac_blacklist.txt`.
* Avvia il demone `hostapd` in background con pid-file dedicato in `config/run/hostapd.pid`.

### 2.3 Assegnazione IP e Servizi DHCP/DNS (dnsmasq)
* Assegna l'indirizzo IP del gateway all'interfaccia wireless (default `192.168.50.1/24`) tramite comandi `ip addr add`.
* Genera la configurazione `config/run/dnsmasq.conf` configurando:
  * Pool di indirizzi DHCP (es. `192.168.50.10` - `192.168.50.250`).
  * Opzioni router e DNS predefinito verso il gateway.
  * File dei lease in `config/run/dnsmasq.leases`.
  * Opzione **RFC 8908 DHCP 114** per Captive Portal.
* Avvia `dnsmasq` con pid-file in `config/run/dnsmasq.pid`.

### 2.4 Condivisione Internet e NAT (iptables / nftables)
* Se la condivisione internet è abilitata (`--sharing`), abilita l'IP forwarding del kernel Linux (`sysctl net.ipv4.ip_forward=1`).
* Applica la regola di Masquerade:
  `iptables -t nat -A POSTROUTING -o <upstream> -j MASQUERADE`
* Configura il forwarding tra l'interfaccia AP e l'interfaccia upstream (`eth0`, `wlan0`, ecc.).

### 2.5 Captive Portal e Architettura RFC 8908
* Avvia un server HTTP asincrono e non-bloccante C++ nativo in ascolto sulla porta `8080`.
* Se il portale è abilitato, `iptables` reindirizza tutto il traffico HTTP in ingresso sulla porta 80 verso la porta 8080:
  `iptables -t nat -A PREROUTING -i <ap_iface> -p tcp --dport 80 -j REDIRECT --to-ports 8080`
* Il server intercetta gli endpoint di probe dei sistemi operativi:
  * Android: `/generate_204`, `/gen_204`
  * Apple (iOS / macOS): `/hotspot-detect.html`, `/canonical.html`
  * Windows: `/ncsi.txt`, `/connecttest.txt`
* Quando l'utente preme il pulsante di accesso sul portale, viene inviata una richiesta POST su `/login`. Il server riceve il pacchetto, registra l'IP e inserisce una regola `iptables -t nat -I PREROUTING 1 -s <client_ip> -j ACCEPT`, sbloccando immediatamente l'accesso a Internet per quel dispositivo.

### 2.6 Monitoraggio Client in Tempo Reale e Blacklist
* Il modulo `ClientMonitor` interroga `iw dev <iface> station dump` per estrarre la potenza del segnale radio (**RSSI in dBm**), i byte/pacchetti ricevuti (`RX`) e trasmessi (`TX`).
* Unisce questi dati con il file dei lease di `dnsmasq` per associare a ciascun MAC address il rispettivo indirizzo IP e l'hostname del dispositivo.
* Il comando `kick <mac>` invia un frame di de-autenticazione tramite `hostapd_cli deauthenticate <mac>`.

### 2.7 Sistema di Internazionalizzazione (i18n)
* Gestito tramite la classe `I18n` singleton.
* All'avvio legge la variabile `LANG` del sistema. Se la lingua inizia per `it`, imposta l'Italiano; altrimenti imposta l'Inglese.
* La preferenza utente può essere modificata a runtime con `language <it|en>` e viene memorizzata in `config/settings.json`.

---

## 3. Riferimento Completo dei Comandi

### `interfaces` / `ifaces`
* **Cosa fa**: Scansiona il sistema ed elenca tutte le schede di rete Wi-Fi rilevate.
* **Informazioni mostrate**: Nome interfaccia (`wlan0`, `wlp0s20f3`), indirizzo MAC hardware, driver del kernel, dispositivo PHY, stato del link (UP/DOWN), se connessa a una rete, indirizzo IP attuale, supporto AP Mode, supporto WPA3 (SAE) e bande supportate (2.4 GHz / 5 GHz).
* **Esempio**:
  ```bash
  ap-generator: interfaces
  ```

---

### `capabilities <interfaccia>`
* **Cosa fa**: Esegue un'analisi approfondita delle capacità hardware e frequenze della scheda specificata.
* **Informazioni mostrate**: Elenco completo dei canali radio disponibili per la banda a 2.4 GHz (1-14) e 5 GHz (36-165), supporto al roaming protetto, supporto WPA3-Personal e capacità di operare in modalità contemporanea AP+STA.
* **Esempio**:
  ```bash
  ap-generator: capabilities wlan0
  ```

---

### `wizard`
* **Cosa fa**: Avvia la procedura guidata interattiva passo-passo da terminale.
* **Come funziona**:
  1. Mostra l'elenco numerato delle schede Wi-Fi e chiede quale utilizzare.
  2. Richiede il nome della rete Wi-Fi (SSID).
  3. Richiede il canale radio Wi-Fi.
  4. Richiede la modalità di sicurezza (`[1] Open`, `[2] WPA2-Personal`, `[3] WPA3-Personal`) e la relativa password.
  5. Chiede se abilitare la condivisione Internet (NAT) e quale scheda usare come sorgente Internet (es. `eth0`).
  6. Chiede se attivare il Captive Portal e mostra la lista dei 13 template grafici selezionabili.
  7. Consente di salvare la configurazione come preset permanente.
  8. Chiede se avviare subito l'Access Point.
* **Esempio**:
  ```bash
  ap-generator: wizard
  ```

---

### `start <preset>`
* **Cosa fa**: Avvia l'Access Point utilizzando i parametri definiti nel preset indicato.
* **Operazioni eseguite**:
  1. Valida la configurazione.
  2. Configura l'interfaccia di rete e assegna l'IP statico del gateway.
  3. Avvia `hostapd` per trasmettere il segnale Wi-Fi.
  4. Avvia `dnsmasq` per fornire DHCP e DNS.
  5. Se abilitato, attiva il NAT e il routing con l'interfaccia upstream.
  6. Se abilitato, avvia il server Captive Portal e le regole firewall di redirect.
  7. Salva lo stato in `config/run/ap-state.json`.
* **Esempio**:
  ```bash
  ap-generator: start LabHotspot
  ```

---

### `stop`
* **Cosa fa**: Arresta in modo sicuro l'Access Point attivo e ripristina la configurazione di sistema.
* **Operazioni eseguite**:
  1. Arresta il demone `hostapd`.
  2. Arresta il demone `dnsmasq`.
  3. Arresta il server HTTP del Captive Portal.
  4. Rimuove le regole `iptables` di NAT Masquerade e di redirect.
  5. Rimuove gli indirizzi IP assegnati all'interfaccia wireless.
  6. Rimuove il file di stato `config/run/ap-state.json`.
* **Esempio**:
  ```bash
  ap-generator: stop
  ```

---

### `status`
* **Cosa fa**: Mostra lo stato in tempo reale dell'Access Point.
* **Informazioni mostrate**: Stato (ATTIVO / ARRESTATO), SSID, interfaccia wireless, canale, modalità di sicurezza, IP gateway, stato condivisione Internet, template captive portal attivo, tempo di attività (**uptime**) e numero di client attualmente connessi.
* **Esempio**:
  ```bash
  ap-generator: status
  ```

---

### `clients`
* **Cosa fa**: Mostra una tabella dettagliata di tutti i dispositivi client attualmente associati alla rete Wi-Fi.
* **Informazioni mostrate**:
  * `MAC Address`: Indirizzo fisico del dispositivo client.
  * `IP Address`: Indirizzo IPv4 assegnato dal server DHCP.
  * `Hostname`: Nome del dispositivo (se comunicato via DHCP).
  * `Signal`: Potenza del segnale radio in **dBm** (rilevato da `iw station dump`).
  * `RX Bytes`: Quantità di dati ricevuti dal client (formattati in B, KB, MB, GB).
  * `TX Bytes`: Quantità di dati trasmessi verso il client.
* **Esempio**:
  ```bash
  ap-generator: clients
  ```

---

### `kick <mac>`
* **Cosa fa**: Disconnette forzatamente un client connesso inviando un frame di de-autenticazione standard IEEE 802.11 tramite `hostapd_cli`.
* **Esempio**:
  ```bash
  ap-generator: kick 0e:70:42:65:48:5e
  ```

---

### `blacklist <list|add|del> [mac]`
* **Cosa fa**: Gestisce la lista nera dei dispositivi non autorizzati a connettersi all'Access Point.
* **Sottocomandi**:
  * `blacklist list`: Mostra tutti i MAC address presenti nella blacklist.
  * `blacklist add <mac>`: Aggiunge un MAC alla blacklist, salva il file in `config/mac_blacklist.txt` e disconnette immediatamente il client se connesso.
  * `blacklist del <mac>` (o `remove`): Rimuove il MAC dalla blacklist consentendogli nuovamente l'accesso.
* **Esempio**:
  ```bash
  ap-generator: blacklist add aa:bb:cc:11:22:33
  ap-generator: blacklist list
  ap-generator: blacklist del aa:bb:cc:11:22:33
  ```

---

### `preset list`
* **Cosa fa**: Elenca tutti i profili di configurazione memorizzati nella directory `config/presets/`.
* **Esempio**:
  ```bash
  ap-generator: preset list
  ```

---

### `preset show <nome>`
* **Cosa fa**: Mostra l'intero contenuto JSON del preset specificato.
* **Esempio**:
  ```bash
  ap-generator: preset show GuestPortalAP
  ```

---

### `preset save <nome> [opzioni]`
* **Cosa fa**: Crea o aggiorna un file di configurazione preset in `config/presets/<nome>.json`.
* **Opzioni disponibili**:
  * `--ssid <nome>`: Nome della rete Wi-Fi
  * `--interface <iface>`: Interfaccia wireless da usare (es. `wlan0`)
  * `--channel <1-165>`: Canale Wi-Fi
  * `--security <open|wpa2|wpa3>`: Modalità di sicurezza
  * `--password <pass>`: Password WPA2/WPA3
  * `--sharing`: Abilita NAT e condivisione Internet
  * `--upstream <iface>`: Interfaccia sorgente Internet (es. `eth0`)
  * `--portal`: Abilita il Captive Portal
  * `--portal-path <path>`: Nome del template o percorso personalizzato
  * `--gateway <ip>`: Indirizzo IP del gateway (default `192.168.50.1`)
* **Esempio**:
  ```bash
  ap-generator: preset save Ufficio --interface wlan0 --ssid ReteUfficio --channel 6 --security wpa2 --password MiaPassword123 --sharing --upstream eth0
  ```

---

### `preset delete <nome>`
* **Cosa fa**: Rimuove definitivamente il file `config/presets/<nome>.json`.
* **Esempio**:
  ```bash
  ap-generator: preset delete Ufficio
  ```

---

### `validate <nome|file>`
* **Cosa fa**: Esegue un controllo semantico formale su una configurazione senza avviare la rete, verificando la validità dei canali, la lunghezza dell'SSID, la password, la correttezza del BSSID e la non-sovrapposizione tra interfaccia AP e upstream.
* **Esempio**:
  ```bash
  ap-generator: validate LabHotspot
  ```

---

### `portal list`
* **Cosa fa**: Elenca tutti i 13 template HTML/CSS pronti all'uso inclusi nella cartella `portal/presets/`.
* **Esempio**:
  ```bash
  ap-generator: portal list
  ```

---

### `portal test <template> [--port <p>]`
* **Cosa fa**: Avvia un'istanza locale di prova del server HTTP asincrono per verificare l'anteprima grafica del template Captive Portal nel proprio browser (senza bisogno di attivare l'Access Point).
* **Parametri**:
  * `<template>`: Nome del template (es. `Google_Modern`, `Starlink`, `TP-LINK-Var1`).
  * `--port <p>`: Porta TCP su cui mettersi in ascolto (default `8080`).
* **Esempio**:
  ```bash
  ap-generator: portal test Google_Modern --port 8080
  ```
  *Apri il browser su `http://127.0.0.1:8080` per testare la pagina e il form di login.*

---

### `language <it|en>`
* **Cosa fa**: Modifica istantaneamente la lingua di visualizzazione dell'interfaccia (Italiano o Inglese) e salva la preferenza in `config/settings.json`.
* **Esempio**:
  ```bash
  ap-generator: language en
  ap-generator: language it
  ```

---

### `clear`
* **Cosa fa**: Pulisce lo schermo del terminale (`\033[2J\033[H`) e ristampa il banner iniziale con il riepilogo.
* **Esempio**:
  ```bash
  ap-generator: clear
  ```

---

### `help` / `?`
* **Cosa fa**: Mostra la tabella di riferimento di tutti i comandi suddivisi per categorie (Rete, Preset, Access Point, Captive Portal, Sistema).
* **Esempio**:
  ```bash
  ap-generator: help
  ```

---

### `exit` / `quit` / `q`
* **Cosa fa**: Esce dalla shell interattiva di AP-Generator e ritorna al terminale di sistema.
* **Esempio**:
  ```bash
  ap-generator: exit
  ```

---

## 4. Risoluzione dei Problemi e FAQ

### D: Il comando `start` fallisce con "Device or resource busy"?
* **Causa**: `NetworkManager` o `wpa_supplicant` potrebbero avere il controllo esclusivo sulla scheda Wi-Fi.
* **Soluzione**: Rilasciare la scheda da NetworkManager prima di avviare l'AP:
  ```bash
  sudo nmcli device set <interfaccia> managed no
  ```

### D: La scheda wireless non appare o è in stato DOWN?
* **Causa**: Blocco hardware o software via `rfkill`.
* **Soluzione**: Eseguire:
  ```bash
  sudo rfkill unblock wifi
  sudo ip link set dev <interfaccia> up
  ```

### D: Le richieste DNS o il Captive Portal non rispondono?
* **Causa**: La porta 53 potrebbe essere occupata da `systemd-resolved`.
* **Soluzione**: `dnsmasq` si collega specificamente all'IP del gateway dell'AP (`bind-interfaces` su `192.168.50.1`), ma è buona norma verificare che non ci siano altri demoni DNS in ascolto su tutte le interfacce (`0.0.0.0:53`).
