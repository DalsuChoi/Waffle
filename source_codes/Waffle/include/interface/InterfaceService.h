#ifndef INTERFACE_INTERFACESERVICE_H
#define INTERFACE_INTERFACESERVICE_H

#include "InterfaceResource.h"
#include "InterfaceServiceSettings.h"

class InterfaceService
{
  public:
    InterfaceService();
    void operator()();

  private:
    std::shared_ptr<Service> _service;
    std::shared_ptr<InterfaceServiceSettings> _settings_factory;
    std::shared_ptr<InterfaceResource> _resource_factory;
};

#endif