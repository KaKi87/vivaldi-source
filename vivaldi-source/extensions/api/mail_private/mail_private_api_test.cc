#include "extensions/api/mail_private/mail_private_api.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "extensions/browser/api_test_utils.h"
#include "extensions/browser/extension_function.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {
constexpr std::string_view kInitialEmailData =
    "X-Vivaldi-Status: 110000000000--------\r\n"
    "X-Vivaldi-Location: Inbox                                               "
    "      \r\n"
    "Return-Path: <noreply+de309110@eu-notifications.some.com>\r\n"
    "Delivered-To: thedude@lebowsky.net\r\n"
    "\r\n"
    "This is the body of the test email.";

constexpr std::string_view kInitialInvalidEmailData =
    "X-Vivaldi-Status: 110000000000--------SSS\r\n"
    "X-Vivaldi-Location: Inbox                                               "
    "      \r\n"
    "Return-Path: <noreply+de309110@eu-notifications.some.com>\r\n"
    "Delivered-To: thedude@lebowsky.net\r\n"
    "\r\n"
    "This is the body of the test email.";
}  // namespace

class MailPrivateWriteVivaldiHeadersTest : public ExtensionServiceTestBase {
 protected:
  void SetUp() override {
    ExtensionServiceTestBase::SetUp();
    InitializeEmptyExtensionService();
  }

  void TearDown() override {
    // Clean up the mail directory
    base::FilePath mail_dir = profile()->GetPath().AppendASCII("Mail");
    EXPECT_TRUE(base::DeletePathRecursively(mail_dir));
    ExtensionServiceTestBase::TearDown();
  }

  base::FilePath CreateMessageFile(const std::string& account_id,
                                   const std::string& file_name,
                                   bool invalid = false) {
    base::FilePath target_folder =
        profile()->GetPath().AppendASCII("Mail").AppendASCII(account_id);
    base::FilePath final_file_path = target_folder.AppendASCII(file_name);

    EXPECT_TRUE(base::CreateDirectory(target_folder));

    if (invalid) {
      EXPECT_TRUE(base::WriteFile(final_file_path, kInitialInvalidEmailData));
    } else {
      EXPECT_TRUE(base::WriteFile(final_file_path, kInitialEmailData));
    }

    return final_file_path;
  }

  std::string GenerateArgsJson(const std::string& status,
                               const std::string& location,
                               const std::string& account_id,
                               const std::string& file_name) {
    base::ListValue args_list;
    args_list.Append(status);
    args_list.Append(location);

    base::ListValue paths_array;
    paths_array.Append(account_id);
    args_list.Append(std::move(paths_array));

    args_list.Append(file_name);

    std::string args_json;
    base::JSONWriter::Write(args_list, &args_json);
    return args_json;
  }
};

TEST_F(MailPrivateWriteVivaldiHeadersTest, WritesHeadersSuccessfully) {
  base::FilePath file_path = CreateMessageFile("account1", "test_message.eml");
  std::string args_json = GenerateArgsJson("111111111111--------", "Archive",
                                           "account1", "test_message.eml");
  auto function =
      base::MakeRefCounted<MailPrivateWriteVivaldiHeadersFunction>();
  EXPECT_TRUE(
      api_test_utils::RunFunction(function.get(), args_json, profile()));

  // Verify the file headers were successfully updated
  std::string modified_file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &modified_file_contents));

  EXPECT_TRUE(modified_file_contents.find(
                  "X-Vivaldi-Status: 111111111111--------") != std::string::npos);
  EXPECT_TRUE(modified_file_contents.find("X-Vivaldi-Location: Archive") !=
              std::string::npos);
}

TEST_F(MailPrivateWriteVivaldiHeadersTest, ReturnsEmptyResultOnSuccess) {
  CreateMessageFile("account1", "test_message.eml");
  std::string args_json = GenerateArgsJson("111111111111--------", "Archive",
                                           "account1", "test_message.eml");
  auto function =
      base::MakeRefCounted<MailPrivateWriteVivaldiHeadersFunction>();

  std::optional<base::Value> result =
      api_test_utils::RunFunctionAndReturnSingleResult(function.get(),
                                                       args_json, profile());

  std::string api_response;

  // Check if the optional actually holds a value
  if (result.has_value()) {
    // If it's a value, ensure it's a string, then grab it
    ASSERT_TRUE(result->is_string());
    api_response = result->GetString();
  } else {
    // If it's std::nullopt, the API returned no arguments (void success)
    api_response = "";
  }

  EXPECT_TRUE(api_response.empty())
      << "API reported an error: " << api_response;
}

TEST_F(MailPrivateWriteVivaldiHeadersTest,
       ReturnsErrorMessageWhenFileDoesNotExist) {
  std::string args_json = GenerateArgsJson("111111111111--------", "Archive",
                                           "account1", "not_exists.eml");
  auto function =
      base::MakeRefCounted<MailPrivateWriteVivaldiHeadersFunction>();

  bool success =
      api_test_utils::RunFunction(function.get(), args_json, profile());
  EXPECT_FALSE(success);

  std::string error_message = function->GetError();

  EXPECT_FALSE(error_message.empty());
  EXPECT_THAT(error_message,
              testing::StartsWith("Failed to open message file:"));
  EXPECT_THAT(error_message, testing::HasSubstr("not_exists.eml"));
}

TEST_F(MailPrivateWriteVivaldiHeadersTest,
       ReturnsErrorMessageWhenLocationIsToLong) {
  CreateMessageFile("account1", "test_message.eml");

  const std::string long_location_header =  // clang-format off
      "ArchiveLongerThan80, Nordvestanattarstadur, Herdisarvikurfjallsskridur, Austurbaejarhjallbakki";
                                            // clang-format on

  std::string args_json = GenerateArgsJson(
      "111111111111--------", long_location_header, "account1", "test_message.eml");
  auto function =
      base::MakeRefCounted<MailPrivateWriteVivaldiHeadersFunction>();

  bool success =
      api_test_utils::RunFunction(function.get(), args_json, profile());

  EXPECT_FALSE(success);

  std::string error_message = function->GetError();
  LOG(ERROR) << "error: " << error_message;
  EXPECT_FALSE(error_message.empty());

  EXPECT_THAT(
      error_message,
      testing::HasSubstr("Location header value to be written is too long."));
}

TEST_F(MailPrivateWriteVivaldiHeadersTest,
       NotReturnErrorMessageWhenReadingToLongLocation) {
  base::FilePath file_path =
      CreateMessageFile("account1", "test_message.eml", true);

  std::string args_json = GenerateArgsJson("111111111111--------", "Inbox",
                                           "account1", "test_message.eml");
  auto function =
      base::MakeRefCounted<MailPrivateWriteVivaldiHeadersFunction>();

  EXPECT_TRUE(
      api_test_utils::RunFunction(function.get(), args_json, profile()));

  std::string modified_file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &modified_file_contents));

  EXPECT_TRUE(modified_file_contents.find(
                  "X-Vivaldi-Status: 111111111111--------") != std::string::npos);
}

}  // namespace extensions
