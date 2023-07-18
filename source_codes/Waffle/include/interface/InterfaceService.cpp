#include "InterfaceService.h"

InterfaceService::InterfaceService()
{
    _settings_factory = std::make_shared<InterfaceServiceSettings>();
    _resource_factory = std::make_shared<InterfaceResource>();

    _service = std::make_shared<Service>();
    _service->publish(_resource_factory->get_resource());
}

void InterfaceService::operator()()
{
    _service->start(_settings_factory->get_settings());
}