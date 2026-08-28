// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef COMPONENTS_POLICY_VIVALDI_VPN_POLICY_HANDLER_H_
#define COMPONENTS_POLICY_VIVALDI_VPN_POLICY_HANDLER_H_

#include "components/policy/core/browser/configuration_policy_handler.h"

class PrefValueMap;

namespace policy {
class PolicyErrorMap;
class PolicyMap;
}  // namespace policy

namespace vivaldi {

class VivaldiVpnPolicyHandler : public policy::ConfigurationPolicyHandler {
 public:
  VivaldiVpnPolicyHandler();
  ~VivaldiVpnPolicyHandler() override;
  VivaldiVpnPolicyHandler(const VivaldiVpnPolicyHandler&) = delete;
  VivaldiVpnPolicyHandler& operator=(const VivaldiVpnPolicyHandler&) = delete;

  // policy::ConfigurationPolicyHandler:
  bool CheckPolicySettings(const policy::PolicyMap& policies,
                           policy::PolicyErrorMap* errors) override;
  void ApplyPolicySettings(const policy::PolicyMap& policies,
                           PrefValueMap* prefs) override;
};

}  // namespace vivaldi

#endif  // COMPONENTS_POLICY_VIVALDI_VPN_POLICY_HANDLER_H_
