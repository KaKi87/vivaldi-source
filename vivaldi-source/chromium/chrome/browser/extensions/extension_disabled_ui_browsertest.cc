// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_disabled_ui.h"

#include <stddef.h>

#include <algorithm>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_uninstall_dialog.h"
#include "chrome/browser/extensions/external_install_error.h"
#include "chrome/browser/extensions/sync/extension_sync_data.h"
#include "chrome/browser/extensions/sync/extension_sync_service.h"
#include "chrome/browser/extensions/updater/extension_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "components/sync/base/client_tag_hash.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "components/sync/protocol/extension_specifics.pb.h"
#include "components/sync/test/fake_sync_change_processor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/common/extension.h"
#include "extensions/common/verifier_formats.h"
#include "extensions/test/extension_test_message_listener.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "app/vivaldi_apptools.h"
#include "extensions/schema/browser_action_utilities.h"
#include "extensions/vivaldi_browser_component_wrapper.h"
#include "ui/vivaldi_rootdocument_handler.h"

using content::BrowserThread;
using extensions::Extension;
using extensions::ExtensionRegistry;
using extensions::ExtensionPrefs;
using extensions::ExtensionSyncData;

// Mock ExternalInstallError for VB-122765 regression test.
// Simulates the production crash path where a
// VivaldiExtensionDisabledGlobalError is created with the second constructor
// (WeakPtr<ExternalInstallError>), and the ExternalInstallError is destroyed
// before GetGlobalErrors() is called, leaving the WeakPtr expired.
class MockExternalInstallError : public extensions::ExternalInstallError {
 public:
  explicit MockExternalInstallError(const extensions::Extension* extension)
      : extension_(extension), extension_id_(extension->id()) {}

  // ExternalInstallError:
  void OnInstallPromptDone(
      ExtensionInstallPrompt::DoneCallbackPayload payload) override {}
  void DidOpenBubbleView() override {}
  void DidCloseBubbleView() override {}
  const extensions::Extension* GetExtension() const override {
    return extension_.get();
  }
  const extensions::ExtensionId& extension_id() const override {
    return extension_id_;
  }
  extensions::ExternalInstallError::AlertType alert_type() const override {
    return extensions::ExternalInstallError::AlertType::MENU_ALERT;
  }
  ExtensionInstallPrompt::Prompt* GetPromptForTesting() const override {
    return nullptr;
  }

  base::WeakPtr<extensions::ExternalInstallError> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  scoped_refptr<const extensions::Extension> extension_;
  extensions::ExtensionId extension_id_;
  base::WeakPtrFactory<MockExternalInstallError> weak_factory_{this};
};

