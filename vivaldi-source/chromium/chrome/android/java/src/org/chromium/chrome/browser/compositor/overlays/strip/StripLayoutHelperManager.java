// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.overlays.strip;

import static org.chromium.build.NullUtil.assertNonNull;
import static org.chromium.build.NullUtil.assumeNonNull;
import static org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutUtils.BUTTON_TOUCH_TARGET_SIZE_DP;
import static org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutUtils.MIN_TAB_WIDTH_DP;
import static org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutUtils.TAB_OVERLAP_WIDTH_DP;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Handler;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.FloatProperty;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnDragListener;
import android.view.ViewStub;
import android.view.animation.Interpolator;

import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.Px;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.build.annotations.EnsuresNonNullIf;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.cc.input.BrowserControlsState;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.browser_controls.BrowserControlsOffsetTagsInfo;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserControlsUtils;
import org.chromium.chrome.browser.compositor.LayerTitleCache;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerHost;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.compositor.layouts.LayoutRenderHost;
import org.chromium.chrome.browser.compositor.layouts.LayoutUpdateHost;
import org.chromium.chrome.browser.compositor.layouts.components.CompositorButton;
import org.chromium.chrome.browser.compositor.layouts.components.CompositorButton.ButtonType;
import org.chromium.chrome.browser.compositor.layouts.components.TintedCompositorButton;
import org.chromium.chrome.browser.compositor.layouts.components.TintedCompositorTextButton;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.AreaMotionEventFilter;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.AreaMotionEventHandler;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutView.StripLayoutViewOnClickHandler;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutView.StripLayoutViewOnKeyboardFocusHandler;
import org.chromium.chrome.browser.compositor.overlays.strip.reorder.TabStripDragHandler;
import org.chromium.chrome.browser.compositor.scene_layer.TabStripSceneLayer;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.layouts.EventFilter;
import org.chromium.chrome.browser.layouts.LayoutStateProvider.LayoutStateObserver;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.layouts.SceneOverlay;
import org.chromium.chrome.browser.layouts.animation.CompositorAnimator;
import org.chromium.chrome.browser.layouts.components.VirtualView;
import org.chromium.chrome.browser.layouts.scene_layer.SceneOverlayLayer;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.lifecycle.PauseResumeWithNativeObserver;
import org.chromium.chrome.browser.lifecycle.TopResumedActivityChangedObserver;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.MediaState;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.Tab.LoadUrlResult;
import org.chromium.chrome.browser.tab.TabClosingSource;
import org.chromium.chrome.browser.tab.TabCreationState;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tab_ui.ActionConfirmationManager;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.tabstrip.StripVisibilityState;
import org.chromium.chrome.browser.tabstrip.TabStripSceneLayerHolder;
import org.chromium.chrome.browser.tasks.tab_management.TabGroupListBottomSheetCoordinator;
import org.chromium.chrome.browser.tasks.tab_management.TabUiThemeUtil;
import org.chromium.chrome.browser.toolbar.ToolbarFeatures;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.ui.desktop_windowing.AppHeaderUtils;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.desktop_windowing.AppHeaderState;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager.AppHeaderObserver;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.components.browser_ui.widget.gesture.BackPressHandler;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.LocalizationUtils;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayUtil;
import org.chromium.ui.dragdrop.DragAndDropDelegate;
import org.chromium.ui.dragdrop.DragDropGlobalState;
import org.chromium.ui.interpolators.Interpolators;
import org.chromium.ui.resources.ResourceManager;
import org.chromium.ui.util.ColorUtils;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

//Vivaldi
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ChromeApplicationImpl;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.overlay_panel.OverlayPanel;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabCreatorUtil;
import org.chromium.chrome.browser.tabmodel.TabGroupModelFilter;
import org.chromium.chrome.browser.toolbar.top.TabSwitcherActionMenuCoordinator;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.ui.listmenu.ListMenuButton;
import org.vivaldi.browser.common.VivaldiUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.build.BuildConfig;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.vivaldi.browser.preferences.VivaldiPreferences;

import android.view.Gravity;
import android.widget.FrameLayout;

/**
 * This class handles managing which StripLayoutHelper is currently active and dispatches all input
 * and model events to the proper destination.
 */
