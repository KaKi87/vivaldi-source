// Copyright (c) 2019 Vivaldi Technologies AS. All rights reserved

#include "components/ad_blocker/public/core/adblock_types.h"

#include "base/hash/hash.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"

namespace adblock_filter {
bool ParsedMetadata::operator==(const ParsedMetadata&) const = default;

/*static*/
std::optional<RuleSourceCore> RuleSourceCore::FromUrl(GURL url) {
  if (!url.is_valid() || url.is_empty())
    return std::nullopt;

  return RuleSourceCore(std::move(url));
}

/*static*/
std::optional<RuleSourceCore> RuleSourceCore::FromFile(base::FilePath file) {
  if (file.empty() || !file.IsAbsolute() || file.ReferencesParent() ||
      file.EndsWithSeparator())
    return std::nullopt;

  return RuleSourceCore(std::move(file));
}

RuleSourceCore::RuleSourceCore(GURL source_url)
    : id_(base::PersistentHash(source_url.spec())),
      source_location_(std::move(source_url)) {}
RuleSourceCore::RuleSourceCore(base::FilePath source_file)
    : id_(base::PersistentHash(source_file.AsUTF8Unsafe())),
      source_location_(std::move(source_file)) {}
RuleSourceCore::~RuleSourceCore() = default;
RuleSourceCore::RuleSourceCore(const RuleSourceCore&) = default;
RuleSourceCore& RuleSourceCore::operator=(const RuleSourceCore&) = default;
RuleSourceCore::RuleSourceCore(RuleSourceCore&&) = default;
RuleSourceCore& RuleSourceCore::operator=(RuleSourceCore&&) = default;

std::string RuleSourceCore::GetPrintableSourceLocation() const {
  return std::visit(
      absl::Overload{[](GURL url) { return url.spec(); },
                     [](base::FilePath path) { return path.AsUTF8Unsafe(); }},
      source_location_);
}

KnownRuleSource::KnownRuleSource(RuleSourceCore core) : core(std::move(core)) {}
KnownRuleSource::~KnownRuleSource() = default;
KnownRuleSource::KnownRuleSource(const KnownRuleSource&) = default;
KnownRuleSource& KnownRuleSource::operator=(const KnownRuleSource&) = default;
KnownRuleSource::KnownRuleSource(KnownRuleSource&&) = default;
KnownRuleSource& KnownRuleSource::operator=(KnownRuleSource&&) = default;

ActiveRuleSource::ActiveRuleSource(RuleSourceCore core)
    : core(std::move(core)) {}
ActiveRuleSource::~ActiveRuleSource() = default;
ActiveRuleSource::ActiveRuleSource(const ActiveRuleSource&) = default;
ActiveRuleSource& ActiveRuleSource::operator=(const ActiveRuleSource&) =
    default;

}  // namespace adblock_filter
