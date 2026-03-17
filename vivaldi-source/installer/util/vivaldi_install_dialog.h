// Copyright (c) 2020 Vivaldi Technologies AS. All rights reserved.

#ifndef INSTALLER_UTIL_VIVALDI_INSTALL_DIALOG_H_
#define INSTALLER_UTIL_VIVALDI_INSTALL_DIALOG_H_

#include <windows.h>

#include <string>
#include <vector>

// DirectDraw
#include <d2d1.h>
#include <ddraw.h>
#include <dwrite.h>

#include <gdiplus.h>

#include "base/files/file_path.h"
#include "base/win/scoped_gdi_object.h"
#include "chrome/installer/util/l10n_string_util.h"
#include "chrome/installer/util/util_constants.h"

#include "base/win/atl.h"

#include "installer/util/vivaldi_install_util.h"
#include "ui/gfx/geometry/rect.h"

namespace installer {

struct VivaldiInstallUIOptions {
  VivaldiInstallUIOptions();
  ~VivaldiInstallUIOptions();

  // This is a move-only struct.
  VivaldiInstallUIOptions(VivaldiInstallUIOptions&&);
  VivaldiInstallUIOptions& operator=(VivaldiInstallUIOptions&&);

  base::FilePath install_dir;
  vivaldi::InstallType install_type = vivaldi::InstallType::kForCurrentUser;

  // On Windows 8 and later this applies only to a standalone installation and
  // skips the registration with Windows as a browser application. Under Windows
  // 7 for a non-standalone install this makes the browser the default
  // automatically after always regisring it. For standalone installs on Windows
  // 7 this both registers the browser and sets it as the default automatically.
  // The case of just registering a standalone browser without making it the
  // default on Windows 7 is not supported.
  bool register_browser = false;

  // Flags indicating that the values above are explicitly set via a command
  // option and any value from the registry should be ignored.
  bool given_install_type = false;
  bool given_register_browser = false;
  // Enables crash log uploads for the installed instance. If false we do not
  // touch the setting, meaning if crashlog uploads are enabled they stay
  // enabled.
  bool allow_crashlog_uploads = false;
};

VivaldiInstallUIOptions ReadRegistryPreferences();

class VivaldiInstallDialog {
 public:
  class SubclassedControl {
   public:
    SubclassedControl(int id, VivaldiInstallDialog* owner)
        : id_(id), owner_(owner) {}
    ~SubclassedControl() = default;

    void SetLeftButtonIsDown(bool leftIsClicked) {
      isLeftClicked_ = leftIsClicked;
    }
    void SetIsHovered(bool isHovered) { isHovered_ = isHovered; }
    bool IsHovered() { return isHovered_; }
    // This is only true for keyboard-focus.
    void SetHasFocus(bool focus) { hasFocus_ = focus; }
    bool HasFocus() { return hasFocus_; }

    bool IsLeftButtonDown() { return isLeftClicked_; }
    int id() { return id_; }
    VivaldiInstallDialog* owner() { return owner_; }

   private:
    bool isHovered_ = false;
    bool isLeftClicked_ = false;
    bool hasFocus_ = false;
    int id_;
    VivaldiInstallDialog* owner_ = nullptr;
  };

  enum DlgResult {
    INSTALL_DLG_ERROR = -1,   // Dialog could not be shown.
    INSTALL_DLG_CANCEL = 0,   // The user cancelled install.
    INSTALL_DLG_INSTALL = 1,  // The user clicked the install button.
  };

  VivaldiInstallDialog(HINSTANCE instance, VivaldiInstallUIOptions options);
  virtual ~VivaldiInstallDialog();
  VivaldiInstallDialog(const VivaldiInstallDialog&) = delete;
  VivaldiInstallDialog& operator=(const VivaldiInstallDialog&) = delete;

  DlgResult ShowModal();

  VivaldiInstallUIOptions ExtractOptions();

  float xscale_ = 1.0f;
  float yscale_ = 1.0f;

