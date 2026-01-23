// Copyright 2025 Vivaldi Technologies. All rights reserved.

#include "ios/favicon/vivaldi_favicon_loader.h"

#include <UIKit/UIKit.h>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/sys_string_conversions.h"
#include "components/favicon_base/favicon_types.h"
#include "components/favicon/core/fallback_url_util.h"
#include "ios/chrome/common/ui/favicon/favicon_attributes.h"
#include "ios/favicon/vivaldi_favicon_service.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace {

int PointsToPixels(float points) {
  if (points <= 0) {
    return 0;
  }
  // Follow scaling approach from Chromium.
  const CGFloat scale = UIScreen.mainScreen.scale;
  return static_cast<int>(scale * points);
}

// Helper function to create FaviconAttributes from LargeIconResult with
// Chromium's validation, following the same pattern as FaviconLoaderImpl.
FaviconAttributes* CreateValidatedFaviconAttributes(
    const GURL& icon_url,
    const favicon_base::LargeIconResult& result) {
  // Follow Chromium's FaviconLoaderImpl pattern for handling LargeIconResult
  if (result.bitmap.is_valid()) {
    scoped_refptr<base::RefCountedMemory> data =
        result.bitmap.bitmap_data.get();

    // The favicon code assumes favicons are PNG-encoded.
    UIImage* favicon = [UIImage
        imageWithData:[NSData dataWithBytes:data->front() length:data->size()]];

    if (favicon && favicon.size.width > 0 && favicon.size.height > 0) {
      return [FaviconAttributes attributesWithImage:favicon];
    }
  }

  // Did not get valid favicon, return default
  FaviconAttributes* attributes = [FaviconAttributes
      attributesWithMonogram:base::SysUTF16ToNSString(
                                 favicon::GetFallbackIconText(icon_url))
                   textColor:[UIColor colorWithWhite:
                                          kFallbackIconDefaultTextColorGrayscale
                                               alpha:1]
             backgroundColor:UIColor.clearColor
      defaultBackgroundColor:YES];
  return attributes;
}

}  // namespace

