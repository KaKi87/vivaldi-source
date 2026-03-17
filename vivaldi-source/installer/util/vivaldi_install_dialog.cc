// Copyright (c) 2020 Vivaldi Technologies AS. All rights reserved.
//

#include "installer/util/vivaldi_install_dialog.h"
#include "installer/util/vivaldi_install_util.h"
#include "installer/win/setup/setup_resource.h"

#include <dwmapi.h>
#include <math.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <windows.h>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/atl.h"
#include "base/win/embedded_i18n/language_selector.h"
#include "base/win/registry.h"
#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "base/win/windows_version.h"
#include "installer/mini_installer/setup/setup_constants.h"
#include "installer/mini_installer/util/google_update_constants.h"
#include "installer/mini_installer/util/html_dialog.h"
#include "installer/mini_installer/util/install_util.h"
#include "installer/mini_installer/util/installer_util_strings.h"
#include "installer/mini_installer/util/shell_util.h"

#include "installer/win/vivaldi_install_l10n.h"
#include "vivaldi/app/grit/vivaldi_installer_strings.h"
#include "vivaldi/installer/win/vivaldi_install_language_names.h"

//
//* Simple
//-  Internal / snapshot
//
//-  Official
//
//* Advanced
//
//\ Standalone
//\ Per user
//\ All Users

#include <Windows.h>

#include <Shellapi.h>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <windowsx.h>

namespace installer {

#define VIVALDI_PRIVACY_LINK L"https://vivaldi.com/privacy"

// Colored emoji font
#define FONT_SIZE 10.0f
#define DIALOG_FONT_NAME TEXT("Segoe UI")
#define DIALOG_FONT_NAME_EMOJI TEXT("Segoe UI Emoji")

// Minimum button width in pixels.
#define MINIMUM_BUTTON_WIDTH 125

// indent
#define LEFT_INDENT 35
#define RIGHT_PADDING 35

// Rounding for all rectangles.
#define ROUND_RECT_RADIUS 4

#define ACTION_BUTTON_PADDING 35

#define BUTTON_PADDING 4

#define EDIT_Y_PADDING 10

#define CHECKBOX_Y_PADDING 10

#define CHECKBOX_TEXT_PADDING 12

#define TITLEBAR_LEFT_PADDING 20

#define TITLEBAR_HEIGHT 44
#define WINDOW_BORDER_WIDTH 1

#define TOOLBAR_BACKGROUND_HEIGHT 82

#define LOGO_SLOGAN_PADDING 10

#define SLOGAN_X_PADDING 20
#define SLOGAN_Y_PADDING 20

#define COMBOBOX_LABEL_PADDING 10

#define COMBOBOX_HEIGHT_INFLATION 5

// background colors
#define TOP_BACKGROUND_COLOR_LIGHT RGB(255, 255, 255)
#define BOTTOM_BACKGROUND_COLOR_LIGHT RGB(243, 243, 243)

#define TOP_BACKGROUND_COLOR_DARK RGB(32, 32, 32)
#define BOTTOM_BACKGROUND_COLOR_DARK RGB(25, 25, 25)

#define TOP_BACKGROUND_COLOR_RGB_DARK \
  D2D1::ColorF((32 / 255.), (32 / 255.), (32 / 255.))
#define TOP_BACKGROUND_COLOR_RGB_LIGHT \
  D2D1::ColorF(255. / 255., 255. / 255., 255. / 255.)

#define TEXT_COLOR_LINK RGB(0x0e, 0x0e, 0x0e)

#define TEXT_COLOR_RGB_LIGHT D2D1::ColorF(10. / 255., 10. / 255., 10. / 255.)
#define TEXT_COLOR_RGB_DARK D2D1::ColorF(201. / 255., 201. / 255., 201. / 255.)

#define TEXT_COLOR_LIGHT RGB(0x33, 0x33, 0x33)
#define TEXT_COLOR_DARK RGB(0xc9, 0xc9, 0xc9)

#define BUTTON_BACKGROUND_COLOR_LIGHT RGB(0xff, 0xff, 0xff)
#define BUTTON_BACKGROUND_COLOR_HOVER_LIGHT RGB(226, 226, 226)

#define BUTTON_BACKGROUND_COLOR_DARK RGB(75, 75, 75)
#define BUTTON_BACKGROUND_COLOR_HOVER_DARK RGB(65, 65, 65)

#define BUTTON_BORDER_COLOR_DARK RGB(69, 69, 69)
#define BUTTON_BORDER_COLOR_LIGHT RGB(0xe5, 0xe5, 0xe5)

#define CHECKBOX_BACKGROUND_COLOR_LIGHT_GDI Gdiplus::Color(255, 255, 255, 255)
#define CHECKBOX_BACKGROUND_COLOR_DARK_GDI Gdiplus::Color(255, 32, 32, 32)

#define TEXT_COLOR_LIGHT_GDI Gdiplus::Color(255, 0x33, 0x33, 0x33)
#define TEXT_COLOR_DARK_GDI Gdiplus::Color(255, 0xc9, 0xc9, 0xc9)

// Checkbox colors.
// static const Gdiplus::Color CHECK_COLOR_BORDER_NORMAL(255, 140, 140, 140);
static const Gdiplus::Color CHECK_COLOR_FILL_LIGHT(255, 255, 255, 255);
static const Gdiplus::Color CHECK_COLOR_FILL_DARK(255, 32, 32, 32);
// static const Gdiplus::Color CHECK_COLOR_FILL_CHECKED(255, 0, 120, 212);
static const Gdiplus::Color CHECK_COLOR_FILL_HOVER_LIGHT(255, 226, 226, 226);
static const Gdiplus::Color CHECK_COLOR_FILL_HOVER_DARK(255, 65, 65, 65);

namespace {
static const uint32_t kuint32max = 0xFFFFFFFFu;

struct {
  const wchar_t* code;
  const wchar_t* name;
} kLanguages[] = {
#define HANDLE_VIVALDI_LANGUAGE_NAME(code, name) {L"" code, L"" name},
    DO_VIVALDI_LANGUAGE_NAMES
#undef HANDLE_VIVALDI_LANGUAGE_NAME
};

// Function to create a font similar to WinUI 2
HFONT CreateWinUIFont(int height, int weight) {
  LOGFONT lf = {0};

  lf.lfHeight = -MulDiv(height, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72);
  lf.lfWeight = weight;
  lf.lfQuality = CLEARTYPE_QUALITY;
  wcscpy_s(lf.lfFaceName, DIALOG_FONT_NAME);

  return CreateFontIndirect(&lf);
}

std::optional<vivaldi::InstallType> GetInstallTypeFromComboIndex(int i) {
  static const vivaldi::InstallType selection_type_map[] = {
      vivaldi::InstallType::kForAllUsers,
      vivaldi::InstallType::kForCurrentUser,
      vivaldi::InstallType::kStandalone,
  };
  if (0 <= i && static_cast<unsigned>(i) < std::size(selection_type_map)) {
    return selection_type_map[i];
  }
  return std::nullopt;
}

DWORD ToComboIndex(vivaldi::InstallType install_type) {
  switch (install_type) {
    case vivaldi::InstallType::kForAllUsers:
      return 0;
    case vivaldi::InstallType::kForCurrentUser:
      return 1;
    case vivaldi::InstallType::kStandalone:
      return 2;
  }
}

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

/// Registry key for app theme preference.
///
/// A value of 0 indicates apps should use dark mode. A non-zero or missing
/// value indicates apps should use light mode.
constexpr const wchar_t kGetPreferredBrightnessRegValue[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
constexpr const wchar_t kGetPreferredBrightnessRegKey[] = L"AppsUseLightTheme";
constexpr wchar_t kGetAccentColorBorderValue[] =
    L"Software\\Microsoft\\Windows\\DWM";
constexpr const wchar_t kGetAccentColorBorderKey[] = L"ColorPrevalence";

int GetCurrentDpi(HWND window, VivaldiInstallDialog* dialog) {
  // GetDpiForMonitor() is available only in Windows 8.1.
  static auto get_dpi_for_monitor_func = []() {
    const HMODULE shcore_dll = ::LoadLibrary(L"shcore.dll");
    return reinterpret_cast<decltype(&::GetDpiForMonitor)>(
        shcore_dll ? ::GetProcAddress(shcore_dll, "GetDpiForMonitor")
                   : nullptr);
  }();
  if (get_dpi_for_monitor_func) {
    HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
    UINT dpi_x;
    UINT dpi_y;
    HRESULT hr =
        get_dpi_for_monitor_func(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);

    dialog->xscale_ = dpi_x / USER_DEFAULT_SCREEN_DPI;
    dialog->yscale_ = dpi_y / USER_DEFAULT_SCREEN_DPI;

    if (SUCCEEDED(hr)) {
      return static_cast<int>(dpi_x);
    }
  }
  HDC screen_dc = GetDC(nullptr);
  int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
  int dpi_y = GetDeviceCaps(screen_dc, LOGPIXELSY);

  dialog->xscale_ = dpi_x / USER_DEFAULT_SCREEN_DPI;
  dialog->yscale_ = dpi_y / USER_DEFAULT_SCREEN_DPI;

  ReleaseDC(nullptr, screen_dc);
  return dpi_x;
}

}  // namespace

VivaldiInstallUIOptions::VivaldiInstallUIOptions() = default;
VivaldiInstallUIOptions::~VivaldiInstallUIOptions() = default;
VivaldiInstallUIOptions::VivaldiInstallUIOptions(VivaldiInstallUIOptions&&) =
    default;
VivaldiInstallUIOptions& VivaldiInstallUIOptions::operator=(
    VivaldiInstallUIOptions&&) = default;

VivaldiInstallDialog* VivaldiInstallDialog::this_ = nullptr;

VivaldiInstallDialog::VivaldiInstallDialog(HINSTANCE instance,
                                           VivaldiInstallUIOptions options)
    : options_(std::move(options)), instance_(instance) {
  ReadLastInstallValues();
  if (options_.install_dir.empty() &&
      options_.install_type != vivaldi::InstallType::kStandalone) {
    // For standalone install there is no default path so we keep it empty
    // forcing the user to make a choice.
    base::FilePath path =
        vivaldi::GetDefaultInstallTopDir(options_.install_type);
    if (!path.empty()) {
      options_.install_dir = std::move(path);
    }
  }

  if (!options_.install_dir.empty()) {
    // An existing type unconditionally overrides any option or registry.
    std::optional<vivaldi::InstallType> existing_install_type =
        vivaldi::FindInstallType(options_.install_dir);
    if (existing_install_type) {
      options_.install_type = *existing_install_type;
    }
  }

  if (!options_.given_register_browser) {
    options_.register_browser =
        (options_.install_type != vivaldi::InstallType::kStandalone);
  }

  browse_destination_button_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_BTN_BROWSE, this);

  ok_button_subclassed_ = std::make_unique<SubclassedControl>(IDOK, this);
  cancel_button_subclassed_ =
      std::make_unique<SubclassedControl>(IDCANCEL, this);
  close_button_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_BTN_CLOSE, this);
  mode_button_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_BTN_MODE, this);
  update_checkbutton_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_CHECK_NO_AUTOUPDATE, this);

  register_app_checkbutton_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_CHECK_REGISTER, this);
  update_checkbutton_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_CHECK_NO_AUTOUPDATE, this);
  allow_crashuploads_checkbutton_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_CHECK_ALLOW_CRASHLOGS, this);

  language_combo_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_COMBO_LANGUAGE, this);

  install_type_combo_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_COMBO_INSTALLTYPES, this);

  destination_folder_edit_subclassed_ =
      std::make_unique<SubclassedControl>(IDC_EDIT_DEST_FOLDER, this);

  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
}

VivaldiInstallDialog::~VivaldiInstallDialog() {
  ClearAll();

  pRenderTarget_->Release();
  pTextFormat_->Release();
  pDWriteFactory_->Release();
  pD2DFactory_->Release();
  pTextLayout_->Release();

  DeleteObject(topLinePen_light_);
  DeleteObject(bottomLinePen_light_);
  DeleteObject(topLinePen_dark_);
  DeleteObject(bottomLinePen_dark_);

  DeleteObject(dialog_font_);

  if (lpddsprimary_) {
    lpddsprimary_->Release();
    lpddsprimary_ = NULL;
  }
  if (lpdd_) {
    lpdd_->Release();
    lpdd_ = NULL;
  }

  Gdiplus::GdiplusShutdown(gdiplusToken_);
}

// Returns minimal |size| of control in dialog pixels.
bool VivaldiInstallDialog::GetControlSize(HWND control, SIZE& size) {
  RECT control_rect = {0, 0, 0, 0};

  // special handling for bitmap logo and directdraw  IDC_STATIC_LOGO
  // IDC_STATIC_SLOGAN , used even when invisible.
  if (control == logo_) {
    size.cx = logo_bmp_->GetWidth();
    size.cy = logo_bmp_->GetHeight();
    return true;
  } else if (control == slogan_text_) {
    // Calculated in LayoutSlogan.
    size.cx = slogan_width_;
    size.cy = slogan_height_;
    return true;
  }

  base::win::ScopedGetDC dc(control);
  base::win::ScopedSelectObject font(dc, dialog_font_);

  std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
  if (!buffer.get()) {
    return false;
  }

  // Send in the current width of the dialog to allow for multiline.
  RECT text_rect = {0, 0, 0, 0};

  UINT format = DT_CALCRECT;
  // Allow for multilines for certain controls.
  if (control == agree_link_) {
    // Dialog width is 3 x logo width + padding
    int logo_width = logo_bmp_->GetWidth();

    format |= DT_WORDBREAK;
    text_rect = {LEFT_INDENT, 0, (logo_width * 3) - LEFT_INDENT, 0};
  }

  // Make sure controls that might have no text gets sample text.
  int textlen = GetWindowText(control, buffer.get(), MAX_PATH - 1);
  if (!DrawText(dc, !textlen ? L"AAA" : buffer.get(), -1, &text_rect, format)) {
    return false;
  }

  gfx::Rect xrect(text_rect);

  // Special handling of our comboboxes.
  // IDC_COMBO_INSTALLTYPES
  if (control == install_type_combo_ || control == language_combo_) {
    xrect.set_width(((dialog_rect_.width()) / 2) - LEFT_INDENT - RIGHT_PADDING);
    RECT combo_rect;
    if (!GetWindowRect(control, &combo_rect)) {
      return false;
    }
    xrect.set_height(combo_rect.bottom - combo_rect.top +
                     this_->GetPixelsFromDPI(10));
  }

  // Add some spacing for labels.
  if (control == install_type_label_ || control == language_label_ ||
      control == destination_folder_label_) {
    xrect.set_height(xrect.height() + this_->GetPixelsFromDPI(2));
  }

  // Add some spacing for buttons.
  if (control == destination_folder_button_ || control == cancel_button_ ||
      control == install_button_ || control == toggle_mode_button_ ||
      control == destination_folder_button_) {
    int button_padding = this_->GetPixelsFromDPI(12);
    xrect.Outset(button_padding);
    xrect.set_height(xrect.height() + this_->GetPixelsFromDPI(2));

    // Minimum width for the push-buttons.
    xrect.set_width(std::max(MINIMUM_BUTTON_WIDTH, xrect.width()));
  }

  // Add some height-padding to the edit fields.
  if (control == destination_folder_edit_) {
    int button_padding = this_->GetPixelsFromDPI(26);
    xrect.set_height(xrect.height() + button_padding);
  }

  size.cx = control_rect.right - control_rect.left;
  size.cy = control_rect.bottom - control_rect.top;

  // Size the checkboxes correctly, it has a box in addition to the text.
  if (control == register_default_app_check_ ||
      control == allow_crash_reports_check_ || control == auto_update_check_) {
    int checkbox_width =
        GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE);
    xrect.set_width(xrect.width() + checkbox_width);
  }

  // Check if the text is bigger than the current control size.

  int new_control_width = xrect.width();
  int new_control_height = xrect.height();

  if (size.cx < new_control_width) {
    size.cx = new_control_width;
  }
  if (size.cy < new_control_height) {
    size.cy = new_control_height;
  }

  return true;
}

