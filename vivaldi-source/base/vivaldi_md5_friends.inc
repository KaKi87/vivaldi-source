// Included at top of //crypto/obsolete/md5.h
// Declares friends of the MD5 class

// namespace crypto::obsolete {
// class CRYPTO_EXPORT Md5 {

//  private:

  friend class vivaldi::NotesCodec;
  friend std::array<uint8_t, 16> vivaldi::obsolete::ObsoleteMd5Hash(std::string_view data);
  friend std::array<uint8_t, 16> vivaldi::obsolete::ObsoleteMd5Hash(base::span<const uint8_t> data);


// }
