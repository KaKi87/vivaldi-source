// Copyright (c) 2015-2025 Vivaldi Technologies AS. All rights reserved.
#include "extensions/api/site_permissions/site_permissions_api.h"

#include "base/scoped_observation.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/permissions/permission_request.h"
#include "extensions/browser/api/content_settings/content_settings_helpers.h"
#include "extensions/browser/extension_function.h"
#include "extensions/schema/site_permissions.h"
#include "extensions/tools/vivaldi_tools.h"

namespace extensions {

constexpr std::array<ContentSetting, 4> kAllContentSettings = {
    CONTENT_SETTING_BLOCK,
    CONTENT_SETTING_ALLOW,
    CONTENT_SETTING_ASK,
    CONTENT_SETTING_SESSION_ONLY,
    // NOTE: CONTENT_SETTING_DETECT_IMPORTANT_CONTENT
};

// Returns true if the scoping type requires a secondary origin.
bool RequiresSecondaryOrigin(
    content_settings::WebsiteSettingsInfo::ScopingType scoping_type) {
  return scoping_type == content_settings::WebsiteSettingsInfo::
                             REQUESTING_AND_TOP_SCHEMEFUL_SITE_SCOPE ||
         scoping_type == content_settings::WebsiteSettingsInfo::
                             REQUESTING_ORIGIN_AND_TOP_SCHEMEFUL_SITE_SCOPE;
}

vivaldi::site_permissions::PermissionSetting toPermissionSetting(
    ContentSetting setting) {
  using vivaldi::site_permissions::PermissionSetting;
  switch (setting) {
    case CONTENT_SETTING_ALLOW:
      return PermissionSetting::kAllow;
    case CONTENT_SETTING_BLOCK:
      return PermissionSetting::kBlock;
    case CONTENT_SETTING_ASK:
      return PermissionSetting::kAsk;
    case CONTENT_SETTING_SESSION_ONLY:
      return PermissionSetting::kSessionOnly;
    case CONTENT_SETTING_DEFAULT:
      return PermissionSetting::kDefault;
    default:
      break;
  }

  return PermissionSetting::kNone;
}

std::optional<ContentSetting> fromPermissionSetting(
    vivaldi::site_permissions::PermissionSetting setting) {
  switch (setting) {
    case vivaldi::site_permissions::PermissionSetting::kAllow:
      return CONTENT_SETTING_ALLOW;
    case vivaldi::site_permissions::PermissionSetting::kBlock:
      return CONTENT_SETTING_BLOCK;
    case vivaldi::site_permissions::PermissionSetting::kAsk:
      return CONTENT_SETTING_ASK;
    case vivaldi::site_permissions::PermissionSetting::kSessionOnly:
      return CONTENT_SETTING_SESSION_ONLY;
    case vivaldi::site_permissions::PermissionSetting::kDefault:
      return CONTENT_SETTING_DEFAULT;
    case vivaldi::site_permissions::PermissionSetting::kNone:
      return std::nullopt;
  }
  return std::nullopt;
}

class ContentSettingsObserver : public content_settings::Observer {
 public:
  ContentSettingsObserver(SitePermissionsAPI* owner)
      : owner_{owner},
        host_content_settings_map_{
            HostContentSettingsMapFactory::GetForProfile(owner->profile_)} {
    observer_.Observe(host_content_settings_map_);
  }

  // Per profile observation of content settings.
  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      observer_{this};

  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override {
    DCHECK(owner_);
    SitePermissionsAPI::SendPermissionChanged(
        static_cast<content::BrowserContext*>(owner_->profile_.get()),
        primary_pattern.ToString(), content_type);
  }

  SitePermissionsAPI* owner_;
  HostContentSettingsMap* host_content_settings_map_;
};

SitePermissionsAPI::SitePermissionsAPI(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)) {
  settings_observer_.reset(new ContentSettingsObserver(this));
}

SitePermissionsAPI::~SitePermissionsAPI() {}

SitePermissionsAPI* SitePermissionsAPI::FromBrowserContext(
    content::BrowserContext* browser_context) {
  SitePermissionsAPI* api = GetFactoryInstance()->Get(browser_context);
  DCHECK(api);
  return api;
}

// static
BrowserContextKeyedAPIFactory<SitePermissionsAPI>*
SitePermissionsAPI::GetFactoryInstance() {
  static base::NoDestructor<BrowserContextKeyedAPIFactory<SitePermissionsAPI>>
      instance;
  return instance.get();
}

// static
void SitePermissionsAPI::SendPermissionChanged(
    content::BrowserContext* browser_context,
    const std::string& origin,
    ContentSettingsType type) {
  vivaldi::site_permissions::PermissionChangeEvent event;

  if (!content_settings::mojom::IsKnownEnumValue(type))
    return;

  // Don't propagate DEFAULT it will crash...
  if (type == ContentSettingsType::DEFAULT)
    return;

  event.origin = origin;
  event.permission =
      content_settings_helpers::ContentSettingsTypeToString(type);

  ::vivaldi::BroadcastEvent(
      vivaldi::site_permissions::OnPermissionChanged::kEventName,
      vivaldi::site_permissions::OnPermissionChanged::Create(event),
      browser_context);
}

