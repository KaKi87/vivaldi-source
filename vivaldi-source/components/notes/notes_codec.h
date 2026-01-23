// Copyright (c) 2013-2017 Vivaldi Technologies AS. All rights reserved
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NOTES_NOTES_CODEC_H_
#define COMPONENTS_NOTES_NOTES_CODEC_H_

#include <set>
#include <string>
#include <vector>

#include "base/uuid.h"
#include "components/notes/note_node.h"
#include "components/notes/notes_model.h"
#include "crypto/hash.h"
#include "crypto/obsolete/md5.h"

namespace base {
class Value;
}  // namespace base

namespace vivaldi {

// NotesCodec is responsible for encoding and decoding the NotesModel
// into JSON values. The encoded values are written to disk via the
// NotesStorage.
class NotesCodec {
 public:
  // Creates an instance of the codec. During decoding, if the IDs in the file
  // are not unique, we will reassign IDs to make them unique. There are no
  // guarantees on how the IDs are reassigned or about doing minimal
  // reassignments to achieve uniqueness.
  NotesCodec();
  ~NotesCodec();
  NotesCodec(const NotesCodec&) = delete;
  NotesCodec& operator=(const NotesCodec&) = delete;

  // Encodes the model to a JSON value. This is invoked to encode the contents
  // of the notes model and is currently a convenience to invoking Encode that
  // takes the notes node and other folder node.
  base::Value::Dict Encode(NotesModel* model,
                           const std::string& sync_metadata_str);

  // Encodes the notes folder returning the JSON value.
  base::Value::Dict Encode(const NoteNode* notes_node,
                           const NoteNode* other_notes_node,
                           const NoteNode* trash_notes_node,
                           std::string sync_metadata_str);

  // Decodes the previously encoded value to the specified nodes as well as
  // setting |max_node_id| to the greatest node id. Returns true on success,
  // false otherwise. If there is an error (such as unexpected version) all
  // children are removed from the notes and other folder nodes. On exit
  // |max_node_id| is set to the max id of the nodes.
  bool Decode(NoteNode* notes_node,
              NoteNode* other_notes_node,
              NoteNode* trash_notes_node,
              int64_t* max_node_id,
              const base::Value::Dict& value,
              std::string* sync_metadata_str);

  // Updates the check-sum with the given string.
  void UpdateChecksum(const std::string& str);
  void UpdateChecksum(const std::u16string& str);

  // Returns whether the IDs were reassigned during decoding. Always returns
  // false after encoding.
  bool ids_reassigned() const { return ids_reassigned_; }

  // Returns whether the UUIDs were reassigned during decoding. Always returns
  // false after encoding.
  bool uuids_reassigned() const { return uuids_reassigned_; }

  // Returns whether attachments using the old, deprecated format were found
  // during decoding.
  bool has_deprecated_attachments() const {
    return has_deprecated_attachments_;
  }

  // Names of the various keys written to the Value.
  static const char kVersionKey[];
  static const char kChecksumKey[];
  static const char kChecksumSHA256Key[];
  static const char kIdKey[];
  static const char kTypeKey[];
  static const char kSubjectKey[];
  static const char kDateAddedKey[];
  static const char kDateModifiedKey[];
  static const char kURLKey[];
  static const char kChildrenKey[];
  static const char kContentKey[];
  static const char kGuidKey[];
  static const char kAttachmentsKey[];
  static const char kSyncMetadata[];
  static const char kTypeNote[];
  static const char kTypeFolder[];
  static const char kTypeSeparator[];
  static const char kTypeAttachment[];
  static const char kTypeOther[];
  static const char kTypeTrash[];

 private:
  // Encodes node and all its children into a Value object and returns it.
  // The caller takes ownership of the returned object.
  base::Value::Dict EncodeNode(const NoteNode* node,
                               const std::vector<const NoteNode*>* extra_nodes);

  // Helper to perform decoding.
  bool DecodeHelper(NoteNode* notes_node,
                    NoteNode* other_notes_node,
                    NoteNode* trash_notes_node,
                    const base::Value::Dict& value,
                    std::string* sync_metadata_str);
  void ExtractSpecialNode(NoteNode::Type type,
                          NoteNode* source,
                          NoteNode* target);

  // Reassigns note IDs for those that require doing so (if any).
  void ReassignIDsIfRequired();

  // Decodes the supplied node from the supplied value. Child nodes are
  // created appropriately by way of DecodeChildren. If node is NULL a new
  // node is created and added to parent (parent must then be non-NULL),
  // otherwise node is used.
  void DecodeNode(const base::Value::Dict& value,
                  NoteNode* parent,
                  NoteNode* node,
                  NoteNode* child_other_node,
                  NoteNode* child_trash_node);

  // Updates the check-sum with the given contents of the note node/folder.
  // NOTE: These functions take in individual properties of a note node
  // instead of taking in a NoteNode for efficiency so that we don't convert
  // various data-types to UTF16 strings multiple times - once for serializing
  // and once for computing the check-sum.
  // The url parameter should be a valid UTF8 string.
  void UpdateChecksumWithNoteNode(const std::string& id,
                                  const std::u16string& title,
                                  const std::string& type,
                                  const std::u16string& content,
                                  const std::string& url);
  void UpdateChecksumWithNoteFolder(const std::string& id,
                                    const std::u16string& title,
                                    const std::string& type);

  // Initializes/Finalizes the checksum.
  void InitializeChecksum();
  void FinalizeChecksum();

  // Whether or not IDs were reassigned by the codec.
  bool ids_reassigned_{false};

  // Nodes with an invalid ID, which require reassignment.
  std::vector<raw_ptr<NoteNode>> nodes_requiring_id_reassignment_;

  // Mapping from old ID to new IDs if IDs were reassigned. Note that old IDs
  // may contain duplicates, and therefore the mapping could be ambiguous.
  std::multimap<int64_t, int64_t> reassigned_ids_per_old_id_;

  // Whether or not UUIDs were reassigned by the codec.
  bool uuids_reassigned_{false};

  // Whether the loaded notes have attachments using the old, deprecated format.
  bool has_deprecated_attachments_{false};

  // Contains the id of each of the nodes found in the file. Used to determine
  // if we have duplicates.
  std::set<int64_t> ids_;

  // Contains the UUID of each of the nodes found in the file. Used to determine
  // if we have duplicates.
  std::set<base::Uuid> uuids_;

  // MD5 context used to compute MD5 hash of all notes data.
  crypto::obsolete::Md5 md5_hasher_;

  // SHA context used to compute SHA256 hash of all note data.
  // Intended to replace MD5 hasher (crbug.com/426243026)
  crypto::hash::Hasher sha256_hasher_{crypto::hash::kSha256};

  // MD5 checksum computed during last encoding call.
  std::string computed_checksum_;

  // SHA256 checksum computed during last encoding call.
  std::string computed_sha256_checksum_;

  // Maximum ID assigned when decoding data.
  int64_t maximum_id_{0};
};

}  // namespace vivaldi

#endif  // COMPONENTS_NOTES_NOTES_CODEC_H_