@NullMarked
public class StripLayoutHelperManager
        implements SceneOverlay,
                PauseResumeWithNativeObserver,
                TabStripSceneLayerHolder,
                TopResumedActivityChangedObserver,
                AppHeaderObserver,
                OnSharedPreferenceChangeListener {
    /**
     * POD type that contains the necessary tab model info on startup. Used in the startup flicker
     * fix experiment where we create a placeholder tab strip on startup to mitigate jank as tabs
     * are rapidly restored (perceived as a flicker/tab strip scroll).
     */
    public static class TabModelStartupInfo {
        public final int standardCount;
        public final int incognitoCount;
        public final int standardActiveIndex;
        public final int incognitoActiveIndex;
        public final boolean createdStandardTabOnStartup;
        public final boolean createdIncognitoTabOnStartup;

        public TabModelStartupInfo(
                int standardCount,
                int incognitoCount,
                int standardActiveIndex,
                int incognitoActiveIndex,
                boolean createdStandardTabOnStartup,
                boolean createdIncognitoTabOnStartup) {
            this.standardCount = standardCount;
            this.incognitoCount = incognitoCount;
            this.standardActiveIndex = standardActiveIndex;
            this.incognitoActiveIndex = incognitoActiveIndex;
            this.createdStandardTabOnStartup = createdStandardTabOnStartup;
            this.createdIncognitoTabOnStartup = createdIncognitoTabOnStartup;
        }
    }

    private static final FloatProperty<StripLayoutHelperManager> SCRIM_OPACITY =
            new FloatProperty<>("scrimOpacity") {
                @Override
                public void setValue(StripLayoutHelperManager object, float value) {
                    object.mStripTransitionScrimOpacity = value;
                }

                @Override
                public Float get(StripLayoutHelperManager object) {
                    return object.mStripTransitionScrimOpacity;
                }
            };

    // Shared button constants (Model selector and Glic).
    private static final float BUTTON_DESIRED_TOUCH_TARGET_SIZE =
            StripLayoutUtils.shouldApplyMoreDensity() ? 32.f : 48.f;

    // Model selector button constants.
    private static float MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP = 3.f; // Vivaldi: not final.
    private static final float MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP = 24.f; // Vivaldi
    private static final float MODEL_SELECTOR_BUTTON_BACKGROUND_HEIGHT_DP = 24.f; // Vivaldi
    private static final float MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY = 0.12f;
    private static final float MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY = 0.08f;
    private static final float MODEL_SELECTOR_BUTTON_CLICK_SLOP_DP =
            (BUTTON_DESIRED_TOUCH_TARGET_SIZE - MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP) / 2;

    // Glic button constants.
    private static final float GLIC_BUTTON_BACKGROUND_Y_OFFSET_DP = 5.f;
    private static final float GLIC_BUTTON_BACKGROUND_WIDTH_DP = 28.f;
    private static final float GLIC_BUTTON_BACKGROUND_HEIGHT_DP = 28.f;
    private static final float GLIC_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY = 0.30f;
    private static final float GLIC_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY = 0.20f;
    private static final float GLIC_BUTTON_UNFOCUSED_OPACITY = 0.65f;
    private static final float GLIC_BUTTON_CLICK_SLOP_DP =
            (BUTTON_DESIRED_TOUCH_TARGET_SIZE - GLIC_BUTTON_BACKGROUND_WIDTH_DP) / 2;
    // Total vertical margin (Tab Strip Height(40dp) - Glic Background Height(28dp) = 12dp).
    public static final float GLIC_BUTTON_MARGIN_HEIGHT_DP = 12.f;
    private static final float GLIC_BUTTON_START_PADDING_DP = 6.f;
    private static final float GLIC_ICON_WIDTH_DP = 16.f;
    private static final float GLIC_ICON_TEXT_PADDING_DP = 4.f;
    private static final float GLIC_BUTTON_END_PADDING_DP = 10.f;
    private static final float GLIC_BUTTON_CORNER_RADIUS = 12.f;
    // 2dp accounts for the smaller Glic background and aligns it with the toolbar buttons
    private static final float GLIC_ALIGNMENT_OFFSET_DP =
            (MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP - GLIC_BUTTON_BACKGROUND_WIDTH_DP) / 2;

    // Tab strip transition constants.
    @VisibleForTesting
    static final Interpolator TAB_STRIP_TRANSITION_INTERPOLATOR =
            Interpolators.STANDARD_DEFAULT_EFFECTS;

    // Fade constants.
    static final float FADE_SHORT_WIDTH_DP = 60;
    static final float FADE_MEDIUM_WIDTH_DP = 72;
    static final float FADE_LONG_WIDTH_DP = 136;

    // Caching Variables
    private final RectF mStripFilterArea = new RectF();
    private final boolean mIsHeaderCustomizationSupported;

    // External influences
    private @MonotonicNonNull TabModelSelector mTabModelSelector; // Set on native initialization.
    private final LayoutManagerHost mManagerHost;
    private final LayoutUpdateHost mUpdateHost;
    private final LayoutRenderHost mRenderHost;
    private @Nullable ResourceManager mResourceManager;

    // Event Filters
    private @Nullable AreaMotionEventFilter mEventFilter;

    // Internal state
    private boolean mIsIncognito;
    private final StripLayoutHelper mNormalHelper;
    private final StripLayoutHelper mIncognitoHelper;

    // UI State
    private float mWidth; // in dp units
    private float mHeight; // Height of the entire tab strip compositor layer in DP.
    private final float mScrollableStripHeight; // Height of the scrollable tab strip layer in DP.

    // Padding regions that tabs should remain untouchable.
    private float mLeftPadding; // in dp units
    private float mRightPadding; // in dp units
    private float mTopPadding; // in dp units
    private final float mDensity;
    private int mOrientation;
    private final Runnable mGlicClickHandler;
    private @Nullable TintedCompositorTextButton mGlicButton;
    private @Nullable TintedCompositorButton mModelSelectorButton;
    private final Context mContext;
    private float mStripTransitionScrimOpacity;
    private @Nullable Animator mFadeTransitionAnimator;
    // This will be set only when a strip height transition runs to update the strip visibility and
    // not when this transition runs to solely update the strip top padding.
    private boolean mIsHeightTransitioning;
    private final ToolbarManager mToolbarManager;
    private final StatusBarColorController mStatusBarColorController;
    private TabStripSceneLayer mTabStripTreeProvider;
    private TabStripEventHandler mTabStripEventHandler;
    private final TabSwitcherLayoutObserver mTabSwitcherLayoutObserver;
    private final View mToolbarControlContainer;
    private final ViewStub mTabHoverCardViewStub;
    private float mLastVisibleViewportOffsetY;
    private float mSceneLayerYOffset;
    private float mSceneLayerVisibleHeight; // Used during height transition.

    /**
     * Whether the current activity is the top resumed activity. This is only relevant for use in
     * the desktop windowing mode, to determine the tab strip background color and the Glic button
     * opacity.
     */
    private boolean mIsTopResumedActivity;

    private final SettableNonNullObservableSupplier<Boolean> mStaticLayoutNeedsOffsetTagSupplier =
            ObservableSuppliers.createNonNull(false);

    private final @Nullable DesktopWindowStateManager mDesktopWindowStateManager;

    // 3-dots menu button with tab strip end padding
    private final float mStripEndPadding;
    private @MonotonicNonNull TabModelSelectorTabModelObserver mTabModelSelectorTabModelObserver;
    private @MonotonicNonNull TabModelSelectorTabObserver mTabModelSelectorTabObserver;
    private final Callback<TabModel> mCurrentTabModelObserver =
            (tabModel) -> {
                tabModelSwitched(tabModel.isIncognito());
            };

    private @MonotonicNonNull TabModelObserver mTabModelObserver; // Set on native initialization.
    private final ActivityLifecycleDispatcher mLifecycleDispatcher;
    private final String mDefaultTitle;
    private final SettableMonotonicObservableSupplier<LayerTitleCache> mLayerTitleCacheSupplier; // Vivaldi
    private final BrowserControlsStateProvider mBrowserControlsStateProvider;
    private final Callback<Integer> mStripVisibilityStateObserver;
    private final SettableNonNullObservableSupplier<@StripVisibilityState Integer>
            mStripVisibilityStateSupplier =
                    ObservableSuppliers.createNonNull(StripVisibilityState.VISIBLE);
    private final SettableNonNullObservableSupplier<Integer> mStripBottomPxSupplier =
            ObservableSuppliers.createNonNull(0);
    private final @Nullable NonNullObservableSupplier<Boolean> mXrSpaceModeObservableSupplier;

    // Vivaldi
    private final ChromeTabbedActivity mActivity;
    private float mViewportHeightOffset;
    private final boolean mShouldHideOverlay;
    private boolean mIsStackStrip;
    private @Nullable NonNullObservableSupplier<Boolean> mVivaldiMiniBarActiveSupplier; // VAB-12859

    // Drag-Drop
    private @Nullable TabStripDragHandler mTabStripDragHandler;

    private class TabStripEventHandler implements AreaMotionEventHandler {
        @Override
        public void onDown(float x, float y, int buttons) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }
            if (mGlicButton != null && mGlicButton.onDown(x, y, buttons)) {
                return;
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            if (mModelSelectorButton != null && mModelSelectorButton.onDown(x, y, buttons)) {
                return;
            }
            getActiveStripLayoutHelper().onDown(x, y, buttons);
        }

        @Override
        public void onUpOrCancel() {
            if (mGlicButton != null && mGlicButton.isVisible() && mGlicButton.onUpOrCancel()) {
                handleGlicButtonClick();
                return;
            }

            if (ChromeApplicationImpl.isVivaldi())
                if (mModelSelectorButton != null
                        && mModelSelectorButton.onUpOrCancel()) return;
            else
            if (mModelSelectorButton != null
                    && mModelSelectorButton.onUpOrCancel()
                    && mTabModelSelector != null) {
                getActiveStripLayoutHelper().finishAnimationsAndPushTabUpdates();
                if (!mModelSelectorButton.isVisible()) return;
                mTabModelSelector.selectModel(!mTabModelSelector.isIncognitoSelected());
                return;
            }
            getActiveStripLayoutHelper().onUpOrCancel();
        }

        @Override
        public void drag(float x, float y, float dx, float dy, float tx, float ty) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }
            if (mGlicButton != null) {
                mGlicButton.drag(x, y);
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            if (mModelSelectorButton != null) {
                mModelSelectorButton.drag(x, y);
            }
            getActiveStripLayoutHelper().drag(x, y, dx);
        }

        @Override
        public void click(float x, float y, int buttons, int modifiers) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            long time = time();
            if (mGlicButton != null && mGlicButton.click(x, y, buttons)) {
                mGlicButton.handleClick(time, buttons, modifiers);
                return;
            }
            if (mModelSelectorButton != null && mModelSelectorButton.click(x, y, buttons)) {
                mModelSelectorButton.handleClick(time, buttons, modifiers);
                return;
            }
            getActiveStripLayoutHelper().click(time(), x, y, buttons, modifiers);
        }

        @Override
        public void fling(float x, float y, float velocityX, float velocityY) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            getActiveStripLayoutHelper().fling(time(), velocityX);
        }

        @Override
        public void onLongPress(float x, float y) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }

            /* Vivaldi - Handling long click event on + button in tab strip **/
            if (ChromeApplicationImpl.isVivaldi()) {
                if (isNewTabButtonClicked(x, y)) {
                    showTabSwitcherPopupMenu();
                    return;
                }
                y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.
            } // End Vivaldi

            if (mModelSelectorButton != null && mModelSelectorButton.click(x, y, 0)) {
                return;
            }
            getActiveStripLayoutHelper().onLongPress(x, y);
        }

        @Override
        public void onPinch(float x0, float y0, float x1, float y1, boolean firstEvent) {
            // Not implemented.
        }

        @Override
        public void onHoverEnter(float x, float y) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }

            // Inflate the hover card ViewStub if not already inflated.
            if (mTabHoverCardViewStub.getParent() != null) {
                mTabHoverCardViewStub.inflate();
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            getActiveStripLayoutHelper().onHoverEnter(x, y);
        }

        @Override
        public void onHoverMove(float x, float y) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            getActiveStripLayoutHelper().onHoverMove(x, y);
        }

        @Override
        public void onHoverExit(boolean inArea) {
            getActiveStripLayoutHelper().onHoverExit(inArea);
        }

        @Override
        public void onScroll(float horizontalAxisScroll, float verticalAxisScroll) {
            getActiveStripLayoutHelper().onScroll(horizontalAxisScroll, verticalAxisScroll);
        }

        private long time() {
            return LayoutManagerImpl.time();
        }

        /** Vivaldi */
        @Override
        public void onUpOrCancel(float x, float y) {
            getActiveStripLayoutHelper().onUpOrCancel(x, getValueOfY(y), time());
        }
    }

    /** Observer for Tab Switcher layout events. */
    class TabSwitcherLayoutObserver implements LayoutStateObserver {
        @Override
        public void onStartedShowing(int layoutType) {
            if (layoutType == LayoutType.TAB_SWITCHER && isActivityInXrFullSpaceModeNow()) {
                setStripVisibilityState(StripVisibilityState.OBSCURED, /* clear= */ false);
            }
        }

        @Override
        public void onFinishedShowing(@LayoutType int layoutType) {
            if (layoutType != LayoutType.TAB_SWITCHER) return;
            setStripVisibilityState(StripVisibilityState.OBSCURED, /* clear= */ false);
        }

        @Override
        public void onStartedHiding(@LayoutType int layoutType) {
            if (layoutType != LayoutType.TAB_SWITCHER) return;
            if (!isActivityInXrFullSpaceModeNow()) {
                setStripVisibilityState(StripVisibilityState.OBSCURED, /* clear= */ true);
            }

            // Expand tab group on GTS exit.
            mNormalHelper.expandGroupOnGtsExit();
            mIncognitoHelper.expandGroupOnGtsExit();
        }

        @Override
        public void onFinishedHiding(int layoutType) {
            if (layoutType != LayoutType.TAB_SWITCHER) return;
            if (isActivityInXrFullSpaceModeNow()) {
                setStripVisibilityState(StripVisibilityState.OBSCURED, /* clear= */ true);
            }
        }
    }

    /**
     * @return Returns layout observer for tab switcher.
     */
    public LayoutStateObserver getTabSwitcherObserver() {
        return mTabSwitcherLayoutObserver;
    }

    /**
     * Creates an instance of the StripLayoutHelperManager.
     *
     * @param context The current Android Context.
     * @param managerHost The parent LayoutManagerHost.
     * @param updateHost The parent LayoutUpdateHost.
     * @param renderHost The LayoutRenderHost.
     * @param layerTitleCacheSupplier A supplier of the cache that holds the title textures.
     * @param tabModelStartupInfoSupplier A supplier for the TabModelStartupInfo.
     * @param lifecycleDispatcher The ActivityLifecycleDispatcher for registering this class to
     *     lifecycle events.
     * @param multiInstanceManager The {@link MultiInstanceManager} used to move tabs to other
     *     windows.
     * @param dragDropDelegate DragAndDropDelegate passed to {@link TabStripDragHandler} to initiate
     *     tab drag and drop.
     * @param toolbarContainerView View passed to {@link TabStripDragHandler} for drag and drop.
     * @param tabHoverCardViewStub The ViewStub representing the strip tab hover card.
     * @param tabContentManagerSupplier Supplier of the TabContentManager instance.
     * @param browserControlsStateProvider BrowserControlsStateProvider for drag drop.
     * @param windowAndroid The {@link WindowAndroid} instance to access Activity.
     * @param toolbarManager The ToolbarManager instance.
     * @param desktopWindowStateManager The DesktopWindowStateManager for the app header.
     * @param actionConfirmationManager The {@link ActionConfirmationManager} for group actions.
     * @param dataSharingTabManager The {@link DataSharingTabManager} for shared groups.
     * @param bottomSheetController The {@link BottomSheetController} used to show bottom sheets.
     * @param shareDelegateSupplier Supplies {@link ShareDelegate} to share tab URLs.
     * @param xrSpaceModeObservableSupplier Supplies current XR space mode status. True for XR full
     *     space mode, false otherwise.
     * @param backPressManager The {@link BackPressManager} for handling back press.
     * @param snackbarManager The {@link SnackbarManager} used to show snackbar UI.
     * @param glicClickHandler The click handler for the Glic button.
     * @param isStackStrip Vivaldi: Set to true if this is a stack strip.
     */
    // TODO(crbug.com/484116872): Suppressing to observe SharedPreferences, which is discouraged;
    // should use another messaging channel instead.
    @SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
    public StripLayoutHelperManager(
            Context context,
            LayoutManagerHost managerHost,
            LayoutUpdateHost updateHost,
            LayoutRenderHost renderHost,
            SettableMonotonicObservableSupplier<LayerTitleCache> layerTitleCacheSupplier, // Vivaldi
            MonotonicObservableSupplier<TabModelStartupInfo> tabModelStartupInfoSupplier,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            MultiInstanceManager multiInstanceManager,
            DragAndDropDelegate dragDropDelegate,
            View toolbarContainerView,
            ViewStub tabHoverCardViewStub,
            MonotonicObservableSupplier<TabContentManager> tabContentManagerSupplier,
            BrowserControlsStateProvider browserControlsStateProvider,
            WindowAndroid windowAndroid,
            // TODO(crbug.com/40939440): Avoid passing the ToolbarManager instance. Potentially
            // implement an interface to manage strip transition states.
            ToolbarManager toolbarManager,
            @Nullable DesktopWindowStateManager desktopWindowStateManager,
            ActionConfirmationManager actionConfirmationManager,
            DataSharingTabManager dataSharingTabManager,
            BottomSheetController bottomSheetController,
            MonotonicObservableSupplier<ShareDelegate> shareDelegateSupplier,
            @Nullable NonNullObservableSupplier<Boolean> xrSpaceModeObservableSupplier,
            BackPressManager backPressManager,
            SnackbarManager snackbarManager,
            Runnable glicClickHandler,
            boolean isStackStrip) { // Vivaldi
        mContext = context;
        Resources res = context.getResources();
        mManagerHost = managerHost;
        mUpdateHost = updateHost;
        mRenderHost = renderHost;
        mLayerTitleCacheSupplier = layerTitleCacheSupplier;
        mDensity = res.getDisplayMetrics().density;
        mTabStripTreeProvider = new TabStripSceneLayer(mDensity);
        mTabStripEventHandler = new TabStripEventHandler();
        mTabSwitcherLayoutObserver = new TabSwitcherLayoutObserver();
        mLifecycleDispatcher = lifecycleDispatcher;
        mLifecycleDispatcher.register(this);
        mBrowserControlsStateProvider = browserControlsStateProvider;
        mDefaultTitle = context.getString(R.string.tab_loading_default_title);
        mToolbarControlContainer = toolbarContainerView;
        mEventFilter =
                new AreaMotionEventFilter(context, mTabStripEventHandler, null, false, false) {
                    @Override
                    protected boolean isMotionEventInArea(MotionEvent e) {
                        if (super.isMotionEventInArea(e)) return true;

                        // Allow right-clicks in empty spaces of the tab strip (e.g., top/side
                        // paddings) to be intercepted by the tab strip to show the context menu.
                        // Regular touch events in these regions should still fall through to the
                        // OS for window dragging.
                        if (e.getButtonState() == MotionEvent.BUTTON_SECONDARY) {
                            float x = e.getX() / mDensity;
                            float y = e.getY() / mDensity;
                            if (x >= 0 && x <= mWidth && y >= 0 && y <= mStripFilterArea.bottom) {
                                return true;
                            }
                        }
                        return false;
                    }
                };

        mIsHeaderCustomizationSupported =
                ToolbarFeatures.isAppHeaderCustomizationSupported(
                        /* isTablet= */ true, DisplayUtil.isContextInDefaultDisplay(mContext));
        mScrollableStripHeight = res.getDimension(R.dimen.tab_strip_height) / mDensity;
        mHeight =
                mIsHeaderCustomizationSupported
                        ? toolbarManager.getTabStripHeightSupplier().get() / mDensity
                        : mScrollableStripHeight;
        mTopPadding = mHeight - mScrollableStripHeight;
        mDesktopWindowStateManager = desktopWindowStateManager;
        mStripVisibilityStateObserver =
                state -> {
                    if (mEventFilter == null) return;

                    if (BuildConfig.IS_VIVALDI) { // Vivaldi VAB-9744 //
                        mEventFilter.setEventArea(mStripFilterArea);
                        return;
                    } // End Vivaldi

                    // Consume motion events only on a visible strip.
                    mEventFilter.setEventArea(
                            state == StripVisibilityState.VISIBLE ? mStripFilterArea : null);
                };
        mStripVisibilityStateSupplier.addSyncObserverAndPostIfNonNull(
                mStripVisibilityStateObserver);
        mGlicClickHandler = glicClickHandler;

        if (isGlicButtonEnabled()) {
            StripLayoutViewOnClickHandler glicClickHandlerOnButton =
                    (time, view, motionEventButtonState, modifiers) -> handleGlicButtonClick();
            StripLayoutViewOnKeyboardFocusHandler glicKeyboardFocusHandler =
                    (isFocused, view) -> {
                        getActiveStripLayoutHelper().onKeyboardFocus(isFocused, view);
                    };
            createGlicButton(context, glicClickHandlerOnButton, glicKeyboardFocusHandler);
            ContextUtils.getAppSharedPreferences().registerOnSharedPreferenceChangeListener(this);
        }
        if (!IncognitoUtils.shouldOpenIncognitoAsWindow()) {
            StripLayoutViewOnClickHandler selectorClickHandler =
                    (time, view, motionEventButtonState, modifiers) ->
                            handleModelSelectorButtonClick();
            StripLayoutViewOnKeyboardFocusHandler selectorKeyboardFocusHandler =
                    (isFocused, view) -> {
                        getActiveStripLayoutHelper().onKeyboardFocus(isFocused, view);
                    };
            createModelSelectorButton(context, selectorClickHandler, selectorKeyboardFocusHandler);
        }
        // Use toolbar menu button padding to align MSB with menu button.
        mStripEndPadding = res.getDimension(R.dimen.button_end_padding) / mDensity;

        mTabHoverCardViewStub = tabHoverCardViewStub;

        if (MultiWindowUtils.isMultiInstanceApi31Enabled()) {
            mTabStripDragHandler =
                    new TabStripDragHandler(
                            context,
                            this::getActiveStripLayoutHelper,
                            () ->
                                    getStripVisibilityStateSupplier().get()
                                            == StripVisibilityState.VISIBLE,
                            tabContentManagerSupplier,
                            mLayerTitleCacheSupplier,
                            multiInstanceManager,
                            dragDropDelegate,
                            browserControlsStateProvider,
                            () -> windowAndroid.getActivity().get(),
                            toolbarManager.getTabStripHeightSupplier());

            if (ChromeFeatureList.sEscCancelDrag.isEnabled()) {
                backPressManager.addHandler(
                        mTabStripDragHandler, BackPressHandler.Type.CANCEL_TAB_STRIP_DRAG);
            }
        }

        mToolbarManager = toolbarManager;
        mStatusBarColorController = mToolbarManager.getStatusBarColorController();

        mNormalHelper =
                new StripLayoutHelper(
                        context,
                        this,
                        managerHost,
                        updateHost,
                        renderHost,
                        /* incognito= */ false,
                        mGlicButton,
                        mModelSelectorButton,
                        mTabStripDragHandler,
                        toolbarContainerView,
                        windowAndroid,
                        actionConfirmationManager,
                        dataSharingTabManager,
                        () ->
                                getStripVisibilityStateSupplier().get()
                                        == StripVisibilityState.VISIBLE,
                        bottomSheetController,
                        multiInstanceManager,
                        shareDelegateSupplier,
                        TabGroupListBottomSheetCoordinator::new,
                        snackbarManager);
        mIncognitoHelper =
                new StripLayoutHelper(
                        context,
                        this,
                        managerHost,
                        updateHost,
                        renderHost,
                        /* incognito= */ true,
                        /* glicButton= */ null,
                        mModelSelectorButton,
                        mTabStripDragHandler,
                        toolbarContainerView,
                        windowAndroid,
                        actionConfirmationManager,
                        dataSharingTabManager,
                        () ->
                                getStripVisibilityStateSupplier().get()
                                        == StripVisibilityState.VISIBLE,
                        bottomSheetController,
                        multiInstanceManager,
                        shareDelegateSupplier,
                        TabGroupListBottomSheetCoordinator::new,
                        snackbarManager);

        tabHoverCardViewStub.setOnInflateListener(
                (viewStub, view) -> {
                    var hoverCardView = (StripTabHoverCardView) view;
                    hoverCardView.initialize(
                            assumeNonNull(mTabModelSelector), tabContentManagerSupplier);
                    mNormalHelper.setTabHoverCardView(hoverCardView);
                    mIncognitoHelper.setTabHoverCardView(hoverCardView);
                });

        if (tabModelStartupInfoSupplier != null) {
            var tabModelStartupInfo = tabModelStartupInfoSupplier.get();
            if (tabModelStartupInfo != null) {
                setTabModelStartupInfo(tabModelStartupInfo);
            } else {
                tabModelStartupInfoSupplier.addSyncObserverAndPostIfNonNull(
                        this::setTabModelStartupInfo);
            }
        }

        mLayerTitleCacheSupplier.addSyncObserverAndPostIfNonNull(
                (LayerTitleCache layerTitleCache) -> {
                    mNormalHelper.setLayerTitleCache(layerTitleCache);
                    mIncognitoHelper.setLayerTitleCache(layerTitleCache);
                    setGlicButtonText(mContext.getString(R.string.glic_button_entrypoint_label));
                });

        if (mDesktopWindowStateManager != null) {
            mDesktopWindowStateManager.addObserver(this);
            mIsTopResumedActivity = !mDesktopWindowStateManager.isInUnfocusedDesktopWindow();
        } else {
            mIsTopResumedActivity = AppHeaderUtils.isActivityFocusedAtStartup(lifecycleDispatcher);
        }
        if (isAppInDesktopWindow()) {
            @Nullable AppHeaderState appHeaderState =
                    mDesktopWindowStateManager.getAppHeaderState();
            if (appHeaderState != null) {
                onAppHeaderStateChanged(appHeaderState);
            }
        }

        mXrSpaceModeObservableSupplier = xrSpaceModeObservableSupplier;
        // Vivaldi
        setIsStackStrip(isStackStrip);
        
        mActivity = (ChromeTabbedActivity)context;
        mShouldHideOverlay = false;
        // End Vivaldi
    }

    @EnsuresNonNullIf("mDesktopWindowStateManager")
    private boolean isAppInDesktopWindow() {
        return AppHeaderUtils.isAppInDesktopWindow(mDesktopWindowStateManager)
                && mDesktopWindowStateManager != null;
    }

    private void setTabModelStartupInfo(TabModelStartupInfo startupInfo) {
        mNormalHelper.setTabModelStartupInfo(
                startupInfo.standardCount,
                startupInfo.standardActiveIndex,
                startupInfo.createdStandardTabOnStartup);
        mIncognitoHelper.setTabModelStartupInfo(
                startupInfo.incognitoCount,
                startupInfo.incognitoActiveIndex,
                startupInfo.createdIncognitoTabOnStartup);
    }

    private void createGlicButton(
            Context context,
            StripLayoutViewOnClickHandler selectorClickHandler,
            StripLayoutViewOnKeyboardFocusHandler keyboardFocusHandler) {
        mGlicButton =
                new TintedCompositorTextButton(
                        context,
                        ButtonType.GLIC,
                        /* parentView= */ null,
                        GLIC_BUTTON_BACKGROUND_WIDTH_DP,
                        GLIC_BUTTON_BACKGROUND_HEIGHT_DP,
                        (tooltipText) -> {
                            mToolbarControlContainer.setTooltipText(tooltipText);
                        },
                        selectorClickHandler,
                        keyboardFocusHandler,
                        R.drawable.ic_spark_16dp,
                        GLIC_BUTTON_CLICK_SLOP_DP,
                        /* hasLongClickAction= */ false,
                        /* dismissButton= */ null);

        // y-offset for folio = lowered tab container + (tab container size - bg size)/2 -
        // folio tab title y-offset = 2 + (38 - 28)/2 - 2 = 5dp
        mGlicButton.setDrawY(GLIC_BUTTON_BACKGROUND_Y_OFFSET_DP);
        mGlicButton.setVisible(false);

        @ColorInt
        int backgroundDefaultColor = SemanticColorUtils.getColorSurfaceContainerLow(context);

        @ColorInt
        int apsBackgroundHoveredColor =
                ColorUtils.setAlphaComponentWithFloat(
                        SemanticColorUtils.getColorPrimary(context),
                        GLIC_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY);
        @ColorInt
        int backgroundPressedColor =
                ColorUtils.setAlphaComponentWithFloat(
                        SemanticColorUtils.getColorPrimary(context),
                        GLIC_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY);

        @ColorInt int iconDefaultColor = SemanticColorUtils.getDefaultIconColor(context);

        mGlicButton.setTint(
                iconDefaultColor, iconDefaultColor, Color.TRANSPARENT, Color.TRANSPARENT);

        mGlicButton.setBackgroundTint(
                backgroundDefaultColor,
                backgroundPressedColor,
                Color.TRANSPARENT,
                Color.TRANSPARENT,
                apsBackgroundHoveredColor,
                backgroundPressedColor,
                Color.TRANSPARENT,
                Color.TRANSPARENT);

        updateGlicButtonOpacity();

        mGlicButton.setAccessibilityDescription(
                context.getString(R.string.glic_tab_strip_button_tooltip),
                /* incognitoDescription= */ "");
    }

    private void createModelSelectorButton(
            Context context,
            StripLayoutViewOnClickHandler selectorClickHandler,
            StripLayoutViewOnKeyboardFocusHandler keyboardFocusHandler) {
        mModelSelectorButton =
                new TintedCompositorButton(
                        context,
                        ButtonType.INCOGNITO_SWITCHER,
                        /* parentView= */ null,
                        MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP,
                        MODEL_SELECTOR_BUTTON_BACKGROUND_HEIGHT_DP,
                        (tooltipText) -> {
                            mToolbarControlContainer.setTooltipText(tooltipText);
                        },
                        selectorClickHandler,
                        keyboardFocusHandler,
                        R.drawable.btn_tabstrip_new_tab, // Vivaldi
                        MODEL_SELECTOR_BUTTON_CLICK_SLOP_DP); // Vivaldi

        // Button bg size is 32 * 32.
        mModelSelectorButton.setBackgroundResourceId(R.drawable.bg_circle_tab_strip_button);

        // Model selector button background color.
        // Default bg color is surface inverse.
        @ColorInt
        int backgroundDefaultColor = context.getColor(R.color.model_selector_button_bg_color);

        // Incognito bg color is surface 1 baseline.
        @ColorInt
        int backgroundIncognitoColor =
                context.getColor(R.color.default_bg_color_dark_elev_1_baseline);

        @ColorInt
        int apsBackgroundHoveredColor =
                ColorUtils.setAlphaComponentWithFloat(
                        SemanticColorUtils.getDefaultTextColor(context),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY);
        @ColorInt
        int apsBackgroundPressedColor =
                ColorUtils.setAlphaComponentWithFloat(
                        SemanticColorUtils.getDefaultTextColor(context),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY);
        @ColorInt
        int apsBackgroundHoveredIncognitoColor =
                ColorUtils.setAlphaComponentWithFloat(
                        context.getColor(R.color.tab_strip_button_hover_bg_color),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY);
        @ColorInt
        int apsBackgroundPressedIncognitoColor =
                ColorUtils.setAlphaComponentWithFloat(
                        context.getColor(R.color.tab_strip_button_hover_bg_color),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY);

        @ColorInt
        int iconDefaultColor =
                AppCompatResources.getColorStateList(context, R.color.default_icon_color_tint_list)
                        .getDefaultColor();
        @ColorInt
        int iconIncognitoColor = context.getColor(R.color.default_icon_color_secondary_light);

        mModelSelectorButton.setTint(
                iconDefaultColor, iconDefaultColor, iconIncognitoColor, iconIncognitoColor);

        mModelSelectorButton.setBackgroundTint(
                backgroundDefaultColor,
                backgroundDefaultColor,
                backgroundIncognitoColor,
                backgroundIncognitoColor,
                apsBackgroundHoveredColor,
                apsBackgroundPressedColor,
                apsBackgroundHoveredIncognitoColor,
                apsBackgroundPressedIncognitoColor);

        determineModelSelectorButtonYOffset(); // Vivaldi

        // y-offset for folio = lowered tab container + (tab container size - bg size)/2 -
        // folio tab title y-offset = 2 + (38 - 32)/2 - 2 = 3dp
        mModelSelectorButton.setDrawY(MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP);

        mModelSelectorButton.setIncognito(false);
        mModelSelectorButton.setVisible(false);

        mModelSelectorButton.setAccessibilityDescription(
                context.getString(R.string.accessibility_tabstrip_btn_incognito_toggle_incognito),
                context.getString(R.string.accessibility_tabstrip_btn_incognito_toggle_incognito));

        // Note(david@vivaldi.com): As we are abusing the |mModelSelectorButton| to create a new tab
        // we also need to update the tooltips accordingly.
        if (ChromeApplicationImpl.isVivaldi())
            mModelSelectorButton.setAccessibilityDescription(
                    context.getString(R.string.accessibility_toolbar_btn_new_tab),
                    context.getString(R.string.accessibility_new_incognito_tab_page));
    }

    /** Cleans up internal state. An instance should not be used after this method is called. */
    @SuppressWarnings({"NullAway", "UseSharedPreferencesManagerFromChromeCheck"})
    public void destroy() {
        mTabStripTreeProvider.destroy();
        mTabStripTreeProvider = null;
        mLifecycleDispatcher.unregister(this);
        // Remove the observer to prevent any updates on a destroyed EventFilter.
        mStripVisibilityStateSupplier.removeObserver(mStripVisibilityStateObserver);
        // Delete the EventFilter to avoid any updates on destroyed StripLayoutHelpers.
        mEventFilter = null;
        mTabStripEventHandler = null;
        ContextUtils.getAppSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
        mIncognitoHelper.destroy();
        mNormalHelper.destroy();
        if (mTabModelSelector != null) {
            mTabModelSelector.removeTabGroupModelFilterObserver(mTabModelObserver);

            mTabModelSelector.getCurrentTabModelSupplier().removeObserver(mCurrentTabModelObserver);
            mTabModelSelectorTabModelObserver.destroy();
            mTabModelSelectorTabObserver.destroy();
        }
        if (mTabStripDragHandler != null) {
            mTabStripDragHandler.destroy();
            mTabStripDragHandler = null;
        }
        if (mDesktopWindowStateManager != null) {
            mDesktopWindowStateManager.removeObserver(this);
        }
    }

    /** Mark whether tab strip is hidden by a height transition. */
    public void setIsTabStripHiddenByHeightTransition(boolean isHidden) {
        setStripVisibilityState(StripVisibilityState.HIDDEN_BY_HEIGHT_TRANSITION, !isHidden);
        mStatusBarColorController.setTabStripHiddenOnTablet(isHidden);
    }

    @Override
    public void onResumeWithNative() {
        if (mTabModelSelector == null) return;
        Tab currentTab = mTabModelSelector.getCurrentTab();
        if (currentTab == null) return;
        getStripLayoutHelper(currentTab.isIncognito())
                .scrollTabToView(LayoutManagerImpl.time(), true);

        // Note(david@vivaldi.com): Update the y offset and pass it to the model selector button.
        determineModelSelectorButtonYOffset();
        if (mModelSelectorButton != null)
            mModelSelectorButton.setDrawY(MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP);
    }

    @Override
    public void onPauseWithNative() {
        // Clear any persisting tab strip hover state when the activity is paused.
        getActiveStripLayoutHelper().onHoverExit(/* inTabStrip= */ false);
    }

    private void handleGlicButtonClick() {
        mGlicClickHandler.run();
    }

    /** Sets the Glic button text and refreshes the layout to accommodate new width. */
    public void setGlicButtonText(@Nullable String text) {
        if (mGlicButton == null || TextUtils.equals(mGlicButton.getText(), text)) return;
        mGlicButton.setText(text);

        LayerTitleCache titleCache = mLayerTitleCacheSupplier.get();
        if (titleCache != null) {
            mGlicButton.setTextResourceId(titleCache.getUpdatedGlicButtonText(text));
        } else {
            mGlicButton.setTextResourceId(Resources.ID_NULL);
        }

        updateGlicButtonWidth();
        updateGlicButtonPosition();
        updateButtonMargins();
        mUpdateHost.requestUpdate();
    }

    private void updateGlicButtonWidth() {
        if (mGlicButton == null) return;
        String glicButtonText = mGlicButton.getText();

        float width = GLIC_BUTTON_BACKGROUND_WIDTH_DP;
        if (!TextUtils.isEmpty(glicButtonText)) {
            LayerTitleCache titleCache = mLayerTitleCacheSupplier.get();
            if (titleCache != null) {
                float textWidthDp = titleCache.getButtonTextWidth(glicButtonText) / mDensity;
                width =
                        GLIC_BUTTON_START_PADDING_DP
                                + GLIC_ICON_WIDTH_DP
                                + GLIC_ICON_TEXT_PADDING_DP
                                + textWidthDp
                                + GLIC_BUTTON_END_PADDING_DP;
            }
        }
        mGlicButton.setWidth(width);
    }

    private void updateGlicButtonPosition() {
        if (mGlicButton == null) return;

        mGlicButton.setDrawY(GLIC_BUTTON_BACKGROUND_Y_OFFSET_DP);
        mGlicButton.setTouchTargetInsets(null, mTopPadding, null, -mTopPadding);

        boolean isMsbVisible = mModelSelectorButton != null && mModelSelectorButton.isVisible();

        if (!LocalizationUtils.isLayoutRtl()) {
            // We anchor the right side so when the Glic button expands, it will grow to the left
            // while the right edge stays fixed.
            float rightSideAnchor =
                    mWidth - mRightPadding - mStripEndPadding - GLIC_ALIGNMENT_OFFSET_DP;
            if (isMsbVisible) {
                rightSideAnchor -= BUTTON_DESIRED_TOUCH_TARGET_SIZE;
            }
            mGlicButton.setDrawX(rightSideAnchor - mGlicButton.getWidth());
        } else {
            // Symmetric logic for RTL: anchor the left side.
            float leftSideAnchor = mLeftPadding + mStripEndPadding + GLIC_ALIGNMENT_OFFSET_DP;
            if (isMsbVisible) {
                leftSideAnchor += BUTTON_DESIRED_TOUCH_TARGET_SIZE;
            }
            mGlicButton.setDrawX(leftSideAnchor);
        }
    }

    private void updateGlicButtonOpacity() {
        if (mGlicButton == null) return;
        boolean isUnfocusedInDw = isAppInDesktopWindow() && !mIsTopResumedActivity;
        mGlicButton.setOpacity(isUnfocusedInDw ? GLIC_BUTTON_UNFOCUSED_OPACITY : 1.0f);
    }

    private void handleModelSelectorButtonClick() {
        // Note(david@vivaldi.com): We are abusing the |mModelSelectorButton| to create a new tab.
        if (ChromeApplicationImpl.isVivaldi()) {
            createNewTab();
            return;
        } // End Vivaldi

        if (mTabModelSelector == null) return;
        getActiveStripLayoutHelper().finishAnimationsAndPushTabUpdates();
        if (mModelSelectorButton == null || !mModelSelectorButton.isVisible()) return;
        mTabModelSelector.selectModel(!mTabModelSelector.isIncognitoSelected());
        RecordUserAction.record("MobileToolbarModelSelected");
    }

    @VisibleForTesting
    public void simulateClick(float x, float y, int buttons, int modifiers) {
        mTabStripEventHandler.click(x, y, buttons, modifiers);
    }

    @VisibleForTesting
    public void simulateLongPress(float x, float y) {
        mTabStripEventHandler.onLongPress(x, y);
    }

    @Override
    public SceneOverlayLayer getUpdatedSceneOverlayTree(
            RectF viewport, RectF visibleViewport, ResourceManager resourceManager) {
        assert mTabStripTreeProvider != null;
        mResourceManager = resourceManager;

        // When refactor is enabled, the mSceneLayerYOffset / mSceneLayerVisibleHeight wil be
        // calculated externally, so we can skip the adjustment here.
        if (!BrowserControlsUtils.isTopControlsRefactorOffsetEnabled()) {
            float topControlOffsetDp =
                    mBrowserControlsStateProvider.getTopControlOffset() / mDensity;
            mSceneLayerVisibleHeight = getVisibleHeightDp(topControlOffsetDp);
            mSceneLayerYOffset = getAdjustedYOffset(topControlOffsetDp);
            updateStripBottomPx();
        }

        // Note(david@vivaldi.com): Apply the correct |yOffset| according to the toolbar position.
        if (!VivaldiUtils.isTopToolbarOn() && !mShouldHideOverlay) {
            mViewportHeightOffset = viewport.height()
                    * (1.f / mActivity.getResources().getDisplayMetrics().density);
            mSceneLayerYOffset += mViewportHeightOffset - mHeight;
            // Pass the yOffset of the main strip here as it is used to calculate the visibility of
            // the tab strip container.
            mTabStripTreeProvider.setMainYOffset(mSceneLayerYOffset);
            // Place the stack strip above the main strip.
            if (mIsStackStrip) mSceneLayerYOffset = -getHeight();
        } else {
            mViewportHeightOffset = 0;
            // Add vertical offset when there is an active panel.
            if (mActivity.getLayoutManager() != null) {
                OverlayPanel panel =
                        mActivity.getLayoutManager().getOverlayPanelManager().getActivePanel();
                if (panel != null) mSceneLayerYOffset += panel.getBasePageY();
            }
            mSceneLayerYOffset += 0;

            // Vivaldi
            // Pass the yOffset of the main strip here as it is used to calculate the visibility of
            // the tab strip container.
            mTabStripTreeProvider.setMainYOffset(mSceneLayerYOffset);
            // Place the stack strip under the main strip.
            if (mIsStackStrip) mSceneLayerYOffset = getHeight();
        }
        // End Vivaldi

        // Vivaldi VAB-12859: hide tab strip compositor layer when mini origin bar is showing.
        if (BuildConfig.IS_VIVALDI) {
            mTabStripTreeProvider.shouldHideOverlay(mVivaldiMiniBarActiveSupplier != null
                    && mVivaldiMiniBarActiveSupplier.get());
        }
        pushAndUpdateStrip(mSceneLayerYOffset, mSceneLayerVisibleHeight);
        return mTabStripTreeProvider;
    }

    private void pushAndUpdateStrip(float yOffsetDp, float visibleHeightDp) {
        if (mResourceManager == null) return;

        setStripVisibilityState(
                StripVisibilityState.HIDDEN_BY_SCROLL,
                /* clear= */ mBrowserControlsStateProvider.getTopControlOffset() >= 0);
        Tab selectedTab =
                mTabModelSelector == null
                        ? null
                        : mTabModelSelector
                                .getCurrentModel()
                                .getTabAt(mTabModelSelector.getCurrentModel().index());

        // Note(david@vivaldi.com): We show a loading text while restoring tabs.
        if (selectedTab != null)
            mTabStripTreeProvider.updateLoadingState(mResourceManager, this,
                    selectedTab.isIncognitoBranded(),
                    getBackgroundStripColor(selectedTab.getThemeColor()),
                    mToolbarManager.getTabStripHeightSupplier().get());

        int selectedTabId = selectedTab == null ? TabModel.INVALID_TAB_INDEX : selectedTab.getId();
        int hoveredTabId =
                getActiveStripLayoutHelper().getLastHoveredTab() == null
                        ? TabModel.INVALID_TAB_INDEX
                        : getActiveStripLayoutHelper().getLastHoveredTab().getTabId();

        // When tab strip is hiding, animation will trigger the toolbar moving up and tab strip
        // fade-out in place. We use the visible height to decide the transition progress then
        // update the scrim opacity.
        if (duringTabStripHeightTransition()) {
            // The fade-out is implemented by adding a scrim layer on top of the tab strip, with the
            // same bg as the toolbar background color.
            calculateScrimOpacityDuringHeightTransition(visibleHeightDp);
            mStatusBarColorController.setTabStripColorOverlay(
                    getStripTransitionScrimColor(), mStripTransitionScrimOpacity);
        }

        mTabStripTreeProvider.pushAndUpdateStrip(
                this,
                assertNonNull(mLayerTitleCacheSupplier.get()),
                mResourceManager,
                getActiveStripLayoutHelper().getStripLayoutTabsToRender(),
                getActiveStripLayoutHelper().getStripLayoutGroupTitlesToRender(),
                yOffsetDp,
                selectedTabId,
                hoveredTabId,
                getStripTransitionScrimColor(),
                mStripTransitionScrimOpacity,
                getActiveStripLayoutHelper().getLeftPaddingToDraw(),
                getActiveStripLayoutHelper().getRightPaddingToDraw(),
                mTopPadding);
    }

    private float getVisibleHeightDp(float topControlOffsetDp) {
        if (!duringTabStripHeightTransition()) return getHeight();

        // During tab strip transition, make the yOffset stick to the top of the browser
        // controls. This assumes on tablet there are no other components on top of the control
        // container.
        float visibleHeightDp = topControlOffsetDp;
        if (visibleHeightDp < 0) visibleHeightDp += getHeight();
        return visibleHeightDp;
    }

    private float getAdjustedYOffset(float topControlsOffset) {
        // When tab strip is hiding, animation will trigger the toolbar moving up and tab
        // strip fade-out in place. In this case the tab strip should not move at all.
        if (duringTabStripHeightTransition()) {
            return 0;
        }

        if ((getStripVisibilityStateSupplier().get()
                        & StripVisibilityState.HIDDEN_BY_HEIGHT_TRANSITION)
                != 0) {
            // When the tab strip is hidden by a height transition, the stable offset of this scene
            // layer should be a negative value.
            // Use mScrollableStripHeight as the baseline height because mHeight may have already
            // changed during a height transition to hide the strip.
            return topControlsOffset - (mHeight > 0 ? mHeight : mScrollableStripHeight);
        }

        if (!ChromeApplicationImpl.isVivaldi()) // We always return the offset.
        if (!mBrowserControlsStateProvider.isVisibilityForced()) {
            // With bciv, as long as if the visibility isn't forced by the browser, and if the
            // tabstrip isn't hidden, the composited layers should positioned at their fully visible
            // positions.
            return 0;
        }

        return topControlsOffset;
    }

    @Override
    public void removeFromParent() {
        mTabStripTreeProvider.removeFromParent();
    }

    private int getStripTransitionScrimColor() {
        return mToolbarManager.getPrimaryColor();
    }

    @Override
    public boolean isSceneOverlayTreeShowing() {
        // TODO(mdjones): This matches existing behavior but can be improved to return false if
        // the browser controls offset is equal to the browser controls height.
        return (getStripVisibilityStateSupplier().get() & StripVisibilityState.OBSCURED) == 0;
    }

    @Override
    public @Nullable EventFilter getEventFilter() {
        return mEventFilter;
    }

    public Context getContext() {
        return mContext;
    }

    @Override
    public void onSizeChanged(
            float width, float height, float visibleViewportOffsetY, int orientation) {
        mWidth = width;
        mLastVisibleViewportOffsetY = visibleViewportOffsetY;
        boolean orientationChanged = false;
        if (mOrientation != orientation) {
            mOrientation = orientation;
            orientationChanged = true;
        }
        updateGlicButtonPosition();
        if (mModelSelectorButton != null) {
            determineModelSelectorButtonYOffset(); // Vivaldi;

            mModelSelectorButton.setDrawY(MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP);
            mModelSelectorButton.setTouchTargetInsets(null, mTopPadding, null, -mTopPadding);
            if (!LocalizationUtils.isLayoutRtl()) {
                // Vivaldi Ref. VAB-8862
                int offset = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext) ?
                        (int) mContext.getResources().getDimension(R.dimen.new_tab_padding_tablet) :
                        (int) mContext.getResources().getDimension(R.dimen.new_tab_padding);
                mModelSelectorButton.setDrawX(
                        mWidth - mRightPadding - getModelSelectorButtonWidthWithEndPadding() - offset /* Vivaldi*/);
            } else {
                mModelSelectorButton.setDrawX(mLeftPadding + mStripEndPadding);
            }
        }

        // Note(david@vivaldi.com): We need to take the orientation into account.
        if (ChromeApplicationImpl.isVivaldi()) {
            mNormalHelper.onSizeChanged(mWidth, mHeight, orientationChanged);
            mIncognitoHelper.onSizeChanged(mWidth, mHeight, orientationChanged);
        } else {
        mNormalHelper.onSizeChanged(
                mWidth,
                mScrollableStripHeight,
                orientationChanged,
                LayoutManagerImpl.time(),
                mLeftPadding,
                mRightPadding,
                mTopPadding);
        mIncognitoHelper.onSizeChanged(
                mWidth,
                mScrollableStripHeight,
                orientationChanged,
                LayoutManagerImpl.time(),
                mLeftPadding,
                mRightPadding,
                mTopPadding);
        }// End Vivaldi

        // Note(david@vivaldi.com): Apply the correct clicking area for all possible scenarios.
        if (ChromeApplicationImpl.isVivaldi()) {
            if (VivaldiUtils.isTopToolbarOn()) {
                float top = mIsStackStrip ? getHeight() : 0;
                mStripFilterArea.set(0, top, mWidth, mIsStackStrip ? getHeight() * 2 : getHeight());
            } else {
                mStripFilterArea.set(0,
                        mIsStackStrip ? getHeight() : height - getHeight(), mWidth,
                        mIsStackStrip ? getHeight() * 2 : height);
                if (!VivaldiUtils.isTabStackVisible())
                    mStripFilterArea.set(0, 0, mWidth, getHeight());
            }
            if (mEventFilter != null)
                mEventFilter.setEventArea(mStripFilterArea); // VAB-9797
        } else
        mStripFilterArea.set(
                mLeftPadding,
                mTopPadding,
                mWidth - mRightPadding,
                Math.min(getHeight(), visibleViewportOffsetY));
        // Avoid handling motion events when invisible strip state persists after a size change.
        if (mEventFilter != null
                && getStripVisibilityStateSupplier().get() == StripVisibilityState.VISIBLE) {
            mEventFilter.setEventArea(mStripFilterArea);
        }
    }

    // Implements TabStripTransitionDelegate.

    @Override
    public void onHeightChanged(int newHeightPx, boolean applyScrimOverlay) {
        if (applyScrimOverlay && !isFadeTransitionRunning()) {
            mIsHeightTransitioning = true;
            boolean hideStrip = newHeightPx == 0;
            mStripTransitionScrimOpacity = hideStrip ? 0f : 1f;
            // Update the strip visibility state in StatusBarController just after the margins are
            // updated during a hide->show transition so that the status bar assumes the base tab
            // strip color for the remaining duration of the transition while a scrim is applied.
            if (!hideStrip) {
                mStatusBarColorController.setTabStripHiddenOnTablet(false);
            }
            // Set the status bar color and scrim overlay at the start of the transition.
            mStatusBarColorController.setTabStripColorOverlay(
                    getStripTransitionScrimColor(), mStripTransitionScrimOpacity);
            // The height transition is running to update strip visibility. Ensure that any stale
            // state set by a previous fade transition is cleared at this time.
            setStripVisibilityState(StripVisibilityState.HIDDEN_BY_HEIGHT_TRANSITION, !hideStrip);
            setStripVisibilityState(StripVisibilityState.HIDDEN_BY_FADE, /* clear= */ true);
        }

        if (mIsHeaderCustomizationSupported) {
            // Convert the input HeightPx to Dp.
            mHeight = newHeightPx / mDensity;

            // TODO(crbug/331490430): Revisit how we position the scrollable strip.
            mTopPadding = Math.max(0, mHeight - mScrollableStripHeight);
            onSizeChanged(mWidth, mHeight, mLastVisibleViewportOffsetY, mOrientation);
        }
    }

    @Override
    public void onLayerYOffsetChanged(int yOffsetPx, int visibleHeightPx) {
        float yOffsetDp = yOffsetPx / mDensity;
        float visibleHeightDp = visibleHeightPx / mDensity;

        // If yOffset does not change (e.g. other layers are moving), no need to push for update.
        if (mSceneLayerYOffset != yOffsetDp || mSceneLayerVisibleHeight != visibleHeightDp) {
            mSceneLayerYOffset = yOffsetDp;
            mSceneLayerVisibleHeight = visibleHeightDp;
            pushAndUpdateStrip(mSceneLayerYOffset, mSceneLayerVisibleHeight);
            updateStripBottomPx();
        }
    }

    @Override
    public void onFadeTransitionRequested(float newOpacity, int durationMs) {
        // Opacity is already the desired value, return early.
        if (newOpacity == mStripTransitionScrimOpacity) return;

        if (mIsHeightTransitioning) {
            // If a height transition is currently running to update the scrim when a fade
            // transition is also requested, the fade transition should be prioritized to update the
            // strip visibility so immediately set this boolean to false to avoid a race to update
            // the strip scrim opacity.
            mIsHeightTransitioning = false;
        }
        boolean showStrip = newOpacity == 0f;

        // Update the status bar color to ensure that it reflects the current strip visibility state
        // and to override any stale value impacted by visibility set during a previous height
        // transition.
        mStatusBarColorController.setTabStripHiddenOnTablet(!showStrip);
        mStatusBarColorController.setTabStripColorOverlay(
                getStripTransitionScrimColor(), newOpacity);

        if (isFadeTransitionRunning()) {
            mFadeTransitionAnimator.cancel();
        }
        mFadeTransitionAnimator =
                CompositorAnimator.ofFloatProperty(
                        mUpdateHost.getAnimationHandler(),
                        this,
                        StripLayoutHelperManager.SCRIM_OPACITY,
                        mStripTransitionScrimOpacity,
                        newOpacity,
                        durationMs);
        mFadeTransitionAnimator.addListener(
                new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        onFadeTransitionEnd(showStrip);
                    }
                });
        mFadeTransitionAnimator.start();
    }

    private void onFadeTransitionEnd(boolean showStrip) {
        assert !mIsHeightTransitioning
                : "Height transition to update the scrim should not be running when a fade"
                        + " transition is finishing.";
        mFadeTransitionAnimator = null;
        // The fade transition is running to update strip visibility. Ensure that any stale
        // state set by a previous height transition is cleared at this time.
        setStripVisibilityState(StripVisibilityState.HIDDEN_BY_FADE, showStrip);
        setStripVisibilityState(
                StripVisibilityState.HIDDEN_BY_HEIGHT_TRANSITION, /* clear= */ true);
    }

    @EnsuresNonNullIf("mFadeTransitionAnimator")
    private boolean isFadeTransitionRunning() {
        return mFadeTransitionAnimator != null && mFadeTransitionAnimator.isRunning();
    }

    @Override
    public void onHeightTransitionFinished(boolean success) {
        if (!mIsHeightTransitioning) return;

        assert !isFadeTransitionRunning()
                : "Fade transition should not be running when a height transition to update the"
                        + " scrim is finishing.";
        mIsHeightTransitioning = false;
        mStripTransitionScrimOpacity = 0f;
        // Update the strip visibility state in StatusBarColorController only after a show->hide
        // transition, so that the status bar assumes the toolbar color when the strip is hidden.
        if ((getStripVisibilityStateSupplier().get()
                        & StripVisibilityState.HIDDEN_BY_HEIGHT_TRANSITION)
                != 0) {
            mStatusBarColorController.setTabStripHiddenOnTablet(true);
        }
        mStatusBarColorController.setTabStripColorOverlay(
                Color.TRANSPARENT, mStripTransitionScrimOpacity);
    }

    @Override
    public boolean isHiddenByFadeTransition() {
        return (getStripVisibilityStateSupplier().get() & StripVisibilityState.HIDDEN_BY_FADE) != 0;
    }

    @Override
    public int getFadeTransitionThresholdDp() {
        if (mTabModelSelector == null) return 0;
        boolean shouldShowGlic = isGlicButtonEnabled() && !mIsIncognito;
        TabModel incognitoTabModel = mTabModelSelector.getModel(/* incognito= */ true);
        boolean hasIncognitoTabs = incognitoTabModel != null && incognitoTabModel.getCount() > 0;
        boolean shouldShowMsb = !IncognitoUtils.shouldOpenIncognitoAsWindow() && hasIncognitoTabs;

        // Tablet: 332 = 2 * minTabWidth(108) - tabOverlap(28) + newTabButton (48) +
        // [optional] glicButton(48) + [optional] modelSelectorButton(48)
        // Desktop: 220 = 2 * minTabWidth(76) - tabOverlap(28) + newTabButton (32) +
        // [optional] glicButton(32) + [optional] modelSelectorButton(32)
        float thresholdDp =
                (2 * MIN_TAB_WIDTH_DP)
                        - TAB_OVERLAP_WIDTH_DP
                        + BUTTON_TOUCH_TARGET_SIZE_DP
                        + (shouldShowGlic ? BUTTON_TOUCH_TARGET_SIZE_DP : 0f)
                        + (shouldShowMsb ? BUTTON_TOUCH_TARGET_SIZE_DP : 0f);
        return Math.round(thresholdDp);
    }

    private boolean duringTabStripHeightTransition() {
        return mIsHeightTransitioning;
    }

    @VisibleForTesting
    float calculateScrimOpacityDuringHeightTransition(float visibleHeight) {
        if (!duringTabStripHeightTransition()) {
            return 0.0f;
        }

        // Otherwise, the alpha fraction is based on the percent of the tab strip visibility.
        // Use mScrollableStripHeight as the baseline height because mHeight may have already
        // changed during a height transition to hide the strip.
        float divisor = mHeight > 0 ? mHeight : mScrollableStripHeight;
        float ratio = 1 - visibleHeight / divisor;
        float newOpacity = TAB_STRIP_TRANSITION_INTERPOLATOR.getInterpolation(ratio);
        boolean isHidden =
                (getStripVisibilityStateSupplier().get()
                                & StripVisibilityState.HIDDEN_BY_HEIGHT_TRANSITION)
                        != 0;

        // There is a known issue where the scrim opacity for a hide->show transition incorrectly
        // gets updated to 1f (when yOffset = 0) in concluding frame updates during the transition,
        // thereby making the transition janky (b/324130906). This could be due to frame updates
        // initiated potentially by other sources before a timely dispatch of #onTransitionFinished.
        // The following logic is to prevent such jank from surfacing in both directions of
        // transition.
        // If the tab strip is hiding, new opacity should be >= current opacity; if the tab strip is
        // showing, new opacity should be <= current opacity. Otherwise, ignore the new value and
        // use the current value.
        if ((isHidden && newOpacity >= mStripTransitionScrimOpacity)
                || (!isHidden && newOpacity <= mStripTransitionScrimOpacity)) {
            mStripTransitionScrimOpacity = newOpacity;
        }

        return mStripTransitionScrimOpacity;
    }

    @Override
    public void updateOffsetTagsInfo(@Nullable BrowserControlsOffsetTagsInfo offsetTagsInfo) {
        // LINT.IfChange(updateOffsetTagsInfo)
        if (offsetTagsInfo != null) {
            // Use the content OffsetTag here, because the tab strip and content are part of
            // the same subtree and move together with the same offset. We need to release the
            // content offset tag from the static layout first before adding it to the tab strip.
            mStaticLayoutNeedsOffsetTagSupplier.set(false);
            // TODO CHR 146: This will crash the browser. Figure out why.
            // mTabStripTreeProvider.updateOffsetTag(offsetTagsInfo.getContentOffsetTag());
        } else {
            mTabStripTreeProvider.updateOffsetTag(null);
            mStaticLayoutNeedsOffsetTagSupplier.set(true);
        }
        // LINT.ThenChange(chrome/android/java/src/org/chromium/chrome/browser/compositor/layouts/StaticLayout.java:updateOffsetTag)
    }

    public NonNullObservableSupplier<Boolean> getLayoutNeedOffsetTagSupplier() {
        return mStaticLayoutNeedsOffsetTagSupplier;
    }

    @Override
    public void onTopResumedActivityChanged(boolean isTopResumedActivity) {
        // TODO (crbug/328055199): Check if losing focus to a non-Chrome task.
        if (!mIsHeaderCustomizationSupported) return;
        mIsTopResumedActivity = isTopResumedActivity;

        updateGlicButtonOpacity();

        mUpdateHost.requestUpdate();
    }

    private float getGlicButtonWidthWithEndPadding() {
        if (mGlicButton == null) return 0.f;
        return mGlicButton.getWidth() + mStripEndPadding + GLIC_ALIGNMENT_OFFSET_DP;
    }

    /**
     * @return The start padding needed for Glic button to ensure there is enough space for touch
     *     target.
     */
    private float getGlicButtonStartPaddingForTouchTarget() {
        if (mGlicButton != null && mGlicButton.isVisible()) {
            return mStripEndPadding + GLIC_ALIGNMENT_OFFSET_DP;
        } else {
            return 0.f;
        }
    }

    private float getModelSelectorButtonWidthWithEndPadding() {
        return MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP + mStripEndPadding;
    }

    /**
     * @return The start padding needed for model selector button to ensure there is enough space
     *     for touch target.
     */
    private float getMsbStartPaddingForTouchTarget() {
        if (mModelSelectorButton != null && mModelSelectorButton.isVisible()) {
            return BUTTON_DESIRED_TOUCH_TARGET_SIZE
                    - mModelSelectorButton.getWidth()
                    - mStripEndPadding;
        } else {
            return 0.f;
        }
    }

    public TintedCompositorButton getNewTabButton() {
        return getActiveStripLayoutHelper().getNewTabButton();
    }

    public @ColorInt int getSelectedOutlineGroupTint(int id, boolean shouldShowOutline) {
        return getActiveStripLayoutHelper().getSelectedOutlineGroupTint(id, shouldShowOutline);
    }

    public boolean shouldShowTabOutline(StripLayoutTab tab) {
        return getActiveStripLayoutHelper().shouldShowTabOutline(tab);
    }

    /**
     * @return The touch target offset to be applied to the new tab button.
     */
    public float getNewTabBtnVisualOffset() {
        return getActiveStripLayoutHelper().getNewTabButtonVisualOffset();
    }

    public @Nullable TintedCompositorTextButton getGlicButton() {
        return mGlicButton;
    }

    public @Nullable CompositorButton getModelSelectorButton() {
        return mModelSelectorButton;
    }

    @Override
    public void getVirtualViews(List<VirtualView> views) {
        if (duringTabStripHeightTransition()
                || getStripVisibilityStateSupplier().get() != StripVisibilityState.VISIBLE) {
            return;
        }
        // Remove the a11y views when top controls is partially invisible.
        if (mBrowserControlsStateProvider.getTopControlOffset() < 0) return;

        getActiveStripLayoutHelper().getVirtualViews(views);
        if (mGlicButton != null && mGlicButton.isVisible()) {
            views.add(mGlicButton);
        }
        if (mModelSelectorButton != null && mModelSelectorButton.isVisible()) {
            views.add(mModelSelectorButton);
        }
    }

    /** Allow / disallow system gestures on touchable areas on the strip. */
    private void updateTouchableAreas() {
        if (!mIsHeaderCustomizationSupported) return;

        if ((getStripVisibilityStateSupplier().get() & StripVisibilityState.HIDDEN_BY_FADE) != 0) {
            // Reset the system gesture exclusion rects to allow system gestures on the tab strip
            // area.
            mToolbarControlContainer.setSystemGestureExclusionRects(List.of(new Rect(0, 0, 0, 0)));
            return;
        }

        // #setSystemGestureExclusionRects allows Chrome to receive touch events on the tab strip
        // when it is drawn under the system gesture area so that the strip remains accessible.
        List<Rect> rects = new ArrayList<>();
        RectF tabStripRectDp = new RectF(getActiveStripLayoutHelper().getTouchableRect());
        tabStripRectDp.top = mTopPadding;
        tabStripRectDp.bottom = mHeight;

        Rect tabStripRect =
                new Rect(
                        (int) Math.floor(tabStripRectDp.left * mDensity),
                        (int) Math.floor(tabStripRectDp.top * mDensity),
                        (int) Math.ceil(tabStripRectDp.right * mDensity),
                        (int) Math.ceil(tabStripRectDp.bottom * mDensity));
        rects.add(tabStripRect);

        TintedCompositorButton ntb = getNewTabButton();
        if (ntb != null && ntb.isVisible()) {
            var ntbTouchRect = new RectF();
            ntb.getTouchTarget(ntbTouchRect);
            // The click slop in `CompositorButton` can extend the touchable region of the new
            // tab button into the `mTopPadding` region, so the "top" coordinate  of `ntbRect`
            // intentionally isn't bound by `mTopPadding`. Doing so causes an inaccurate region
            // to ultimately be reported in `setSystemGestureExclusionRects()`.
            Rect ntbRect =
                    new Rect(
                            (int) Math.floor(ntbTouchRect.left * mDensity),
                            (int) Math.floor(ntbTouchRect.top * mDensity),
                            (int) Math.ceil(ntbTouchRect.right * mDensity),
                            (int) Math.ceil(ntbTouchRect.bottom * mDensity));
            rects.add(ntbRect);
        }

        if (mGlicButton != null && mGlicButton.isVisible()) {
            var glicTouchRect = new RectF();
            mGlicButton.getTouchTarget(glicTouchRect);
            Rect glicRect =
                    new Rect(
                            (int) Math.floor(glicTouchRect.left * mDensity),
                            (int) Math.floor(Math.max(glicTouchRect.top, mTopPadding) * mDensity),
                            (int) Math.ceil(glicTouchRect.right * mDensity),
                            (int) Math.ceil(Math.min(glicTouchRect.bottom, mHeight) * mDensity));
            rects.add(glicRect);
        }

        if (mModelSelectorButton != null && mModelSelectorButton.isVisible()) {
            var msbTouchRect = new RectF();
            mModelSelectorButton.getTouchTarget(msbTouchRect);
            Rect msbRect =
                    new Rect(
                            (int) Math.floor(msbTouchRect.left * mDensity),
                            (int) Math.floor(Math.max(msbTouchRect.top, mTopPadding) * mDensity),
                            (int) Math.ceil(msbTouchRect.right * mDensity),
                            (int) Math.ceil(Math.min(msbTouchRect.bottom, mHeight) * mDensity));
            rects.add(msbRect);
        }
        mToolbarControlContainer.setSystemGestureExclusionRects(rects);
    }

    /**
     * @return The opacity to use for the fade on the left side of the tab strip.
     */
    public float getLeftFadeOpacity() {
        return getActiveStripLayoutHelper().getLeftFadeOpacity();
    }

    /**
     * @return The opacity to use for the fade on the right side of the tab strip.
     */
    public float getRightFadeOpacity() {
        return getActiveStripLayoutHelper().getRightFadeOpacity();
    }

    public int getLeftFadeDrawable() {
        int leftFadeDrawable;
        if (LocalizationUtils.isLayoutRtl()) {
            if (mModelSelectorButton != null && mModelSelectorButton.isVisible()) {
                leftFadeDrawable = R.drawable.tab_strip_fade_long;
                mNormalHelper.setLeftFadeWidth(FADE_LONG_WIDTH_DP);
                mIncognitoHelper.setLeftFadeWidth(FADE_LONG_WIDTH_DP);
            } else {
                // Use fade_medium for left fade when RTL and model selector button not
                // visible.
                leftFadeDrawable = R.drawable.tab_strip_fade_medium;
                mNormalHelper.setLeftFadeWidth(FADE_MEDIUM_WIDTH_DP);
                mIncognitoHelper.setLeftFadeWidth(FADE_MEDIUM_WIDTH_DP);
            }
        } else {
            leftFadeDrawable = R.drawable.tab_strip_fade_short;
            mNormalHelper.setLeftFadeWidth(FADE_SHORT_WIDTH_DP);
            mIncognitoHelper.setLeftFadeWidth(FADE_SHORT_WIDTH_DP);
        }
        return leftFadeDrawable;
    }

    public @DrawableRes int getRightFadeDrawable() {
        @DrawableRes int rightFadeDrawable;
        // Note(david@vivaldi.com): In order to get more space between the new tab button and the
        // tabs we always set the fade width to medium. See ref. VAB-10777.
        if (ChromeApplicationImpl.isVivaldi()) {
            rightFadeDrawable = R.drawable.tab_strip_fade_medium;
            mNormalHelper.setRightFadeWidth(FADE_MEDIUM_WIDTH_DP);
            mIncognitoHelper.setRightFadeWidth(FADE_MEDIUM_WIDTH_DP);
            return rightFadeDrawable;
        }
        if (!LocalizationUtils.isLayoutRtl()) {
            if (mModelSelectorButton != null && mModelSelectorButton.isVisible()) {
                rightFadeDrawable = R.drawable.tab_strip_fade_long;
                mNormalHelper.setRightFadeWidth(FADE_LONG_WIDTH_DP);
                mIncognitoHelper.setRightFadeWidth(FADE_LONG_WIDTH_DP);
            } else {
                // Use fade_medium for right fade when model selector button not visible.
                rightFadeDrawable = R.drawable.tab_strip_fade_medium;
                mNormalHelper.setRightFadeWidth(FADE_MEDIUM_WIDTH_DP);
                mIncognitoHelper.setRightFadeWidth(FADE_MEDIUM_WIDTH_DP);
            }
        } else {
            rightFadeDrawable = R.drawable.tab_strip_fade_short;
            mNormalHelper.setRightFadeWidth(FADE_SHORT_WIDTH_DP);
            mIncognitoHelper.setRightFadeWidth(FADE_SHORT_WIDTH_DP);
        }
        return rightFadeDrawable;
    }

    /** Returns drag listener for tab strip. */
    public @Nullable OnDragListener getDragListener() {
        return mTabStripDragHandler;
    }

    void setGlicButtonVisibleForTesting(boolean isVisible) {
        assumeNonNull(mGlicButton).setVisible(isVisible);
    }

    void setModelSelectorButtonVisibleForTesting(boolean isVisible) {
        assumeNonNull(mModelSelectorButton).setVisible(isVisible);
    }

    /** Update the title cache for the available tabs in the model. */
    private void updateTitleCacheForInit() {
        LayerTitleCache titleCache = mLayerTitleCacheSupplier.get();
        if (mTabModelSelector == null || titleCache == null) return;

        // Make sure any tabs already restored get loaded into the title cache.
        List<TabModel> models = mTabModelSelector.getModels();
        for (int i = 0; i < models.size(); i++) {
            TabModel model = models.get(i);
            for (Tab tab : model) {
                if (tab != null) {
                    titleCache.getUpdatedTitle(
                            tab, tab.getContext().getString(R.string.tab_loading_default_title));
                }
            }
        }
    }

    /**
     * Sets the TabModelSelector that this StripLayoutHelperManager will visually represent, and
     * various objects associated with it.
     *
     * @param modelSelector The TabModelSelector to visually represent.
     * @param tabCreatorManager The TabCreatorManager, used to create new tabs.
     */
    public void setTabModelSelector(
            TabModelSelector modelSelector, TabCreatorManager tabCreatorManager) {
        if (mTabModelSelector == modelSelector) return;

        mTabModelObserver =
                new TabModelObserver() {
                    @Override
                    public void didAddTab(
                            Tab tab,
                            @TabLaunchType int launchType,
                            @TabCreationState int creationState,
                            boolean markedForSelection) {
                        updateTitleForTab(tab);
                    }
                };
        modelSelector.addTabGroupModelFilterObserver(mTabModelObserver);

        mTabModelSelector = modelSelector;

        updateTitleCacheForInit();

        if (mTabModelSelector.isTabStateInitialized()) {
            updateStripButtons();
        } else {
            mTabModelSelector.addObserver(
                    new TabModelSelectorObserver() {
                        @Override
                        public void onTabStateInitialized() {
                            updateStripButtons();
                            // mTabModelSelector should be non-null because it is set to non-null
                            // `modelSelector` parameter in enclosing function `setTabModelSelector`
                            new Handler().post(() -> mTabModelSelector.removeObserver(this));

                            mNormalHelper.onTabStateInitialized();
                            mIncognitoHelper.onTabStateInitialized();
                        }
                    });
        }

        // Note(david@vivaldi.com): Always start off with the correct background color as the tab
        // strip will be recreated when themes has been changed (see VAB-2809).
        if (modelSelector.getCurrentTab() != null)
            mTabStripTreeProvider.setTabStripBackgroundColor(
                    getBackgroundStripColor(modelSelector.getCurrentTab().getThemeColor()));

        boolean tabStateInitialized = mTabModelSelector.isTabStateInitialized();
        mNormalHelper.setTabModel(
                mTabModelSelector.getModel(false),
                tabCreatorManager.getTabCreator(false),
                tabStateInitialized);
        mIncognitoHelper.setTabModel(
                mTabModelSelector.getModel(true),
                tabCreatorManager.getTabCreator(true),
                tabStateInitialized);
        mNormalHelper.setTabGroupModelFilter(mTabModelSelector.getModel(false));
        mIncognitoHelper.setTabGroupModelFilter(mTabModelSelector.getModel(true));
        tabModelSwitched(mTabModelSelector.isIncognitoSelected());
        // Manually called on initialization, since the logic in #tabModelSwitched only runs if the
        // Incognito state actually changes. Since mIncognito defaults to false, it may not actually
        // change on initialization.
        getActiveStripLayoutHelper().setSelected(/* selected= */ true);

        mTabModelSelectorTabModelObserver =
                new TabModelSelectorTabModelObserver(modelSelector) {
                    /**
                     * @return The actual current time of the app in ms.
                     */
                    public long time() {
                        return SystemClock.uptimeMillis();
                    }

                    @Override
                    public void willCloseTab(Tab tab, boolean didCloseAlone) {
                        getStripLayoutHelper(tab.isIncognitoBranded()).willCloseTab(tab);
                    }

                    @Override
                    public void tabRemoved(Tab tab) {
                        getStripLayoutHelper(tab.isIncognitoBranded()).tabClosed(tab);
                        updateStripButtons();
                    }

                    @Override
                    public void didMoveTab(Tab tab, int newIndex, int curIndex) {
                        // For right-direction move, layout helper re-ordering logic
                        // expects destination index = position + 1
                        getStripLayoutHelper(tab.isIncognitoBranded())
                                .tabMoved(
                                        tab.getId(),
                                        curIndex,
                                        newIndex > curIndex ? newIndex + 1 : newIndex);
                    }

                    @Override
                    public void tabClosureUndone(Tab tab) {
                        getStripLayoutHelper(tab.isIncognitoBranded())
                                .tabClosureCancelled(time(), tab.getId());
                        updateStripButtons();
                    }

                    @Override
                    public void tabClosureCommitted(Tab tab) {
                        LayerTitleCache titleCache = mLayerTitleCacheSupplier.get();
                        if (titleCache != null) {
                            titleCache.removeTabTitle(tab.getId());
                        }
                    }

                    @Override
                    public void onTabClosePending(
                            List<Tab> tabs,
                            boolean isAllTabs,
                            @TabClosingSource int closingSource) {
                        if (tabs.isEmpty()) return;
                        getStripLayoutHelper(tabs.get(0).isIncognitoBranded())
                                .multipleTabsClosed(tabs);
                        updateStripButtons();
                    }

                    @Override
                    public void onFinishingTabClosure(
                            Tab tab, @TabClosingSource int closingSource) {
                        getStripLayoutHelper(tab.isIncognitoBranded()).tabClosed(tab);
                        updateStripButtons();
                    }

                    @Override
                    public void willCloseAllTabs(boolean incognito) {
                        getStripLayoutHelper(incognito).willCloseAllTabs();
                        updateStripButtons();
                    }

                    @Override
                    public void didSelectTab(Tab tab, @TabSelectionType int type, int lastId) {
                        if (tab.getId() == lastId) return;
                        getStripLayoutHelper(tab.isIncognitoBranded())
                                .tabSelected(time(), tab.getId(), lastId);
                        // Vivaldi: Update previous and selected tab title.
                        Tab previousTab = mTabModelSelector.getTabById(lastId);
                        if (previousTab != null) updateTitleForTab(previousTab);
                        updateTitleForTab(tab);
                    }

                    @Override
                    public void didAddTab(
                            Tab tab,
                            @TabLaunchType int type,
                            @TabCreationState int creationState,
                            boolean markedForSelection) {
                        boolean onStartup = type == TabLaunchType.FROM_RESTORE;
                        getStripLayoutHelper(tab.isIncognitoBranded())
                                .tabCreated(
                                        time(), tab.getId(), markedForSelection, false, onStartup);

                        // Vivaldi
                        updateStripButtons();
                    }
                };

        mTabModelSelectorTabObserver =
                new TabModelSelectorTabObserver(modelSelector) {
                    @Override
                    public void onLoadUrl(
                            Tab tab, LoadUrlParams params, LoadUrlResult loadUrlResult) {
                        if (params.getTransitionType() == PageTransition.HOME_PAGE
                                || (params.getTransitionType() & PageTransition.FROM_ADDRESS_BAR)
                                        == PageTransition.FROM_ADDRESS_BAR) {
                            getStripLayoutHelper(tab.isIncognito())
                                    .scrollTabToView(LayoutManagerImpl.time(), false);
                        }
                    }

                    @Override
                    public void onLoadStarted(Tab tab, boolean toDifferentDocument) {
                        getStripLayoutHelper(tab.isIncognito()).tabLoadStarted(tab.getId());
                    }

                    @Override
                    public void onLoadStopped(Tab tab, boolean toDifferentDocument) {
                        getStripLayoutHelper(tab.isIncognito()).tabLoadFinished(tab.getId());
                    }

                    @Override
                    public void onCrash(Tab tab) {
                        getStripLayoutHelper(tab.isIncognito()).tabLoadFinished(tab.getId());
                    }

                    @Override
                    public void onTitleUpdated(Tab tab) {
                        updateTitleForTab(tab);
                    }

                    @Override
                    public void onFaviconUpdated(
                            Tab tab, @Nullable Bitmap icon, @Nullable GURL iconUrl) {
                        updateTitleForTab(tab);
                    }

                    @Override
                    public void onOffsetTagsInfoChanged(
                            Tab tab,
                            BrowserControlsOffsetTagsInfo oldOffsetTagsInfo,
                            BrowserControlsOffsetTagsInfo offsetTagsInfo,
                            @BrowserControlsState int constraints) {
                        if (!BrowserControlsUtils.isTopControlsRefactorOffsetEnabled()) {
                            updateOffsetTagsInfo(offsetTagsInfo);
                        }
                    }

                    @Override
                    public void onMediaStateChanged(Tab tab, @MediaState int mediaState) {
                        getStripLayoutHelper(tab.isIncognito())
                                .onMediaStateChanged(tab, mediaState);
                        mRenderHost.requestRender();
                    }

                    // Vivaldi
                    @Override
                    public void onDidChangeThemeColor(Tab tab, int color) {
                        if (mTabModelSelector.getCurrentTab() == tab)
                            mTabStripTreeProvider.setTabStripBackgroundColor(
                                    getBackgroundStripColor(color));
                    } // End Vivaldi
                };

        // Vivaldi
        new ActivityTabProvider.ActivityTabTabObserver(mActivity.getActivityTabProvider()) {
            @Override
            public void onObservingDifferentTab(@Nullable Tab tab) {
                if (tab != null  && mTabStripTreeProvider != null) // Vivaldi VAB-10496
                    mTabStripTreeProvider.setTabStripBackgroundColor(
                            getBackgroundStripColor(tab.getThemeColor()));
            }
        }; // End Vivaldi

        mTabModelSelector
                .getCurrentTabModelSupplier()
                .addSyncObserverAndPostIfNonNull(mCurrentTabModelObserver);
        if (mTabStripDragHandler != null) {
            mTabStripDragHandler.setTabModelSelector(mTabModelSelector);
        }

        // Note(david@vivaldi.com): The main tab strip and the tab stack strip can have different
        // tab titles. Because of this, we need to create a separate |layerTitleCache| for the each
        // tab strip. We also pass the tab model selector to the internal |StripLayoutHelper|s.
        LayerTitleCache layerTitleCache =
                new LayerTitleCache(mContext, mActivity.getLayoutManager().getResourceManager(),
                        mToolbarManager.getTabStripHeightSupplier().get(), mTabModelSelector);
        layerTitleCache.setIsStackStrip(mIsStackStrip);
        mLayerTitleCacheSupplier.set(layerTitleCache);
        mNormalHelper.setTabModelSelector(mTabModelSelector);
        mIncognitoHelper.setTabModelSelector(mTabModelSelector);
    }

    @Override
    public void onAppHeaderStateChanged(AppHeaderState newState) {
        assert mDesktopWindowStateManager != null;
        // We do not update the layer's height in this method. The height adjustment will be
        // triggered by #onHeightChanged.

        mDesktopWindowStateManager.updateForegroundColor(getBackgroundColor());
        updateHorizontalPaddings(newState.getLeftPadding(), newState.getRightPadding());

        updateGlicButtonOpacity();
    }

    /**
     * Update the start / end padding for the tab strip.
     *
     * @param leftPaddingPx Left padding for the tab strip in px.
     * @param rightPaddingPx Right padding for the tab strip in px.
     */
    private void updateHorizontalPaddings(int leftPaddingPx, int rightPaddingPx) {
        mLeftPadding = leftPaddingPx / mDensity;
        mRightPadding = rightPaddingPx / mDensity;

        onSizeChanged(mWidth, mHeight, mLastVisibleViewportOffsetY, mOrientation);
    }

    public void updateTitleForTab(Tab tab) { // Vivaldi
        LayerTitleCache layerCache = mLayerTitleCacheSupplier.get();
        if (layerCache == null) return;

        String title = layerCache.getUpdatedTitle(tab, mDefaultTitle);
        getStripLayoutHelper(tab.isIncognito()).tabTitleChanged(tab.getId(), title);
        mUpdateHost.requestUpdate();
    }

    public float getHeight() {
        return mHeight;
    }

    public float getWidth() {
        return mWidth;
    }

    public @ColorInt int getBackgroundColor() {
        return TabUiThemeUtil.getTabStripBackgroundColor(
                mContext, mIsIncognito, isAppInDesktopWindow(), mIsTopResumedActivity);
    }

    /**
     * Returns the tint color for a given media state.
     *
     * @param mediaState The {@link MediaState} for which to get the tint.
     * @param defaultTint The default tint to use.
     */
    public @ColorInt int getMediaIndicatorTintColor(
            @MediaState int mediaState, @ColorInt int defaultTint) {
        if (mediaState == MediaState.RECORDING) {
            return mContext.getColor(R.color.tab_recording_media_color);
        } else if (mediaState == MediaState.SHARING) {
            return mContext.getColor(R.color.tab_sharing_media_color);
        } else if (mediaState == MediaState.PICTURE_IN_PICTURE) {
            return mContext.getColor(R.color.tab_pip_media_color);
        }
        return defaultTint;
    }

    @Override
    public boolean updateOverlay(long time, long dt) {
        getInactiveStripLayoutHelper().finishAnimationsAndPushTabUpdates();
        boolean animationFinished = getActiveStripLayoutHelper().updateLayout(time);
        if (animationFinished) {
            // Update the touchable area when tab strip has an update on its layout. This is
            // probably an overkill, since the touch size does not change when the tab is full.
            // TODO(crbug/332957442): Reduce the call freq for this method.
            updateTouchableAreas();
        }
        return animationFinished;
    }

    private void tabModelSwitched(boolean incognito) {
        if (incognito == mIsIncognito) return;
        mIsIncognito = incognito;

        mIncognitoHelper.tabModelSelected(mIsIncognito);
        mNormalHelper.tabModelSelected(!mIsIncognito);

        updateStripButtons();

        // If we are in DW mode, notify DW state provider since the model changed.
        if (isAppInDesktopWindow()) {
            mDesktopWindowStateManager.updateForegroundColor(getBackgroundColor());
        }

        mManagerHost.resetKeyboardFocus(); // Reset virtual views index & keyboard focus state.
        mUpdateHost.requestUpdate();
    }

    private boolean isGlicButtonEnabled() {
    	if (ChromeApplicationImpl.isVivaldi()) return false;

        return ChromeFeatureList.sGlic.isEnabled();
    }

    @Override
    public void onSharedPreferenceChanged(
            SharedPreferences sharedPreferences, @Nullable String key) {
        if (ChromePreferenceKeys.GLIC_BUTTON_PINNED.equals(key)) {
            updateStripButtons();
        }
    }

    private void updateStripButtons() {
        // Use helper methods to calculate new visibility of strip buttons.
        boolean newGlicVisibility = !ChromeApplicationImpl.isVivaldi() && shouldGlicBeVisible();
        boolean newMsbVisibility = shouldMsbBeVisible();

        // Note (david@vivaldi.com): The ModelSelectorButton is always visible since this is our
        // new tab button but we don't occupy any margins with setting |isVisible| to false;
        if (ChromeApplicationImpl.isVivaldi() && mModelSelectorButton != null) {
            mModelSelectorButton.setVisible(true);
            // Vivaldi VAB-9775
            @ColorInt
            int iconIncognitoColor = mContext.getColor(R.color.default_icon_color_secondary_light);
            if (GlobalNightModeStateProviderHolder.getInstance().isInNightMode()) {
                @ColorInt
                int iconNightModeColor = mContext.getColor(R.color.default_bg_color_light);
                mModelSelectorButton.setTint(iconNightModeColor, iconNightModeColor, iconIncognitoColor, iconIncognitoColor);
            } else {
                @ColorInt
                int iconDefaultColor =
                        AppCompatResources.getColorStateList(mContext, R.color.default_bg_color_baseline)
                                .getDefaultColor();
                mModelSelectorButton.setTint(iconDefaultColor, iconDefaultColor, iconIncognitoColor, iconIncognitoColor);
            } // End Vivaldi VAB-9775
        } else { // Vivaldi

        // Early exit if visibility of both buttons hasn't changed.
        boolean glicChanged = mGlicButton != null && mGlicButton.isVisible() != newGlicVisibility;
        boolean msbChanged =
                mModelSelectorButton != null
                        && mModelSelectorButton.isVisible() != newMsbVisibility;
        if (!glicChanged && !msbChanged) return;

        // Set updated visibilities (of both buttons for simplicity).
        if (mGlicButton != null) mGlicButton.setVisible(newGlicVisibility);
        if (mModelSelectorButton != null) mModelSelectorButton.setVisible(newMsbVisibility);

        // The Glic button position depends on the MSB's visibility.
        if (msbChanged) {
            updateGlicButtonPosition();
        }

        } // End Vivaldi

        updateButtonMargins();
    }

    private void updateButtonMargins() {
        boolean isGlicVisible = !ChromeApplicationImpl.isVivaldi() && mGlicButton != null && mGlicButton.isVisible();
        boolean isMsbVisible = mModelSelectorButton != null && mModelSelectorButton.isVisible();

        // Calculate layout sizes and update margins. We use (width + end padding + start spacing)
        // to create a larger gap between buttons to meet touch target size requirements.
        float glicTouchTargetSize =
                isGlicVisible
                        ? (getGlicButtonWidthWithEndPadding()
                                + getGlicButtonStartPaddingForTouchTarget())
                        : 0.0f;
        float msbTouchTargetSize =
                isMsbVisible
                        ? (getModelSelectorButtonWidthWithEndPadding()
                                + getMsbStartPaddingForTouchTarget())
                        : 0.0f;

        // Note(david@vivaldi.com): We do not have the glic button and this is also causing an ANR.
        // We return here.
        if (ChromeApplicationImpl.isVivaldi()) return;
        // In Incognito, glic is always hidden so use touch target size of 0.
        mNormalHelper.updateEndMarginForStripButtons(glicTouchTargetSize, msbTouchTargetSize);
        mIncognitoHelper.updateEndMarginForStripButtons(
                /* glicTouchTargetSize= */ 0.0f, msbTouchTargetSize);
    }

    private boolean shouldGlicBeVisible() {
        if (ChromeApplicationImpl.isVivaldi()) return false;

        if (mGlicButton == null || mIsIncognito) return false;
        return ChromeSharedPreferences.getInstance()
                .readBoolean(ChromePreferenceKeys.GLIC_BUTTON_PINNED, true);
    }

    public float getGlicButtonStartPadding() {
        return GLIC_BUTTON_START_PADDING_DP;
    }

    public float getGlicIconTextPadding() {
        return GLIC_ICON_TEXT_PADDING_DP;
    }

    public float getGlicButtonCornerRadius() {
        return GLIC_BUTTON_CORNER_RADIUS;
    }

    private boolean shouldMsbBeVisible() {
        if (mModelSelectorButton == null) return false;

        // Sync the incognito state whenever the button exists
        mModelSelectorButton.setIncognito(mIsIncognito);

        return mTabModelSelector != null && mTabModelSelector.getModel(true).getCount() != 0;
    }

    /**
     * @param incognito Whether or not you want the incognito StripLayoutHelper
     * @return The requested StripLayoutHelper.
     */
    @VisibleForTesting
    public StripLayoutHelper getStripLayoutHelper(boolean incognito) {
        return incognito ? mIncognitoHelper : mNormalHelper;
    }

    /**
     * @return The currently visible strip layout helper.
     */
    @VisibleForTesting
    public StripLayoutHelper getActiveStripLayoutHelper() {
        return getStripLayoutHelper(mIsIncognito);
    }

    private StripLayoutHelper getInactiveStripLayoutHelper() {
        return mIsIncognito ? mNormalHelper : mIncognitoHelper;
    }

    public NonNullObservableSupplier<@StripVisibilityState Integer>
            getStripVisibilityStateSupplier() {
        // TODO(crbug.com/417238089): get() returns a stale value during height transitions.
        return mStripVisibilityStateSupplier;
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PACKAGE_PRIVATE)
    public void setStripVisibilityState(@StripVisibilityState int visibilityState, boolean clear) {
        @StripVisibilityState int curVisibility = mStripVisibilityStateSupplier.get();
        mStripVisibilityStateSupplier.set(
                clear ? (curVisibility & ~visibilityState) : (curVisibility | visibilityState));
    }

    /** Returns a {@link NonNullObservableSupplier} for the bottom of the tab strip in px. */
    public NonNullObservableSupplier<Integer> getStripBottomPxSupplier() {
        return mStripBottomPxSupplier;
    }

    private void updateStripBottomPx() {
        @Px
        int tabStripBottomPx =
                Math.round(mDensity * (mSceneLayerYOffset + mSceneLayerVisibleHeight));
        mStripBottomPxSupplier.set(tabStripBottomPx);
    }

    void simulateHoverEventForTesting(int event, float x, float y) {
        if (event == MotionEvent.ACTION_HOVER_ENTER) {
            mTabStripEventHandler.onHoverEnter(x, y);
        } else if (event == MotionEvent.ACTION_HOVER_MOVE) {
            mTabStripEventHandler.onHoverMove(x, y);
        } else if (event == MotionEvent.ACTION_HOVER_EXIT) {
            mTabStripEventHandler.onHoverExit();
        }
    }

    void simulateOnDownForTesting(float x, float y, int buttons) {
        mTabStripEventHandler.onDown(x, y, buttons);
    }

    void setTabStripTreeProviderForTesting(TabStripSceneLayer tabStripTreeProvider) {
        mTabStripTreeProvider = tabStripTreeProvider;
    }

    ViewStub getTabHoverCardViewStubForTesting() {
        return mTabHoverCardViewStub;
    }

    public @Nullable TabStripDragHandler getTabStripDragHandlerForTesting() {
        return mTabStripDragHandler;
    }

    public void setIsIncognitoForTesting(boolean isIncognito) {
        mIsIncognito = isIncognito;
    }

    public boolean isStripScrimVisibleForTesting() {
        return mStripTransitionScrimOpacity == 1f;
    }

    /** Request keyboard focus on the tab strip. */
    public void requestKeyboardFocus() {
        mManagerHost.requestKeyboardFocus(this);
    }

    /**
     * @return Whether the tab strip contains keyboard focus.
     */
    public boolean containsKeyboardFocus() {
        return mManagerHost.containsKeyboardFocus(this);
    }

    /**
     * Opens the context menu for the currently keyboard-focused item, if applicable.
     *
     * @return Whether the context menu was successfully opened.
     */
    public boolean openKeyboardFocusedContextMenu() {
        return getActiveStripLayoutHelper().openKeyboardFocusedContextMenu();
    }

    /**
     * Reorders the currently keyboard-focused item, if applicable.
     *
     * @param toLeft Whether the focused item should be reordered to the left (note: this is still
     *     left in RTL).
     * @return Whether the item was successfully reordered.
     */
    public boolean reorderKeyboardFocusedItem(boolean toLeft) {
        return getActiveStripLayoutHelper().moveSelectedStripView(toLeft);
    }

    /**
     * Toggles multiselection on the keyboard focused tab.
     *
     * @return Whether the multiselect action was successfully performed.
     */
    public boolean multiselectKeyboardFocusedItem() {
        return getActiveStripLayoutHelper().multiselectKeyboardFocusedItem();
    }

    private boolean isActivityInXrFullSpaceModeNow() {
        return mXrSpaceModeObservableSupplier != null && mXrSpaceModeObservableSupplier.get();
    }

    /** Vivaldi **/
    private void setIsStackStrip(boolean isStackStrip) {
        mIsStackStrip = isStackStrip;
        mTabStripTreeProvider.setIsStackStrip(mIsStackStrip);
        mNormalHelper.setIsStackStrip(mIsStackStrip);
        mIncognitoHelper.setIsStackStrip(mIsStackStrip);
    }

    /** Vivaldi **/
    private void createNewTab() {
        if (mTabModelSelector == null) return;
        TabModel currentTabModel = mTabModelSelector.getCurrentModel();
        if (currentTabModel == null) return;
        if (!currentTabModel.isIncognitoBranded()) currentTabModel.commitAllTabClosures();
        // Open tab within a group.
        if (mIsStackStrip) {
            TabGroupModelFilter tabGroupModelFilter =
                    assertNonNull(mTabModelSelector.getCurrentTabGroupModelFilter());
            List<Tab> relatedTabs =
                    tabGroupModelFilter.getRelatedTabList(mTabModelSelector.getCurrentTabId());
            assert !relatedTabs.isEmpty();
            Tab parentTabToAttach = relatedTabs.get(relatedTabs.size() - 1);
            mActivity.getTabCreator(currentTabModel.isIncognito())
                    .createNewTab(new LoadUrlParams(UrlConstants.NTP_URL),
                            TabLaunchType.FROM_TAB_GROUP_UI, parentTabToAttach);
        } else // Open a normal one.
            TabCreatorUtil.launchNtp(
                    mActivity.getTabCreator(mTabModelSelector.isIncognitoBrandedModelSelected()));
    }

    /** Vivaldi **/
    public boolean isSceneOffScreen() {
        boolean isOffScreen = mActivity.getBrowserControlsManager().areBrowserControlsAtMinHeight()
                || !VivaldiUtils.isTabStripOn();
        if (mActivity.isInOverviewMode()) return true;
        if (mIsStackStrip && !VivaldiUtils.isTabStackVisible()) return true;
        return isOffScreen;
    }

    /** Vivaldi **/
    private boolean isNewTabButtonClicked(float x, float y) {
        if (VivaldiUtils.isTopToolbarOn()) {
            if (mIsStackStrip) y -= getHeight();
        } else {
            y += mIsStackStrip ? getHeight() : 0;
            y = mViewportHeightOffset - y; // consider the offset in case of bottom address bar
        }

        return (mModelSelectorButton != null) && mModelSelectorButton.checkClickedOrHovered(x, y);
    }

    /** Vivaldi - Handling long click event on + button in tab strip **/
    private void showTabSwitcherPopupMenu() {
        // For now we don't show the popup on the stack strip.
        if (mIsStackStrip) return;
        if (mTabModelSelector == null || mTabModelSelector.getCurrentModel() == null) return;
        Profile profile = mTabModelSelector.getCurrentModel().getProfile();
        if (profile == null) return;
        TabSwitcherActionMenuCoordinator menu = new TabSwitcherActionMenuCoordinator(
                profile,
                mActivity.getTabModelSelectorSupplier());
        View anchorView = mActivity.findViewById(R.id.tab_switcher_menu_helper_button);
        // Note(nagamani@vivaldi.com): We are using dummy button with 0dp width and 0dp height as
        // anchorView
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(0, 0);
        params.gravity = Gravity.TOP | Gravity.END;
        // Apply correct margin when we have double tab bar.
        int margin =
                !mIsStackStrip && VivaldiUtils.isTabStackVisible() ? -(int) getHeight() * 2 : 0;
        if (VivaldiUtils.isTopToolbarOn())
            params.topMargin = margin;
        else
            params.bottomMargin = margin;

        // Note(nagamani@vivaldi.com): change layout gravity according to address bar location
        if (!VivaldiUtils.isTopToolbarOn()) {
            params.gravity = Gravity.BOTTOM | Gravity.END;
        }
        anchorView.setLayoutParams(params);

        menu.displayMenu(mContext, (ListMenuButton) anchorView, menu.buildMenuItems(),
                (id) -> { mActivity.onOptionsItemSelected(id, null); });
    }

    /** Vivaldi - Function to return value of y with correct offset value **/
    private float getValueOfY(float y) {
        if (!VivaldiUtils.isTopToolbarOn()) {
            y += mIsStackStrip ? getHeight() : 0;
            return mViewportHeightOffset - y;
        }
        float dpToPx = (1.f / mActivity.getResources().getDisplayMetrics().density);
        if (mIsStackStrip) y -= getHeight();
        return y - (mActivity.getBrowserControlsManager().getTopControlsMinHeight() *dpToPx);
    }

    /** Vivaldi - Get the right background color for the current strip **/
    private int getBackgroundStripColor(int color) {
        int newColor = color;
        // The stack strip color is slightly darker.
        if (mIsStackStrip) newColor = ColorUtils.overlayColor(color, 0xFF000000, 0.2f);
        return newColor;
    }

    /** Vivaldi - Determines the y offset of the model selector button according to its setting **/
    private void determineModelSelectorButtonYOffset() {
        boolean isFloatingTabEnabled = VivaldiPreferences.getSharedPreferencesManager().readBoolean(
                VivaldiPreferences.ENABLE_FLOATING_TABS, true);
        if (isFloatingTabEnabled)
            MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP =
                    VivaldiUtils.isTopToolbarOn() ? 6.5f : 7.0f;
        else
            MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP =
                    VivaldiUtils.isTopToolbarOn() ? 10.5f : 5.5f;
    }

    /** Vivaldi VAB-12859 **/
    public void setVivaldiMiniBarActiveSupplier(
            NonNullObservableSupplier<Boolean> supplier) {
        mVivaldiMiniBarActiveSupplier = supplier;
        supplier.addObserver(
                active -> mUpdateHost.requestUpdate(),
                MonotonicObservableSupplier.NotifyBehavior.NONE);
    }
    // End Vivaldi
}