VivaldiInstallDialog::DlgResult VivaldiInstallDialog::ShowModal() {
  if (base::win::OSInfo::IsRunningEmulatedOnArm64()) {
    MessageBox(
        nullptr,
        GetLocalizedString(IDS_INSTALL_RUNNING_EMULATED_ON_ARM64_BASE).c_str(),
        GetLocalizedString(IDS_INSTALL_INSTALLER_NAME_BASE).c_str(),
        MB_ICONINFORMATION | MB_SETFOREGROUND);
    // Fallthrough, let the user install.
  }

  if (!InstallUtil::IsOSSupported()) {
    MessageBox(
        nullptr,
        GetLocalizedString(IDS_INSTALL_OUTDATED_WINDOWS_VERSION_BASE).c_str(),
        GetLocalizedString(IDS_INSTALL_INSTALLER_NAME_BASE).c_str(),
        MB_ICONERROR | MB_SETFOREGROUND);
    return INSTALL_DLG_ERROR;
  }

  INITCOMMONCONTROLSEX iccx;
  iccx.dwSize = sizeof(iccx);
  iccx.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES |
               ICC_USEREX_CLASSES;
  ::InitCommonControlsEx(&iccx);

  hdlg_ = CreateDialogParam(instance_, MAKEINTRESOURCE(IDD_INSTALL_DIALOG),
                            nullptr, DlgProc, reinterpret_cast<LPARAM>(this));

  if (!hdlg_) {
    MessageBox(nullptr, L" VivaldiInstallDialog::ShowModal.",
               L"Failed to initialize installer.",
               MB_ICONINFORMATION | MB_SETFOREGROUND);
    return INSTALL_DLG_ERROR;
  }

  ShowWindow(hdlg_, SW_HIDE);

  if (base::win::GetVersion() >= base::win::Version::WIN11) {
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hdlg_, DWMWA_WINDOW_CORNER_PREFERENCE, &preference,
                          sizeof(preference));
  } else {
    const MARGINS shadow_on = {1};
    DwmExtendFrameIntoClientArea(hdlg_, &shadow_on);
  }

  SetWindowLong(hdlg_, GWL_STYLE, WS_POPUP);

  UpdateTheme(hdlg_, accent_color_);
  InitDialog();

  Layout();

  if (!AnimateWindow(hdlg_, 500, AW_SLIDE)) {
    ShowWindow(hdlg_, SW_SHOWNORMAL);
  }

  DoDialog();  // main message loop

  if (dlg_result_ == INSTALL_DLG_INSTALL) {
    SaveInstallValues();
    if (changed_language_) {
      vivaldi::WriteInstallerRegistryLanguage();
    }
  }

  return dlg_result_;
}

void VivaldiInstallDialog::UpdateTheme(HWND const window,
                                       COLORREF accent_color) {
  DWORD color = 0;
  BOOL opaque_blend = FALSE;
  HRESULT hr = DwmGetColorizationColor(&color, &opaque_blend);

  if (SUCCEEDED(hr)) {
    // Note the color order since DwmGetColorizationColor return 0xAARRGGBB.

    // ARGB = 0xFF000000 | ((0BGR -> RGB0) >> 8)
    color = 0xFF000000u | (_byteswap_ulong(color) >> 8);

    accent_color_ = RGB(GetRValue(color), GetGValue(color), GetBValue(color));
  }

  DWORD accent_on_borders;
  base::win::RegKey key(HKEY_CURRENT_USER, kGetAccentColorBorderValue,
                        KEY_QUERY_VALUE);
  key.ReadValueDW(kGetAccentColorBorderKey, &accent_on_borders);

  use_accent_color_on_borders_ = accent_on_borders == 1;

  DWORD light_mode = 1;
  base::win::RegKey lightmode_key(
      HKEY_CURRENT_USER, kGetPreferredBrightnessRegValue, KEY_QUERY_VALUE);
  lightmode_key.ReadValueDW(kGetPreferredBrightnessRegKey, &light_mode);

  dark_mode_ = (light_mode == 0);

  BOOL enable_dark_mode = light_mode == 0;
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE,
                        &enable_dark_mode, sizeof(enable_dark_mode));

  // Create different brushes based on darkmode.

  if (light_mode) {
    background_brush_ = base::win::ScopedGDIObject<HBRUSH>(
        CreateSolidBrush(TOP_BACKGROUND_COLOR_LIGHT));
    buttondrop_brush_ = base::win::ScopedGDIObject<HBRUSH>(
        CreateSolidBrush(BOTTOM_BACKGROUND_COLOR_LIGHT));
  } else {
    background_brush_ = base::win::ScopedGDIObject<HBRUSH>(
        CreateSolidBrush(TOP_BACKGROUND_COLOR_DARK));
    buttondrop_brush_ = base::win::ScopedGDIObject<HBRUSH>(
        CreateSolidBrush(BOTTOM_BACKGROUND_COLOR_DARK));
  }

  // Free up locked resources.
  ClearAll();

  UpdateSize();

  // For DPI above 240 we scale bitmaps, but to avoid artifacts we limit the
  // scale factor only to integers.
  int dpi = current_dpi_;
  // g_DPIScale USER_DEFAULT_SCREEN_DPI
  int bitmap_scale = 1.0;
  constexpr int kMaxUnscaledDpi = 240;
  if (dpi > kMaxUnscaledDpi) {
    bitmap_scale = (dpi + kMaxUnscaledDpi - 1) / kMaxUnscaledDpi;
    dpi /= bitmap_scale;
  }

  int bitmap_id;
  if (dpi < 96) {
    bitmap_id = IDB_BITMAP_LOGO_100;
  } else if (dpi < 120) {
    bitmap_id = IDB_BITMAP_LOGO_125;
  } else if (dpi < 144) {
    bitmap_id = IDB_BITMAP_LOGO_150;
  } else if (dpi < 192) {
    bitmap_id = IDB_BITMAP_LOGO_200;
  } else {
    bitmap_id = IDB_BITMAP_LOGO_250;
  }

  // Note: This is assuming the ids are 10 apart in setup_resource.h.
  if (!light_mode) {
    bitmap_id += 10;
  }

  logo_bmp_ = GetGDIBitmapFromResource(bitmap_id);
  CHECK(logo_bmp_);  // We cannot recover from this.

  // Since Windows 10 is no behaving for alpha channels, make a mask and use
  // this when drawing the logo cache these resources as we use them all the
  // time and the mask creation is heavy
  image_dc_ = CreateCompatibleDC(NULL);
  mask_dc_ = CreateCompatibleDC(NULL);

  Gdiplus::Color backgroundColor(255, 255, 255, 0);
  logo_bmp_->GetHBITMAP(backgroundColor, &image_bitmap_);

  BITMAP bm;

  // Create monochrome (1 bit) mask bitmap.

  GetObject(image_bitmap_, sizeof(BITMAP), &bm);
  mask_bitmap_ = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, nullptr);

  // save old to restore and delete
  SelectBitmap(image_dc_, image_bitmap_);
  SelectBitmap(mask_dc_, mask_bitmap_);

  // use white as the transparent color
  SetBkColor(image_dc_, RGB(255, 255, 255));
  SetBkColor(mask_dc_, RGB(255, 255, 255));

  RECT rect(0, 0, bm.bmWidth, bm.bmHeight);

  FillRect(image_dc_, &rect, background_brush_.get());
  FillRect(mask_dc_, &rect, background_brush_.get());

  BitBlt(mask_dc_, 0, 0, bm.bmWidth, bm.bmHeight, image_dc_, 0, 0, SRCCOPY);
  BitBlt(image_dc_, 0, 0, bm.bmWidth, bm.bmHeight, mask_dc_, 0, 0, SRCINVERT);

  // mask_bmp_ this is now updated and ready

  //  debug helper
  //  wchar_t text_buffer[500] = {0};
  //  swprintf(text_buffer, _countof(text_buffer), L"bitmap loaded w:%d, h:%d",
  //           logo_bmp_->GetWidth(), logo_bmp_->GetHeight());
  //  MessageBox(window, text_buffer, nullptr, MB_ICONERROR);
}

COLORREF VivaldiInstallDialog::GetWindowBorderColor() {
  COLORREF border_color = GetSysColor(COLOR_WINDOWFRAME);

  if (active_) {
    if (use_accent_color_on_borders_) {
      border_color = accent_color_;
    }
  } else {
    // Dim by 30%
    float brightnessFactor = 0.7f;
    COLOR16 r = GetRValue(border_color);
    COLOR16 g = GetGValue(border_color);
    COLOR16 b = GetBValue(border_color);
    r = fmin(255, r * brightnessFactor);
    g = fmin(255, g * brightnessFactor);
    b = fmin(255, b * brightnessFactor);
    border_color = RGB(r, g, b);
  }

  return border_color;
}

COLORREF VivaldiInstallDialog::GetTextColor() {
  return dark_mode_ ? TEXT_COLOR_DARK : TEXT_COLOR_LIGHT;
}

COLORREF VivaldiInstallDialog::GetButtonColor() {
  return dark_mode_ ? BUTTON_BACKGROUND_COLOR_DARK
                    : BUTTON_BACKGROUND_COLOR_LIGHT;
}

COLORREF VivaldiInstallDialog::GetButtonBorderColor() {
  return dark_mode_ ? BUTTON_BORDER_COLOR_DARK : BUTTON_BORDER_COLOR_LIGHT;
}

COLORREF VivaldiInstallDialog::GetTopBackgroundColor() {
  return dark_mode_ ? TOP_BACKGROUND_COLOR_DARK : TOP_BACKGROUND_COLOR_LIGHT;
}

D2D1::ColorF VivaldiInstallDialog::GetTopBackgroundColorF() {
  return dark_mode_ ? TOP_BACKGROUND_COLOR_RGB_DARK
                    : TOP_BACKGROUND_COLOR_RGB_LIGHT;
}

COLORREF VivaldiInstallDialog::GetBottomBackgroundColor() {
  return dark_mode_ ? BOTTOM_BACKGROUND_COLOR_DARK
                    : BOTTOM_BACKGROUND_COLOR_LIGHT;
}

Gdiplus::Color VivaldiInstallDialog::GetCheckboxBackgroundColor() {
  return dark_mode_ ? CHECKBOX_BACKGROUND_COLOR_DARK_GDI
                    : CHECKBOX_BACKGROUND_COLOR_LIGHT_GDI;
}

Gdiplus::Color VivaldiInstallDialog::GetCheckboxBackgroundHoverColor() {
  return dark_mode_ ? CHECK_COLOR_FILL_HOVER_DARK
                    : CHECK_COLOR_FILL_HOVER_LIGHT;
}

Gdiplus::Bitmap* VivaldiInstallDialog::GetGDIBitmapFromResource(int bitmap_id) {
  Gdiplus::Bitmap* bitmap = nullptr;

  HRSRC hResource =
      ::FindResource(instance_, MAKEINTRESOURCE(bitmap_id), L"PNG");
  if (!hResource) {
    return bitmap;
  }

  DWORD imageSize = ::SizeofResource(instance_, hResource);
  if (!imageSize) {
    return bitmap;
  }

  const void* pResourceData =
      ::LockResource(::LoadResource(instance_, hResource));
  if (!pResourceData) {
    return bitmap;
  }

  HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
  if (hBuffer) {
    void* pBuffer = ::GlobalLock(hBuffer);
    if (pBuffer) {
      CopyMemory(pBuffer, pResourceData, imageSize);

      IStream* pStream = NULL;
      if (::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK) {
        bitmap = Gdiplus::Bitmap::FromStream(pStream);
        pStream->Release();
        if (bitmap) {
          if (bitmap->GetLastStatus() != Gdiplus::Ok) {
            delete bitmap;
            bitmap = nullptr;
          }
        }
      }
    }
  }
  return bitmap;
}

void VivaldiInstallDialog::ReadLastInstallValues() {
  base::win::RegKey key = vivaldi::OpenRegistryKeyToRead(
      HKEY_CURRENT_USER, vivaldi::constants::kVivaldiKey);
  if (!key.Valid())
    return;

  base::FilePath registry_install_dir(vivaldi::ReadRegistryString(
      vivaldi::constants::kVivaldiInstallerDestinationFolder, key));

  std::optional<vivaldi::InstallType> registry_install_type;
  if (std::optional<uint32_t> value = vivaldi::ReadRegistryUint32(
          vivaldi::constants::kVivaldiInstallerInstallType, key)) {
    registry_install_type = GetInstallTypeFromComboIndex(*value);
    if (!registry_install_type) {
      LOG(ERROR) << "Unsupported install type in "
                 << vivaldi::constants::kVivaldiInstallerInstallType
                 << " registry value - " << *value;
    }
  }

  if (options_.install_dir.empty() && !registry_install_dir.empty()) {
    // The installation directory was not given on the command line. We use the
    // registry value unless the installation type was also given on the command
    // line and it does not match the registry type. In that case we want to use
    // the default value for the path.
    if (!options_.given_install_type || !registry_install_type ||
        *registry_install_type == options_.install_type) {
      options_.install_dir = std::move(registry_install_dir);
    }
  }

  if (!options_.given_install_type && registry_install_type) {
    options_.install_type = *registry_install_type;
    options_.given_install_type = true;
  }

  // Initialize the last standalone.
  if (options_.install_type == vivaldi::InstallType::kStandalone &&
      !options_.install_dir.empty()) {
    last_standalone_folder_ = options_.install_dir;
  } else if (registry_install_type &&
             *registry_install_type == vivaldi::InstallType::kStandalone) {
    // The type was overwritten from the command line, but the registry still
    // points to the standalone, so use that.
    last_standalone_folder_ = std::move(registry_install_dir);
  }

  if (!options_.given_register_browser) {
    if (std::optional<bool> bool_value = vivaldi::ReadRegistryBool(
            vivaldi::constants::kVivaldiInstallerDefaultBrowser, key)) {
      options_.register_browser = *bool_value;
      options_.given_register_browser = true;
    }
  }

  if (std::optional<bool> bool_value = vivaldi::ReadRegistryBool(
          vivaldi::constants::kVivaldiInstallerAdvancedMode, key)) {
    advanced_mode_ = *bool_value;
  }

  if (std::optional<bool> bool_value = vivaldi::ReadRegistryBool(
          vivaldi::constants::kVivaldiInstallerDisableStandaloneAutoupdate,
          key)) {
    disable_standalone_autoupdates_ = *bool_value;
  }

  if (std::optional<bool> bool_value = vivaldi::ReadRegistryBool(
          vivaldi::constants::kVivaldiInstallerUpLoadCrashReports, key)) {
    options_.allow_crashlog_uploads = *bool_value;
  }
}

