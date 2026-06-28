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
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Handler;
import android.os.SystemClock;
import android.util.FloatProperty;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnDragListener;
import android.view.ViewStub;
import android.view.animation.Interpolator;

import androidx.annotation.ColorInt;
import androidx.annotation.Px;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.Callback;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.build.annotations.EnsuresNonNullIf;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.actor.ui.ActorUiTabController;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.browser_controls.BrowserControlsOffsetTagsInfo;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
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
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutHelper.TrailingButtonDelegate;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutView.StripLayoutViewOnClickHandler;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutView.StripLayoutViewOnKeyboardFocusHandler;
import org.chromium.chrome.browser.compositor.overlays.strip.reorder.TabStripDragHandler;
import org.chromium.chrome.browser.compositor.scene_layer.TabStripSceneLayer;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
/*import org.chromium.chrome.browser.glic.GlicButtonDelegate;
import org.chromium.chrome.browser.glic.GlicKeyedService; Vivaldi */
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
import org.chromium.chrome.browser.profiles.Profile;
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
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTaskTrackerFactory;
import org.chromium.chrome.browser.ui.desktop_windowing.AppHeaderUtils;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.side_panel.AndroidSidePanelEnabledFn;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.desktop_windowing.AppHeaderState;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager.AppHeaderObserver;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.components.browser_ui.widget.gesture.BackPressHandler;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.LocalizationUtils;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayUtil;
import org.chromium.ui.dragdrop.DragAndDropDelegate;
import org.chromium.ui.dragdrop.DragDropGlobalState;
import org.chromium.ui.interpolators.Interpolators;
import org.chromium.ui.resources.ResourceManager;
import org.chromium.ui.util.ColorUtils;
import org.chromium.ui.util.StyleUtils;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Supplier;