class ExtensionDisabledGlobalErrorTest
    : public extensions::ExtensionBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kAppsGalleryUpdateURL,
                                    "http://localhost/autoupdate/updates.xml");
  }

  void SetUpOnMainThread() override {
    extensions::ExtensionBrowserTest::SetUpOnMainThread();
    // Required so AddExtensionDisabledError creates
    // VivaldiExtensionDisabledGlobalError instead of the plain Chromium one.
    vivaldi::ForceVivaldiRunning(true);
    EXPECT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    const base::FilePath test_dir =
        test_data_dir_.AppendASCII("permissions_increase");
    const base::FilePath pem_path = test_dir.AppendASCII("permissions.pem");
    path_v1_ = PackExtensionWithOptions(
        test_dir.AppendASCII("v1"),
        scoped_temp_dir_.GetPath().AppendASCII("permissions1.crx"), pem_path,
        base::FilePath());
    path_v2_ = test_dir.AppendASCII("v2.crx");
    path_v3_ = PackExtensionWithOptions(
        test_dir.AppendASCII("v3"),
        scoped_temp_dir_.GetPath().AppendASCII("permissions3.crx"), pem_path,
        base::FilePath());
  }

  // Returns the ExtensionDisabledGlobalError, if present.
  // Caution: currently only supports one error at a time.
  GlobalError* GetExtensionDisabledGlobalError() {
    return GlobalErrorServiceFactory::GetForProfile(profile())->
        GetGlobalErrorByMenuItemCommandID(IDC_EXTENSION_INSTALL_ERROR_FIRST);
  }

  // Install the initial version, which should happen just fine.
  const Extension* InstallIncreasingPermissionExtensionV1() {
    size_t size_before = extension_registry()->enabled_extensions().size();
    const Extension* extension = InstallExtension(path_v1_, 1);
    if (!extension)
      return nullptr;
    if (extension_registry()->enabled_extensions().size() != size_before + 1)
      return nullptr;
    return extension;
  }

  // Upgrade to a version that wants more permissions. We should disable the
  // extension and prompt the user to reenable.
  const Extension* UpdateIncreasingPermissionExtension(
      const Extension* extension,
      const base::FilePath& crx_path,
      int expected_change) {
    size_t size_before = extension_registry()->enabled_extensions().size();
    if (UpdateExtension(extension->id(), crx_path, expected_change))
      return nullptr;
    content::RunAllTasksUntilIdle();
    EXPECT_EQ(size_before + expected_change,
              extension_registry()->enabled_extensions().size());
    if (extension_registry()->disabled_extensions().size() != 1u)
      return nullptr;

    return extension_registry()->disabled_extensions().begin()->get();
  }

  // Helper function to install an extension and upgrade it to a version
  // requiring additional permissions. Returns the new disabled Extension.
  const Extension* InstallAndUpdateIncreasingPermissionsExtension() {
    const Extension* extension = InstallIncreasingPermissionExtensionV1();
    extension = UpdateIncreasingPermissionExtension(extension, path_v2_, -1);
    return extension;
  }

  base::ScopedTempDir scoped_temp_dir_;
  base::FilePath path_v1_;
  base::FilePath path_v2_;
  base::FilePath path_v3_;
};

// Tests the process of updating an extension to one that requires higher
// permissions, and accepting the permissions.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest, AcceptPermissions) {
  const Extension* extension = InstallAndUpdateIncreasingPermissionsExtension();
  ASSERT_TRUE(extension);
  ASSERT_TRUE(GetExtensionDisabledGlobalError());
  const size_t size_before = extension_registry()->enabled_extensions().size();

  ExtensionTestMessageListener listener("v2.onInstalled");
  listener.set_failure_message("FAILED");
  extensions::ExtensionRegistrar::Get(profile())
      ->GrantPermissionsAndEnableExtension(*extension);
  EXPECT_EQ(size_before + 1, extension_registry()->enabled_extensions().size());
  EXPECT_EQ(0u, extension_registry()->disabled_extensions().size());
  ASSERT_FALSE(GetExtensionDisabledGlobalError());
  // Expect onInstalled event to fire.
  EXPECT_TRUE(listener.WaitUntilSatisfied());
}

// Tests uninstalling an extension that was disabled due to higher permissions.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest, Uninstall) {
  const Extension* extension = InstallAndUpdateIncreasingPermissionsExtension();
  ASSERT_TRUE(extension);
  ASSERT_TRUE(GetExtensionDisabledGlobalError());
  const size_t size_before = extension_registry()->enabled_extensions().size();

  UninstallExtension(extension->id());
  EXPECT_EQ(size_before, extension_registry()->enabled_extensions().size());
  EXPECT_EQ(0u, extension_registry()->disabled_extensions().size());
  ASSERT_FALSE(GetExtensionDisabledGlobalError());
}

// Tests uninstalling a disabled extension with an uninstall dialog.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest, UninstallFromDialog) {
  extensions::ScopedTestDialogAutoConfirm auto_confirm(
      extensions::ScopedTestDialogAutoConfirm::ACCEPT);
  const Extension* extension = InstallAndUpdateIncreasingPermissionsExtension();
  ASSERT_TRUE(extension);
  std::string extension_id = extension->id();
  GlobalErrorWithStandardBubble* error =
      static_cast<GlobalErrorWithStandardBubble*>(
          GetExtensionDisabledGlobalError());
  ASSERT_TRUE(error);

  // The "cancel" button is the uninstall button on the browser.
  extensions::TestExtensionRegistryObserver test_observer(extension_registry(),
                                                          extension_id);
  error->BubbleViewCancelButtonPressed(browser());
  test_observer.WaitForExtensionUninstalled();

  EXPECT_FALSE(extension_registry()->GetExtensionById(
      extension_id, ExtensionRegistry::EVERYTHING));
  EXPECT_FALSE(GetExtensionDisabledGlobalError());
}

IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest,
                       UninstallWhilePromptBeingShown) {
  const Extension* extension = InstallAndUpdateIncreasingPermissionsExtension();
  ASSERT_TRUE(extension);
  ASSERT_TRUE(GetExtensionDisabledGlobalError());

  // Navigate a tab to the disabled extension, it will show a permission
  // increase dialog.
  GURL url = extension->url();
  int starting_tab_count = browser()->tab_strip_model()->count();
  NavigateToURLInNewTab(url);
  int tab_count = browser()->tab_strip_model()->count();
  EXPECT_EQ(starting_tab_count + 1, tab_count);

  // Uninstall the extension while the dialog is being shown.
  // Although the dialog is modal, a user can still uninstall the extension by
  // other means, e.g. if the user had two browser windows open they can use the
  // second browser window that does not contain the modal dialog, navigate to
  // chrome://extensions and uninstall the extension.
  UninstallExtension(extension->id());
}

// Tests that no error appears if the user disabled the extension.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest, UserDisabled) {
  const Extension* extension = InstallIncreasingPermissionExtensionV1();
  DisableExtension(extension->id());
  extension = UpdateIncreasingPermissionExtension(extension, path_v2_, 0);
  ASSERT_FALSE(GetExtensionDisabledGlobalError());
}

// Test that an error appears if the extension gets disabled because a
// version with higher permissions was installed by sync.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest,
                       HigherPermissionsFromSync) {
  // Get sync data for extension v2 (disabled).
  const Extension* extension = InstallAndUpdateIncreasingPermissionsExtension();
  std::string extension_id = extension->id();
  ExtensionSyncService* sync_service = ExtensionSyncService::Get(profile());
  extensions::ExtensionSyncData sync_data =
      sync_service->CreateSyncData(*extension);
  UninstallExtension(extension_id);
  extension = nullptr;

  // Install extension v1.
  InstallIncreasingPermissionExtensionV1();

  content::URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](content::URLLoaderInterceptor::RequestParams* params) {
        std::string path = params->url_request.url.GetPath();
        if (path == "/autoupdate/updates.xml") {
          content::URLLoaderInterceptor::WriteResponse(
              test_data_dir_.AppendASCII("permissions_increase")
                  .AppendASCII("updates.json"),
              params->client.get());
          return true;
        } else if (path == "/autoupdate/v2.crx") {
          content::URLLoaderInterceptor::WriteResponse(path_v2_,
                                                       params->client.get());
          return true;
        }
        return false;
      }));

  sync_service->MergeDataAndStartSyncing(
      syncer::EXTENSIONS, syncer::SyncDataList(),
      std::make_unique<syncer::FakeSyncChangeProcessor>());
  extensions::TestExtensionRegistryObserver install_observer(
      extension_registry());
  sync_service->ProcessSyncChanges(
      FROM_HERE,
      syncer::SyncChangeList(
          1, sync_data.GetSyncChange(syncer::SyncChange::ACTION_ADD)));

  install_observer.WaitForExtensionWillBeInstalled();
  content::RunAllTasksUntilIdle();

  extension = extension_registry()->disabled_extensions().GetByID(extension_id);
  ASSERT_TRUE(extension);
  EXPECT_EQ("2", extension->VersionString());
  EXPECT_EQ(1u, extension_registry()->disabled_extensions().size());
  EXPECT_THAT(ExtensionPrefs::Get(extension_service()->profile())
                  ->GetDisableReasons(extension_id),
              testing::UnorderedElementsAre(
                  extensions::disable_reason::DISABLE_PERMISSIONS_INCREASE));
  EXPECT_TRUE(GetExtensionDisabledGlobalError());
}