void VivaldiInstallDialog::SaveInstallValues() {
  base::win::RegKey key = vivaldi::OpenRegistryKeyToWrite(
      HKEY_CURRENT_USER, vivaldi::constants::kVivaldiKey);
  if (!key.Valid())
    return;
  vivaldi::WriteRegistryString(
      vivaldi::constants::kVivaldiInstallerDestinationFolder,
      options_.install_dir.value(), key);
  vivaldi::WriteRegistryUint32(vivaldi::constants::kVivaldiInstallerInstallType,
                               ToComboIndex(options_.install_type), key);
  vivaldi::WriteRegistryBool(
      vivaldi::constants::kVivaldiInstallerDefaultBrowser,
      options_.register_browser, key);
  vivaldi::WriteRegistryBool(vivaldi::constants::kVivaldiInstallerAdvancedMode,
                             advanced_mode_, key);
  // The registry key is used outside of the dialog to disable auto updates.
  if (disable_standalone_autoupdates_ && advanced_mode_) {
    vivaldi::WriteRegistryBool(
        vivaldi::constants::kVivaldiInstallerDisableStandaloneAutoupdate, true,
        key);
  } else {
    // Remove the key not to advertise this option.
    key.DeleteValue(
        vivaldi::constants::kVivaldiInstallerDisableStandaloneAutoupdate);
  }
  vivaldi::WriteRegistryBool(
      vivaldi::constants::kVivaldiInstallerUpLoadCrashReports,
      options_.allow_crashlog_uploads, key);
}

bool VivaldiInstallDialog::InternalSelectLanguage() {
  if (DCHECK_IS_ON()) {
    for (const auto& pair : kLanguages) {
      std::wstring language_code = pair.code;
      vivaldi::NormalizeLanguageCode(language_code);
      DCHECK_EQ(language_code, pair.code) << "The language code " << pair.code
                                          << " in kLanguages is not normalized";
    }
  }
  std::wstring code = vivaldi::GetInstallerLanguage();
  bool found = false;
  std::map<const std::wstring, const std::wstring>::iterator it;
  for (const auto& pair : kLanguages) {
    if (pair.code == code) {
      ComboBox_SelectString(GetDlgItem(hdlg_, IDC_COMBO_LANGUAGE), -1,
                            pair.name);
      found = true;
      break;
    }
  }
  return found;
}

void VivaldiInstallDialog::InitDialog() {
  dialog_ended_ = false;
  is_upgrade_ = vivaldi::IsVivaldiInstalled(options_.install_dir);

  INITCOMMONCONTROLSEX icex;

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_USEREX_CLASSES;

  InitCommonControlsEx(&icex);

  window_title_label_ = CreateWindowEx(
      WS_EX_TRANSPARENT, L"STATIC", L"Install Vivaldi", WS_CHILD | WS_VISIBLE,
      0, 0, 0, 0, hdlg_, (HMENU)IDC_STATIC_DIALOG_TITLE, nullptr, nullptr);

  logo_ =
      CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, 0,
                     0, 0, 0, hdlg_, (HMENU)IDC_STATIC_LOGO, nullptr, nullptr);

  slogan_text_ = CreateWindowEx(
      0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, 0, 0, 0, 0,
      hdlg_, (HMENU)IDC_STATIC_SLOGAN, nullptr, nullptr);

  install_type_label_ =
      CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                     hdlg_, (HMENU)IDC_STATIC_INSTALLTYPES, nullptr, nullptr);

  language_label_ =
      CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                     hdlg_, (HMENU)IDC_STATIC_LANGUAGE, nullptr, nullptr);

  destination_folder_label_ =
      CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                     hdlg_, (HMENU)IDC_STATIC_DEST_FOLDER, nullptr, nullptr);

  x_button_ = CreateWindowEx(
      0, L"BUTTON", L"X", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
      hdlg_, (HMENU)IDC_BTN_CLOSE, nullptr, reinterpret_cast<LPVOID>(this));

  progress_bar_ = CreateWindowEx(0, L"msctls_progress32", L"",
                                 WS_CHILD | WS_VISIBLE | LWS_TRANSPARENT, 0, 0,
                                 0, 0, hdlg_, nullptr, nullptr, nullptr);

  ////  TABSTOPS

  agree_link_ =
      CreateWindowEx(0, L"SYSLINK", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0,
                     0, 0, 0, hdlg_, (HMENU)IDC_SYSLINK_TOS, nullptr, nullptr);

  install_button_ = CreateWindowEx(
      0, L"BUTTON", L"Install",
      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP | BS_DEFPUSHBUTTON, 0,
      0, 0, 0, hdlg_, (HMENU)IDOK, nullptr, reinterpret_cast<LPVOID>(this));

  cancel_button_ = CreateWindowEx(
      0, L"BUTTON", L"Cancel",
      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP, 0, 0, 0, 0, hdlg_,
      (HMENU)IDCANCEL, nullptr, reinterpret_cast<LPVOID>(this));

  toggle_mode_button_ = CreateWindowEx(
      0, L"BUTTON", L"Advanced",
      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP, 0, 0, 0, 0, hdlg_,
      (HMENU)IDC_BTN_MODE, nullptr, reinterpret_cast<LPVOID>(this));

  language_combo_ = CreateWindowEx(
      0, L"COMBOBOX", L"",
      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, 0,
      0, 0, hdlg_, (HMENU)IDC_COMBO_LANGUAGE, nullptr, nullptr);

  install_type_combo_ = CreateWindowEx(
      WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, 0,
      0, 0, hdlg_, (HMENU)IDC_COMBO_INSTALLTYPES, nullptr, nullptr);

  destination_folder_edit_ = CreateWindowEx(
      WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0,
      0, 0, hdlg_, (HMENU)IDC_EDIT_DEST_FOLDER, nullptr, nullptr);

  destination_folder_button_ = CreateWindowEx(
      0, L"BUTTON", L"Browse...",
      WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 0, 0, hdlg_,
      (HMENU)IDC_BTN_BROWSE, nullptr, reinterpret_cast<LPVOID>(this));

  register_default_app_check_ = CreateWindowEx(
      0, L"BUTTON", L"",
      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP | BS_OWNERDRAW, 0, 0,
      0, 0, hdlg_, (HMENU)IDC_CHECK_REGISTER, nullptr, nullptr);

  auto_update_check_ = CreateWindowEx(
      0, L"BUTTON", L"",
      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP | BS_OWNERDRAW, 0, 0,
      0, 0, hdlg_, (HMENU)IDC_CHECK_NO_AUTOUPDATE, nullptr, nullptr);

  allow_crash_reports_check_ = CreateWindowEx(
      0, L"BUTTON", L"",
      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP | BS_OWNERDRAW, 0, 0,
      0, 0, hdlg_, (HMENU)IDC_CHECK_ALLOW_CRASHLOGS, nullptr, nullptr);

  target_directory_tooltip_ = CreateWindowEx(
      WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
      WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, hdlg_, nullptr, nullptr, nullptr);

  // Add all the buttons
  SetWindowSubclass(GetDlgItem(hdlg_, IDOK),
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)ok_button_subclassed_.get());
  SetWindowSubclass(GetDlgItem(hdlg_, IDCANCEL),
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)cancel_button_subclassed_.get());
  SetWindowSubclass(GetDlgItem(hdlg_, IDC_BTN_CLOSE),
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)close_button_subclassed_.get());
  SetWindowSubclass(GetDlgItem(hdlg_, IDC_BTN_MODE),
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)mode_button_subclassed_.get());
  SetWindowSubclass(auto_update_check_,
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)update_checkbutton_subclassed_.get());
  SetWindowSubclass(register_default_app_check_,
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)register_app_checkbutton_subclassed_.get());
  SetWindowSubclass(
      allow_crash_reports_check_, VivaldiInstallDialog::OwnerDrawButtonProc, 0,
      (DWORD_PTR)allow_crashuploads_checkbutton_subclassed_.get());

  SetWindowSubclass(GetDlgItem(hdlg_, IDC_BTN_BROWSE),
                    VivaldiInstallDialog::OwnerDrawButtonProc, 0,
                    (DWORD_PTR)browse_destination_button_subclassed_.get());

  SetWindowSubclass(language_combo_,
                    VivaldiInstallDialog::OwnerDrawComboboxProc, 0,
                    (DWORD_PTR)language_combo_subclassed_.get());

  SetWindowSubclass(install_type_combo_,
                    VivaldiInstallDialog::OwnerDrawComboboxProc, 0,
                    (DWORD_PTR)install_type_combo_subclassed_.get());

  SetWindowSubclass(destination_folder_edit_,
                    VivaldiInstallDialog::OwnerDrawEditProc, 0,
                    (DWORD_PTR)destination_folder_edit_subclassed_.get());

  // Add all buttons that need to do painting inside the dialog painthandler.
  child_controls_.push_back(ok_button_subclassed_.get());
  child_controls_.push_back(cancel_button_subclassed_.get());
  child_controls_.push_back(mode_button_subclassed_.get());
  child_controls_.push_back(update_checkbutton_subclassed_.get());
  child_controls_.push_back(register_app_checkbutton_subclassed_.get());
  child_controls_.push_back(allow_crashuploads_checkbutton_subclassed_.get());
  child_controls_.push_back(browse_destination_button_subclassed_.get());
  child_controls_.push_back(update_checkbutton_subclassed_.get());
  child_controls_.push_back(install_type_combo_subclassed_.get());
  child_controls_.push_back(destination_folder_edit_subclassed_.get());
  child_controls_.push_back(language_combo_subclassed_.get());

  std::map<const std::wstring, const std::wstring>::iterator it;
  for (const auto& pair : kLanguages) {
    ComboBox_AddString(language_combo_, pair.name);
  }
  if (!InternalSelectLanguage()) {
    ::vivaldi::SetInstallerLanguage(L"en-us");
    changed_language_ = true;
    InternalSelectLanguage();
  }

  // After this all the controls has localized strings and we need to use them
  // to calculate control sizes.
  TranslateDialog();

  SetWindowText(destination_folder_edit_, options_.install_dir.value().c_str());

  SendMessage(register_default_app_check_, BM_SETCHECK,
              options_.register_browser ? BST_CHECKED : BST_UNCHECKED, 0);

  SendMessage(auto_update_check_, BM_SETCHECK,
              disable_standalone_autoupdates_ ? BST_CHECKED : BST_UNCHECKED, 0);

  InitUIFont();
}

void VivaldiInstallDialog::LayoutSlogan() {
  if (pRenderTarget_) {
    pRenderTarget_->Release();
  }
  if (pTextLayout_) {
    pTextLayout_->Release();
  }

  if (!pD2DFactory_) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory_);
  }
  if (!pDWriteFactory_) {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&pDWriteFactory_));
  }

  float fontsize = 16.f;  // This should be DIPs.

  if (!pTextFormat_) {
    pDWriteFactory_->CreateTextFormat(
        DIALOG_FONT_NAME /*_EMOJI*/, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontsize, L"",
        &pTextFormat_);
  }

  pTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_WHOLE_WORD);

  // See
  // https://www.charlespetzold.com/blog/2014/01/Character-Formatting-Extensions-with-DirectWrite.html
  // and
  // https://www.codeproject.com/Articles/5351958/Direct2D-Tutorial-Part-5-Text-Display-and-Font-Enu
  // for references.

  // * Measure text size using DirectWrite
  // Layout the slogan text based on the width of the logo, which also control
  // the size of the window.
  SIZE logo_size;
  GetControlSize(logo_, logo_size);
  logo_size.cx *= 3;  // Same as in ::Layout(). Dialog is 3 x logo.

  // Minus the padding.
  logo_size.cx -= LEFT_INDENT;
  float max_width = GetDIPsFromPixels(logo_size.cx);

  pDWriteFactory_->CreateGdiCompatibleTextLayout(
      txt_slogan_str_.c_str(), wcslen(txt_slogan_str_.c_str()), pTextFormat_,
      max_width,                               // Max width for paragraph in DPI
      std::numeric_limits<float>::infinity(),  // max height in DPI
      current_dpi_, nullptr, true, &pTextLayout_);

  // Get text metrics
  // NOTE: All coordinates are in device independent pixels (DIPs) so convert to
  // screen.
  DWRITE_TEXT_METRICS textMetrics;
  pTextLayout_->GetMetrics(&textMetrics);

  slogan_width_ = GetPixelsFromDPI(
      static_cast<int>(ceil(textMetrics.widthIncludingTrailingWhitespace)));
  slogan_height_ = GetPixelsFromDPI(static_cast<int>(ceil(textMetrics.height)));

  // Note this is pixelSize!
  D2D1_SIZE_U size = D2D1::SizeU(slogan_width_, slogan_height_);

  pD2DFactory_->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(slogan_text_, size), &pRenderTarget_);
}

