#pragma once 

#include <vector>
#include <string>
#include <iostream>

using namespace std;

#include "apmanager/core/types.hpp"

namespace apm {

    //abstract interface 

    class WifiBackend {
        public:
        virtual ~WifiBackend() = default; 

        virtual vector<WifiInterface>
        discover_interfaces() = 0;

        virtual bool create_ap() = 0;
    };

    class LinuxWifiBackend : public WifiBackend {
        public:
        vector<WifiInterface>
        discover_interfaces() override;

        bool create_ap(
            const AccesPointConfig& config 
        ) override;

        bool stop_ap() override; 
    };
}