// Test that an error appears if an extension gets installed server side.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest, RemoteInstall) {
  static const char extension_id[] = "pgdpcfcocojkjfbgpiianjngphoopgmo";

  auto reset = extensions::DisablePublisherKeyVerificationForTests();
  content::URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](content::URLLoaderInterceptor::RequestParams* params) {
        std::string path = params->url_request.url.GetPath();
        if (path == "/autoupdate_nonwebstore/updates.xml") {
          content::URLLoaderInterceptor::WriteResponse(
              test_data_dir_.AppendASCII("permissions_increase")
                  .AppendASCII("updates.xml"),
              params->client.get());
          return true;
        } else if (path == "/autoupdate/v2.crx") {
          content::URLLoaderInterceptor::WriteResponse(path_v2_,
                                                       params->client.get());
          return true;
        }
        return false;
      }));

  sync_pb::EntitySpecifics specifics;
  specifics.mutable_extension()->set_id(extension_id);
  specifics.mutable_extension()->set_enabled(false);
  specifics.mutable_extension()->set_remote_install(true);
  specifics.mutable_extension()->set_disable_reasons(
      extensions::disable_reason::DISABLE_REMOTE_INSTALL);
  specifics.mutable_extension()->set_update_url(
      "http://localhost/autoupdate_nonwebstore/updates.xml");
  specifics.mutable_extension()->set_version("2");
  syncer::SyncData sync_data = syncer::SyncData::CreateRemoteData(
      specifics, syncer::ClientTagHash::FromHashed("unused"));

  ExtensionSyncService* sync_service = ExtensionSyncService::Get(profile());
  sync_service->MergeDataAndStartSyncing(
      syncer::EXTENSIONS, syncer::SyncDataList(),
      std::make_unique<syncer::FakeSyncChangeProcessor>());
  extensions::TestExtensionRegistryObserver install_observer(
      extension_registry());
  sync_service->ProcessSyncChanges(
      FROM_HERE,
      syncer::SyncChangeList(
          1, syncer::SyncChange(FROM_HERE, syncer::SyncChange::ACTION_ADD,
                                sync_data)));

  install_observer.WaitForExtensionWillBeInstalled();
  content::RunAllTasksUntilIdle();

  const Extension* extension =
      extension_registry()->disabled_extensions().GetByID(extension_id);
  ASSERT_TRUE(extension);
  EXPECT_EQ("2", extension->VersionString());
  EXPECT_EQ(1u, extension_registry()->disabled_extensions().size());
  EXPECT_THAT(ExtensionPrefs::Get(extension_service()->profile())
                  ->GetDisableReasons(extension_id),
              testing::UnorderedElementsAre(
                  extensions::disable_reason::DISABLE_REMOTE_INSTALL));
  EXPECT_TRUE(GetExtensionDisabledGlobalError());
}

// Test that an error appears if an extension gets installed server side.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest,
                       RemoteInstallFromWebstore) {
  static const char extension_id[] = "pgdpcfcocojkjfbgpiianjngphoopgmo";

  auto reset = extensions::DisablePublisherKeyVerificationForTests();
  content::URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](content::URLLoaderInterceptor::RequestParams* params) {
        std::string path = params->url_request.url.GetPath();
        if (path == "/autoupdate/updates.xml") {
          content::URLLoaderInterceptor::WriteResponse(
              test_data_dir_.AppendASCII("permissions_increase")
                  .AppendASCII("updates.json"),
              params->client.get());
          return true;
        } else if (path == "/autoupdate/v2.crx") {
          content::URLLoaderInterceptor::WriteResponse(path_v2_,
                                                       params->client.get());
          return true;
        }
        return false;
      }));

  sync_pb::EntitySpecifics specifics;
  specifics.mutable_extension()->set_id(extension_id);
  specifics.mutable_extension()->set_enabled(false);
  specifics.mutable_extension()->set_remote_install(true);
  specifics.mutable_extension()->set_disable_reasons(
      extensions::disable_reason::DISABLE_REMOTE_INSTALL);
  specifics.mutable_extension()->set_update_url(
      "http://localhost/autoupdate/updates.xml");
  specifics.mutable_extension()->set_version("2");
  syncer::SyncData sync_data = syncer::SyncData::CreateRemoteData(
      specifics, syncer::ClientTagHash::FromHashed("unused"));

  ExtensionSyncService* sync_service = ExtensionSyncService::Get(profile());
  sync_service->MergeDataAndStartSyncing(
      syncer::EXTENSIONS, syncer::SyncDataList(),
      std::make_unique<syncer::FakeSyncChangeProcessor>());
  extensions::TestExtensionRegistryObserver install_observer(
      extension_registry());
  sync_service->ProcessSyncChanges(
      FROM_HERE,
      syncer::SyncChangeList(
          1, syncer::SyncChange(FROM_HERE, syncer::SyncChange::ACTION_ADD,
                                sync_data)));

  install_observer.WaitForExtensionWillBeInstalled();
  content::RunAllTasksUntilIdle();

  const Extension* extension =
      extension_registry()->disabled_extensions().GetByID(extension_id);
  ASSERT_TRUE(extension);
  EXPECT_EQ("2", extension->VersionString());
  EXPECT_EQ(1u, extension_registry()->disabled_extensions().size());
  EXPECT_THAT(ExtensionPrefs::Get(extension_service()->profile())
                  ->GetDisableReasons(extension_id),
              testing::UnorderedElementsAre(
                  extensions::disable_reason::DISABLE_REMOTE_INSTALL));
  EXPECT_TRUE(GetExtensionDisabledGlobalError());
}

