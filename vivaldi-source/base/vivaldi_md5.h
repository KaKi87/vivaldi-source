// Copyright (c) 2025 Vivaldi Technologies AS. All rights reserved

#include <array>
#include <string_view>
#include "base/containers/span.h"

namespace vivaldi {
namespace obsolete {

std::array<uint8_t, 16> ObsoleteMd5Hash(std::string_view data);
std::array<uint8_t, 16> ObsoleteMd5Hash(::base::span<const uint8_t> data);

}  // namespace obsolete
}  // namespace vivaldi