namespace ios {

namespace {

void RunCompletion(FaviconAttributesCompletionBlock completion,
                   FaviconAttributes* attributes,
                   bool cached) {
#ifdef __OBJC__
  completion(attributes, cached);
#else
  std::move(completion).Run(attributes, cached);
#endif
}

}  // namespace

VivaldiFaviconLoader::VivaldiFaviconLoader(VivaldiFaviconService* service)
    : service_(service) {
  CHECK(service_);
}

VivaldiFaviconLoader::~VivaldiFaviconLoader() = default;

// Helper to create a callback that converts LargeIconResult to
// FaviconAttributes
base::OnceCallback<void(const favicon_base::LargeIconResult&)>
VivaldiFaviconLoader::CreateLargeIconResultToAttributesCallback(
    const GURL& icon_url,
    FaviconAttributesCompletionBlock completion,
    bool cached) {
  return base::BindOnce(
      [](FaviconAttributesCompletionBlock completion, bool cached,
         const GURL& icon_url,
         const favicon_base::LargeIconResult& result) {
        FaviconAttributes* attributes =
            CreateValidatedFaviconAttributes(icon_url, result);
        RunCompletion(completion, attributes, cached);
      },
      completion, cached, icon_url);
}

// Generic helper for handling favicon requests with different service methods
template <typename ServiceMethod>
void VivaldiFaviconLoader::RequestFaviconWithMode(
    const GURL& icon_url,
    ServiceMethod service_method,
    VivaldiFaviconOptions options,
    FaviconAttributesCompletionBlock completion) {
  // Consider responses cached unless explicitly forcing
  // a fresh download.
  bool cached = options != VivaldiFaviconOptions::kAlwaysFresh;
  auto callback = CreateLargeIconResultToAttributesCallback(
      icon_url, completion, cached);
  service_method(options, std::move(callback));
}

void VivaldiFaviconLoader::GetFaviconForPageUrl(
    const GURL& page_url,
    float size_in_points,
    float min_size_in_points,
    VivaldiFaviconMode mode,
    FaviconAttributesCompletionBlock completion) {
  auto service_method = [this, page_url, size_in_points, min_size_in_points](
                            VivaldiFaviconOptions opts,
                            VivaldiFaviconCallback callback) {
    service_->FaviconForPageUrl(page_url, PointsToPixels(size_in_points),
                                PointsToPixels(min_size_in_points), opts,
                                std::move(callback));
  };

  RequestFaviconWithMode(page_url, service_method, mode, completion);
}

void VivaldiFaviconLoader::GetFaviconForIconUrl(
    const GURL& icon_url,
    float size_in_points,
    float min_size_in_points,
    VivaldiFaviconMode mode,
    FaviconAttributesCompletionBlock completion) {
  auto service_method = [this, icon_url, size_in_points, min_size_in_points](
                            VivaldiFaviconOptions opts,
                            VivaldiFaviconCallback callback) {
    service_->FaviconForIconUrl(icon_url, PointsToPixels(size_in_points),
                                PointsToPixels(min_size_in_points), opts,
                                std::move(callback));
  };

  RequestFaviconWithMode(icon_url, service_method, mode, completion);
}

void VivaldiFaviconLoader::GetFaviconForPageUrlOrHost(
    const GURL& page_url,
    float size_in_points,
    VivaldiFaviconMode mode,
    FaviconAttributesCompletionBlock completion) {
  auto service_method = [this, page_url, size_in_points](
                            VivaldiFaviconOptions opts,
                            VivaldiFaviconCallback callback) {
    service_->FaviconForPageUrlOrHost(page_url, PointsToPixels(size_in_points),
                                      opts, std::move(callback));
  };

  RequestFaviconWithMode(page_url, service_method, mode, completion);
}

void VivaldiFaviconLoader::FaviconForPageUrl(
    const GURL& page_url,
    float size_in_points,
    float min_size_in_points,
    bool fallback_to_google_server,
    FaviconAttributesCompletionBlock favicon_block_handler) {
  GetLargeIconRawBitmapOrFallbackStyleForPageUrl(
      page_url, min_size_in_points, size_in_points, favicon_block_handler);
}

void VivaldiFaviconLoader::FaviconForPageUrlOrHost(
    const GURL& page_url,
    float size_in_points,
    FaviconAttributesCompletionBlock favicon_block_handler) {
  GetIconRawBitmapOrFallbackStyleForPageUrl(page_url, size_in_points,
                                            favicon_block_handler);
}

void VivaldiFaviconLoader::FaviconForIconUrl(
    const GURL& icon_url,
    float size_in_points,
    float min_size_in_points,
    FaviconAttributesCompletionBlock favicon_block_handler) {
  GetLargeIconRawBitmapOrFallbackStyleForIconUrl(
      icon_url, min_size_in_points, size_in_points, favicon_block_handler);
}

void VivaldiFaviconLoader::CancelAllRequests() {
  service_->CancelAllRequests();
}

void VivaldiFaviconLoader::GetLargeIconRawBitmapOrFallbackStyleForPageUrl(
    const GURL& page_url,
    float min_size_in_points,
    float size_in_points,
    FaviconAttributesCompletionBlock completion) {
  auto cpp_callback = CreateLargeIconResultToAttributesCallback(page_url,
                                                                completion,
                                                                false);

  service_->GetLargeIconRawBitmapOrFallbackStyleForPageUrl(
      page_url, PointsToPixels(min_size_in_points),
      PointsToPixels(size_in_points), std::move(cpp_callback));
}

void VivaldiFaviconLoader::GetIconRawBitmapOrFallbackStyleForPageUrl(
    const GURL& page_url,
    float size_in_points,
    FaviconAttributesCompletionBlock completion) {
  auto cpp_callback = CreateLargeIconResultToAttributesCallback(page_url,
                                                                completion,
                                                                false);

  service_->GetIconRawBitmapOrFallbackStyleForPageUrl(
      page_url, PointsToPixels(size_in_points), std::move(cpp_callback));
}

void VivaldiFaviconLoader::GetLargeIconRawBitmapOrFallbackStyleForIconUrl(
    const GURL& icon_url,
    float min_size_in_points,
    float size_in_points,
    FaviconAttributesCompletionBlock completion) {
  auto cpp_callback = CreateLargeIconResultToAttributesCallback(icon_url,
                                                                completion,
                                                                false);

  service_->GetLargeIconRawBitmapOrFallbackStyleForIconUrl(
      icon_url, PointsToPixels(min_size_in_points),
      PointsToPixels(size_in_points), std::move(cpp_callback));
}

}  // namespace ios
