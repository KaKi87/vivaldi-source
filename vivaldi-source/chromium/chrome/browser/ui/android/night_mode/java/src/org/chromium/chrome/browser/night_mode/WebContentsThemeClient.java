// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.night_mode;

import android.content.Context;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.util.ColorUtils;

// Vivaldi
import org.chromium.build.BuildConfig;
import org.chromium.components.browser_ui.site_settings.ContentSettingException;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridge;
import org.chromium.components.content_settings.ContentSetting;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.url.GURL;

import org.vivaldi.browser.preferences.VivaldiPreferences;

import java.net.URL;
import java.util.List;

/** A client to provide dark mode information for a given WebContents. */
@NullMarked
@JNINamespace("night_mode")
public class WebContentsThemeClient {
    @CalledByNative
    public static boolean isNightModeEnabled(
            @JniType("content::WebContents*") WebContents webContents) {
        if (webContents == null) return false;
        WindowAndroid window = webContents.getTopLevelNativeWindow();
        if (window == null) return false;
        Context context = window.getContext().get();
        if (context == null) return false;
        return ColorUtils.inNightMode(context);
    }

    @CalledByNative
    public static boolean isForceDarkWebContentEnabled(
            @JniType("content::WebContents*") WebContents webContents) {
        if (webContents == null) return false;
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.FORCE_WEB_CONTENTS_DARK_MODE)) {
            return true;
        }
        if (!ChromeFeatureList.isEnabled(
                ChromeFeatureList.DARKEN_WEBSITES_CHECKBOX_IN_THEMES_SETTING)) {
            return false;
        }
        Profile profile = Profile.fromWebContents(webContents);
        if (profile == null) return false;

        // Vivaldi
        if (BuildConfig.IS_VIVALDI) {
            return getWebsiteAutoDarkStatus(profile, webContents.getVisibleUrl());
        } // End Vivaldi

        return isNightModeEnabled(webContents)
                && WebContentsDarkModeController.isEnabledForUrl(
                        profile, webContents.getVisibleUrl());
    }

    /** Vivaldi: Returns the current website status (Light/Dark) for the current url. */
    private static boolean getWebsiteAutoDarkStatus(BrowserContextHandle browserContextHandle, GURL url) {
        List<ContentSettingException> exceptions = new WebsitePreferenceBridge().getContentSettingsExceptions(
                browserContextHandle, ContentSettingsType.AUTO_DARK_WEB_CONTENT);
        @ThemeType
        int websiteTheme = VivaldiPreferences.getSharedPreferencesManager().readInt(
                VivaldiPreferences.PREF_WEBSITE_THEME_APPEARANCE, ThemeType.SYSTEM_DEFAULT);
        for (ContentSettingException exception : exceptions) {
            try {
                String siteExceptionHost = new URL (exception.getPrimaryPattern()).getHost();
                String currentUrlHost = new URL(url.getSpec()).getHost();
                // Check if the user has set an exception for the website. If so, return the value
                // chosen by the user. Otherwise, return the default value for AutoDark Website
                // setting
                if (currentUrlHost.equals(siteExceptionHost)) {
                    return exception.getContentSetting() == ContentSetting.ALLOW;
                }
            } catch (Exception e) {
                return websiteTheme == ThemeType.DARK;
            }
        }
        return websiteTheme == ThemeType.DARK;
    }
}
