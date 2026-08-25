#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#include "apgen/ui/banner.hpp"

namespace apgen::ui {

// ── Terminal size detection ──────────────────────────────────────────────
static int get_terminal_width() {
    struct winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return static_cast<int>(w.ws_col);
    }
    return 80; // safe fallback
}

// ── Banner art (large – designed for ≥134 cols) ─────────────────────────
static const char* BANNER_LARGE = R"(
 _____                                                                                                                          _____ 
( ___ )------------------------------------------------------------------------------------------------------------------------( ___ )
 |   |                                                                                                                          |   | 
 |   |  @@@@@@   @@@@@@@              @@@@@@@@  @@@@@@@@  @@@  @@@  @@@@@@@@  @@@@@@@    @@@@@@   @@@@@@@   @@@@@@   @@@@@@@    |   | 
 |   | @@@@@@@@  @@@@@@@@            @@@@@@@@@  @@@@@@@@  @@@@ @@@  @@@@@@@@  @@@@@@@@  @@@@@@@@  @@@@@@@  @@@@@@@@  @@@@@@@@   |   | 
 |   | @@!  @@@  @@!  @@@            !@@        @@!       @@!@!@@@  @@!       @@!  @@@  @@!  @@@    @@!    @@!  @@@  @@!  @@@   |   | 
 |   | !@!  @!@  !@!  !@!            !@!        !@!       !@!!@!@!  !@!       !@!  @!@  !@!  !@!    !@!    !@!  !@!  !@!  !@!   |   | 
 |   | @!@!@!@!  @!@@!@!  @!@!@!@!@  !@! @!@!@  @!!!:!    @!@ !!@!  @!!!:!    @!@!!@!   @!@!@!@!    @!!    @!@  !@!  @!@!!@!    |   | 
 |   | !!!@!!!!  !!@!!!   !!!@!@!!!  !!! !!@!!  !!!!!:    !@!  !!!  !!!!!:    !!@!@!    !!!@!!!!    !!!    !!@  !!!  !!@!@!     |   | 
 |   | !!:  !!!  !!:                 :!!   !!:  !!:       !!:  !!!  !!:       !!: :!!   !!:  !!!    !!:    !!:  !!!  !!: :!!    |   | 
 |   | :!:  !:!  :!:                 :!:   !::  :!:       :!:  !:!  :!:       :!:  !:!  :!:  !:!    :!:    :!:  !:!  :!:  !:!   |   | 
 |   | ::   :::   ::                  ::: ::::   :: ::::   ::   ::   :: ::::  ::   :::  ::   :::     ::    ::::: ::  ::   :::   |   | 
 |   |  :   : :   :                   :: :: :   : :: ::   ::    :   : :: ::    :   : :   :   : :     :      : :  :    :   : :   |   | 
 |   |                                                                                                                          |   | 
 |___|                                                                                                                          |___| 
(_____)------------------------------------------------------------------------------------------------------------------------(_____))";

// ── Banner art (medium – designed for ≥70 cols) ─────────────────────────
static const char* BANNER_MEDIUM = R"(
 ╔══════════════════════════════════════════════════════════════╗
 ║     _    ____        ____                           _       ║
 ║    / \  |  _ \      / ___| ___ _ __   ___ _ __ __ _| |_ ___ ║
 ║   / _ \ | |_) |____| |  _ / _ \ '_ \ / _ \ '__/ _` | __/ _ \║
 ║  / ___ \|  __/|____| |_| |  __/ | | |  __/ | | (_| | || (_) ║
 ║ /_/   \_\_|         \____|\___|_| |_|\___|_|  \__,_|\__\___/║
 ╚══════════════════════════════════════════════════════════════╝)";

// ── Banner art (small – for narrow terminals <70 cols) ──────────────────
static const char* BANNER_SMALL = R"(
 ┌─────────────────────────┐
 │   AP-Generator v1.0     │
 │   Wi-Fi Access Point    │
 │   Management Tool       │
 └─────────────────────────┘)";

// ── Helper: split a string by newlines ───────────────────────────────────
static std::vector<std::string> split_lines(const char* text) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    return lines;
}

// ── Main banner printer ─────────────────────────────────────────────────
void print_banner() {
    int tw = get_terminal_width();

    // Select the best banner variant for the current terminal width.
    const char* chosen;
    if (tw >= 134) {
        chosen = BANNER_LARGE;
    } else if (tw >= 70) {
        chosen = BANNER_MEDIUM;
    } else {
        chosen = BANNER_SMALL;
    }

    auto lines = split_lines(chosen);

    for (const auto& line : lines) {
        int len = static_cast<int>(line.size());
        if (len >= tw) {
            // Truncate if the line somehow exceeds terminal width.
            std::cout << line.substr(0, tw) << "\n";
        } else {
            // Center the line.
            int pad = (tw - len) / 2;
            if (pad > 0) std::cout << std::string(pad, ' ');
            std::cout << line << "\n";
        }
    }
}

} // namespace apgen::ui