void VivaldiInstallDialog::InitUIFont() {
  dialog_font_ = CreateWinUIFont(FONT_SIZE, FW_NORMAL);

  // Look into loop over "child" controls.
  ::SendMessage(window_title_label_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));

  ::SendMessage(language_label_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(language_combo_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(install_type_label_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(install_type_combo_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));

  ::SendMessage(destination_folder_label_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(destination_folder_edit_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(destination_folder_button_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));

  ::SendMessage(register_default_app_check_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(allow_crash_reports_check_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(agree_link_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(auto_update_check_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));

  ::SendMessage(cancel_button_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(install_button_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(x_button_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
  ::SendMessage(toggle_mode_button_, WM_SETFONT, (WPARAM)dialog_font_,
                MAKELPARAM(TRUE, 0));
}

void VivaldiInstallDialog::TranslateDialog() {
  txt_tos_accept_update_str_ =
      GetLocalizedString(IDS_INSTALL_TXT_TOS_ACCEPT_AND_UPDATE_BASE);
  btn_tos_accept_update_str_ =
      GetLocalizedString(IDS_INSTALL_TOS_ACCEPT_AND_UPDATE_BASE);
  txt_tos_accept_install_str_ =
      GetLocalizedString(IDS_INSTALL_TXT_TOS_ACCEPT_AND_INSTALL_BASE);
  btn_tos_accept_install_str_ =
      GetLocalizedString(IDS_INSTALL_TOS_ACCEPT_AND_INSTALL_BASE);
  btn_simple_mode_str_ = GetLocalizedString(IDS_INSTALL_MODE_SIMPLE_BASE);
  btn_advanced_mode_str_ = GetLocalizedString(IDS_INSTALL_MODE_ADVANCED_BASE);
  tooltip_warn_illegal_destination_str_ =
      GetLocalizedString(IDS_INSTALL_DEST_FOLDER_INVALID_BASE);

  select_folder_str_ =
      GetLocalizedString(IDS_INSTALL_SELECT_A_FOLDER_BASE).c_str();
  txt_slogan_str_ = GetLocalizedString(IDS_INSTALL_SLOGAN_BASE);

  btn_browse_destination_str_ = GetLocalizedString(IDS_INSTALL_BROWSE_BASE);

  auto caption_string = GetLocalizedString(IDS_INSTALL_INSTALL_CAPTION_BASE);
#if !defined(NDEBUG)
  caption_string += L" [" + vivaldi::GetInstallerLanguage() + L"]";
#endif  // NDEBUG
  SetWindowText(hdlg_, caption_string.c_str());

  SetWindowText(window_title_label_, caption_string.c_str());

  SetWindowText(agree_link_, is_upgrade_ ? txt_tos_accept_update_str_.c_str()
                                         : txt_tos_accept_install_str_.c_str());

  SetWindowText(slogan_text_, txt_slogan_str_.c_str());

  SetWindowText(language_label_,
                GetLocalizedString(IDS_INSTALL_LANGUAGE_BASE).c_str());

  SetWindowText(allow_crash_reports_check_,
                GetLocalizedString(IDS_INSTALL_ALLOW_CRASHLOGS_BASE).c_str());

  SetDlgItemText(hdlg_, IDC_STATIC_LANGUAGE,
                 GetLocalizedString(IDS_INSTALL_LANGUAGE_BASE).c_str());
  SetDlgItemText(
      hdlg_, IDC_STATIC_INSTALLTYPES,
      GetLocalizedString(IDS_INSTALL_INSTALLATION_TYPE_BASE).c_str());
  SetDlgItemText(hdlg_, IDC_STATIC_DEST_FOLDER,
                 GetLocalizedString(IDS_INSTALL_DEST_FOLDER_BASE).c_str());
  SetDlgItemText(hdlg_, IDC_BTN_BROWSE,
                 GetLocalizedString(IDS_INSTALL_BROWSE_BASE).c_str());
  SetDlgItemText(
      hdlg_, IDC_CHECK_REGISTER,
      GetLocalizedString(IDS_INSTALL_MAKE_STANDALONE_AVAIL_BASE).c_str());
  SetDlgItemText(hdlg_, IDC_CHECK_NO_AUTOUPDATE,
                 GetLocalizedString(IDS_DISABLE_AUTOUPDATE_BASE).c_str());
  //  SetDlgItemText(hdlg_, IDC_STATIC_WARN,
  //                 GetLocalizedString(IDS_INSTALL_RELAUNCH_WARNING_BASE).c_str());
  SetDlgItemText(hdlg_, IDCANCEL,
                 GetLocalizedString(IDS_INSTALL_CANCEL_BASE).c_str());
  SetDlgItemText(hdlg_, IDC_BTN_CANCEL_SIMPLE,
                 GetLocalizedString(IDS_INSTALL_CANCEL_BASE).c_str());
  SetDlgItemText(hdlg_, IDOK,
                 is_upgrade_ ? btn_tos_accept_update_str_.c_str()
                             : btn_tos_accept_install_str_.c_str());
  SetDlgItemText(hdlg_, IDC_BTN_OK_SIMPLE,
                 is_upgrade_ ? btn_tos_accept_update_str_.c_str()
                             : btn_tos_accept_install_str_.c_str());
  SetDlgItemText(hdlg_, IDC_BTN_MODE,
                 advanced_mode_ ? btn_simple_mode_str_.c_str()
                                : btn_advanced_mode_str_.c_str());

  SetDlgItemText(hdlg_, IDC_BTN_BROWSE, btn_browse_destination_str_.c_str());

  base::Time::Exploded time_exploded;
  base::Time::Now().LocalExplode(&time_exploded);
  auto copyright_year = std::to_wstring(time_exploded.year);

  SetDlgItemText(hdlg_, IDC_SYSLINK_PRIVACY_POLICY_SIMPLE,
                 vivaldi_installer::GetLocalizedStringF(
                     IDS_INSTALL_COPYRIGHT_AND_POLICY_BASE, copyright_year)
                     .c_str());
  SetDlgItemText(hdlg_, IDC_SYSLINK_PRIVACY_POLICY,
                 vivaldi_installer::GetLocalizedStringF(
                     IDS_INSTALL_COPYRIGHT_AND_POLICY_BASE, copyright_year)
                     .c_str());

  std::wstring all_users_str =
      GetLocalizedString(IDS_INSTALL_INSTALL_FOR_ALL_USERS_BASE);
  std::wstring current_user_str =
      GetLocalizedString(IDS_INSTALL_INSTALL_PER_USER_BASE);
  std::wstring standalone_str =
      GetLocalizedString(IDS_INSTALL_INSTALL_STANDALONE_BASE);

  ComboBox_ResetContent(install_type_combo_);
  ComboBox_AddString(install_type_combo_, all_users_str.c_str());

  ComboBox_AddString(install_type_combo_, current_user_str.c_str());
  ComboBox_AddString(install_type_combo_, standalone_str.c_str());
  ComboBox_SetCurSel(install_type_combo_, ToComboIndex(options_.install_type));

  Layout();
}

// Finds the tree view of the SHBrowseForFolder dialog
static BOOL CALLBACK EnumChildProcFindTreeView(HWND hwnd, LPARAM lparam) {
  HWND* tree_view = reinterpret_cast<HWND*>(lparam);
  DCHECK(tree_view);

  const int MAX_BUF_SIZE = 80;
  const wchar_t TREE_VIEW_CLASS_NAME[] = L"SysTreeView32";
  std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_BUF_SIZE]);

  GetClassName(hwnd, buffer.get(), MAX_BUF_SIZE - 1);
  if (std::wstring(buffer.get()) == TREE_VIEW_CLASS_NAME) {
    *tree_view = hwnd;
    return FALSE;
  }
  *tree_view = nullptr;
  return TRUE;
}

static int CALLBACK BrowseCallbackProc(HWND hwnd,
                                       UINT msg,
                                       LPARAM lparam,
                                       LPARAM lpdata) {
  static HWND tree_view = nullptr;
  switch (msg) {
    case BFFM_INITIALIZED:
      if (lpdata)
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpdata);
      EnumChildWindows(hwnd, EnumChildProcFindTreeView, (LPARAM)&tree_view);
      break;
    case BFFM_SELCHANGED:
      if (IsWindow(tree_view)) {
        // Make sure the current selection is scrolled into view
        HTREEITEM item = TreeView_GetSelection(tree_view);
        if (item)
          TreeView_EnsureVisible(tree_view, item);
      }
      break;
  }
  return 0;
}

void VivaldiInstallDialog::ShowBrowseFolderDialog() {
  BROWSEINFO bi;
  memset(&bi, 0, sizeof(bi));

  bi.hwndOwner = hdlg_;
  bi.lpszTitle = select_folder_str_.c_str();
  bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
  bi.lpfn = BrowseCallbackProc;
  bi.lParam = (LPARAM)options_.install_dir.value().c_str();

  OleInitialize(nullptr);

  LPITEMIDLIST pIDL = SHBrowseForFolder(&bi);

  if (!pIDL)
    return;

  std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
  if (!SHGetPathFromIDList(pIDL, buffer.get())) {
    CoTaskMemFree(pIDL);
    return;
  }
  options_.install_dir = base::FilePath(buffer.get());

  CoTaskMemFree(pIDL);
  OleUninitialize();
}

void VivaldiInstallDialog::DoDialog() {
  MSG msg;
  BOOL ret;
  while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
    if (ret == -1) {
      dlg_result_ = INSTALL_DLG_ERROR;
      return;
    }

    if (!IsDialogMessage(hdlg_, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    if (dialog_ended_)
      return;
  }
}

void VivaldiInstallDialog::OnInstallTypeSelection() {
  int i = ComboBox_GetCurSel(GetDlgItem(hdlg_, IDC_COMBO_INSTALLTYPES));
  std::optional<vivaldi::InstallType> type = GetInstallTypeFromComboIndex(i);
  if (!type || *type == options_.install_type)
    return;

  if (options_.install_type == vivaldi::InstallType::kStandalone) {
    last_standalone_folder_ = options_.install_dir;
  }
  options_.install_type = *type;
  if (options_.install_type == vivaldi::InstallType::kStandalone) {
    options_.install_dir = last_standalone_folder_;
  } else {
    base::FilePath path =
        vivaldi::GetDefaultInstallTopDir(options_.install_type);
    if (!path.empty()) {
      options_.install_dir = std::move(path);
    }
  }
  SetDlgItemText(hdlg_, IDC_EDIT_DEST_FOLDER,
                 options_.install_dir.value().c_str());

  Layout();
}

void VivaldiInstallDialog::OnLanguageSelection() {
  int i = ComboBox_GetCurSel(language_combo_);
  if (i != CB_ERR) {
    const int len = ComboBox_GetLBTextLen(language_combo_, i);
    if (len <= 0)
      return;

    std::wstring buf;
    buf.resize(len);
    ComboBox_GetLBText(language_combo_, i, &buf[0]);
    std::map<const std::wstring, const std::wstring>::iterator it;
    for (const auto& pair : kLanguages) {
      if (pair.name == buf) {
        vivaldi::SetInstallerLanguage(pair.code);
        changed_language_ = true;
        TranslateDialog();
        break;
      }
    }
  }
}

bool VivaldiInstallDialog::IsInstallPathValid(const base::FilePath& path) {
  bool has_illegal_chars = wcspbrk(path.value().c_str(), L"/*?\"<>|");
  bool path_is_valid = !path.empty() && !has_illegal_chars;
  return path_is_valid;
}

InstallStatus VivaldiInstallDialog::ShowEULADialog() {
  VLOG(1) << "About to show EULA";
  std::wstring eula_path = GetLocalizedEulaResource();
  if (eula_path.empty()) {
    LOG(ERROR) << "No EULA path available";
    return EULA_REJECTED;
  }
  std::wstring inner_frame_path = GetInnerFrameEULAResource();
  if (inner_frame_path.empty()) {
    LOG(ERROR) << "No EULA inner frame path available";
    return EULA_REJECTED;
  }
  // Newer versions of the caller pass an inner frame parameter that must
  // be given to the html page being launched.
  EulaHTMLDialog dlg(eula_path, inner_frame_path);
  EulaHTMLDialog::Outcome outcome = dlg.ShowModal();
  if (EulaHTMLDialog::REJECTED == outcome) {
    LOG(ERROR) << "EULA rejected or EULA failure";
    return EULA_REJECTED;
  }
  if (EulaHTMLDialog::ACCEPTED_OPT_IN == outcome) {
    VLOG(1) << "EULA accepted (opt-in)";
    return EULA_ACCEPTED_OPT_IN;
  }
  VLOG(1) << "EULA accepted (no opt-in)";
  return EULA_ACCEPTED;
}

std::wstring VivaldiInstallDialog::GetInnerFrameEULAResource() {
  wchar_t full_exe_path[MAX_PATH];
  int len = ::GetModuleFileName(nullptr, full_exe_path, MAX_PATH);
  if (len == 0 || len == MAX_PATH)
    return L"";

  const wchar_t* inner_frame_resource = L"IDR_OEM_EULA_VIV.HTML";
  if (!FindResource(nullptr, inner_frame_resource, RT_HTML))
    return L"";
  // spaces and DOS paths must be url encoded.
  std::wstring url_path = base::UTF8ToWide(base::StringPrintf(
      "res://%ls/#23/%ls", full_exe_path, inner_frame_resource));

  // the cast is safe because url_path has limited length
  // (see the definition of full_exe_path and resource).
  DCHECK(kuint32max > (url_path.size() * 3));
  DWORD count = static_cast<DWORD>(url_path.size() * 3);
  std::unique_ptr<wchar_t[]> url_canon(new wchar_t[count]);
  HRESULT hr = ::UrlCanonicalizeW(url_path.c_str(), url_canon.get(), &count,
                                  URL_ESCAPE_UNSAFE);
  if (SUCCEEDED(hr))
    return std::wstring(url_canon.get());
  return url_path;
}

void VivaldiInstallDialog::OnInstallModeSelection() {
  SetWindowText(toggle_mode_button_, advanced_mode_
                                         ? btn_simple_mode_str_.c_str()
                                         : btn_advanced_mode_str_.c_str());
}

void VivaldiInstallDialog::ClearAll() {
  if (logo_bmp_) {
    DeleteObject(logo_bmp_);
    logo_bmp_ = nullptr;
  }

  DeleteDC(image_dc_);
  DeleteDC(mask_dc_);
}

void VivaldiInstallDialog::UpdateSize() {
  DCHECK(instance_);

  InitUIFont();
  current_dpi_ = GetCurrentDpi(hdlg_, this);

  RECT updateRect;
  GetClientRect(hdlg_, &updateRect);
  InvalidateRect(hdlg_, &updateRect, FALSE);
}

void VivaldiInstallDialog::CenterOnScreen() {
  RECT rect_window;
  GetWindowRect(hdlg_, &rect_window);

  RECT rect_parent;
  HWND hwnd_parent = GetDesktopWindow();
  GetWindowRect(hwnd_parent, &rect_parent);

  int w = rect_window.right - rect_window.left;
  int h = rect_window.bottom - rect_window.top;
  int x = ((rect_parent.right - rect_parent.left) - w) / 2 + rect_parent.left;
  int y = ((rect_parent.bottom - rect_parent.top) - h) / 2 + rect_parent.top;

  wchar_t text_buffer[500] = {0};
  swprintf(text_buffer, _countof(text_buffer),
           L"dialogSize centered %d, %d, %d %d", x, y, w, h);

  MoveWindow(hdlg_, x, y, w, h, FALSE);
}

int VivaldiInstallDialog::GetPixelsFromDPI(int pixels) {
  return static_cast<int>(
      ceil(pixels * current_dpi_ / USER_DEFAULT_SCREEN_DPI));
}

int VivaldiInstallDialog::GetDIPsFromPixels(int pixels) {
  return static_cast<int>(pixels / (current_dpi_ / USER_DEFAULT_SCREEN_DPI));
}

int VivaldiInstallDialog::GetTitlebarHeight() {
  return GetPixelsFromDPI(TITLEBAR_HEIGHT);
}

int VivaldiInstallDialog::GetFooterbarHeight() {
  return GetPixelsFromDPI(TOOLBAR_BACKGROUND_HEIGHT);
}

HBRUSH VivaldiInstallDialog::OnCtlColorEdit(HWND hwnd_ctl, HDC hdc) {
  // editbox coloring.
  SetTextColor(hdc, dark_mode_ ? TEXT_COLOR_DARK : TEXT_COLOR_LIGHT);
  SetBkColor(
      hdc, dark_mode_ ? TOP_BACKGROUND_COLOR_DARK : TOP_BACKGROUND_COLOR_LIGHT);
  return background_brush_.get();
}

HBRUSH VivaldiInstallDialog::OnCtlColor(HWND hwnd_ctl, HDC hdc) {
  int id = GetDlgCtrlID(hwnd_ctl);

  // Make controls transparent.
  SetTextColor(hdc, TRANSPARENT);
  SetBkColor(hdc, GetTopBackgroundColor());

  if (id == IDC_SYSLINK_TOS) {
    SetTextColor(hdc, GetTextColor());  // dark gray
    return background_brush_.get();
  }

  // Dim the combobox labels a bit.
  if (id == IDC_STATIC_LANGUAGE || id == IDC_STATIC_INSTALLTYPES ||
      id == IDC_STATIC_DEST_FOLDER) {
    SetTextColor(hdc, GetTextColor());
  }

  if (id == IDC_STATIC_DIALOG_TITLE) {
    SetTextColor(hdc, GetTextColor());
  }

  // Prevent any futher background paint.
  return background_brush_.get();
}

// Layout the controls in the dialog and resizes the window. Note we only change
// height.
bool VivaldiInstallDialog::Layout() {
  //  ----------------------------
  //  |title                    X|
  //  |--------------------------|
  //  | LOGO                     |
  //  | SLOGAN                   |
  //  | LANGUAGE title           |
  //  | LANGUAGE combo           |
  //  | TOS                      |
  //  |--------------------------|
  //  | <OK> <Cancel>     <Mode> |
  //  ----------------------------

  SIZE control_size;
  SIZE previous_control_size;
  RECT window_rect;
  if (!GetWindowRect(hdlg_, &window_rect)) {
    return false;
  }

  LayoutSlogan();

  this_->dialog_rect_ = gfx::Rect(window_rect);

  int dialog_height = 0;

  // Grow height as we go, and check width against a maximum.

  // Common header section
  GetControlSize(window_title_label_, control_size);
  MoveWindow(window_title_label_, TITLEBAR_LEFT_PADDING,
             (GetTitlebarHeight() / 2) - (control_size.cy / 2), control_size.cx,
             control_size.cy, FALSE);
  dialog_height += GetTitlebarHeight();
  window_rect.bottom += control_size.cy;

  // titlebar is always included

  // IDC_STATIC_LOGO
  GetControlSize(logo_, control_size);

  dialog_height += 20;

  // center the bitmap
  //  int dx = ((window_rect.right - window_rect.left) - control_size.cx) / 2;
  MoveWindow(logo_, LEFT_INDENT /*dx*/, dialog_height, control_size.cx,
             control_size.cy, FALSE);

  // Adjust the dialog width to the logo. We will adjust below for certain known
  // wide controls.

  window_rect.left = 0;
  // Multiply the logo width by 3 to get a wider dialog with some space on the
  // right. Windows older than 11 will not paint a background for the control,
  // so keep inside bounds.
  control_size.cx *= 3;
  window_rect.right = (control_size.cx) + LEFT_INDENT + 2 * RIGHT_PADDING;

  dialog_height += control_size.cy;
  dialog_height += 25;

  // dialog action buttons as well

  if (!advanced_mode_) {  // simple mode
                          // height of titlebar is constant

    // Hide the ones not applicable.

    ShowWindow(slogan_text_, SW_SHOW);

    ShowWindow(install_type_label_, SW_HIDE);
    ShowWindow(install_type_combo_, SW_HIDE);
    ShowWindow(language_label_, SW_HIDE);
    ShowWindow(language_combo_, SW_HIDE);
    ShowWindow(destination_folder_label_, SW_HIDE);
    ShowWindow(destination_folder_edit_, SW_HIDE);
    ShowWindow(destination_folder_button_, SW_HIDE);
    ShowWindow(register_default_app_check_, SW_HIDE);
    ShowWindow(allow_crash_reports_check_,
               SW_SHOW);  // This might go into both modes?
    ShowWindow(auto_update_check_, SW_HIDE);

    dialog_height += LOGO_SLOGAN_PADDING;

    // IDC_STATIC_SLOGAN
    GetControlSize(slogan_text_, control_size);

    MoveWindow(slogan_text_, LEFT_INDENT, dialog_height,
               control_size.cx + LEFT_INDENT, control_size.cy, FALSE);
    dialog_height += control_size.cy;

    dialog_height += 7 * CHECKBOX_Y_PADDING;

    // IDC_CHECK_ALLOW_CRASHLOGS
    GetControlSize(allow_crash_reports_check_, control_size);
    MoveWindow(allow_crash_reports_check_, LEFT_INDENT, dialog_height,
               control_size.cx + LEFT_INDENT, control_size.cy, FALSE);
    dialog_height += control_size.cy + GetPixelsFromDPI(CHECKBOX_Y_PADDING);

    // In simple mode we always calculate a correct target directory.
    EnableWindow(GetDlgItem(hdlg_, IDOK), true);

  } else {
    // Hide the simple entries.

    ShowWindow(slogan_text_, SW_HIDE);

    ShowWindow(install_type_label_, SW_SHOW);
    ShowWindow(install_type_combo_, SW_SHOW);
    ShowWindow(language_label_, SW_SHOW);
    ShowWindow(language_combo_, SW_SHOW);

    int doNotShowForAllUserInstall =
        options_.install_type == vivaldi::InstallType::kForAllUsers ? SW_HIDE
                                                                    : SW_SHOW;

    ShowWindow(destination_folder_label_, doNotShowForAllUserInstall);
    ShowWindow(destination_folder_edit_, doNotShowForAllUserInstall);
    ShowWindow(destination_folder_button_, doNotShowForAllUserInstall);

    int showForStandaloneInstall =
        options_.install_type == vivaldi::InstallType::kStandalone ? SW_SHOW
                                                                   : SW_HIDE;

    ShowWindow(register_default_app_check_, showForStandaloneInstall);
    ShowWindow(allow_crash_reports_check_, SW_SHOW);
    ShowWindow(auto_update_check_, showForStandaloneInstall);

    // IDC_STATIC_LANGUAGE
    GetControlSize(language_label_, control_size);
    MoveWindow(language_label_, LEFT_INDENT, dialog_height, control_size.cx,
               control_size.cy, FALSE);

    int middleGround = LEFT_INDENT + (window_rect.right - window_rect.left) / 2;
    // IDC_STATIC_INSTALLTYPES
    GetControlSize(install_type_label_, control_size);
    MoveWindow(install_type_label_, middleGround, dialog_height,
               control_size.cx, control_size.cy, FALSE);

    dialog_height += control_size.cy;
    // Padding.
    dialog_height += GetPixelsFromDPI(COMBOBOX_LABEL_PADDING);

    // IDC_COMBO_LANGUAGE
    GetControlSize(language_combo_, control_size);
    MoveWindow(language_combo_, LEFT_INDENT, dialog_height, control_size.cx,
               control_size.cy, FALSE);

    // IDC_COMBO_INSTALLTYPES
    GetControlSize(install_type_combo_, control_size);
    MoveWindow(install_type_combo_, middleGround, dialog_height,
               control_size.cx, control_size.cy, FALSE);
    dialog_height += control_size.cy;

    // IDC_STATIC_DEST_FOLDER
    if (options_.install_type != vivaldi::InstallType::kForAllUsers) {
      GetControlSize(destination_folder_label_, control_size);
      MoveWindow(destination_folder_label_, LEFT_INDENT, dialog_height,
                 control_size.cx, control_size.cy, FALSE);
      dialog_height += control_size.cy;

      // IDC_EDIT_DEST_FOLDER
      GetControlSize(destination_folder_edit_, control_size);
      GetControlSize(destination_folder_button_, previous_control_size);

      MoveWindow(destination_folder_edit_, LEFT_INDENT, dialog_height,
                 (window_rect.right - window_rect.left) -
                     (previous_control_size.cx + ACTION_BUTTON_PADDING +
                      LEFT_INDENT + 16),
                 control_size.cy, FALSE);

      MoveWindow(destination_folder_button_,
                 (window_rect.right - window_rect.left) -
                     previous_control_size.cx - ACTION_BUTTON_PADDING +
                     BUTTON_PADDING,
                 dialog_height, previous_control_size.cx,
                 previous_control_size.cy, FALSE);

      control_size.cy += EDIT_Y_PADDING;
      dialog_height += previous_control_size.cy + 8;

      std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
      if (buffer.get()) {
        GetWindowText(destination_folder_edit_, buffer.get(), MAX_PATH - 1);
        this_->UpdateTargetPathResult(base::FilePath(buffer.get()));
      }
    }

    dialog_height += GetPixelsFromDPI(CHECKBOX_Y_PADDING);

    // IDC_CHECK_REGISTER
    if (options_.install_type == vivaldi::InstallType::kStandalone) {
      GetControlSize(register_default_app_check_, control_size);
      MoveWindow(register_default_app_check_, LEFT_INDENT, dialog_height,
                 control_size.cx + LEFT_INDENT, control_size.cy, FALSE);
      dialog_height += control_size.cy + GetPixelsFromDPI(CHECKBOX_Y_PADDING);
      // IDC_CHECK_NO_AUTOUPDATE
      GetControlSize(auto_update_check_, control_size);
      MoveWindow(auto_update_check_, LEFT_INDENT, dialog_height,
                 control_size.cx + LEFT_INDENT, control_size.cy, FALSE);
      dialog_height += control_size.cy + GetPixelsFromDPI(CHECKBOX_Y_PADDING);
    }
    // IDC_CHECK_ALLOW_CRASHLOGS
    GetControlSize(allow_crash_reports_check_, control_size);
    MoveWindow(allow_crash_reports_check_, LEFT_INDENT, dialog_height,
               control_size.cx + LEFT_INDENT, control_size.cy, FALSE);
    dialog_height += control_size.cy + GetPixelsFromDPI(CHECKBOX_Y_PADDING);
  }

  // Padding.
  dialog_height += control_size.cy;

  // IDC_SYSLINK_TOS
  GetControlSize(agree_link_, control_size);
  MoveWindow(agree_link_, LEFT_INDENT, dialog_height, control_size.cx,
             control_size.cy, FALSE);

  dialog_height += control_size.cy;

  // Padding.
  dialog_height += 6 * CHECKBOX_Y_PADDING;

  // ************************************** Common footer section

  dialog_height += GetFooterbarHeight();

  GetControlSize(install_button_, control_size);

  int middleFromBottom = (GetFooterbarHeight() / 2) + (control_size.cy / 2);

  MoveWindow(install_button_, ACTION_BUTTON_PADDING,
             dialog_height - middleFromBottom, control_size.cx, control_size.cy,
             FALSE);

  GetControlSize(cancel_button_, previous_control_size);
  MoveWindow(cancel_button_, control_size.cx + ACTION_BUTTON_PADDING + 10,
             dialog_height - middleFromBottom, previous_control_size.cx,
             previous_control_size.cy, FALSE);

  GetControlSize(toggle_mode_button_, control_size);
  MoveWindow(toggle_mode_button_,
             (window_rect.right - window_rect.left) - control_size.cx -
                 ACTION_BUTTON_PADDING,
             dialog_height - middleFromBottom, control_size.cx, control_size.cy,
             FALSE);

  // The other buttons are on the same line.

  // All right aligned controls need to position itself as last to make sure we
  // know the dialog width.

  GetControlSize(x_button_, control_size);

  int closeBoxSize = GetTitlebarHeight() - WINDOW_BORDER_WIDTH;

  MoveWindow(x_button_, (window_rect.right - window_rect.left) - closeBoxSize,
             1, closeBoxSize - WINDOW_BORDER_WIDTH,
             closeBoxSize - WINDOW_BORDER_WIDTH, FALSE);

  // Resize the whole window to fit the resized controls.
  int width = window_rect.right - window_rect.left;
  int height = dialog_height;

  if (!SetWindowPos(hdlg_, nullptr, 0, 0, width, height,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW)) {
    return false;
  }

  RedrawWindow(hdlg_, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);

  return true;
}

/*static*/
INT_PTR CALLBACK VivaldiInstallDialog::DlgProc(HWND hdlg,
                                               UINT msg,
                                               WPARAM wparam,
                                               LPARAM lparam) {
  switch (msg) {
    case WM_INITDIALOG: {
      // Dialog transparency test.
      /*
                SetWindowLong(hdlg, GWL_EXSTYLE, GetWindowLong(hdlg,
         GWL_EXSTYLE) | WS_EX_LAYERED); SetWindowPos(hdlg, NULL, 0, 0, 0, 0,
         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                // Extend the frame to the whole window to make everything
         transparent. MARGINS margins = {-1, -1, -1, -1};
                DwmExtendFrameIntoClientArea(hdlg, &margins);
                COLORREF crColor = GetSysColor(COLOR_BTNFACE);
                SetLayeredWindowAttributes(hdlg, crColor, 0, LWA_COLORKEY);
            */

      this_ = reinterpret_cast<VivaldiInstallDialog*>(lparam);

      this_->topLinePen_light_ = CreatePen(PS_SOLID, 2, RGB(242, 242, 242));
      this_->bottomLinePen_light_ = CreatePen(PS_SOLID, 2, RGB(235, 235, 235));

      this_->topLinePen_dark_ = CreatePen(PS_SOLID, 2, RGB(52, 52, 52));
      this_->bottomLinePen_dark_ = CreatePen(PS_SOLID, 2, RGB(50, 50, 50));

      DCHECK(this_);
      DCHECK(!this_->hdlg_);
      this_->hdlg_ = hdlg;

      RECT dialog_rect;
      GetWindowRect(hdlg, &dialog_rect);

      this_->dialog_rect_ = gfx::Rect(dialog_rect);

      {
        // Tell the syslink control to accept color change.
        LITEM item = {0};
        item.iLink = 0;
        item.mask = LIF_ITEMINDEX | LIF_STATE;
        item.state = LIS_DEFAULTCOLORS;
        item.stateMask = LIS_DEFAULTCOLORS;
        SendMessage(GetDlgItem(hdlg, IDC_SYSLINK_PRIVACY_POLICY), LM_SETITEM, 0,
                    (LPARAM)&item);
        SendMessage(GetDlgItem(hdlg, IDC_SYSLINK_PRIVACY_POLICY_SIMPLE),
                    LM_SETITEM, 0, (LPARAM)&item);
      }
      this_->UpdateSize();
      this_->CenterOnScreen();

      // This plays together with WM_NCALCSIZE.
      MARGINS m{0, 0, 0, 1};
      DwmExtendFrameIntoClientArea(hdlg, &m);

      return (INT_PTR)TRUE;
    }

    case WM_ACTIVATE: {
      this_->active_ = wparam != WA_INACTIVE;
      if (base::win::GetVersion() >= base::win::Version::WIN11) {
        COLORREF border_color = this_->GetWindowBorderColor();
        DwmSetWindowAttribute(hdlg, DWMWA_BORDER_COLOR, &border_color,
                              sizeof(COLORREF));
      } else {
        // make sure ncpaint is triggerd to update the border color.
        UINT flags = RDW_INVALIDATE | RDW_FRAME;
        RedrawWindow(hdlg, nullptr, nullptr, flags);
      }
    } break;

    case WM_NCCALCSIZE: {
      if (base::win::GetVersion() < base::win::Version::WIN11) {
        SetWindowLong(hdlg, DWLP_MSGRESULT, 0);

        // Allow for custom border
        if (wparam == TRUE) {
          NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lparam;
          // Add space for custom border (2 pixels on each side)
          params->rgrc[0].left += WINDOW_BORDER_WIDTH;
          params->rgrc[0].top += WINDOW_BORDER_WIDTH;
          params->rgrc[0].right -= WINDOW_BORDER_WIDTH;
          params->rgrc[0].bottom -= WINDOW_BORDER_WIDTH;
        }
      }
      return 0;
    }
    case WM_NCPAINT: {
      // Handle non-client area painting
      HDC hdc = GetWindowDC(hdlg);

      // This is handled by the system on Windows11 and newer.
      if (base::win::GetVersion() < base::win::Version::WIN11) {
        RECT windowRect;
        GetWindowRect(hdlg, &windowRect);
        RECT clientRect;
        GetClientRect(hdlg, &clientRect);

        // Convert to screen coordinates
        POINT pt = {0, 0};
        ClientToScreen(hdlg, &pt);
        OffsetRect(&clientRect, pt.x, pt.y);

        // Create a region for the border area
        HRGN hRgn = CreateRectRgn(0, 0, windowRect.right - windowRect.left,
                                  windowRect.bottom - windowRect.top);

        // Get accent color
        COLORREF border_color = this_->GetWindowBorderColor();

        // Draw the border
        HPEN hPen = CreatePen(PS_SOLID, WINDOW_BORDER_WIDTH, border_color);
        HBRUSH hBrush = CreateSolidBrush(border_color);

        SelectObject(hdc, hPen);
        SelectObject(hdc, hBrush);

        // Draw top border
        MoveToEx(hdc, 0, 0, NULL);
        LineTo(hdc, windowRect.right - windowRect.left, 0);

        // Draw left border
        MoveToEx(hdc, 0, 0, NULL);
        LineTo(hdc, 0, windowRect.bottom - windowRect.top);

        // Draw right border
        MoveToEx(hdc, windowRect.right - windowRect.left - WINDOW_BORDER_WIDTH,
                 0, NULL);
        LineTo(hdc, windowRect.right - windowRect.left - WINDOW_BORDER_WIDTH,
               windowRect.bottom - windowRect.top);

        // Draw bottom border
        MoveToEx(hdc, 0,
                 windowRect.bottom - windowRect.top - WINDOW_BORDER_WIDTH,
                 NULL);
        LineTo(hdc, windowRect.right - windowRect.left,
               windowRect.bottom - windowRect.top - WINDOW_BORDER_WIDTH);

        DeleteObject(hPen);
        DeleteObject(hBrush);
        DeleteObject(hRgn);
      }
      ReleaseDC(hdlg, hdc);
      return 0;
    }

    case WM_SETTINGCHANGE:
      // Make sure the dialog is visible if the resolution changes.
      if (wparam == SPI_SETWORKAREA && this_) {
        this_->UpdateSize();
        this_->CenterOnScreen();
      }
      this_->UpdateTheme(hdlg, this_->accent_color_);

      this_->Layout();

      InvalidateRect(hdlg, NULL, TRUE);
      return TRUE;

    case WM_NCHITTEST: {
      int xPos = GET_X_LPARAM(lparam);
      int yPos = GET_Y_LPARAM(lparam);
      RECT titleRect;
      GetWindowRect(hdlg, &titleRect);
      titleRect.bottom = titleRect.top + this_->GetTitlebarHeight();
      if (PtInRect(&titleRect, POINT(xPos, yPos))) {
        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, HTCAPTION);
        return TRUE;
      }
    } break;

    case WM_ERASEBKGND:
      return 1;  // We do this in WM_PAINT.

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
      if (this_) {
        return (INT_PTR)this_->OnCtlColorEdit((HWND)lparam, (HDC)wparam);
      }
      break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
      if (this_)
        return (INT_PTR)this_->OnCtlColor((HWND)lparam, (HDC)wparam);
      break;

    case WM_NOTIFY: {
      LPNMHDR pnmh = (LPNMHDR)lparam;
      if (pnmh->idFrom == IDC_SYSLINK_TOS ||
          pnmh->idFrom == IDC_SYSLINK_TOS_SIMPLE) {
        if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_RETURN)) {
          this_->ShowEULADialog();
        }
      } else if (pnmh->idFrom == IDC_SYSLINK_PRIVACY_POLICY ||
                 pnmh->idFrom == IDC_SYSLINK_PRIVACY_POLICY_SIMPLE) {
        if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_RETURN)) {
          ShellExecute(nullptr, L"open", VIVALDI_PRIVACY_LINK, nullptr, nullptr,
                       SW_SHOWNORMAL);
        }
      }
    } break;

    case WM_COMMAND:
      switch (LOWORD(wparam)) {
        case IDC_CHECK_NO_AUTOUPDATE:
          this_->disable_standalone_autoupdates_ =
              !this_->disable_standalone_autoupdates_;
          break;

        case IDC_CHECK_ALLOW_CRASHLOGS:
          this_->options_.allow_crashlog_uploads =
              !this_->options_.allow_crashlog_uploads;
          break;

        case IDC_CHECK_REGISTER:
          this_->options_.register_browser = !this_->options_.register_browser;
          break;
        case IDOK:
        case IDC_BTN_OK_SIMPLE: {
          this_->dlg_result_ = INSTALL_DLG_INSTALL;
          std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
          if (buffer.get()) {
            GetDlgItemText(hdlg, IDC_EDIT_DEST_FOLDER, buffer.get(),
                           MAX_PATH - 1);
            this_->options_.install_dir = base::FilePath(buffer.get());
          }
          int i = SendMessage(GetDlgItem(hdlg, IDC_COMBO_INSTALLTYPES),
                              CB_GETCURSEL, 0, 0);
          std::optional<vivaldi::InstallType> type =
              GetInstallTypeFromComboIndex(i);
          if (type) {
            this_->options_.install_type = *type;
          }

          // TODO: What we want to do here is to keep this dialog open remove
          // the action buttons cancel and mode + close buttons then draw the
          // "progressbar" inside the install button.

          AnimateWindow(hdlg, 200, AW_BLEND | AW_HIDE);

          EndDialog(hdlg, 0);
          this_->dialog_ended_ = true;
        } break;

        case IDC_BTN_CLOSE:
        case IDCANCEL:
        case IDC_BTN_CANCEL_SIMPLE:
          if (MessageBox(
                  hdlg,
                  GetLocalizedString(IDS_INSTALL_NOT_FINISHED_PROMPT_BASE)
                      .c_str(),
                  GetLocalizedString(IDS_INSTALL_INSTALLER_NAME_BASE).c_str(),
                  MB_YESNO | MB_ICONQUESTION) == IDYES) {
            AnimateWindow(hdlg, 200, AW_BLEND | AW_HIDE);

            this_->dlg_result_ = INSTALL_DLG_CANCEL;
            EndDialog(hdlg, 0);
            this_->dialog_ended_ = true;
          }
          break;
        case IDC_BTN_BROWSE: {
          std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
          if (buffer.get()) {
            GetDlgItemText(hdlg, IDC_EDIT_DEST_FOLDER, buffer.get(),
                           MAX_PATH - 1);
            this_->options_.install_dir = base::FilePath(buffer.get());
          }
          this_->ShowBrowseFolderDialog();
          SetDlgItemText(hdlg, IDC_EDIT_DEST_FOLDER,
                         this_->options_.install_dir.value().c_str());
        } break;
        case IDC_BTN_MODE:

          // change the dialogtype and re-layout
          this_->advanced_mode_ = !this_->advanced_mode_;

          this_->OnInstallModeSelection();
          this_->Layout();

          InvalidateRect(this_->hdlg_, nullptr, FALSE);

          break;
        case IDC_COMBO_INSTALLTYPES:
          if (HIWORD(wparam) == CBN_SELCHANGE) {
            // change the dialog
            this_->OnInstallTypeSelection();
            this_->Layout();
          }
          break;
        case IDC_COMBO_LANGUAGE:
          if (HIWORD(wparam) == CBN_SELCHANGE)
            this_->OnLanguageSelection();
          break;
        case IDC_EDIT_DEST_FOLDER: {
          if (HIWORD(wparam) == EN_CHANGE) {
            std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
            if (buffer.get()) {
              GetDlgItemText(hdlg, IDC_EDIT_DEST_FOLDER, buffer.get(),
                             MAX_PATH - 1);
              base::FilePath new_path(buffer.get());

              // Only layout when changes are done.
              bool s_upgrade = this_->is_upgrade_;
              bool s_valid_target = this_->is_valid_target_;

              this_->is_upgrade_ = vivaldi::IsVivaldiInstalled(new_path);
              this_->is_valid_target_ = this_->UpdateTargetPathResult(new_path);
              if (s_upgrade != this_->is_upgrade_ ||
                  s_valid_target != this_->is_valid_target_) {
                SetWindowText(this_->agree_link_,
                              this_->is_upgrade_
                                  ? this_->txt_tos_accept_update_str_.c_str()
                                  : this_->txt_tos_accept_install_str_.c_str());

                SetWindowText(this_->install_button_,
                              this_->is_upgrade_
                                  ? this_->btn_tos_accept_update_str_.c_str()
                                  : this_->btn_tos_accept_install_str_.c_str());
                this_->Layout();
              }
            }
          }
        } break;
      }
      break;

    case WM_DRAWITEM: {
      int controlId = wparam;

      if (controlId == IDC_STATIC_LOGO) {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lparam;

        // Because of differences in behavior between Windows versions we need
        // to thread carefully to preserve the alpha-channel.

        int width = this_->logo_bmp_->GetWidth();
        int height = this_->logo_bmp_->GetHeight();

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;  // Top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HBITMAP hBitmap =
            CreateDIBSection(dis->hDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        HDC hdcMem = CreateCompatibleDC(dis->hDC);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hBitmap);

        int x = 0;
        int y = 0;
        Gdiplus::Graphics graphics(hdcMem);
        graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
        graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        graphics.SetInterpolationMode(
            Gdiplus::InterpolationModeHighQualityBicubic);

        Gdiplus::Color background =
            this_->dark_mode_ ? Gdiplus::Color(TOP_BACKGROUND_COLOR_DARK)
                              : Gdiplus::Color(TOP_BACKGROUND_COLOR_LIGHT);

        Gdiplus::SolidBrush brush_tr(background);
        graphics.FillRectangle(&brush_tr, 0, 0, width, height);

        graphics.DrawImage(this_->logo_bmp_, 0, 0, width, height);

        BLENDFUNCTION blend = {AC_SRC_OVER, 0, 223, AC_SRC_ALPHA};
        AlphaBlend(dis->hDC, x, y, width, height, hdcMem, 0, 0, width, height,
                   blend);

        SelectObject(hdcMem, hbmOld);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        return true;
      } else if (controlId == IDC_STATIC_SLOGAN) {
        this_->DrawDDtext();
        return true;
      }

    } break;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      RECT dialogRect;
      GetClientRect(hdlg, &dialogRect);

      HDC hdc = BeginPaint(hdlg, &ps);
      HDC hdcMem = CreateCompatibleDC(hdc);
      HBITMAP hbmMem =
          CreateCompatibleBitmap(hdc, dialogRect.right, dialogRect.bottom);
      HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

      RECT bottomRect;
      bottomRect = dialogRect;

      // Let the bottom part behind the buttons be gray.

      bottomRect.top = dialogRect.bottom - this_->GetFooterbarHeight();

      FillRect(hdcMem, &dialogRect, this_->background_brush_.get());

      FillRect(hdcMem, &bottomRect, this_->buttondrop_brush_.get());

      // Draw the top separator line.
      base::win::ScopedSelectObject toplinePen(
          hdcMem, this_->is_dark_mode() ? this_->topLinePen_dark_
                                        : this_->topLinePen_light_);
      MoveToEx(hdcMem, 0, this_->GetTitlebarHeight(), NULL);
      LineTo(hdcMem, dialogRect.right, this_->GetTitlebarHeight());

      // Draw the bottom separator line.
      base::win::ScopedSelectObject bottomPen(
          hdcMem, this_->is_dark_mode() ? this_->bottomLinePen_dark_
                                        : this_->bottomLinePen_light_);
      MoveToEx(hdcMem, 0, bottomRect.top, NULL);
      LineTo(hdcMem, bottomRect.right, bottomRect.top);

      // Paint anything from the children here.
      for (auto* child_control : this_->child_controls_) {
        if (child_control->HasFocus()) {
          RECT focus_rect;
          HWND control_hwnd = GetDlgItem(hdlg, child_control->id());

          GetWindowRect(control_hwnd, &focus_rect);
          MapWindowPoints(nullptr, hdlg, (POINT*)&focus_rect, 2);

          if (child_control->id() == IDC_COMBO_LANGUAGE ||
              child_control->id() == IDC_COMBO_INSTALLTYPES) {
            InflateRect(&focus_rect, 4, 7);
          } else {
            InflateRect(&focus_rect, 3, 3);
          }

          VivaldiInstallDialog::DrawRoundRect(
              hdcMem, gfx::Rect(focus_rect),
              child_control->owner()->GetTextColor(), false, 3.f);
        }
      }

      BitBlt(hdc, 0, 0, dialogRect.right, dialogRect.bottom, hdcMem, 0, 0,
             SRCCOPY);

      SelectObject(hdcMem, hbmOld);
      DeleteObject(hbmMem);
      DeleteDC(hdcMem);

      EndPaint(hdlg, &ps);

      return 0;
    }

    case WM_DPICHANGED: {
      this_->current_dpi_ = GetCurrentDpi(this_->hdlg_, this_);

      this_->xscale_ = HIWORD(wparam) / (float)USER_DEFAULT_SCREEN_DPI;
      this_->yscale_ = LOWORD(wparam) / (float)USER_DEFAULT_SCREEN_DPI;

      RECT* suggested = (RECT*)lparam;
      SetWindowPos(this_->hdlg_, HWND_TOP, suggested->left, suggested->top,
                   suggested->right - suggested->left,
                   suggested->bottom - suggested->top,
                   SWP_NOACTIVATE | SWP_NOZORDER);

      this_->UpdateSize();

      InvalidateRect(this_->hdlg_, nullptr, FALSE);

      break;
    }
  }
  return false;
}

