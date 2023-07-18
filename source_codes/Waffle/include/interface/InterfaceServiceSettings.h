#ifndef INTERFACE_INTERFACESERVICESETTINGS_H
#define INTERFACE_INTERFACESERVICESETTINGS_H

#include <memory>
#include <restbed>

using namespace restbed;

class InterfaceServiceSettings
{
  public:
    InterfaceServiceSettings();
    std::shared_ptr<Settings> get_settings() const;

  private:
    std::shared_ptr<Settings> _settings;
};

#endif