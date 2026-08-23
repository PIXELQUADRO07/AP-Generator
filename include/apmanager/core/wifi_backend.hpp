#include <vector>
#include <string>


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

class LinuxWifiBackend : public WifiBackend {
    public:
       std::vector<WifiInterface>
       discover_interfaces() override;

       bool create_ap(
        const AccessPointConfig& config 
       ) override;

       bool stop_ap() override;
};