void VivaldiInstallDialog::DrawDDtext() {
  if (!pRenderTarget_) {
    return;
  }

  // Draw the slogan text that can include colored emojis.

  ID2D1SolidColorBrush* apBrush = NULL;

  D2D1::ColorF textcolor = GetTextColor();

  pRenderTarget_->CreateSolidColorBrush(textcolor, &apBrush);

  pRenderTarget_->BeginDraw();

  pRenderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());

  pRenderTarget_->Clear(GetTopBackgroundColorF());

  D2D1_SIZE_F renderTargetSize = pRenderTarget_->GetSize();
  D2D1_RECT_F textrect =
      D2D1::RectF(0, 0, renderTargetSize.width, renderTargetSize.height);

  pRenderTarget_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

  pRenderTarget_->DrawText(
      txt_slogan_str_.c_str(), wcslen(txt_slogan_str_.c_str()), pTextFormat_,
      textrect, apBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

  apBrush->Release();

  pRenderTarget_->EndDraw();
}

void GetComboButtonRect(HWND hwndCombo, RECT* rcButton) {
  RECT rcWindow;
  GetClientRect(hwndCombo, &rcWindow);

  int buttonWidth = GetSystemMetrics(SM_CXVSCROLL);
  rcButton->left = rcWindow.right - buttonWidth;
  rcButton->right = rcWindow.right;
  rcButton->top = 0;
  rcButton->bottom = rcWindow.bottom;
}

