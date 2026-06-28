// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef BRAVE_COMMON_CACHED_RULE_SERVICE_H_
#define BRAVE_COMMON_CACHED_RULE_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/base/backoff_entry.h"

class Profile;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

// Base class for services that download JSON rule sets, cache them on disk,
// and periodically refresh. Derived classes provide the relative URL path,
// cache file name, and parsing/notification logic via virtual overrides.
class CachedRuleService : public KeyedService {
 public:
  // Observer notified when rules are (re)loaded from download or cache.
  class Observer : public base::CheckedObserver {
   public:
    virtual ~Observer() = default;
    // Called after rules have been loaded successfully, either from a fresh
    // network download or from a local cache file.
    virtual void OnRulesLoaded(CachedRuleService* service) {}
  };

  CachedRuleService();
  ~CachedRuleService() override;

  CachedRuleService(const CachedRuleService&) = delete;
  CachedRuleService& operator=(const CachedRuleService&) = delete;

  // Initialize the service for the given profile: set up cache paths, load
  // from disk, and kick off the first network download.
#if BUILDFLAG(IS_IOS)
  void Load(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      PrefService* prefs);
#else
  void Load(Profile* profile);
#endif  // IS_IOS

  // Returns true if rules have been loaded (from cache or download).
  bool IsLoaded() const { return is_loaded_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  // --- Virtual hooks for derived classes ---

  // Name of the cache file on disk (e.g. "query_filter.json").
  virtual std::string CacheFileName() const = 0;

  // Relative path appended to kBaseUrl to form the download URL.
  // e.g. "clean-urls-current.json"
  virtual std::string RelativeUrlPath() const = 0;

  // Parse |json_data| and apply rules. Called for both fresh downloads
  // and cached loads. Return true on success.
  // Called on the default sequenced task runner.
  virtual bool ParseAndApplyRules(const std::string& json_data) = 0;

  // --- Protected members available to derived classes ---

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::FilePath cache_dir_;
  base::FilePath cache_file_path_;
  bool is_loaded_ = false;
  base::WeakPtrFactory<CachedRuleService> weak_ptr_factory_{this};

 private:
  // Start downloading the rule set from the network.
  void StartDownload();

  // Called when download completes. |file| is empty on failure.
  void OnDownloadDone(base::FilePath file);

  // Called after the temp file is (attempted to be) moved to the cache path.
  void OnDownloadReplaced(bool success);

  // Called after the downloaded file is read from disk.
  void OnDownloadLoaded(std::string downloaded_data);

  // Read cached rule file from disk and call OnCacheLoaded.
  void LoadFromCache();

  // Called after cache file is read from disk.
  void OnCacheLoaded(std::string cached_data);

  // Common handling of the ready rule json data string - parse and notification.
  void ParseAndNotify(const std::string &data);
  
  // Schedule the next periodic download check.
  void ScheduleNextDownload();

  std::unique_ptr<network::SimpleURLLoader> download_loader_;
  net::BackoffEntry backoff_entry_;
  base::ObserverList<Observer> observers_;
};

#endif  // BRAVE_COMMON_CACHED_RULE_SERVICE_H_
