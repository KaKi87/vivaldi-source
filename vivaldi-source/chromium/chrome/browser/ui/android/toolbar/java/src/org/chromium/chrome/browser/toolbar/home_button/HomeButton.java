// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.home_button;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.base.TraceEvent;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.ui.listmenu.ListMenuButton;

// Vivaldi
import android.content.res.ColorStateList;
import androidx.core.widget.ImageViewCompat;
import org.chromium.chrome.browser.theme.ThemeColorProvider;

/** The home button. */
@NullMarked
public class HomeButton extends ListMenuButton implements ThemeColorProvider.TintObserver { // Vivaldi
    private boolean mIsInitialized;
    private int mVisibility;
    private boolean mHasSpaceToShow;

    /** A provider that notifies components when the theme color changes.*/  // Vivaldi
    private @Nullable ThemeColorProvider mThemeColorProvider; // Vivaldi

    public HomeButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        mHasSpaceToShow = true;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        try (TraceEvent e = TraceEvent.scoped("HomeButton.onMeasure")) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        try (TraceEvent e = TraceEvent.scoped("HomeButton.onLayout")) {
            super.onLayout(changed, left, top, right, bottom);
        }
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mIsInitialized = true;
        mVisibility = getVisibility();
        // Call with cached value in case it was set before the view was inflated.
        setHasSpaceToShow(mHasSpaceToShow);
    }

    @Override
    public void setVisibility(int visibility) {
        mVisibility = visibility;
        super.setVisibility(mHasSpaceToShow ? mVisibility : GONE);
    }

    /**
     * Sets whether there is enough space for the button to be shown.
     *
     * @param hasSpaceToShow indicates whether the button view has space to show.
     */
    public void setHasSpaceToShow(boolean hasSpaceToShow) {
        mHasSpaceToShow = hasSpaceToShow;
        // This may be called before the view is initialized. If so, hold off until the view is
        // inflated.
        if (mIsInitialized) {
            setVisibility(mVisibility);
        }
    }

    // Vivaldi
    @Override
    public void onTintChanged(@Nullable ColorStateList tint,
            @Nullable ColorStateList activityFocusTint, int brandedColorScheme) {
        ImageViewCompat.setImageTintList(this, tint); // Ref. VAB-7901
    }

    public void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeTintObserver(this);
            mThemeColorProvider = null;
        }
    }

    public void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addTintObserver(this);
        ImageViewCompat.setImageTintList(this, mThemeColorProvider.getTint());
    }
    // End Vivaldi
}