  ULONG_PTR gdiplusToken_;

 private:
  void InitDialog();
  void InitUIFont();
  void TranslateDialog();
  void ShowBrowseFolderDialog();
  void DoDialog();
  void ReadLastInstallValues();
  void SaveInstallValues();
  bool InternalSelectLanguage();

  void OnInstallTypeSelection();
  void OnLanguageSelection();
  void OnInstallModeSelection();
  bool IsInstallPathValid(const base::FilePath& path);
  installer::InstallStatus ShowEULADialog();
  std::wstring GetInnerFrameEULAResource();

  // Layout the directdraw slogan text-box.
  void LayoutSlogan();

  Gdiplus::Bitmap* GetGDIBitmapFromResource(int bitmap_id);

  void UpdateSize();
  void ClearAll();
  void CenterOnScreen();

  void UpdateTheme(HWND const window, COLORREF accent_color);

  COLORREF GetTextColor();
  COLORREF GetButtonColor();
  COLORREF GetTopBackgroundColor();
  D2D1::ColorF GetTopBackgroundColorF();
  COLORREF GetBottomBackgroundColor();
  COLORREF GetButtonBorderColor();
  Gdiplus::Color GetCheckboxBackgroundColor();
  Gdiplus::Color GetCheckboxBackgroundHoverColor();

  static void DrawRoundRect(HDC hdc,
                            gfx::Rect rect,
                            COLORREF color,
                            bool fill /* otherwise outline*/,
                            float borderWidth = 1.f);

  // returns screen value taking current dpi into account.
  int GetPixelsFromDPI(int pixels);
  // returns actual screen value taking current dpi into account.
  int GetDIPsFromPixels(int pixels);

  // returns true if size was set, size cannot be used of false.
  bool GetControlSize(HWND control, SIZE& size);

  // Height of titlebar aree.
  int GetTitlebarHeight();

  int GetFooterbarHeight();

  bool CreateSurface();

  // Layout visible controls in the dialog. Returns success.
  bool Layout();

  BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);
  HBRUSH OnCtlColorEdit(HWND hwnd_ctl, HDC hdc);
  HBRUSH OnCtlColor(HWND hwnd_ctl, HDC hdc);

  bool is_dark_mode() { return dark_mode_; }

  // Check target directory validity and updates ui.
  bool UpdateTargetPathResult(base::FilePath file_path);

  static INT_PTR CALLBACK DlgProc(HWND hdlg,
                                  UINT msg,
                                  WPARAM wparam,
                                  LPARAM lparam);

  static LRESULT CALLBACK OwnerDrawButtonProc(HWND hWnd,
                                              UINT uMsg,
                                              WPARAM wParam,
                                              LPARAM lParam,
                                              UINT_PTR uIdSubclass,
                                              DWORD_PTR dwRefData);

  static LRESULT CALLBACK OwnerDrawComboboxProc(HWND hWnd,
                                                UINT uMsg,
                                                WPARAM wParam,
                                                LPARAM lParam,
                                                UINT_PTR uIdSubclass,
                                                DWORD_PTR dwRefData);

  static LRESULT CALLBACK OwnerDrawEditProc(HWND hWnd,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            UINT_PTR uIdSubclass,
                                            DWORD_PTR dwRefData);

  void DrawDDtext();

  COLORREF GetWindowBorderColor();

 private:
  VivaldiInstallUIOptions options_;
  bool disable_standalone_autoupdates_ = false;

  std::wstring txt_tos_accept_install_str_;
  std::wstring btn_tos_accept_install_str_;
  std::wstring txt_tos_accept_update_str_;
  std::wstring btn_tos_accept_update_str_;
  std::wstring btn_simple_mode_str_;
  std::wstring btn_advanced_mode_str_;
  std::wstring select_folder_str_;
  std::wstring txt_slogan_str_;
  std::wstring btn_browse_destination_str_;
  std::wstring tooltip_warn_illegal_destination_str_;