namespace {
int GetExtensionDisabledErrorCount(GlobalErrorService* service,
                                   const std::u16string_view extension_name) {
  DCHECK(!extension_name.empty());
  DCHECK(service);
  return std::ranges::count_if(service->errors(), [extension_name](
                                                      GlobalError* error) {
    return error->MenuItemCommandID() >= IDC_EXTENSION_INSTALL_ERROR_FIRST &&
           error->MenuItemCommandID() <= IDC_EXTENSION_INSTALL_ERROR_LAST &&
           error->MenuItemLabel().find(extension_name) != std::string::npos;
  });
}
}  // namespace

IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest,
                       AllErrorsRemovedWhenExtensionRemoved) {
  const Extension* extension = InstallIncreasingPermissionExtensionV1();
  ASSERT_TRUE(extension);
  AddExtensionDisabledError(browser()->profile(), extension, false);
  extension = UpdateIncreasingPermissionExtension(extension, path_v2_, -1);
  ASSERT_TRUE(extension);

  const auto extension_name = base::UTF8ToUTF16(extension->name());
  auto* global_error_service =
      GlobalErrorServiceFactory::GetForProfile(profile());
  // There must be two errors associated with the same extension; if this is not
  // true, then this test isn't relevant anymore.
  EXPECT_EQ(
      GetExtensionDisabledErrorCount(global_error_service, extension_name), 2);

  // Remove extension and make sure no errors left.
  extensions::TestExtensionRegistryObserver test_observer(extension_registry(),
                                                          extension->id());
  UninstallExtension(extension->id());
  test_observer.WaitForExtensionUninstalled();
  // All ExtensionDisabledGlobalErrors related to removed extension should be
  // removed too.
  EXPECT_EQ(
      GetExtensionDisabledErrorCount(global_error_service, extension_name), 0);
}

