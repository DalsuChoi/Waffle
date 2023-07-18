#ifndef INTERFACE_INTERFACERESOURCE_H
#define INTERFACE_INTERFACERESOURCE_H

#include <memory>
#include <restbed>
#include <string>
#include <tuple>
#include <vector>

using namespace restbed;

class InterfaceResource
{
  public:
    InterfaceResource();
    std::shared_ptr<Resource> get_resource() const;

  private:
    std::vector<std::string> get_path_parameters(std::shared_ptr<Session> session) const;

    std::shared_ptr<Resource> _resource;
    void get_handler(std::shared_ptr<Session> session);
    void set_user_parameters(std::vector<std::string> &parameters);
    std::string try_new_hyperparameters();
    void evaluate_hyperparameters();
    std::string best_hyperparameter();
    void convergence();
    std::string statistics();
    std::string current_knob_setting();
    std::string additional_info();
};

#endif