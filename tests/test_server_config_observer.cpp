#include "core/server_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "config_observer.hpp"
#include <iostream>
#include <memory>

class TestServerConfigObserver : public ServerConfigObserver
{
public:
    bool server_port_changed = false;
    bool server_host_changed = false;
    int new_port = 0;
    std::string new_host;

    void onConfigUpdate(const ConfigUpdateEvent &event) override
    {
        ServerConfigObserver::onConfigUpdate(event);

        // Track what was changed for testing
        for (const auto &key : event.changed_keys)
        {
            if (key == "server_port")
            {
                server_port_changed = true;
                new_port = PocoConfigAdapter::getInstance().getServerPort();
            }
            else if (key == "server_host")
            {
                server_host_changed = true;
                new_host = PocoConfigAdapter::getInstance().getServerHost();
            }
        }
    }
};
