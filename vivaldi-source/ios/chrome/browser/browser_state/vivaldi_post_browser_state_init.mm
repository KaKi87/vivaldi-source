// Copyright (c) 2022 Vivaldi Technologies AS. All rights reserved

#import "ios/chrome/browser/browser_state/vivaldi_post_browser_state_init.h"

#import <vector>

#import "base/apple/backup_util.h"
#import "base/files/file_util.h"
#import "base/memory/raw_ptr.h"
#import "base/task/thread_pool.h"
#import "browser/removed_partners_tracker.h"
#import "browser/search_engines/vivaldi_search_engines_updater.h"
#import "browser/vivaldi_default_bookmarks.h"
#import "components/ad_blocker/core/utils.h"
#import "components/application_locale_storage/application_locale_storage.h"
#import "components/bookmarks/browser/bookmark_model.h"
#import "components/datasource/vivaldi_image_store_constants.h"
#import "components/keyed_service/core/service_access_type.h"
#import "components/search_engines/default_search_engine_observer.h"
#import "ios/chrome/browser/ad_blocker/adblock_rule_service_factory.h"
#import "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#import "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#import "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/snapshots/model/constants.h"
#import "ios/notes/notes_factory.h"
#import "ios/translate/vivaldi_ios_translate_client.h"
#import "ios/translate/vivaldi_ios_translate_service.h"
#import "services/network/public/cpp/shared_url_loader_factory.h"

namespace vivaldi_default_bookmarks {
namespace {
class UpdaterClientImpl : public UpdaterClient {
 public:
  ~UpdaterClientImpl() override;
  UpdaterClientImpl(const UpdaterClientImpl&) = delete;
  UpdaterClientImpl& operator=(const UpdaterClientImpl&) = delete;

  static std::unique_ptr<UpdaterClientImpl> Create(ProfileIOS* profile);

  bookmarks::BookmarkModel* GetBookmarkModel() override;
  FaviconServiceGetter GetFaviconServiceGetter() override;
  PrefService* GetPrefService() override;
  const std::string& GetApplicationLocale() override;

 private:
  UpdaterClientImpl(ProfileIOS* profile);
  const raw_ptr<ProfileIOS> profile_;
};

/*static*/
std::unique_ptr<UpdaterClientImpl> UpdaterClientImpl::Create(
    ProfileIOS* profile) {
  // Allow to upgrade bookmarks even with a private profile as a command line
  // switch can trigger the first window in Vivaldi to be incognito one. So
  // get the original recording profile.
  return std::unique_ptr<UpdaterClientImpl>(
      new UpdaterClientImpl(profile->GetOriginalProfile()));
}

UpdaterClientImpl::UpdaterClientImpl(ProfileIOS* profile) : profile_(profile) {}
UpdaterClientImpl::~UpdaterClientImpl() = default;

bookmarks::BookmarkModel* UpdaterClientImpl::GetBookmarkModel() {
  return ios::BookmarkModelFactory::GetForProfile(profile_);
}

PrefService* UpdaterClientImpl::GetPrefService() {
  return profile_->GetPrefs();
}

const std::string& UpdaterClientImpl::GetApplicationLocale() {
  return GetApplicationContext()->GetApplicationLocaleStorage()->Get();
}

FaviconServiceGetter UpdaterClientImpl::GetFaviconServiceGetter() {
  auto get_favicon_service = [](ProfileIOS* profile) {
    return ios::FaviconServiceFactory::GetForProfile(
        profile, ServiceAccessType::IMPLICIT_ACCESS);
  };
  return base::BindRepeating(get_favicon_service, profile_);
}
}  // namespace
}  // namespace vivaldi_default_bookmarks

namespace {

void SetBackupExclusions(std::vector<base::FilePath> paths) {
  for (const auto& path : paths) {
    if (!base::PathExists(path)) {
      continue;
    }
    base::apple::SetBackupExclusion(path);
  }
}

}  // namespace

namespace vivaldi {
void PostBrowserStateInit(ProfileIOS* profile) {
  vivaldi::SearchEnginesUpdater::UpdateSearchEngines(
      profile->GetSharedURLLoaderFactory());
  vivaldi::SearchEnginesUpdater::UpdateSearchEnginesPrompt(
      profile->GetSharedURLLoaderFactory());

  vivaldi_partners::RemovedPartnersTracker::Create(
      profile->GetPrefs(), ios::BookmarkModelFactory::GetForProfile(profile));

  vivaldi_default_bookmarks::UpdatePartners(
      vivaldi_default_bookmarks::UpdaterClientImpl::Create(profile));

  NotesModelFactory::GetForProfile(profile);
  adblock_filter::RuleServiceFactory::GetForProfile(profile);

  VivaldiIOSTranslateService::Initialize();
  VivaldiIOSTranslateClient::LoadTranslationScript();
  vivaldi::DefaultSearchEngineObserver::Create(
      ios::TemplateURLServiceFactory::GetForProfile(profile),
      profile->GetPrefs());

  // iCloud backup exclusions:
  base::FilePath profile_path = profile->GetStatePath();

  std::vector<base::FilePath> backup_exclusion_paths;
  backup_exclusion_paths.reserve(5);
  backup_exclusion_paths.push_back(
      profile_path.Append(adblock_filter::GetRulesFolderName()));
  backup_exclusion_paths.push_back(
      profile_path.DirName().DirName().DirName().Append("WebKit").Append(
          "ContentRuleLists"));
  backup_exclusion_paths.push_back(profile_path.DirName().Append(
      vivaldi_image_store::kDirectMatchImageDirectory));
  backup_exclusion_paths.push_back(profile_path.Append("Sync Data"));
  backup_exclusion_paths.push_back(profile_path.Append(kSnapshotsDirName));

  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&SetBackupExclusions, std::move(backup_exclusion_paths)));
  // End iCloud backup exclusions
}
}  // namespace vivaldi