  base::FilePath last_standalone_folder_;
  bool is_upgrade_ = false;
  bool is_valid_target_ = false;
  bool dialog_ended_ = false;
  bool advanced_mode_ = false;
  HWND hdlg_ = nullptr;
  HINSTANCE instance_ = nullptr;
  DlgResult dlg_result_ = INSTALL_DLG_ERROR;
  Gdiplus::Bitmap* logo_bmp_ = nullptr;

  // offscreen DCs used when masking
  HDC image_dc_;
  HDC mask_dc_;
  HBITMAP image_bitmap_;
  HBITMAP mask_bitmap_;

  bool changed_language_ = false;

  float current_dpi_ = 96.0f;

  bool dark_mode_ = false;
  // Use accent color on borders, otherwise we use regular border color.
  bool use_accent_color_on_borders_ = false;

  gfx::Rect dialog_rect_;

  typedef std::map<int, SIZE> IdToSIZEMap;

  int slogan_width_ = 1;
  int slogan_height_ = 0;

  // If we need to use these in the parent.
  std::vector<SubclassedControl*> child_controls_;

  IdToSIZEMap control_sizes_;

  // Used to paint separator lines between titlearea and the rest.
  HPEN topLinePen_light_;
  HPEN bottomLinePen_light_;
  HPEN topLinePen_dark_;
  HPEN bottomLinePen_dark_;

  COLORREF accent_color_ = RGB(200, 200, 200);

  // DirectDraw2 members.
  ID2D1Factory* pD2DFactory_ = nullptr;
  IDWriteFactory* pDWriteFactory_ = nullptr;
  IDWriteTextFormat* pTextFormat_ = nullptr;
  ID2D1HwndRenderTarget* pRenderTarget_ = nullptr;
  IDWriteTextLayout* pTextLayout_ = nullptr;

  LPDIRECTDRAW lpdd_ = nullptr;
  LPDIRECTDRAWSURFACE lpddsprimary_ = nullptr;

  // Raw controls to layout.
  HWND logo_;
  HWND slogan_text_;
  HWND language_label_;
  HWND language_combo_;
  HWND install_type_label_;
  HWND install_type_combo_;
  HWND destination_folder_label_;
  HWND destination_folder_edit_;
  HWND destination_folder_button_;
  HWND register_default_app_check_;
  HWND allow_crash_reports_check_;
  HWND agree_link_;
  HWND auto_update_check_;
  HWND cancel_button_;
  HWND install_button_;
  HWND x_button_;
  HWND toggle_mode_button_;
  HWND window_title_label_;
  HWND progress_bar_;
  HWND target_directory_tooltip_;

  base::win::ScopedGDIObject<HBRUSH> background_brush_;
  base::win::ScopedGDIObject<HBRUSH> buttondrop_brush_;

  // SubclassedButton ok_button_(IDOK);
  std::unique_ptr<SubclassedControl> ok_button_subclassed_;
  std::unique_ptr<SubclassedControl> cancel_button_subclassed_;
  std::unique_ptr<SubclassedControl> close_button_subclassed_;
  std::unique_ptr<SubclassedControl> mode_button_subclassed_;
  std::unique_ptr<SubclassedControl> browse_destination_button_subclassed_;

  std::unique_ptr<SubclassedControl> update_checkbutton_subclassed_;
  std::unique_ptr<SubclassedControl> register_app_checkbutton_subclassed_;
  std::unique_ptr<SubclassedControl> allow_crashuploads_checkbutton_subclassed_;

  std::unique_ptr<SubclassedControl> language_combo_subclassed_;
  std::unique_ptr<SubclassedControl> install_type_combo_subclassed_;

  std::unique_ptr<SubclassedControl> destination_folder_edit_subclassed_;

  static VivaldiInstallDialog* this_;

  bool active_ = true;

 protected:
  HFONT dialog_font_;
};

}  // namespace installer
#endif  // INSTALLER_UTIL_VIVALDI_INSTALL_DIALOG_H_
