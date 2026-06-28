// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_POLICY_VIVALDI_POLICY_HANDLERS_H_
#define COMPONENTS_POLICY_VIVALDI_POLICY_HANDLERS_H_

namespace policy {
class ConfigurationPolicyHandlerList;
}  // namespace policy

namespace vivaldi {

// Called by configuration_policy_handler_list_factory.cc BuildHandlerList()
void AddVivaldiPolicyHandlers(policy::ConfigurationPolicyHandlerList* handlers);

}  // namespace vivaldi

#endif  // COMPONENTS_POLICY_VIVALDI_POLICY_HANDLERS_H_
