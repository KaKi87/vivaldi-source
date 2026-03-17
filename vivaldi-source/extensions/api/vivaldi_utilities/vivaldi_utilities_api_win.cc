//
// Copyright (c) 2015 Vivaldi Technologies AS. All rights reserved.
//

#include "extensions/api/vivaldi_utilities/vivaldi_utilities_api.h"

#include <windows.h>
#include <winnls.h>
#include <optional>
#include <string>

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/win/registry.h"
#include "chrome/browser/win/taskbar_manager.h"
#include "chrome/common/chrome_features.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"

namespace extensions {

bool UtilitiesGetSystemDateFormatFunction::ReadDateFormats(
    vivaldi::utilities::DateFormats* date_formats) {
  // According to MSDN documentation max len is 80
  // https://msdn.microsoft.com/en-us/library/windows/desktop/
  //   dd373896(v=vs.85).aspx
  wchar_t result_buffer[80];
  int len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT,
                            result_buffer, std::size(result_buffer));

  if (len == 0) {
    return false;
  }

  std::string timeformat = base::WideToUTF8(result_buffer);
  len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SSHORTDATE,
                        result_buffer, std::size(result_buffer));

  if (len == 0) {
    return false;
  }

  std::string shortformat = base::WideToUTF8(result_buffer);
  len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SLONGDATE,
                        result_buffer, std::size(result_buffer));

  if (len == 0) {
    return false;
  }

  std::string longdateformat = base::WideToUTF8(result_buffer);
  len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                        result_buffer, std::size(result_buffer));

  if (len == 0) {
    return false;
  }
  int first_day_of_week = _wtoi(result_buffer);

  // convert Win API MSDN to js standard 0-6 (Sunday to Saturday)
  // https://msdn.microsoft.com/en-us/library/windows/desktop/
  //   dd373771(v=vs.85).aspx
  date_formats->first_day_of_week =
      first_day_of_week < 6 ? ++first_day_of_week : 0;

  date_formats->short_date_format = shortformat;
  date_formats->long_date_format = longdateformat;
  date_formats->time_format = timeformat;
  return true;
}

void UtilitiesIsVivaldiPinnedToLaunchBarFunction::CheckIsPinned(
    IsPinnedCallback callback) {
  task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  // `callback` is called with true if pinning is supported, and the app is not
  // currently pinned to the taskbar, false otherwise. There must be a shortcut
  // with `app_user_model_id` in the start menu for pinning to be supported.

  // Rebind to adjust the signature expected in browser_util.
  base::OnceCallback<void(bool)> can_pin_result_callback = base::BindOnce(
      [](IsPinnedCallback passed_callback, bool should_offer_to_pin) {
        // NOTE(andre@vivaldi.com) : On Windows |isPinned| will be true if
        // pinning is supported, and the app is not currently pinned to the
        // taskbar, false otherwise. So, should_offer_to_pin is not isPinned ==
        // ask for pinning.
        should_offer_to_pin = !should_offer_to_pin;
        std::move(passed_callback).Run(should_offer_to_pin);
      },
      std::move(callback));

  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &browser_util::ShouldOfferToPin,
          ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall()),
          browser_util::PinAppToTaskbarChannel::kFirstRunExperience,
          std::move(can_pin_result_callback)));
}

void UtilitiesPinVivaldiToLaunchBarFunction::PinToLaunchBar(
    HasPinnedCallback callback) {
  task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  if (!base::FeatureList::IsEnabled(features::kWinPinPWAShortcutWithLAF)) {
    LOG(INFO) << "kWinPinPWAShortcutWithLAF not enabled.";
    std::move(callback).Run(false);
    return;
  }

  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &browser_util::PinAppToTaskbar,
          ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall()),
          browser_util::PinAppToTaskbarChannel::kFirstRunExperience,
          std::move(callback)));
}

}  // namespace extensions