inline void GetRoundRectPath(Gdiplus::GraphicsPath* pPath,
                             Gdiplus::Rect r,
                             int dia) {
  // diameter can't exceed width or height
  if (dia > r.Width)
    dia = r.Width;
  if (dia > r.Height)
    dia = r.Height;

  // define a corner
  Gdiplus::Rect corner(r.X, r.Y, dia, dia);

  // begin path
  pPath->Reset();

  // top left
  pPath->AddArc(corner, 180, 90);

  // top right
  corner.X += (r.Width - dia - 1);
  pPath->AddArc(corner, 270, 90);

  // bottom right
  corner.Y += (r.Height - dia - 1);
  pPath->AddArc(corner, 0, 90);

  // bottom left
  corner.X -= (r.Width - dia - 1);
  pPath->AddArc(corner, 90, 90);

  // end path
  pPath->CloseFigure();
}

bool VivaldiInstallDialog::UpdateTargetPathResult(base::FilePath file_path) {
  bool is_target_valid = IsInstallPathValid(file_path);
  EnableWindow(GetDlgItem(hdlg_, IDOK), is_target_valid);

  TOOLINFO ti = {};
  ti.cbSize = sizeof(TOOLINFO);
  ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_CENTERTIP;

  if (!is_target_valid) {
    ti.hwnd = hdlg_;
    ti.uId = (UINT_PTR)destination_folder_edit_;
    ti.lpszText =
        const_cast<wchar_t*>(tooltip_warn_illegal_destination_str_.c_str());
    GetClientRect(destination_folder_edit_, &ti.rect);
    SendMessage(target_directory_tooltip_, TTM_ADDTOOL, 0, (LPARAM)&ti);
  }

  SendMessage(target_directory_tooltip_, TTM_TRACKACTIVATE, is_target_valid,
              (LPARAM)&ti);
  return is_target_valid;
}

