// Included at top of //crypto/obsolete/md5.h
// Declares types of friends of the MD5 class

namespace vivaldi {
class NotesCodec;

namespace obsolete {
std::array<uint8_t, 16> ObsoleteMd5Hash(std::string_view data);
std::array<uint8_t, 16> ObsoleteMd5Hash(::base::span<const uint8_t> data);
}  // namespace base

}
