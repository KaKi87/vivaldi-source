// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SEARCH_ENGINES_MODEL_UI_THREAD_SEARCH_TERMS_DATA_H_
#define IOS_CHROME_BROWSER_SEARCH_ENGINES_MODEL_UI_THREAD_SEARCH_TERMS_DATA_H_

#include "base/sequence_checker.h"
#include "components/search_engines/search_terms_data.h"

#if defined(VIVALDI_BUILD)
#include "base/memory/raw_ptr.h"
class ProfileIOS;
#endif

namespace ios {

// Implementation of SearchTermsData that is only usable on UI thread.
class UIThreadSearchTermsData : public SearchTermsData {
 public:
#if defined(VIVALDI_BUILD)
  UIThreadSearchTermsData(ProfileIOS* profile);
#else
  UIThreadSearchTermsData();
#endif  // End Vivaldi

  UIThreadSearchTermsData(const UIThreadSearchTermsData&) = delete;
  UIThreadSearchTermsData& operator=(const UIThreadSearchTermsData&) = delete;

  ~UIThreadSearchTermsData() override;

  // SearchTermsData implementation.
  std::string GoogleBaseURLValue() const override;
  std::string GetApplicationLocale() const override;
  std::u16string GetRlzParameterValue(bool from_app_list) const override;
  std::string GetSearchClient() const override;
  std::string GoogleImageSearchSource() const override;

  std::string_view VivaldiGetKagiToken() const override;
 private:
  SEQUENCE_CHECKER(sequence_checker_);

#if defined(VIVALDI_BUILD)
  raw_ptr<ProfileIOS> profile_;
#endif  // End Vivaldi
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_SEARCH_ENGINES_MODEL_UI_THREAD_SEARCH_TERMS_DATA_H_
