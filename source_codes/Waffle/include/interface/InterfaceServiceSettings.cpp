#include "InterfaceServiceSettings.h"

InterfaceServiceSettings::InterfaceServiceSettings()
{
    _settings = std::make_shared<Settings>();
    _settings->set_port(8080);
    _settings->set_default_header("Connection", "close");
    _settings->set_default_header("Access-Control-Allow-Origin", "*");
}

std::shared_ptr<Settings> InterfaceServiceSettings::get_settings() const
{
    return _settings;
}