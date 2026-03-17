// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include "vivaldi_md5.h"

#include "crypto/obsolete/md5.h"

namespace vivaldi {
namespace obsolete {

std::array<uint8_t, 16> ObsoleteMd5Hash(std::string_view data) {
  return crypto::obsolete::Md5::Hash(data);
}

std::array<uint8_t, 16> ObsoleteMd5Hash(base::span<const uint8_t> data) {
  return crypto::obsolete::Md5::Hash(data);
}

}  // namespace obsolete
}  // namespace vivaldi