// Tests that GetGlobalErrors() survives when the ExternalInstallError
// backing a VivaldiExtensionDisabledGlobalError is destroyed before the
// errors are queried. This is the exact production crash path for VB-122765:
// the ExternalInstallError is destroyed (e.g. during uninstall or shutdown),
// expiring the WeakPtr inside VivaldiExtensionDisabledGlobalError, so
// GetExtension() returns null. Calling GetExtension()->id() on null crashes.
//
// Fix: use GetExtensionId() (cached std::string) instead of
// GetExtension()->id() in GetGlobalErrors().
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest,
                       VivaldiGetGlobalErrorsAfterUninstall) {
  const Extension* extension = InstallIncreasingPermissionExtensionV1();
  ASSERT_TRUE(extension);
  std::string extension_id = extension->id();

  auto* root_doc_handler =
      extensions::VivaldiRootDocumentHandlerFactory::GetForBrowserContext(
          profile());
  ASSERT_TRUE(root_doc_handler);

  // Create a mock ExternalInstallError and use the second constructor,
  // matching what ExternalInstallErrorDesktop::OnDialogReady() does in
  // production (external_install_error_desktop.cc:491).
  auto mock_error = std::make_unique<MockExternalInstallError>(extension);
  base::WeakPtr<extensions::ExternalInstallError> weak_mock =
      mock_error->GetWeakPtr();

  root_doc_handler->AddGlobalError(
      std::make_unique<extensions::VivaldiExtensionDisabledGlobalError>(
          profile(), std::move(weak_mock)));

  // Verify the Vivaldi root document handler has the error.
  {
    const auto& errors = root_doc_handler->errors();
    EXPECT_GT(errors.size(), 0u);
    bool found = std::any_of(errors.begin(), errors.end(),
                             [extension_id](const auto& err) {
                               return err->GetExtensionId() == extension_id;
                             });
    EXPECT_TRUE(found);
  }

  // Destroy the mock ExternalInstallError. This expires the WeakPtr inside
  // VivaldiExtensionDisabledGlobalError, so GetExtension() now returns null.
  // In production this happens when ExternalInstallErrorDesktop::RemoveError()
  // destroys the object during uninstall/shutdown.
  mock_error.reset();

  // GetGlobalErrors must survive even though GetExtension() returns null.
  // Without the fix, GetExtension()->id() dereferences null → segfault.
  // With the fix, GetExtensionId() returns the cached string → clean.
  std::vector<ExtensionInstallError*> jserrors;
  bool ok = VivaldiBrowserComponentWrapper::GetInstance()
                ->GetGlobalErrors(profile(), jserrors);
  EXPECT_TRUE(ok);

  // The error for this extension should still be present with correct data.
  bool found = false;
  for (ExtensionInstallError* jserror : jserrors) {
    auto* actual = reinterpret_cast<
        extensions::vivaldi::extension_action_utils::ExtensionInstallError*>(
        jserror);
    if (actual->id == extension_id) {
      found = true;
      EXPECT_EQ(actual->name, extension->name());
    }
  }
  EXPECT_TRUE(found);
}

// Tests that VivaldiExtensionDisabledGlobalError is properly removed from
// errors_ when the extension is uninstalled, preventing stale entries.
//
// Without the fix: RemoveGlobalError() was commented out, so errors remained
// in errors_ after uninstall → GetGlobalErrors() returns stale entries.
// With the fix: RemoveGlobalError() extracts and erases the error from errors_.
IN_PROC_BROWSER_TEST_F(ExtensionDisabledGlobalErrorTest,
                       VivaldiGlobalErrorRemovedOnUninstall) {
  const Extension* extension = InstallIncreasingPermissionExtensionV1();
  ASSERT_TRUE(extension);
  std::string extension_id = extension->id();

  auto* root_doc_handler =
      extensions::VivaldiRootDocumentHandlerFactory::GetForBrowserContext(
          profile());
  ASSERT_TRUE(root_doc_handler);

  auto mock_error = std::make_unique<MockExternalInstallError>(extension);
  base::WeakPtr<extensions::ExternalInstallError> weak_mock =
      mock_error->GetWeakPtr();

  root_doc_handler->AddGlobalError(
      std::make_unique<extensions::VivaldiExtensionDisabledGlobalError>(
          profile(), std::move(weak_mock)));

  // Verify the error is in errors_.
  {
    const auto& errors = root_doc_handler->errors();
    bool found = std::any_of(errors.begin(), errors.end(),
                             [extension_id](const auto& err) {
                               return err->GetExtensionId() == extension_id;
                             });
    EXPECT_TRUE(found);
  }

  // Uninstall the extension. This triggers OnExtensionUninstalled() →
  // RemoveGlobalError(), which should extract and erase the error from errors_.
  UninstallExtension(extension_id);

  // Process pending tasks (DeleteSoon defers deletion).
  content::RunAllTasksUntilIdle();

  // Verify the error was removed from errors_.
  // Without the fix: error remains in errors_ → stale entry.
  // With the fix: error is erased from errors_.
  {
    const auto& errors = root_doc_handler->errors();
    bool found = std::any_of(errors.begin(), errors.end(),
                             [extension_id](const auto& err) {
                               return err->GetExtensionId() == extension_id;
                             });
    EXPECT_FALSE(found);
  }

  mock_error.reset();
}