ExtensionFunction::ResponseAction
SitePermissionsGetAvailablePermissionsFunction::Run() {
  using vivaldi::site_permissions::PermissionOptions;
  using vivaldi::site_permissions::PermissionSetting;
  namespace Results =
      vivaldi::site_permissions::GetAvailablePermissions::Results;

  std::vector<PermissionOptions> permissions;

  content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (const content_settings::ContentSettingsInfo* info : *registry) {
    auto website_info = info->website_settings_info();

    PermissionOptions permission_entry;

    permission_entry.permission =
        content_settings_helpers::ContentSettingsTypeToString(
            website_info->type());

    // Iterate all possible options (hardcoded)
    for (ContentSetting setting : kAllContentSettings) {
      if (!info->IsSettingValid(setting)) {
        continue;
      }
      auto jsetting = toPermissionSetting(setting);
      if (jsetting != PermissionSetting::kNone) {
        permission_entry.options.push_back(jsetting);
      }
    }

    auto jsetting = toPermissionSetting(info->GetInitialDefaultSetting());
    if (jsetting != PermissionSetting::kNone) {
      permission_entry.default_ = jsetting;
    }

    // Indicate if this permission requires a secondary origin
    permission_entry.requires_secondary_origin =
        RequiresSecondaryOrigin(website_info->scoping_type());

    permissions.push_back(std::move(permission_entry));
  }

  return RespondNow(ArgumentList(Results::Create(std::move(permissions))));
}

ExtensionFunction::ResponseAction
SitePermissionsGetOverriddenSitesFunction::Run() {
  using vivaldi::site_permissions::SitePermissionInfo;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile)
    return RespondNow(Error("Could not get profile"));

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map)
    return RespondNow(Error("Could not access content settings"));

  // Map of origin -> count of overridden permissions
  base::flat_map<std::string, int> origin_counts;

  const content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      std::string origin = entry.primary_pattern.ToString();

      // For wildcard pattern, compare to initial default (only count actual
      // global overrides). For specific sites, count all non-default settings
      // (they're all overrides).
      bool is_wildcard =
          (entry.primary_pattern == ContentSettingsPattern::Wildcard());
      if (is_wildcard && setting == info->GetInitialDefaultSetting())
        continue;

      if (!origin.empty()) {
        origin_counts[origin]++;
      }
    }
  }

  // Convert map to vector of SitePermissionInfo objects
  std::vector<SitePermissionInfo> sites;
  for (const auto& [origin, count] : origin_counts) {
    SitePermissionInfo site_info;
    site_info.origin = origin;
    site_info.count = count;
    sites.push_back(std::move(site_info));
  }

  return RespondNow(ArgumentList(
      vivaldi::site_permissions::GetOverriddenSites::Results::Create(sites)));
}

ExtensionFunction::ResponseAction
SitePermissionsGetOverridesForSiteFunction::Run() {
  using vivaldi::site_permissions::GetOverridesForSite::Params;
  namespace Results = vivaldi::site_permissions::GetOverridesForSite::Results;
  using vivaldi::site_permissions::Permission;

  std::optional<Params> params = Params::Create(args());
  if (!params)
    return RespondNow(Error("Invalid parameters."));

  const std::string& origin_str = params->origin;

  // Parse the origin string as a pattern (not just a URL)
  // This allows querying with patterns like "[*.]example.com" or "*"
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(origin_str);

  if (!primary_pattern.IsValid()) {
    return RespondNow(Error("Invalid origin pattern."));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile)
    return RespondNow(Error("Could not get profile."));

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map)
    return RespondNow(Error("Could not access content settings."));

  const auto* registry =
      content_settings::ContentSettingsRegistry::GetInstance();
  std::vector<Permission> result;

  // Check if we're querying the wildcard pattern
  bool is_wildcard_query =
      (primary_pattern == ContentSettingsPattern::Wildcard());

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    // Get all settings for this type
    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      // Only include entries where primary pattern exactly matches
      if (entry.primary_pattern != primary_pattern)
        continue;

      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      // For wildcard queries, compare to initial default (show what globals are
      // set). For specific sites, show all non-default settings (they're
      // overrides).
      if (is_wildcard_query && setting == info->GetInitialDefaultSetting())
        continue;

      Permission permission;
      permission.origin = origin_str;

      // Include secondary origin if it's not a wildcard
      if (website_info->SupportsSecondaryPattern() &&
          entry.secondary_pattern != ContentSettingsPattern::Wildcard()) {
        permission.secondary_origin = entry.secondary_pattern.ToString();
      }

      permission.permission =
          content_settings_helpers::ContentSettingsTypeToString(type);
      permission.setting = toPermissionSetting(setting);

      result.push_back(std::move(permission));
    }
  }

  return RespondNow(ArgumentList(Results::Create(result)));
}

