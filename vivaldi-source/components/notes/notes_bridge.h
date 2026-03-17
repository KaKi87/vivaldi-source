#ifndef COMPONENTS_NOTES_NOTES_BRIDGE_H_
#define COMPONENTS_NOTES_NOTES_BRIDGE_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/compiler_specific.h"
#include "components/prefs/pref_change_registrar.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_observer.h"

#include "components/notes/note_id.h"
#include "components/notes/note_node.h"
#include "components/notes/notes_factory.h"
#include "components/notes/notes_model.h"
#include "components/notes/notes_model_loaded_observer.h"
#include "components/notes/notes_model_observer.h"

class Profile;

// The delegate to fetch notes information for the Android native
// notes page. This fetches the notes, title, urls, folder
// hierarchy.
class NotesBridge : public vivaldi::NotesModelObserver,
                    public ProfileObserver,
                    public base::SupportsUserData::Data {
 public:
  NotesBridge(Profile* profile, vivaldi::NotesModel* model);
  NotesBridge(JNIEnv* env,
              const base::android::JavaRef<jobject>& obj,
              const base::android::JavaRef<jobject>& j_profile);

  NotesBridge(const NotesBridge&) = delete;
  NotesBridge& operator=(const NotesBridge&) = delete;
  ~NotesBridge() override;
  void Destroy(JNIEnv*, const base::android::JavaRef<jobject>&);

  bool IsDoingExtensiveChanges(JNIEnv* env);

  jboolean IsEditNotesEnabled(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jobject> GetNoteByID(JNIEnv* env, jlong id);

  void GetPermanentNodeIDs(JNIEnv* env,
                           const base::android::JavaRef<jobject>& j_result_obj);

  void GetTopLevelFolderParentIDs(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& j_result_obj);

  void GetTopLevelFolderIDs(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& j_result_obj);

  void GetAllFoldersWithDepths(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& j_folders_obj,
      const base::android::JavaRef<jobject>& j_depths_obj);

  jni_zero::ScopedJavaGlobalRef<jobject> GetJavaNotesModel();

  base::android::ScopedJavaLocalRef<jobject> GetRootFolderId(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jobject> GetMainFolderId(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jobject> GetTrashFolderId(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jobject> GetOtherFolderId(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jobject> GetDesktopFolderId(JNIEnv* env);

  void GetChildIDs(JNIEnv* env,
                   jlong id,
                   jboolean get_folders,
                   jboolean get_notes,
                   jboolean get_separators,
                   const base::android::JavaRef<jobject>& j_result_obj);

  jint GetChildCount(JNIEnv* env, jlong id);

  base::android::ScopedJavaLocalRef<jobject> GetChildAt(JNIEnv* env,
                                                        jlong id,
                                                        jint index);

  // Get the number of notes in the sub tree of the specified bookmark node.
  jint GetTotalNoteCount(JNIEnv* env, jlong id);

  void SetNoteTitle(JNIEnv* env,
                    jlong id,
                    const base::android::JavaRef<jstring>& title);

  void SetNoteContent(JNIEnv* env,
                      jlong id,
                      const base::android::JavaRef<jstring>& content);

  void SetNoteUrl(JNIEnv* env,
                  jlong id,
                  const base::android::JavaRef<jstring>& url);

  bool DoesNoteExist(JNIEnv* env, jlong id);

  void GetNotesForFolder(JNIEnv* env,
                         const base::android::JavaRef<jobject>& j_folder_id_obj,
                         const base::android::JavaRef<jobject>& j_callback_obj,
                         const base::android::JavaRef<jobject>& j_result_obj);

  void GetCurrentFolderHierarchy(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& j_folder_id_obj,
      const base::android::JavaRef<jobject>& j_callback_obj,
      const base::android::JavaRef<jobject>& j_result_obj);
  void SearchNotes(JNIEnv* env,
                   const base::android::JavaRef<jobject>& j_list,
                   const base::android::JavaRef<jstring>& j_query,
                   jint max_results);

  base::android::ScopedJavaLocalRef<jobject> AddFolder(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& j_parent_id_obj,
      jint index,
      const base::android::JavaRef<jstring>& j_title);

  void DeleteNote(JNIEnv* env,
                  const base::android::JavaRef<jobject>& j_bookmark_id_obj);

  void RemoveAllUserNotes(JNIEnv* env);

  void MoveNote(JNIEnv* env,
                const base::android::JavaRef<jobject>& j_bookmark_id_obj,
                const base::android::JavaRef<jobject>& j_parent_id_obj,
                jint index);

  base::android::ScopedJavaLocalRef<jobject> AddNote(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& j_parent_id_obj,
      jint index,
      const base::android::JavaRef<jstring>& j_title,
      const base::android::JavaRef<jstring>& j_url);

  void Undo(JNIEnv* env, const base::android::JavaRef<jobject>& obj);

  void StartGroupingUndos(JNIEnv* env,
                          const base::android::JavaRef<jobject>& obj);

  void EndGroupingUndos(JNIEnv* env,
                        const base::android::JavaRef<jobject>& obj);

  std::u16string GetContent(const vivaldi::NoteNode* node) const;
  std::u16string GetTitle(const vivaldi::NoteNode* node) const;

  // ProfileObserver override
  void OnProfileWillBeDestroyed(Profile* profile) override;

  // notes_model_observer
  // Invoked when the model has finished loading. |ids_reassigned| mirrors
  // TODO void NotesModelLoaded(bool ids_reassigned)
  // override {}

  // Invoked when a node has moved.
  void NotesNodeMoved(const vivaldi::NoteNode* old_parent,
                      size_t old_index,
                      const vivaldi::NoteNode* new_parent,
                      size_t new_index) override;

  // Invoked when a node has been added.
  void NotesNodeAdded(const vivaldi::NoteNode* parent, size_t index) override;

  /*   // Invoked before a node is removed.
     // |parent| the parent of the node that will be removed.
     // |old_index| the index of the node about to be removed in |parent|.
     // |node| is the node to be removed.
    void OnWillRemoveNotes(const vivaldi::NoteNode* parent,
                           int old_index,
                           const vivaldi::NoteNode* node) override {}*/

  // Invoked when a node has been removed, the item may still be starred though.
  // |parent| the parent of the node that was removed.
  // |old_index| the index of the removed node in |parent| before it was
  // removed.
  // |node| is the node that was removed.
  void NotesNodeRemoved(const vivaldi::NoteNode* parent,
                        size_t old_index,
                        const vivaldi::NoteNode* node,
                        const base::Location& location) override;

  /* // Invoked before the title or url of a node is changed.
   void OnWillChangeNotesNode(const vivaldi::NoteNode* node) override
   {}*/

  // Invoked when the title or url of a node changes.
  void NotesNodeChanged(const vivaldi::NoteNode* node) override;

  // Invoked when a attachment has been loaded or changed.

  /*// Invoked before the direct children of |node| have been reordered in some
  // way, such as sorted.
  void OnWillReorderNotesNode(const vivaldi::NoteNode* node) override {}

  // Invoked when the children (just direct children, not descendants) of
  // |node| have been reordered in some way, such as sorted.
  void NotesNodeChildrenReordered(const vivaldi::NoteNode* node) override {}

  // Invoked before an extensive set of model changes is about to begin.
  // This tells UI intensive observers to wait until the updates finish to
  // update themselves.
  // These methods should only be used for imports and sync.
  // Observers should still respond to NotesNodeRemoved immediately,
  // to avoid holding onto stale node pointers.
  void ExtensiveNotesChangesBeginning() override {}

  // Invoked after an extensive set of model changes has ended.
  // This tells observers to update themselves if they were waiting for the
  // update to finish.
  void ExtensiveNotesChangesEnded() override {}

  // Invoked before all non-permanent notes nodes are removed.
  void OnWillRemoveAllNotes() override {}

  // Invoked when all non-permanent notes nodes have been removed.
  void NotesAllNodesRemoved() override {}*/

  void ReorderChildren(JNIEnv* env,
                       const base::android::JavaRef<jobject>& j_note_id_obj,
                       const base::android::JavaRef<jlongArray>& arr);

  bool IsChildOfTrashNode(JNIEnv* env, jlong id);
  void DestroyJavaObject();

 private:
  base::android::ScopedJavaLocalRef<jobject> CreateJavaNote(
      const vivaldi::NoteNode* node);
  void ExtractNoteNodeInformation(
      const vivaldi::NoteNode* node,
      const base::android::JavaRef<jobject>& j_result_obj);
  const vivaldi::NoteNode* GetNodeByID(long node_id);
  const vivaldi::NoteNode* GetFolderWithFallback(long folder_id);
  bool IsEditNotesEnabled() const;
  void EditNotesEnabledChanged();
  // Returns whether |node| can be modified by the user.
  bool IsEditable(const vivaldi::NoteNode* node) const;
  // Returns whether |node| is a managed bookmark.
  bool IsManaged(const vivaldi::NoteNode* node) const;
  const vivaldi::NoteNode* GetParentNode(const vivaldi::NoteNode* node);
  bool IsReachable(const vivaldi::NoteNode* node) const;
  bool IsLoaded() const;
  bool IsFolderAvailable(const vivaldi::NoteNode* folder) const;
  void NotifyIfDoneLoading();

  // Override vivaldi::BaseNotesModelObserver.
  // Called when there are changes to the notes model that don't trigger
  // any of the other callback methods. For example, this is called when
  // partner bookmarks change.
  void NotesModelChanged();
  void NotesModelLoaded(bool ids_reassigned) override;
  void NotesModelBeingDeleted() override;
  void NoteNodeMoved(const vivaldi::NoteNode* old_parent,
                     size_t old_index,
                     const vivaldi::NoteNode* new_parent,
                     size_t new_index);
  void NoteNodeAdded(const vivaldi::NoteNode* parent, size_t index);
  void NoteNodeRemoved(const vivaldi::NoteNode* parent,
                       size_t old_index,
                       const vivaldi::NoteNode* node,
                       const std::set<GURL>& removed_urls,
                       const base::Location& location);
  void NoteAllUserNodesRemoved(const std::set<GURL>& removed_urls,
                               const base::Location& location);
  void NoteNodeChanged(const vivaldi::NoteNode* node);
  void NoteNodeChildrenReordered(const vivaldi::NoteNode* node);
  void ExtensiveNoteChangesBeginning();
  void ExtensiveNoteChangesEnded();

  const raw_ptr<Profile> profile_;
  base::android::ScopedJavaGlobalRef<jobject> java_notes_model_;
  const raw_ptr<vivaldi::NotesModel> notes_model_;
  JavaObjectWeakGlobalRef weak_java_ref_;
  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  // Weak pointers for creating callbacks that won't call into a destroyed
  // object.
  base::WeakPtrFactory<NotesBridge> weak_ptr_factory_;
};

#endif  // COMPONENTS_NOTES_NOTES_BRIDGE_H_
