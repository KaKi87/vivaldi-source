// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved.

#ifndef IMPORTER_IMPORT_LIMITS_H_
#define IMPORTER_IMPORT_LIMITS_H_

#include <cstddef>

namespace vivaldi_importer {

// 64MB - upper bound on the size of any file read during import.
inline constexpr size_t kMaxImportFileSize = 64 * 1024 * 1024;

// Upper bound on the number of items reserved up front.
inline constexpr size_t kMaxImportItemsReserve = 100'000;

// Upper bound on the number extensions we import,
inline constexpr size_t kMaxImportedExtensions = 200;
// and how many pararell install jobs we run.
inline constexpr size_t kMaxConcurrentExtensionsInstalls = 5;

}  // namespace vivaldi_importer

#endif  // IMPORTER_IMPORT_LIMITS_H_