ExtensionFunction::ResponseAction
SitePermissionsSetSitePermissionFunction::Run() {
  using vivaldi::site_permissions::SetSitePermission::Params;
  namespace Results = vivaldi::site_permissions::SetSitePermission::Results;

  std::optional<Params> params = Params::Create(args());
  if (!params) {
    return RespondNow(Error("Invalid parameters."));
  }

  // Parse primary origin as a pattern (supports wildcards like "*" or
  // "[*.]example.com")
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(params->origin);
  if (!primary_pattern.IsValid()) {
    return RespondNow(Error("Invalid origin pattern."));
  }

  // Parse secondary origin as a pattern
  // Empty string is treated as wildcard for convenience
  std::string secondary_origin_str = params->secondary_origin;
  if (secondary_origin_str.empty()) {
    secondary_origin_str = "*";
  }
  ContentSettingsPattern secondary_pattern =
      ContentSettingsPattern::FromString(secondary_origin_str);
  if (!secondary_pattern.IsValid()) {
    return RespondNow(Error("Invalid secondary origin pattern."));
  }

  // Convert permission string to content setting type
  std::optional<ContentSettingsType> type_opt =
      content_settings_helpers::StringToContentSettingsType(params->permission);
  if (!type_opt) {
    return RespondNow(Error("Unknown permission type: " + params->permission));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile) {
    return RespondNow(Error("Could not get profile."));
  }

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map) {
    return RespondNow(Error("Could not access content settings."));
  }

  content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  auto info = registry->Get(*type_opt);
  if (!info) {
    return RespondNow(Error("Could not retrieve the content setting info."));
  }

  auto website_info = info->website_settings_info();

  // Convert setting first, so we can check if it's DEFAULT in validation
  std::optional<ContentSetting> setting_opt =
      fromPermissionSetting(params->setting);
  if (!setting_opt)
    return RespondNow(Error("Invalid setting value"));

  // Validate secondary origin based on permission type requirements
  if (RequiresSecondaryOrigin(website_info->scoping_type())) {
    // Allow wildcard secondary pattern in two cases:
    // 1. Setting to DEFAULT (erases the override)
    // 2. Both patterns are wildcards (setting global default, like Chrome's
    // SetDefaultContentSetting)
    bool is_setting_default = (*setting_opt == CONTENT_SETTING_DEFAULT);
    bool is_global_default =
        (primary_pattern == ContentSettingsPattern::Wildcard() &&
         secondary_pattern == ContentSettingsPattern::Wildcard());

    if (!is_setting_default && !is_global_default) {
      // For non-default settings that aren't global, require a specific
      // secondary origin
      if (params->secondary_origin.empty() ||
          secondary_pattern == ContentSettingsPattern::Wildcard()) {
        return RespondNow(
            Error("Secondary origin is required for permission type: " +
                  params->permission));
      }
    }
  }

  if (!info->IsSettingValid(*setting_opt) &&
      (*setting_opt != CONTENT_SETTING_DEFAULT)) {
    return RespondNow(Error("Invalid setting type for given permission type."));
  }

  // Use SetContentSettingCustomScope to set the exact patterns
  settings_map->SetContentSettingCustomScope(primary_pattern, secondary_pattern,
                                             *type_opt, *setting_opt);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SitePermissionsResetSitePermissionsFunction::Run() {
  using vivaldi::site_permissions::ResetSitePermissions::Params;
  namespace Results = vivaldi::site_permissions::GetOverridesForSite::Results;
  using vivaldi::site_permissions::Permission;

  std::optional<Params> params = Params::Create(args());
  if (!params)
    return RespondNow(Error("Invalid parameters."));

  const std::string& origin_str = params->origin;

  // Parse the origin string as a pattern (not just a URL)
  // This allows resetting patterns like "[*.]example.com" or "*"
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString(origin_str);

  if (!primary_pattern.IsValid()) {
    return RespondNow(Error("Invalid origin pattern."));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (!profile) {
    return RespondNow(Error("Could not get profile."));
  }

  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!settings_map) {
    return RespondNow(Error("Could not access content settings."));
  }

  const auto* registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (const auto* info : *registry) {
    auto website_info = info->website_settings_info();
    ContentSettingsType type = website_info->type();

    // Get all settings for this type
    auto entries = settings_map->GetSettingsForOneType(type);

    for (const auto& entry : entries) {
      // Only reset entries where primary pattern exactly matches
      // This includes all secondary origins for that pattern
      if (entry.primary_pattern != primary_pattern)
        continue;

      ContentSetting setting =
          content_settings::ValueToContentSetting(entry.setting_value);
      if (setting == CONTENT_SETTING_DEFAULT)
        continue;

      // Reset this specific permission by setting it to default
      // This will use the correct patterns (primary and secondary)
      settings_map->SetContentSettingCustomScope(entry.primary_pattern,
                                                 entry.secondary_pattern, type,
                                                 CONTENT_SETTING_DEFAULT);
    }
  }

  return RespondNow(NoArguments());
}

}  // namespace extensions