//Vivaldi
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ChromeApplicationImpl;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.overlay_panel.OverlayPanel;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.TabCreatorUtil;
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
                AppHeaderObserver {
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
    static final float BUTTON_DESIRED_TOUCH_TARGET_SIZE =
            StyleUtils.shouldApplyDesktopDensity() ? 32.f : 48.f;

    // Model selector button constants.
    private static float MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP = 3.f; // Vivaldi: not final.
    private static final float MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP = 24.f; // Vivaldi
    private static final float MODEL_SELECTOR_BUTTON_BACKGROUND_HEIGHT_DP = 24.f; // Vivaldi
    private static final float MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY = 0.12f;
    private static final float MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY = 0.08f;
    private static final float MODEL_SELECTOR_BUTTON_CLICK_SLOP_DP =
            (BUTTON_DESIRED_TOUCH_TARGET_SIZE - MODEL_SELECTOR_BUTTON_BACKGROUND_WIDTH_DP) / 2;

    // Tab strip transition constants.
    @VisibleForTesting
    static final Interpolator TAB_STRIP_TRANSITION_INTERPOLATOR =
            Interpolators.STANDARD_DEFAULT_EFFECTS;

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
    private final StripLayoutTrailingButtonsCoordinator mTrailingButtonsCoordinator;
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
    private final WindowAndroid mWindowAndroid;
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
    private final ActorUiTabController.Observer mActorObserver;

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
    private boolean mIsStackStrip;

    // Drag-Drop
    private @Nullable TabStripDragHandler mTabStripDragHandler;

    private class TabStripEventHandler implements AreaMotionEventHandler {
        @Override
        public void onDown(float x, float y, int buttons) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }
            if (mTrailingButtonsCoordinator.onDown(x, y, buttons)) {
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
            if (mTrailingButtonsCoordinator.onUpOrCancel()) {
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
            mTrailingButtonsCoordinator.drag(x, y);

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
            float tabWidthDp = getActiveStripLayoutHelper().getUnpinnedTabWidth();
            if (mTrailingButtonsCoordinator.click(time, x, y, buttons, modifiers, tabWidthDp)) {
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
            float tabWidthDp = getActiveStripLayoutHelper().getUnpinnedTabWidth();
            if (mTrailingButtonsCoordinator.onLongPress(x, y, tabWidthDp)) {
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

            boolean isTrailingHovered = mTrailingButtonsCoordinator.onHoverEvent(x, y);
            getActiveStripLayoutHelper().onHoverEnter(x, y, isTrailingHovered);
        }

        @Override
        public void onHoverMove(float x, float y) {
            if (DragDropGlobalState.hasValue()) {
                return;
            }

            y = getValueOfY(y); // Vivaldi - Adjust the offset in case of bottom address bar.

            boolean isTrailingHovered = mTrailingButtonsCoordinator.onHoverEvent(x, y);
            getActiveStripLayoutHelper().onHoverMove(x, y, isTrailingHovered);
        }

        @Override
        public void onHoverExit(boolean inArea) {
            getActiveStripLayoutHelper().onHoverExit(inArea);
            mTrailingButtonsCoordinator.onHoverExit();
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
     * @param tabBookmarkerSupplier Supplies {@link TabBookmarker} to add/edit bookmarks.
     * @param xrSpaceModeObservableSupplier Supplies current XR space mode status. True for XR full
     *     space mode, false otherwise.
     * @param backPressManager The {@link BackPressManager} for handling back press.
     * @param snackbarManager The {@link SnackbarManager} used to show snackbar UI.
     * @param activityResultTracker The {@link ActivityResultTracker}.
     * @param glicClickHandler The {@link GlicButtonDelegate} for the Glic button.
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
            Supplier<TabBookmarker> tabBookmarkerSupplier,
            @Nullable NonNullObservableSupplier<Boolean> xrSpaceModeObservableSupplier,
            BackPressManager backPressManager,
            SnackbarManager snackbarManager,
            @Nullable ActivityResultTracker activityResultTracker,
            /* GlicButtonDelegate glicClickHandler,
            @Nullable GlicKeyedService glicKeyedService) { // Vivaldi*/
            boolean isStackStrip) { // Vivaldi
        mContext = context;
        mWindowAndroid = windowAndroid;
        Resources res = context.getResources();
        mManagerHost = managerHost;
        mUpdateHost = updateHost;
        mRenderHost = renderHost;

        mActorObserver =
                state -> {
                    getStripLayoutHelper(false)
                            .onActuationStateChanged(state.tabId, state.tabIndicator);
                    mRenderHost.requestRender();
                };
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

        // Use toolbar menu button padding to align MSB with menu button.
        mStripEndPadding = res.getDimension(R.dimen.button_end_padding) / mDensity;

        StripLayoutViewOnKeyboardFocusHandler glicKeyboardFocusHandler =
                (isFocused, view) -> mRenderHost.requestRender();
        mTrailingButtonsCoordinator =
                new StripLayoutTrailingButtonsCoordinator(
                        context,
                        mUpdateHost,
                        mRenderHost,
                        mWindowAndroid,
                        //glicClickHandler, Vivaldi
                        mDensity,
                        mStripEndPadding,
                        toolbarContainerView,
                        glicKeyboardFocusHandler,
                        isAppInDesktopWindow(),
                        mIsTopResumedActivity,
                        //glicKeyedService, Vivaldi
                        ChromeAndroidTaskTrackerFactory.getInstance(),
                        () -> mIsIncognito,
                        () -> mTabModelSelector,
                        this::updateButtonMargins);

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

        TrailingButtonDelegate trailingButtonDelegate =
                new TrailingButtonDelegate() {
                    @Override
                    public boolean isMenuShowing() {
                        return mTrailingButtonsCoordinator.isMenuShowing();
                    }

                    @Override
                    public void dismissContextMenu() {
                        mTrailingButtonsCoordinator.dismissTrailingButtonsMenu();
                    }

                    @Override
                    public void fadeCompositorButtons(boolean fade) {
                        mTrailingButtonsCoordinator.fadeCompositorButtons(fade);
                    }
                };

        mNormalHelper =
                new StripLayoutHelper(
                        context,
                        this,
                        trailingButtonDelegate,
                        managerHost,
                        updateHost,
                        renderHost,
                        /* incognito= */ false,
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
                        tabBookmarkerSupplier,
                        TabGroupListBottomSheetCoordinator::new,
                        snackbarManager,
                        activityResultTracker);
        mIncognitoHelper =
                new StripLayoutHelper(
                        context,
                        this,
                        trailingButtonDelegate,
                        managerHost,
                        updateHost,
                        renderHost,
                        /* incognito= */ true,
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
                        tabBookmarkerSupplier,
                        TabGroupListBottomSheetCoordinator::new,
                        snackbarManager,
                        activityResultTracker);

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
                    mTrailingButtonsCoordinator.setLayerTitleCache(layerTitleCache);
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

    private void createModelSelectorButton(
            Context context,
            StripLayoutViewOnClickHandler selectorClickHandler,
            StripLayoutViewOnKeyboardFocusHandler keyboardFocusHandler) {
        mModelSelectorButton =
                new TintedCompositorButton(
                        context,
                        mIsIncognito,
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
                        R.drawable.btn_tabstrip_new_tab, // Vivaldi
                        MODEL_SELECTOR_BUTTON_CLICK_SLOP_DP); // Vivaldi


        determineModelSelectorButtonYOffset(); // Vivaldi

        // y-offset for folio = lowered tab container + (tab container size - bg size)/2 -
        // folio tab title y-offset = 2 + (38 - 32)/2 - 2 = 3dp
        mModelSelectorButton.setDrawY(MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP);

        updateModelSelectorButtonProperties();
        mModelSelectorButton.setVisible(false);

        // Note(david@vivaldi.com): As we are abusing the |mModelSelectorButton| to create a new tab
        // we also need to update the tooltips accordingly.
        if (ChromeApplicationImpl.isVivaldi())
            mModelSelectorButton.setAccessibilityDescription(
                    context.getString(R.string.accessibility_toolbar_btn_new_tab));
    }

    /** Cleans up internal state. An instance should not be used after this method is called. */
    @SuppressWarnings({"NullAway", "UseSharedPreferencesManagerFromChromeCheck"})
    public void destroy() {
        mTabStripTreeProvider.destroy();
        mTabStripTreeProvider = null;
        mTrailingButtonsCoordinator.destroy();
        mLifecycleDispatcher.unregister(this);
        // Remove the observer to prevent any updates on a destroyed EventFilter.
        mStripVisibilityStateSupplier.removeObserver(mStripVisibilityStateObserver);
        // Delete the EventFilter to avoid any updates on destroyed StripLayoutHelpers.
        mEventFilter = null;
        mTabStripEventHandler = null;
        mIncognitoHelper.destroy();
        mNormalHelper.destroy();
        if (mTabModelSelector != null) {
            mTabModelSelector.removeObserverFromAllModels(mTabModelObserver);

            mTabModelSelector.getCurrentTabModelSupplier().removeObserver(mCurrentTabModelObserver);

            // Remove observers for Glic actuation icons.
            TabModel standardModel = mTabModelSelector.getModel(false);
            for (int i = 0; i < standardModel.getCount(); i++) {
                unregisterActorObserver(standardModel.getTabAt(i));
            }

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
                mTrailingButtonsCoordinator,
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
        mTrailingButtonsCoordinator.onSizeChanged(mWidth, mRightPadding, mLeftPadding, mTopPadding);
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
            @Px
            int tabStripBottomPx =
                    Math.round(mDensity * (mSceneLayerYOffset + mSceneLayerVisibleHeight));
            mStripBottomPxSupplier.set(tabStripBottomPx);
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

        /* Not needed in Vivaldi
        mTrailingButtonsCoordinator.updateGlicButtonOpacity(
                isAppInDesktopWindow(), mIsTopResumedActivity); */

        mUpdateHost.requestUpdate();
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
        mTrailingButtonsCoordinator.getVirtualViews(views);
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

        TintedCompositorTextButton glicButton = mTrailingButtonsCoordinator.getGlicButton();
        if (glicButton != null && glicButton.isVisible()) {
            var glicTouchRect = new RectF();
            glicButton.getTouchTarget(glicTouchRect);
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

    public float getLeftFadeGradientWidth() {
        return getActiveStripLayoutHelper().getLeftFadeGradientWidth();
    }

    public float getRightFadeGradientWidth() {
        return getActiveStripLayoutHelper().getRightFadeGradientWidth();
    }

    public float getLeftFadeOpaqueWidth() {
        return getActiveStripLayoutHelper().getLeftFadeOpaqueWidth();
    }

    public float getRightFadeOpaqueWidth() {
        return getActiveStripLayoutHelper().getRightFadeOpaqueWidth();
    }

    /** Returns drag listener for tab strip. */
    public @Nullable OnDragListener getDragListener() {
        return mTabStripDragHandler;
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
        modelSelector.addObserverToAllModels(mTabModelObserver);

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
                        unregisterActorObserver(tab);
                    }

                    @Override
                    public void tabRemoved(Tab tab) {
                        getStripLayoutHelper(tab.isIncognitoBranded()).tabClosed(tab);
                        unregisterActorObserver(tab);
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
                        registerActorObserver(tab);
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
                        registerActorObserver(tab);

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
                        if (!toDifferentDocument) return;
                        getStripLayoutHelper(tab.isIncognitoBranded()).tabLoadStarted(tab.getId());
                    }

                    @Override
                    public void onLoadStopped(Tab tab, boolean toDifferentDocument) {
                        if (!toDifferentDocument) return;
                        getStripLayoutHelper(tab.isIncognitoBranded()).tabLoadFinished(tab.getId());
                    }

                    @Override
                    public void onCrash(Tab tab) {
                        getStripLayoutHelper(tab.isIncognitoBranded()).tabLoadFinished(tab.getId());
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

        // Register Glic actor observer for existing standard tabs.
        TabModel standardModel = mTabModelSelector.getModel(false);
        for (int i = 0; i < standardModel.getCount(); i++) {
            Tab tab = standardModel.getTabAt(i);
            if (tab != null) {
                registerActorObserver(tab);
            }
        }

        // Register Glic pref change observer for Glic button pin state.
        Profile profile = standardModel.getProfile();
        if (profile != null) {
            mTrailingButtonsCoordinator.onProfileAvailable(profile);
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
        // End Vivaldi
    }

    @Override
    public void onAppHeaderStateChanged(AppHeaderState newState) {
        assert mDesktopWindowStateManager != null;
        // We do not update the layer's height in this method. The height adjustment will be
        // triggered by #onHeightChanged.

        mDesktopWindowStateManager.updateForegroundColor(getBackgroundColor());
        updateHorizontalPaddings(newState.getLeftPadding(), newState.getRightPadding());

        /* Not needed in Vivaldi
        mTrailingButtonsCoordinator.updateGlicButtonOpacity(
                isAppInDesktopWindow(), mIsTopResumedActivity);*/
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

    private void registerActorObserver(Tab tab) {
        if (tab.isIncognitoBranded()) return;
        ActorUiTabController controller = ActorUiTabController.from(tab);
        if (controller == null) return;

        controller.addObserver(mActorObserver);

        ActorUiTabController.UiTabState state = controller.getUiTabState();
        if (state != null) {
            getStripLayoutHelper(/* incognito= */ false)
                    .onActuationStateChanged(tab.getId(), state.tabIndicator);
        }
    }

    private void unregisterActorObserver(Tab tab) {
        if (tab == null || tab.isIncognitoBranded()) return;
        ActorUiTabController controller = ActorUiTabController.from(tab);
        if (controller != null) {
            controller.removeObserver(mActorObserver);
        }
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

    @VisibleForTesting
    /*package*/ void tabModelSwitched(boolean incognito) {
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

    private void updateStripButtons() {
        // Use helper methods to calculate new visibility of strip buttons.
        boolean newGlicVisibility = !ChromeApplicationImpl.isVivaldi(); /*&& mTrailingButtonsCoordinator.shouldGlicBeVisible(); Vivaldi */
        boolean newGlicActorVisibility = mTrailingButtonsCoordinator.shouldGlicActorBeVisible();
        boolean newMsbVisibility = shouldMsbBeVisible();

        // Update model selector button properties.
        updateModelSelectorButtonProperties();

        // Note (david@vivaldi.com): The ModelSelectorButton is always visible since this is our
        // new tab button but we don't occupy any margins with setting |isVisible| to false;
        if (ChromeApplicationImpl.isVivaldi() && mModelSelectorButton != null) {
            mModelSelectorButton.setVisible(true);
            // Vivaldi VAB-9775
            if (GlobalNightModeStateProviderHolder.getInstance().isInNightMode()) {
                @ColorInt
                int iconNightModeColor = mContext.getColor(R.color.default_bg_color_light);
                mModelSelectorButton.setTint(iconNightModeColor);
            } else {
                @ColorInt
                int iconDefaultColor =
                        AppCompatResources.getColorStateList(mContext, R.color.default_bg_color_baseline)
                                .getDefaultColor();
                mModelSelectorButton.setTint(iconDefaultColor);
            } // End Vivaldi VAB-9775
        } else { // Vivaldi

        // Early exit if visibility of buttons hasn't changed.
        boolean glicChanged =
                mTrailingButtonsCoordinator.isGlicButtonVisible() != newGlicVisibility;
        boolean actorChanged =
                mTrailingButtonsCoordinator.isGlicActorButtonVisible() != newGlicActorVisibility;
        boolean msbChanged =
                mModelSelectorButton != null
                        && mModelSelectorButton.isVisible() != newMsbVisibility;
        if (!glicChanged && !actorChanged && !msbChanged) return;

        // Set updated visibilities (of all buttons for simplicity).
        mTrailingButtonsCoordinator.setGlicButtonVisible(newGlicVisibility);
        mTrailingButtonsCoordinator.setGlicActorButtonVisible(
                newGlicActorVisibility, /* animate= */ false);
        if (mModelSelectorButton != null) mModelSelectorButton.setVisible(newMsbVisibility);

        // The Glic button position depends on the MSB's visibility.
        if (msbChanged) {
            mTrailingButtonsCoordinator.setModelSelectorButtonVisible(newMsbVisibility);
        }

        } // End Vivaldi

        updateButtonMargins();
    }

    private void updateButtonMargins() {
        boolean isTrailingButtonsVisible = !ChromeApplicationImpl.isVivaldi() &&
                (mTrailingButtonsCoordinator.isGlicButtonVisible()
                        || mTrailingButtonsCoordinator.isGlicActorButtonVisible());
        boolean isMsbVisible = mModelSelectorButton != null && mModelSelectorButton.isVisible();

        // Calculate layout sizes and update margins. We use (width + end padding + start spacing)
        // to create a larger gap between buttons to meet touch target size requirements.
        float trailingButtonsTouchTargetSize =
                isTrailingButtonsVisible
                        ? mTrailingButtonsCoordinator.getTrailingButtonsWidthWithPadding()
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
        mNormalHelper.updateEndMarginForStripButtons(
                trailingButtonsTouchTargetSize, msbTouchTargetSize);
        mIncognitoHelper.updateEndMarginForStripButtons(
                /* trailingButtonsTouchTargetSize= */ 0.0f, msbTouchTargetSize);
    }

    private boolean shouldMsbBeVisible() {
        if (mModelSelectorButton == null) return false;

        return mTabModelSelector != null && mTabModelSelector.getModel(true).getCount() != 0;
    }

    private void updateModelSelectorButtonProperties() {
        if (mModelSelectorButton == null) return;
        mModelSelectorButton.setIncognito(mIsIncognito);

        Context context = mContext;
        @ColorInt
        int iconDefaultColor =
                context.getColorStateList(R.color.default_icon_color_tint_list).getDefaultColor();
        @ColorInt
        int iconIncognitoColor = context.getColor(R.color.default_icon_color_secondary_light);

        // Model selector button background color.
        // Default bg color is surface inverse.
        @ColorInt
        int backgroundDefaultColor = context.getColor(R.color.model_selector_button_bg_color);
        // Incognito bg color is surface 1 baseline.
        @ColorInt
        int backgroundIncognitoColor =
                context.getColor(R.color.default_bg_color_dark_elev_1_baseline);

        @ColorInt
        int backgroundHoverColor =
                ColorUtils.setAlphaComponentWithFloat(
                        SemanticColorUtils.getDefaultTextColor(context),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY);
        @ColorInt
        int backgroundPeripheralPressedColor =
                ColorUtils.setAlphaComponentWithFloat(
                        SemanticColorUtils.getDefaultTextColor(context),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY);

        @ColorInt
        int backgroundHoverIncognitoColor =
                ColorUtils.setAlphaComponentWithFloat(
                        context.getColor(R.color.tab_strip_button_hover_bg_color),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_DEFAULT_OPACITY);
        @ColorInt
        int backgroundPeripheralPressedIncognitoColor =
                ColorUtils.setAlphaComponentWithFloat(
                        context.getColor(R.color.tab_strip_button_hover_bg_color),
                        MODEL_SELECTOR_BUTTON_HOVER_BACKGROUND_PRESSED_OPACITY);

        int iconColor = mIsIncognito ? iconIncognitoColor : iconDefaultColor;
        int bgColor = mIsIncognito ? backgroundIncognitoColor : backgroundDefaultColor;
        int hoverBgColor = mIsIncognito ? backgroundHoverIncognitoColor : backgroundHoverColor;
        int peripheralPressedBgColor =
                mIsIncognito
                        ? backgroundPeripheralPressedIncognitoColor
                        : backgroundPeripheralPressedColor;

        TintedCompositorButton button = (TintedCompositorButton) mModelSelectorButton;
        button.setTint(iconColor);
        button.setBackgroundTint(bgColor, hoverBgColor, bgColor, peripheralPressedBgColor);

        button.setAccessibilityDescription(
                mIsIncognito
                        ? context.getString(
                                R.string.accessibility_tabstrip_btn_incognito_toggle_incognito)
                        : context.getString(
                                R.string.accessibility_tabstrip_btn_incognito_toggle_standard));
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
            List<Tab> relatedTabs =
                    currentTabModel.getRelatedTabList(mTabModelSelector.getCurrentTabId());
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

    /** Vivaldi VAB-13105 - Vertically center the model selector ('+') button in the tab strip. */
    private void determineModelSelectorButtonYOffset() {
        // Center the button, then offset to align with the favicons, which sit at different
        // heights per mode.
        float center =
                (mScrollableStripHeight - MODEL_SELECTOR_BUTTON_BACKGROUND_HEIGHT_DP) / 2f;
        boolean floatingTabs =
                VivaldiPreferences.getSharedPreferencesManager()
                        .readBoolean(VivaldiPreferences.ENABLE_FLOATING_TABS, true);
        float offset;
        if (floatingTabs) {
            offset = 2f;
        } else {
            offset = VivaldiUtils.isTopToolbarOn() ? 5f : -1f;
        }
        MODEL_SELECTOR_BUTTON_BACKGROUND_Y_OFFSET_DP = center + offset;
    }
    // End Vivaldi
}
