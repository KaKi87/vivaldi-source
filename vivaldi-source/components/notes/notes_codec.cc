// Copyright (c) 2013-2017 Vivaldi Technologies AS. All rights reserved
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/notes/notes_codec.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "base/values.h"
#include "components/notes/note_node.h"
#include "crypto/hash.h"
#include "url/gurl.h"

using base::Time;

namespace vivaldi {

const char NotesCodec::kVersionKey[] = "version";
const char NotesCodec::kChecksumSHA256Key[] = "checksum_sha256";
const char NotesCodec::kChecksumKey[] = "checksum";
const char NotesCodec::kIdKey[] = "id";
const char NotesCodec::kTypeKey[] = "type";
const char NotesCodec::kSubjectKey[] = "subject";
const char NotesCodec::kGuidKey[] = "guid";
const char NotesCodec::kDateAddedKey[] = "date_added";
const char NotesCodec::kDateModifiedKey[] = "last_modified";
const char NotesCodec::kURLKey[] = "url";
const char NotesCodec::kChildrenKey[] = "children";
const char NotesCodec::kContentKey[] = "content";
const char NotesCodec::kAttachmentsKey[] = "attachments";
const char NotesCodec::kSyncMetadata[] = "sync_metadata";
const char NotesCodec::kTypeNote[] = "note";
const char NotesCodec::kTypeFolder[] = "folder";
const char NotesCodec::kTypeSeparator[] = "separator";
const char NotesCodec::kTypeAttachment[] = "attachment";
const char NotesCodec::kTypeOther[] = "other";
const char NotesCodec::kTypeTrash[] = "trash";

// Current version of the file.
static const int kCurrentVersion = 1;

namespace {
// Encodes Sync metadata and cleans up the input string to decrease peak memory
// usage during encoding.
base::Value EncodeSyncMetadata(std::string sync_metadata_str) {
  return base::Value(base::Base64Encode(sync_metadata_str));
}

// Helper function to convert Time to microseconds since Windows epoch.
int64_t ToMicrosecondsSinceWindowsEpoch(Time time) {
  return time.ToDeltaSinceWindowsEpoch().InMicroseconds();
}

// Helper function to parse date from dictionary, returns nullopt if not found.
std::optional<Time> FindMicrosecondsSinceWindowsEpoch(
    const base::DictValue& dict,
    std::string_view key) {
  const std::string* string_value = dict.FindString(key);
  if (!string_value) {
    return std::nullopt;
  }

  int64_t microseconds = 0;
  if (!base::StringToInt64(*string_value, &microseconds)) {
    return std::nullopt;
  }

  return Time::FromDeltaSinceWindowsEpoch(base::Microseconds(microseconds));
}

}  // namespace

NotesCodec::NotesCodec() = default;

NotesCodec::~NotesCodec() = default;

base::DictValue NotesCodec::Encode(NotesModel* model,
                                   const std::string& sync_metadata_str) {
  return Encode(model->main_node(), model->other_node(), model->trash_node(),
                sync_metadata_str);
}

base::DictValue NotesCodec::Encode(const NoteNode* notes_node,
                                   const NoteNode* other_notes_node,
                                   const NoteNode* trash_notes_node,
                                   std::string sync_metadata_str) {
  ids_reassigned_ = false;
  uuids_reassigned_ = false;

  std::vector<const NoteNode*> extra_nodes;
  extra_nodes.push_back(other_notes_node);
  extra_nodes.push_back(trash_notes_node);

  InitializeChecksum();
  base::DictValue main(EncodeNode(notes_node, &extra_nodes));
  main.Set(kVersionKey, kCurrentVersion);

  if (!sync_metadata_str.empty()) {
    main.Set(kSyncMetadata, EncodeSyncMetadata(sync_metadata_str));
    sync_metadata_str.clear();
  }

  FinalizeChecksum();
  main.Set(kChecksumKey, computed_checksum_);
  main.Set(kChecksumSHA256Key, computed_sha256_checksum_);

  return main;
}

bool NotesCodec::Decode(NoteNode* notes_node,
                        NoteNode* other_notes_node,
                        NoteNode* trash_notes_node,
                        int64_t* max_id,
                        const base::DictValue& value,
                        std::string* sync_metadata_str) {
  if (sync_metadata_str) {
    sync_metadata_str->clear();
  }

  ids_.clear();
  maximum_id_ = ids_.empty() ? 0 : *ids_.rbegin();
  uuids_ = {base::Uuid::ParseLowercase(NoteNode::kRootNodeUuid),
            base::Uuid::ParseLowercase(NoteNode::kMainNodeUuid),
            base::Uuid::ParseLowercase(NoteNode::kOtherNotesNodeUuid),
            base::Uuid::ParseLowercase(NoteNode::kTrashNodeUuid)};
  ids_reassigned_ = false;
  uuids_reassigned_ = false;
  nodes_requiring_id_reassignment_.clear();
  reassigned_ids_per_old_id_.clear();

  bool success = DecodeHelper(notes_node, other_notes_node, trash_notes_node,
                              value, sync_metadata_str);
  ReassignIDsIfRequired();

  *max_id = maximum_id_ + 1;
  return success;
}

base::DictValue NotesCodec::EncodeNode(
    const NoteNode* node,
    const std::vector<const NoteNode*>* extra_nodes) {
  base::DictValue value;
  std::string id = base::NumberToString(node->id());
  value.Set(kIdKey, id);
  std::u16string subject = node->GetTitle();
  value.Set(kSubjectKey, subject);
  const std::string& uuid = node->uuid().AsLowercaseString();
  value.Set(kGuidKey, uuid);

  std::string type;
  bool can_have_children = false;
  switch (node->type()) {
    case NoteNode::FOLDER:
    case NoteNode::MAIN:
      type = kTypeFolder;
      can_have_children = true;
      break;
    case NoteNode::NOTE:
      can_have_children = true;
      type = kTypeNote;
      break;
    case NoteNode::TRASH:
      type = kTypeTrash;
      can_have_children = true;
      break;
    case NoteNode::OTHER:
      type = kTypeOther;
      can_have_children = true;
      break;
    case NoteNode::SEPARATOR:
      type = kTypeSeparator;
      break;
    case NoteNode::ATTACHMENT:
      type = kTypeAttachment;
      break;
  }
  value.Set(kTypeKey, type);
  value.Set(kDateAddedKey, base::NumberToString(ToMicrosecondsSinceWindowsEpoch(
                               node->GetCreationTime())));
  value.Set(kDateModifiedKey,
            base::NumberToString(ToMicrosecondsSinceWindowsEpoch(
                node->GetLastModificationTime())));

  std::u16string content = node->GetContent();
  if (node->type() == NoteNode::NOTE || node->type() == NoteNode::ATTACHMENT) {
    value.Set(kContentKey, content);

    std::string url = node->GetURL().possibly_invalid_spec();
    value.Set(kURLKey, url);

    // NOTE(julien): Currently, we only store those checksums in the file, but
    // do not read them. This matches the behavior implemented in the bookmarks
    // code which aims to completely do away with the checksums once the new
    // method of reassigning ids has been shown to not cause issues. This has to
    // be followed up in the next intake.
    UpdateChecksumWithNoteNode(id, subject, type, content, url);
  } else {
    UpdateChecksumWithNoteFolder(id, subject, type);
  }

  if (can_have_children) {
    base::ListValue child_list;

    for (const auto& child : node->children()) {
      child_list.Append(EncodeNode(child.get(), nullptr));
    }
    if (extra_nodes) {
      for (const auto* child : *extra_nodes) {
        child_list.Append(EncodeNode(child, nullptr));
      }
    }
    value.Set(kChildrenKey, std::move(child_list));
  }

  return value;
}

bool NotesCodec::DecodeHelper(NoteNode* notes_node,
                              NoteNode* other_notes_node,
                              NoteNode* trash_node,
                              const base::DictValue& value,
                              std::string* sync_metadata_str) {
  std::optional<int> version = value.FindInt(kVersionKey);
  if (!version || *version > kCurrentVersion)
    return false;  // Unknown version.

  if (sync_metadata_str) {
    const std::string* sync_metadata_str_base64 =
        value.FindString(kSyncMetadata);
    if (sync_metadata_str_base64)
      base::Base64Decode(*sync_metadata_str_base64, sync_metadata_str);
  }

  DecodeNode(value, nullptr, notes_node, other_notes_node, trash_node);

  if (!other_notes_node || !trash_node) {
    return false;
  }

  return true;
}

void NotesCodec::DecodeNode(const base::DictValue& value,
                            NoteNode* parent,
                            NoteNode* node,
                            NoteNode* child_other_node,
                            NoteNode* child_trash_node) {
  // If no |node| is specified, we'll create one and add it to the |parent|.
  // Therefore, in that case, |parent| must be non-NULL.
  CHECK(node || parent);

  // It's not valid to have both a node and a specified parent.
  CHECK(!node || !parent);

  int64_t id = 0;
  bool id_requires_reassignment = true;

  if (const std::string* string = value.FindString(kIdKey);
      string && base::StringToInt64(*string, &id) && id > 0 &&
      ids_.insert(id).second) {
    id_requires_reassignment = false;
    maximum_id_ = std::max(maximum_id_, id);
  }

  std::u16string title;
  const std::string* string_value = value.FindString(kSubjectKey);
  if (string_value) {
    title = base::UTF8ToUTF16(*string_value);
  }

  base::Uuid uuid;
  // |node| is only passed in for notes of type NotePermanentNode, in
  // which case we do not need to check for UUID validity as their UUIDs are
  // hard-coded and not read from the persisted file.
  if (!node) {
    // UUIDs can be empty for notes that were created before UUIDs were
    // required. When encountering one such note we thus assign to it a new
    // UUID. The same applies if the stored UUID is invalid or a duplicate.
    const std::string* uuid_str = value.FindString(kGuidKey);
    if (uuid_str && !uuid_str->empty()) {
      uuid = base::Uuid::ParseCaseInsensitive(*uuid_str);
    }

    if (!uuid.is_valid()) {
      uuid = base::Uuid::GenerateRandomV4();
      uuids_reassigned_ = true;
    }

    if (uuid.AsLowercaseString() == NoteNode::kBannedUuidDueToPastSyncBug) {
      uuid = base::Uuid::GenerateRandomV4();
      uuids_reassigned_ = true;
    }

    // Guard against UUID collisions, which would violate BookmarkModel's
    // invariant that each UUID is unique.
    if (uuids_.contains(uuid)) {
      uuid = base::Uuid::GenerateRandomV4();
      uuids_reassigned_ = true;
    }

    uuids_.insert(uuid);
  }

  const std::string* type_string = value.FindString(kTypeKey);
  if (!type_string)
    return;

  NoteNode::Type type = NoteNode::NOTE;
  if (*type_string == kTypeNote)
    type = NoteNode::NOTE;
  else if (*type_string == kTypeSeparator)
    type = NoteNode::SEPARATOR;
  else if (*type_string == kTypeAttachment)
    type = NoteNode::ATTACHMENT;
  else if (*type_string == kTypeFolder)
    type = NoteNode::FOLDER;
  else if (!node || (*type_string != kTypeOther && *type_string != kTypeTrash))
    // We can't create a permanent node when loading.
    return;

  const base::ListValue* child_list = value.FindList(kChildrenKey);

  if (*type_string == kTypeNote || *type_string == kTypeAttachment) {
    const std::string* content_string = value.FindString(kContentKey);
    if (!content_string)
      return;

    if (!node) {
      DCHECK(uuid.is_valid());
      node = new NoteNode(id, uuid, type);
    } else {
      return;
    }

    node->SetContent(base::UTF8ToUTF16(*content_string));

    const std::string* url_string = value.FindString(kURLKey);
    if (url_string)
      node->SetURL(GURL(*url_string));

    if (*type_string == kTypeNote) {
      const base::ListValue* attachments = value.FindList(kAttachmentsKey);
      if (attachments) {
        for (const auto& attachment : *attachments) {
          if (!attachment.is_dict())
            continue;
          std::unique_ptr<DeprecatedNoteAttachment> item(
              DeprecatedNoteAttachment::Decode(attachment, this));
          if (item) {
            node->AddAttachmentDeprecated(std::move(*item));
            has_deprecated_attachments_ = true;
          }
        }
      }
    }
  } else if (*type_string != kTypeSeparator) {
    if (!child_list)
      return;

    if (!node) {
      DCHECK(uuid.is_valid());
      node = new NoteNode(id, uuid, type);
    } else {
      node->set_id(id);
    }
  } else {
    DCHECK(*type_string == kTypeSeparator);

    if (!node) {
      DCHECK(uuid.is_valid());

      node = new NoteNode(id, uuid, type);
    } else {
      return;
    }
  }

  if (*type_string != kTypeSeparator && *type_string != kTypeAttachment &&
      child_list) {
    for (const auto& child_value : *child_list) {
      if (!child_value.is_dict()) {
        continue;
      }

      const std::string* type_string2 =
          child_value.GetDict().FindString(kTypeKey);
      if (!type_string2)
        return;
      if (*type_string2 == kTypeOther) {
        if (!child_other_node) {
          return;
        }
        DecodeNode(child_value.GetDict(), nullptr, child_other_node, nullptr,
                   nullptr);
        child_other_node = nullptr;
        continue;
      }
      if (*type_string2 == kTypeTrash) {
        if (!child_trash_node)
          return;
        DecodeNode(child_value.GetDict(), nullptr, child_trash_node, nullptr,
                   nullptr);
        child_trash_node = nullptr;
        continue;
      }

      DecodeNode(child_value.GetDict(), node, nullptr, nullptr, nullptr);
    }
  }

  if (id_requires_reassignment) {
    nodes_requiring_id_reassignment_.push_back(node);
  }

  if (parent)
    parent->Add(base::WrapUnique(node));
  node->SetTitle(title);
  node->SetCreationTime(FindMicrosecondsSinceWindowsEpoch(value, kDateAddedKey)
                            .value_or(Time::Now()));
  node->SetLastModificationTime(
      FindMicrosecondsSinceWindowsEpoch(value, kDateModifiedKey)
          .value_or(Time::Now()));
}

void NotesCodec::ReassignIDsIfRequired() {
  if (nodes_requiring_id_reassignment_.empty()) {
    // Nothing to do.
    return;
  }

  for (NoteNode* node : nodes_requiring_id_reassignment_) {
    const int64_t old_id = node->id();
    node->set_id(++maximum_id_);
    reassigned_ids_per_old_id_.emplace(old_id, node->id());
    ids_.insert(node->id());
  }

  nodes_requiring_id_reassignment_.clear();
  ids_reassigned_ = true;
}

void NotesCodec::UpdateChecksumWithNoteNode(const std::string& id,
                                            const std::u16string& title,
                                            const std::string& type,
                                            const std::u16string& content,
                                            const std::string& url) {
  DCHECK(base::IsStringUTF8(url));
  UpdateChecksum(id);
  UpdateChecksum(title);
  UpdateChecksum(type);
  UpdateChecksum(content);
  UpdateChecksum(url);
}
void NotesCodec::UpdateChecksumWithNoteFolder(const std::string& id,
                                              const std::u16string& title,
                                              const std::string& type) {
  UpdateChecksum(id);
  UpdateChecksum(title);
  UpdateChecksum(type);
}

void NotesCodec::UpdateChecksum(const std::string& str) {
  md5_hasher_.Update(str);
  sha256_hasher_.Update(str);
}

void NotesCodec::UpdateChecksum(const std::u16string& str) {
  auto bytes = base::as_byte_span(str);
  md5_hasher_.Update(bytes);
  sha256_hasher_.Update(bytes);
}

void NotesCodec::InitializeChecksum() {
  md5_hasher_ = crypto::obsolete::Md5();
  sha256_hasher_ = crypto::hash::Hasher(crypto::hash::kSha256);
}

void NotesCodec::FinalizeChecksum() {
  computed_checksum_ = base::HexEncodeLower(md5_hasher_.Finish());
  std::string result(crypto::hash::kSha256Size, 0);
  sha256_hasher_.Finish(base::as_writable_byte_span(result));
  computed_sha256_checksum_ = base::HexEncodeLower(result);
}

}  // namespace vivaldi
