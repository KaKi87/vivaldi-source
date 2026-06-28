// Copyright 2025 Vivaldi Technologies. All rights reserved.

#include "browser/features/vivaldi_features.h"
#include "app/vivaldi_version_info.h"

namespace {

struct BuildTypeDefaults {
  base::FeatureState sopranos;
  base::FeatureState snapshot;
  base::FeatureState final;

  constexpr BuildTypeDefaults(
      base::FeatureState uniform = base::FEATURE_DISABLED_BY_DEFAULT)
      : sopranos(uniform), snapshot(uniform), final(uniform) {}

  constexpr BuildTypeDefaults Sopranos(base::FeatureState state) const {
    BuildTypeDefaults copy = *this;
    copy.sopranos = state;
    return copy;
  }

  constexpr BuildTypeDefaults Snapshot(base::FeatureState state) const {
    BuildTypeDefaults copy = *this;
    copy.snapshot = state;
    return copy;
  }

  constexpr BuildTypeDefaults Final(base::FeatureState state) const {
    BuildTypeDefaults copy = *this;
    copy.final = state;
    return copy;
  }
};

struct FeatureDefaults {
  base::FeatureState default_state;

  std::optional<BuildTypeDefaults> desktop;
  std::optional<BuildTypeDefaults> android;
  std::optional<BuildTypeDefaults> ios;

  constexpr FeatureDefaults(
      base::FeatureState state = base::FEATURE_DISABLED_BY_DEFAULT)
      : default_state(state) {}

  constexpr FeatureDefaults WithDesktop(BuildTypeDefaults defaults) const {
    FeatureDefaults copy = *this;
    copy.desktop = defaults;
    return copy;
  }

  constexpr FeatureDefaults WithAndroid(
      const BuildTypeDefaults defaults) const {
    FeatureDefaults copy = *this;
    copy.android = defaults;
    return copy;
  }

  constexpr FeatureDefaults WithIOS(BuildTypeDefaults defaults) const {
    FeatureDefaults copy = *this;
    copy.ios = defaults;
    return copy;
  }

  constexpr base::FeatureState Get() const {
    BuildTypeDefaults fallback(default_state);

#if BUILDFLAG(IS_ANDROID)
    const BuildTypeDefaults& defaults = android.value_or(fallback);
#elif BUILDFLAG(IS_IOS)
    const BuildTypeDefaults& defaults = ios.value_or(fallback);
#else
    const BuildTypeDefaults& defaults = desktop.value_or(fallback);
#endif

    constexpr auto release_kind = vivaldi::ReleaseKind();
    if constexpr (release_kind <= vivaldi::Release::kSopranos)
      return defaults.sopranos;
    if constexpr (release_kind <= vivaldi::Release::kBeta)
      return defaults.snapshot;
    return defaults.final;
  }
};

}  // namespace

namespace vivaldi_features {

// Names of the features MUST be globally unique, thus prefix those with
// 'Vivaldi'.
BASE_FEATURE(kDoubleClickMenu,
             "VivaldiDoubleClickMenu",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kHorizontalPinnedTabs,
             "VivaldiHorizontalPinnedTabs",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kCssMods, "VivaldiCssMods", base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kChromePages,
             "VivaldiChromePages",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kInternalPageReaderMode,
             "VivaldiInternalPageReaderMode",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNoteEditor,
             "VivaldiNewNoteEditor",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kRelatedTabs,
             "VivaldiRelatedTabs",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSettings20,
             "VivaldiSettings20",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_IOS)
// iOS specific feature flags should be delcared within this block.

BASE_FEATURE(kBankIDDigIDLatencyWorkaround,
             "VivaldiBankIDDigIDLatencyWorkaround",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kVivaldiIOSCopySanitizedLink,
             "VivaldiIOSCopySanitizedLink",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsVivaldiIOSCopySanitizedLinkEnabled() {
  return base::FeatureList::IsEnabled(kVivaldiIOSCopySanitizedLink);
}

BASE_FEATURE(kVivaldiIOSShowRefactoredStartPage,
             "VivaldiIOSShowRefactoredStartPage",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsVivaldiIOSShowRefactoredStartPageEnabled() {
  return base::FeatureList::IsEnabled(kVivaldiIOSShowRefactoredStartPage);
}

#endif  // BUILDFLAG(IS_IOS)

#if defined(OEM_AUTOMOTIVE_BUILD)
BASE_FEATURE(kCinemaMode,
             "VivaldiCinemaMode",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // defined(OEM_AUTOMOTIVE_BUILD)

BASE_FEATURE(kVivaldiUseNewUrlSanitizer,
             "VivaldiUseNewUrlSanitizer",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kMailSavingAttachments,
             "VivaldiSavingMailAttachments",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace vivaldi_features