inline void GetChildRect(HWND hChild, LPRECT rc) {
  GetWindowRect(hChild, rc);
  SIZE si = {rc->right - rc->left, rc->bottom - rc->top};
  ScreenToClient(GetParent(hChild), (LPPOINT)rc);
  rc->right = rc->left + si.cx;
  rc->bottom = rc->top + si.cy;
}

/*static*/
LRESULT CALLBACK
VivaldiInstallDialog::OwnerDrawComboboxProc(HWND hWnd,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            UINT_PTR uIdSubclass,
                                            DWORD_PTR dwRefData) {
  SubclassedControl* control = (SubclassedControl*)dwRefData;

  switch (uMsg) {
    case WM_ERASEBKGND:
      return 1;  // We do this in WM_PAINT.
    case WM_LBUTTONDOWN: {
      control->SetLeftButtonIsDown(true);
      control->SetHasFocus(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InvalidateRect(hWnd, &itemRect, FALSE);
    } break;

    case WM_LBUTTONUP: {
      control->SetLeftButtonIsDown(false);
      control->SetHasFocus(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InvalidateRect(hWnd, &itemRect, FALSE);
    } break;

    case WM_SETFOCUS: {
      control->SetHasFocus(true);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InflateRect(&itemRect, 10, 10);
      MapWindowPoints(hWnd, GetParent(hWnd), (POINT*)&itemRect, 2);
      InvalidateRect(GetParent(hWnd), &itemRect, FALSE);
    } break;

    case WM_KILLFOCUS: {
      control->SetHasFocus(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InflateRect(&itemRect, 10, 10);
      MapWindowPoints(hWnd, GetParent(hWnd), (POINT*)&itemRect, 2);
      InvalidateRect(GetParent(hWnd), &itemRect, FALSE);
    } break;

    case WM_MOUSEMOVE: {
      if (!control->IsHovered()) {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        control->SetIsHovered(TrackMouseEvent(&tme));
        RECT itemRect;
        GetClientRect(hWnd, &itemRect);
        InvalidateRect(hWnd, &itemRect, FALSE);
      }
    } break;

    case WM_MOUSELEAVE: {
      control->SetIsHovered(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InvalidateRect(hWnd, &itemRect, FALSE);
      return true;
    }

    case WM_NCCREATE: {
      DWORD style = GetWindowLong(hWnd, GWL_STYLE);
      if (style & WS_BORDER) {
        // WS_EX_CLIENTEDGE style will make the border 2 pixels thick...
        style = GetWindowLong(hWnd, GWL_EXSTYLE);
        if (!(style & WS_EX_CLIENTEDGE)) {
          style |= WS_EX_CLIENTEDGE;
          SetWindowLong(hWnd, GWL_EXSTYLE, style);
        }
      }
      // to draw on the parent DC, CLIPCHILDREN must be off
      /*HWND hParent = GetParent(hWnd);
      style = GetWindowLong(hParent, GWL_STYLE);
      if (style & WS_CLIPCHILDREN) {
        style &= ~WS_CLIPCHILDREN;
        SetWindowLong(hParent, GWL_STYLE, style);
      }*/
    } break;
    case WM_NCPAINT:
      if (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) {
        COLORREF rgba = 0xFFFFFFFF;
        int radius = 4;

        RECT rc = {0};
        GetChildRect(hWnd, &rc);
        // the normal EX_CLIENTEDGE is 2 pixels thick.
        // up to a radius of 5, this just works out.
        // for a larger radius, the rectangle must be inflated
        /*if (radius > 5) {
          int s = radius / 2 - 2;
          InflateRect(&rc, s, s);
        }*/
        Gdiplus::GraphicsPath path;
        GetRoundRectPath(&path,
                         Gdiplus::Rect(rc.left, rc.top, rc.right - rc.left,
                                       rc.bottom - rc.top),
                         radius * 2);

        HWND hParent = GetParent(hWnd);
        HDC hdc = GetDC(hParent);
        Gdiplus::Graphics graphics(hdc);

        BYTE* c = (BYTE*)&rgba;
        Gdiplus::Pen pen(Gdiplus::Color(c[0], c[1], c[2], c[3]));
        pen.SetAlignment(Gdiplus::PenAlignmentCenter);

        COLORREF edgecolor =
            this_->dark_mode_ ? TEXT_COLOR_DARK : TEXT_COLOR_LIGHT;

        Gdiplus::Color pluscolor(255, GetRValue(edgecolor),
                                 GetGValue(edgecolor), GetBValue(edgecolor));

        Gdiplus::SolidBrush brush(pluscolor);

        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.FillPath(&brush, &path);
        graphics.DrawPath(&pen, &path);

        ReleaseDC(hParent, hdc);

        return 0;
      }
      break;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
      if (control->owner()) {
        return (INT_PTR)control->owner()->OnCtlColorEdit((HWND)lParam,
                                                         (HDC)wParam);
      }
      break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
      if (control->owner())
        return (INT_PTR)control->owner()->OnCtlColor((HWND)lParam, (HDC)wParam);
      break;

    case WM_NCCALCSIZE: {
      if (wParam) {
        NCCALCSIZE_PARAMS* pParams = (NCCALCSIZE_PARAMS*)lParam;
        InflateRect(&pParams->rgrc[0], 0, COMBOBOX_HEIGHT_INFLATION);
        return FALSE;
      }
      break;
    }

    case WM_NCDESTROY:
      RemoveWindowSubclass(hWnd, VivaldiInstallDialog::OwnerDrawComboboxProc,
                           uIdSubclass);
      break;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      RECT clientRect;
      GetClientRect(hWnd, &clientRect);

      HDC hdcMem = CreateCompatibleDC(hdc);
      HBITMAP hbmMem =
          CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
      HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

      FillRect(hdcMem, &clientRect, this_->background_brush_.get());

      COLORREF buttoncolor = this_->GetButtonColor();
      COLORREF buttonbordercolor = this_->GetButtonBorderColor();
      if (control->IsHovered()) {
        COLOR16 r = GetRValue(buttoncolor);
        COLOR16 g = GetGValue(buttoncolor);
        COLOR16 b = GetBValue(buttoncolor);
        float brightnessFactor = 0.98f;
        r = fmin(255, r * brightnessFactor);
        g = fmin(255, g * brightnessFactor);
        b = fmin(255, b * brightnessFactor);
        buttoncolor = RGB(r, g, b);
      }

      if (control->IsLeftButtonDown()) {
        COLOR16 r = GetRValue(buttonbordercolor);
        COLOR16 g = GetGValue(buttonbordercolor);
        COLOR16 b = GetBValue(buttonbordercolor);
        float brightnessFactor = 1.1f;
        r = fmin(255, r * brightnessFactor);
        g = fmin(255, g * brightnessFactor);
        b = fmin(255, b * brightnessFactor);
        buttonbordercolor = RGB(r, g, b);
      }

      gfx::Rect comboRect(clientRect);

      // Draw a border and background drop.
      VivaldiInstallDialog::DrawRoundRect(hdcMem, comboRect, buttoncolor, true);
      VivaldiInstallDialog::DrawRoundRect(hdcMem, comboRect, buttonbordercolor,
                                          false);

      RECT rcButton;
      GetComboButtonRect(hWnd, &rcButton);

      Gdiplus::Graphics graphics(hdcMem);

      Gdiplus::Rect r(rcButton.left, rcButton.top,
                      rcButton.right - rcButton.left,
                      rcButton.bottom - rcButton.top);

      float cx = r.GetRight() - 26.0f;
      float cy = r.Y + r.Height / 2.0f;
      Gdiplus::Pen arrowPen(
          this_->dark_mode_ ? TEXT_COLOR_DARK_GDI : TEXT_COLOR_LIGHT_GDI, 1.5f);
      arrowPen.SetStartCap(Gdiplus::LineCapRound);
      arrowPen.SetEndCap(Gdiplus::LineCapRound);
      graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
      graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

      // Draw the combobox arrow.
      Gdiplus::PointF p1(cx - this_->GetPixelsFromDPI(3),
                         cy - this_->GetPixelsFromDPI(3));
      Gdiplus::PointF p2(cx, cy + this_->GetPixelsFromDPI(1));
      Gdiplus::PointF p3(cx + this_->GetPixelsFromDPI(3),
                         cy - this_->GetPixelsFromDPI(3));
      graphics.DrawLine(&arrowPen, p1, p2);
      graphics.DrawLine(&arrowPen, p2, p3);

      base::win::ScopedSelectObject buttonFont(hdcMem,
                                               control->owner()->dialog_font_);

      // Default text color.
      SetTextColor(hdcMem, this_->GetTextColor());

      std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
      if (buffer.get()) {
        GetWindowText(hWnd, buffer.get(), MAX_PATH - 1);
        SetBkMode(hdcMem, TRANSPARENT);

        clientRect.left += CHECKBOX_TEXT_PADDING;

        DrawText(hdcMem, buffer.get(), -1, &clientRect,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      }

      BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0,
             SRCCOPY);

      SelectObject(hdcMem, hbmOld);
      DeleteObject(hbmMem);
      DeleteDC(hdcMem);

      ReleaseDC(hWnd, hdc);
      EndPaint(hWnd, &ps);

      return 0;
    }
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/*static*/
LRESULT CALLBACK
VivaldiInstallDialog::OwnerDrawButtonProc(HWND hWnd,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          UINT_PTR uIdSubclass,
                                          DWORD_PTR dwRefData) {
  SubclassedControl* button = (SubclassedControl*)dwRefData;

  switch (uMsg) {
    case WM_ERASEBKGND:
      return 1;  // We do this in WM_PAINT.

    case WM_NCDESTROY:
      RemoveWindowSubclass(hWnd, VivaldiInstallDialog::OwnerDrawButtonProc,
                           uIdSubclass);
      break;
    case WM_LBUTTONDOWN: {
      button->SetLeftButtonIsDown(true);
      button->SetHasFocus(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InvalidateRect(hWnd, &itemRect, FALSE);
    } break;

    case WM_LBUTTONUP: {
      button->SetLeftButtonIsDown(false);
      button->SetHasFocus(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InvalidateRect(hWnd, &itemRect, FALSE);
    } break;

    case WM_SETFOCUS: {
      button->SetHasFocus(true);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InflateRect(&itemRect, 5, 7);
      MapWindowPoints(hWnd, GetParent(hWnd), (POINT*)&itemRect, 2);
      InvalidateRect(GetParent(hWnd), &itemRect, FALSE);
    } break;

    case WM_KILLFOCUS: {
      button->SetHasFocus(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InflateRect(&itemRect, 5, 7);
      MapWindowPoints(hWnd, GetParent(hWnd), (POINT*)&itemRect, 2);
      InvalidateRect(GetParent(hWnd), &itemRect, FALSE);
    } break;

    case WM_MOUSEMOVE: {
      if (!button->IsHovered()) {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        button->SetIsHovered(TrackMouseEvent(&tme));
        RECT itemRect;
        GetClientRect(hWnd, &itemRect);
        InvalidateRect(hWnd, &itemRect, FALSE);
      }
    } break;

    case WM_MOUSELEAVE: {
      button->SetIsHovered(false);
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);
      InvalidateRect(hWnd, &itemRect, FALSE);
      return true;
    }

    case WM_CTLCOLORSCROLLBAR: {
      HDC hdc = (HDC)wParam;
      // editbox coloring.
      SetTextColor(hdc, this_->dark_mode_ ? TEXT_COLOR_DARK : TEXT_COLOR_LIGHT);
      SetBkColor(hdc, this_->dark_mode_ ? TOP_BACKGROUND_COLOR_DARK
                                        : TOP_BACKGROUND_COLOR_LIGHT);
      return (LRESULT)button->owner()->background_brush_.get();
    }

    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);

      // Only copy the painted parts from the memhdc.
      RECT clipRect = ps.rcPaint;

      // Paint the whole control to avoid shearing.
      RECT itemRect;
      GetClientRect(hWnd, &itemRect);

      // Only paint within the clipped region.
      RECT paintRect = clipRect;
      IntersectRect(&paintRect, &paintRect, &itemRect);

      // Doublebuffer painting.
      HDC hdcMem = CreateCompatibleDC(hdc);
      HBITMAP hbmMem =
          CreateCompatibleBitmap(hdc, itemRect.right, itemRect.bottom);
      HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

      HRGN hrgn = CreateRectRgnIndirect(&ps.rcPaint);
      SelectClipRgn(hdcMem, hrgn);

      FillRect(hdcMem, &itemRect, this_->background_brush_.get());

      base::win::ScopedSelectObject buttonFont(hdcMem,
                                               button->owner()->dialog_font_);

      // Default text color.
      SetTextColor(hdcMem, this_->GetTextColor());

      // Checkboxes.
      if (button->id() == IDC_CHECK_REGISTER ||
          button->id() == IDC_CHECK_NO_AUTOUPDATE ||
          button->id() == IDC_CHECK_ALLOW_CRASHLOGS) {
        RECT rc = itemRect;

        Gdiplus::Graphics g(hdcMem);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        // Checkbox square (height of control text baseline)
        int size = std::min(rc.bottom - rc.top, rc.right - rc.left);
        int boxSize = size - 4;
        int x = rc.left + 2;
        int y = rc.top + (size - boxSize) / 2;

        Gdiplus::RectF boxRect((Gdiplus::REAL)x, (Gdiplus::REAL)y,
                               (Gdiplus::REAL)boxSize, (Gdiplus::REAL)boxSize);
        const float radius = 4.0f;

        bool is_checked = (button->id() == IDC_CHECK_REGISTER &&
                           button->owner()->options_.register_browser) ||
                          (button->id() == IDC_CHECK_NO_AUTOUPDATE &&
                           button->owner()->disable_standalone_autoupdates_) ||
                          (button->id() == IDC_CHECK_ALLOW_CRASHLOGS &&
                           button->owner()->options_.allow_crashlog_uploads);

        Gdiplus::Color gdipAccentColor;
        gdipAccentColor.SetFromCOLORREF(this_->accent_color_);

        // ----- Background Fill -----
        Gdiplus::Color fill = this_->GetCheckboxBackgroundColor();
        if (is_checked) {
          fill = gdipAccentColor;
        } else if (button->IsHovered() && !is_checked) {
          fill = this_->GetCheckboxBackgroundHoverColor();
        }

        // Clear the canvas.
        FillRect(hdcMem, &rc, this_->background_brush_.get());

        Gdiplus::SolidBrush fillBrush(fill);

        // Rounded background
        Gdiplus::GraphicsPath path;
        path.AddArc(boxRect.X, boxRect.Y, radius * 2, radius * 2, 180, 90);
        path.AddArc(boxRect.GetRight() - radius * 2, boxRect.Y, radius * 2,
                    radius * 2, 270, 90);
        path.AddArc(boxRect.GetRight() - radius * 2,
                    boxRect.GetBottom() - radius * 2, radius * 2, radius * 2, 0,
                    90);
        path.AddArc(boxRect.X, boxRect.GetBottom() - radius * 2, radius * 2,
                    radius * 2, 90, 90);
        path.CloseFigure();

        g.FillPath(&fillBrush, &path);

        Gdiplus::Color gdipBorderColor;
        gdipBorderColor.SetFromCOLORREF(this_->GetTextColor());
        Gdiplus::Color border = is_checked ? gdipAccentColor : gdipBorderColor;

        Gdiplus::Pen pen(border, 1.5f);
        g.DrawPath(&pen, &path);

        if (is_checked) {
          // Fluent style tick: long and short strokes
          Gdiplus::PointF p1(x + boxSize * 0.26f, y + boxSize * 0.54f);
          Gdiplus::PointF p2(x + boxSize * 0.43f, y + boxSize * 0.70f);
          Gdiplus::PointF p3(x + boxSize * 0.74f, y + boxSize * 0.30f);

          Gdiplus::Color tickColor = Gdiplus::Color::White;
          Gdiplus::Pen tick(tickColor, 2.4f);
          tick.SetStartCap(Gdiplus::LineCapRound);
          tick.SetEndCap(Gdiplus::LineCapRound);

          g.DrawLine(&tick, p1, p2);
          g.DrawLine(&tick, p2, p3);
        }

        std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
        if (buffer.get()) {
          RECT checkboxrect(itemRect);
          GetWindowText(hWnd, buffer.get(), MAX_PATH - 1);
          SetBkMode(hdcMem, TRANSPARENT);
          checkboxrect.left += boxSize + CHECKBOX_TEXT_PADDING;
          checkboxrect.right += boxSize + CHECKBOX_TEXT_PADDING;
          DrawText(hdcMem, buffer.get(), -1, &checkboxrect,
                   DT_VCENTER | DT_SINGLELINE);
        }

      } else if (button->id() == IDOK || button->id() == IDCANCEL ||
                 button->id() == IDC_BTN_MODE ||
                 button->id() == IDC_BTN_BROWSE) {
        bool disabled = !IsWindowEnabled(hWnd);

        // Fill the background with the correct color.
        if (button->id() == IDC_BTN_BROWSE) {
          FillRect(hdcMem, &itemRect, this_->background_brush_.get());
        } else {
          FillRect(hdcMem, &itemRect, this_->buttondrop_brush_.get());
        }

        gfx::Rect buttonRect(itemRect);

        base::win::ScopedSelectObject bottomPen(
            hdcMem, button->owner()->is_dark_mode()
                        ? this_->bottomLinePen_dark_
                        : this_->bottomLinePen_light_);

        COLORREF buttoncolor = this_->accent_color_;
        COLORREF bordercolor = this_->GetButtonBorderColor();
        bool draw_button_border = false;
        // default action
        if (button->id() == IDOK) {
          buttoncolor = this_->accent_color_;
          SetTextColor(hdcMem, RGB(255, 255, 255));
        } else {
          // All other buttons.
          draw_button_border = true;
          buttoncolor = this_->GetButtonColor();
          SetTextColor(hdcMem, this_->GetTextColor());
        }
        if (button->IsHovered()) {
          // decrease brightness 10% on hover, click add 40%
          COLOR16 r = GetRValue(buttoncolor);
          COLOR16 g = GetGValue(buttoncolor);
          COLOR16 b = GetBValue(buttoncolor);

          float brightnessFactor = button->IsLeftButtonDown() ? 1.4f : 0.9f;

          r = fmin(255, r * brightnessFactor);
          g = fmin(255, g * brightnessFactor);
          b = fmin(255, b * brightnessFactor);

          buttoncolor = RGB(r, g, b);
        }

        if (disabled) {
          // decrease brightness 50% on disabled mode.
          COLOR16 r = GetRValue(buttoncolor);
          COLOR16 g = GetGValue(buttoncolor);
          COLOR16 b = GetBValue(buttoncolor);

          float brightnessFactor = 0.5f;

          r = fmin(255, r * brightnessFactor);
          g = fmin(255, g * brightnessFactor);
          b = fmin(255, b * brightnessFactor);

          buttoncolor = RGB(r, g, b);
          // Hardcoded should be scaled.
          SetTextColor(hdcMem, RGB(100, 100, 100));
        }

        VivaldiInstallDialog::DrawRoundRect(hdcMem, gfx::Rect(itemRect),
                                            buttoncolor, true);
        if (draw_button_border) {
          VivaldiInstallDialog::DrawRoundRect(hdcMem, gfx::Rect(itemRect),
                                              bordercolor, false);
        }

        std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
        if (buffer.get()) {
          GetWindowText(hWnd, buffer.get(), MAX_PATH - 1);
          SetBkMode(hdcMem, TRANSPARENT);

          DrawText(hdcMem, buffer.get(), -1, &itemRect,
                   DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

      } else if (button->id() == IDC_BTN_CLOSE) {
        COLORREF crosscolor = this_->GetTextColor();

        if (button->IsHovered()) {
          crosscolor = RGB(245, 245, 245);
          base::win::ScopedGDIObject<HBRUSH> background_brush(
              CreateSolidBrush(RGB(186, 42, 28)));
          FillRect(hdcMem, &itemRect, background_brush.get());
        } else {
          COLORREF backgroundcolor = this_->GetTopBackgroundColor();
          base::win::ScopedGDIObject<HBRUSH> background_brush(
              CreateSolidBrush(backgroundcolor));
          FillRect(hdcMem, &itemRect, background_brush.get());
        }

        // Draw the "X" like winui2+
        int width = itemRect.right - itemRect.left;
        int height = itemRect.bottom - itemRect.top;
        int xfactor = width / 2.5;
        int yfactor = height / 2.5;

        HPEN pen = CreatePen(PS_SOLID, 2, crosscolor);
        HGDIOBJ oldPen = SelectObject(hdcMem, pen);
        MoveToEx(hdcMem, itemRect.left + xfactor, itemRect.top + yfactor,
                 nullptr);
        LineTo(hdcMem, itemRect.right - xfactor, itemRect.bottom - yfactor);
        MoveToEx(hdcMem, itemRect.right - xfactor, itemRect.top + yfactor,
                 nullptr);
        LineTo(hdcMem, itemRect.left + xfactor, itemRect.bottom - yfactor);
        SelectObject(hdcMem, oldPen);
        DeleteObject(pen);
      }

      RECT focus_rect;
      GetClientRect(hWnd, &focus_rect);
      // Calculate to dialog coordinates.
      GetChildRect(hWnd, &focus_rect);

      BitBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left,
             clipRect.bottom - clipRect.top, hdcMem, clipRect.left,
             clipRect.top, SRCCOPY);

      /*BitBlt(hdc, 0, 0, itemRect.right, itemRect.bottom, hdcMem, 0, 0,
             SRCCOPY);*/

      SelectObject(hdcMem, hbmOld);
      DeleteObject(hbmMem);
      DeleteDC(hdcMem);
      DeleteObject(hrgn);

      EndPaint(hWnd, &ps);
      return 0;
    }
  }
  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/*static*/
LRESULT CALLBACK VivaldiInstallDialog::OwnerDrawEditProc(HWND hWnd,
                                                         UINT uMsg,
                                                         WPARAM wParam,
                                                         LPARAM lParam,
                                                         UINT_PTR uIdSubclass,
                                                         DWORD_PTR dwRefData) {
  SubclassedControl* control = (SubclassedControl*)dwRefData;

  switch (uMsg) {
    case WM_ERASEBKGND:
      return 1;  // We do this in WM_PAINT.

    case WM_NCDESTROY:
      RemoveWindowSubclass(hWnd, VivaldiInstallDialog::OwnerDrawEditProc,
                           uIdSubclass);
      break;
    case WM_NCCALCSIZE: {
      if (wParam == TRUE) {
        NCCALCSIZE_PARAMS* nc = (NCCALCSIZE_PARAMS*)lParam;
        InflateRect(&nc->rgrc[0], this_->GetPixelsFromDPI(-4),
                    this_->GetPixelsFromDPI(-12));
      }
      return 0;
    }
    case WM_NCPAINT: {
      HDC hdc = GetWindowDC(hWnd);
      if (hdc) {
        RECT rc;
        GetWindowRect(hWnd, &rc);

        OffsetRect(&rc, -rc.left, -rc.top);  // convert to window coords
        gfx::Rect rect(rc);

        COLORREF border_color = control->owner()->GetButtonBorderColor();

        VivaldiInstallDialog::DrawRoundRect(hdc, rect, border_color, false);

        ReleaseDC(hWnd, hdc);
      }
      return 0;
    }
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/* static*/ void VivaldiInstallDialog::DrawRoundRect(
    HDC hdc,
    gfx::Rect rect,
    COLORREF color,
    bool fill,
    float borderWidth /*= 1.f*/) {
  Gdiplus::Graphics graphics(hdc);
  graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
  graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

  Gdiplus::REAL cornerRadius = this_->GetPixelsFromDPI(ROUND_RECT_RADIUS);

  Gdiplus::GraphicsPath path;
  GetRoundRectPath(
      &path, Gdiplus::Rect(rect.x(), rect.y(), rect.width(), rect.height()),
      cornerRadius * 2);

  Gdiplus::Color pluscolor(255, GetRValue(color), GetGValue(color),
                           GetBValue(color));

  if (fill) {
    Gdiplus::SolidBrush brush(pluscolor);
    graphics.FillPath(&brush, &path);
  } else {
    Gdiplus::Pen pen(pluscolor, borderWidth);
    graphics.DrawPath(&pen, &path);
  }
}

VivaldiInstallUIOptions VivaldiInstallDialog::ExtractOptions() {
  if (advanced_mode_) {
    return std::move(options_);
  } else {
    // VB-116554 : Per-user installation type, with the auto-detected
    // language, at the default location, and with auto-update and
    // file/protocol associations enabled. You know, what you’ll get if
    // you never go into advanced.
    VivaldiInstallUIOptions simple_options;

    // default is kForCurrentUser
    base::FilePath path =
        vivaldi::GetDefaultInstallTopDir(simple_options.install_type);
    if (!path.empty()) {
      simple_options.install_dir = std::move(path);
    }

    simple_options.allow_crashlog_uploads = options_.allow_crashlog_uploads;

    return simple_options;
  }
}

}  // namespace installer
