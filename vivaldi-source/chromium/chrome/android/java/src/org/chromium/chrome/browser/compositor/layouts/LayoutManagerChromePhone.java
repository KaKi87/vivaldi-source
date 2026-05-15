// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.layouts;

import static org.chromium.build.NullUtil.assertNonNull;

import android.content.Context;
import android.view.ViewGroup;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.layouts.phone.NewTabAnimationLayout;
import org.chromium.chrome.browser.hub.HubLayoutDependencyHolder;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab_ui.ActionConfirmationManager;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tab_ui.TabSwitcher;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ControlContainer;
import org.chromium.chrome.browser.toolbar.ToolbarManager;
import org.chromium.chrome.browser.ui.edge_to_edge.TopInsetProvider;
import org.chromium.ui.resources.dynamics.DynamicResourceLoader;

import java.util.function.Supplier;

// Vivaldi
import android.view.View;
import android.view.ViewStub;
import androidx.annotation.NonNull;

import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.build.BuildConfig;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutHelperManager;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.ui.dragdrop.DragAndDropDelegate;

import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;
import java.util.List;

/**
 * {@link LayoutManagerChromePhone} is the specialization of {@link LayoutManagerChrome} for the
 * phone.
 */
@NullMarked
public class LayoutManagerChromePhone extends LayoutManagerChrome {
    private final Supplier<@Nullable CompositorViewHolder> mCompositorViewHolderSupplier;
    private final TopInsetProvider mTopInsetProvider;
    private final NonNullObservableSupplier<Boolean> mScrimVisibilitySupplier;
    private final ToolbarManager mToolbarManager;
    private final ViewGroup mContentView;
    private Layout mNewTabAnimationLayout;

    // Vivaldi
    private final List<StripLayoutHelperManager> mTabStrips = new ArrayList<>();
    /**
     * Creates an instance of a {@link LayoutManagerChromePhone}.
     *
     * @param host A {@link LayoutManagerHost} instance.
     * @param contentContainer A {@link ViewGroup} for Android views to be bound to.
     * @param tabSwitcherSupplier Supplier for an interface to talk to the Grid Tab Switcher. Used
     *     to create overviewLayout if it has value, otherwise will use the accessibility overview
     *     layout.
     * @param tabModelSelectorSupplier Supplier for an interface to talk to the Tab Model Selector.
     * @param tabContentManagerSupplier Supplier of the {@link TabContentManager} instance.
     * @param topUiThemeColorProvider {@link ThemeColorProvider} for top UI.
     * @param hubLayoutDependencyHolder The dependency holder for creating {@link HubLayout}.
     * @param compositorViewHolderSupplier Supplier of the {@link CompositorViewHolder} instance.
     * @param contentView The base content view.
     * @param toolbarManager The {@link ToolbarManager} instance.
     * @param scrimVisibilitySupplier Supplier for the Scrim visibility.
     * @param topInsetProvider The {@link TopInsetProvider} instance.
     */
    @SuppressWarnings("NullAway") // TODO(jarle@vivaldi.com): check use of mLayerTitleCache being
    // null when instantiating StripLayoutHelperManager
    public LayoutManagerChromePhone(
            LayoutManagerHost host,
            ViewGroup contentContainer,
            Supplier<TabSwitcher> tabSwitcherSupplier,
            Supplier<TabModelSelector> tabModelSelectorSupplier,
            MonotonicObservableSupplier<TabContentManager> tabContentManagerSupplier,
            Supplier<TopUiThemeColorProvider> topUiThemeColorProvider,
            HubLayoutDependencyHolder hubLayoutDependencyHolder,
            Supplier<@Nullable CompositorViewHolder> compositorViewHolderSupplier,
            ViewGroup contentView,
            ToolbarManager toolbarManager,
            NonNullObservableSupplier<Boolean> scrimVisibilitySupplier,
            TopInsetProvider topInsetProvider,

            @NonNull ViewStub tabStripTooltipViewStub, // Vivaldi
            MonotonicObservableSupplier<StripLayoutHelperManager.TabModelStartupInfo>  // Vivaldi
                    tabModelStartupInfoSupplier,  // Vivaldi
            ActivityLifecycleDispatcher lifecycleDispatcher, // Vivaldi
            MultiInstanceManager multiInstanceManager,  // Vivaldi
            DragAndDropDelegate dragAndDropDelegate, // Vivaldi
            View toolbarContainerView, //Vivaldi
            ViewStub tabHoverCardViewStub, // Vivaldi
            ViewStub tabHoverCardViewStubStack, // Vivaldi
            WindowAndroid windowAndroid, // Vivaldi
            DesktopWindowStateManager desktopWindowStateManager, // Vivaldi
            ActionConfirmationManager actionConfirmationManager, // Vivaldi
            BrowserControlsStateProvider browserControlsStateProvider, // Vivaldi
            DataSharingTabManager dataSharingTabManager, // Vivaldi
            BottomSheetController bottomSheetController, // Vivaldi
            MonotonicObservableSupplier<ShareDelegate> shareDelegateSupplier, // Vivaldi
            BackPressManager backPressManager,
            SnackbarManager snackbarManager) { // Vivaldi
        super(
                host,
                contentContainer,
                tabSwitcherSupplier,
                tabModelSelectorSupplier,
                tabContentManagerSupplier,
                topUiThemeColorProvider,
                hubLayoutDependencyHolder);
        mCompositorViewHolderSupplier = compositorViewHolderSupplier;
        mContentView = contentView;
        mToolbarManager = toolbarManager;
        mScrimVisibilitySupplier = scrimVisibilitySupplier;
        mTopInsetProvider = topInsetProvider;

        // Note(david@vivaldi.com): We create two tab strips here. The first one is the main strip.
        // The second one is the stack strip.
        for (int i = 0; i < 2; i++) {
            mTabStrips.add(
                    new StripLayoutHelperManager(
                            mHost.getContext(),
                            host,
                            this,
                            mHost.getLayoutRenderHost(),
                            ObservableSuppliers.createMonotonic(),
                            tabModelStartupInfoSupplier,
                            lifecycleDispatcher,
                            multiInstanceManager,
                            dragAndDropDelegate,
                            toolbarContainerView,
                            (i == 0) ? tabHoverCardViewStub : tabHoverCardViewStubStack,
                            tabContentManagerSupplier,
                            browserControlsStateProvider,
                            windowAndroid,
                            toolbarManager,
                            desktopWindowStateManager,
                            actionConfirmationManager,
                            dataSharingTabManager,
                            bottomSheetController,
                            shareDelegateSupplier,
                            null,
                            backPressManager,
                            snackbarManager,
                            null,
                            /* isStackStrip */ (i > 0))); // Vivaldi
            addObserver(mTabStrips.get(i).getTabSwitcherObserver());
        }
        updateGlobalSceneOverlay();
        // End Vivaldi
    }

    @Override
    public void destroy() {
        super.destroy();
        mNewTabAnimationLayout.destroy();
        // Vivaldi
        for (int i = 0; i < 2; i++) mTabStrips.get(i).destroy();
        mTabStrips.clear();
    }

    @Override
    @Initializer
    public void init(
            TabModelSelector selector,
            TabCreatorManager creator,
            @Nullable ControlContainer controlContainer,
            DynamicResourceLoader dynamicResourceLoader,
            TopUiThemeColorProvider topUiColorProvider,
            NonNullObservableSupplier<Integer> bottomControlsOffsetSupplier) {
        Context context = mHost.getContext();
        LayoutRenderHost renderHost = mHost.getLayoutRenderHost();

        mNewTabAnimationLayout =
                new NewTabAnimationLayout(
                        context,
                        this,
                        renderHost,
                        this,
                        getContentContainer(),
                        assertNonNull(mCompositorViewHolderSupplier.get()),
                        mContentView,
                        mToolbarManager,
                        getBrowserControlsManager(),
                        mScrimVisibilitySupplier,
                        mTopInsetProvider);

        super.init(
                selector,
                creator,
                controlContainer,
                dynamicResourceLoader,
                topUiColorProvider,
                bottomControlsOffsetSupplier);

        // Initialize Layouts
        TabContentManager tabContentManager = mTabContentManagerSupplier.get();
        assert tabContentManager != null;
        mNewTabAnimationLayout.setTabModelSelector(selector);
        mNewTabAnimationLayout.setTabContentManager(tabContentManager);

        // Vivaldi
        for (int i = 0; i < 2; i++) mTabStrips.get(i).setTabModelSelector(selector, creator);
    }

    @Override
    protected Layout getLayoutForType(int layoutType) {
        if (layoutType == LayoutType.SIMPLE_ANIMATION) {
            return mNewTabAnimationLayout;
        }
        return super.getLayoutForType(layoutType);
    }

    @Override
    protected void tabClosed(int id, int nextId, boolean incognito, boolean tabRemoved) {
        boolean showOverview = nextId == Tab.INVALID_TAB_ID;

        // Vivaldi
        if (BuildConfig.IS_VIVALDI)
            showOverview = false;

        if (getActiveLayoutType() != LayoutType.TAB_SWITCHER && showOverview) {
            // Since there will be no 'next' tab to display, switch to
            // overview mode when the animation is finished.
            if (getActiveLayoutType() == LayoutType.SIMPLE_ANIMATION) {
                setNextLayout(getLayoutForType(LayoutType.TAB_SWITCHER), true);
            } else {
                super.tabClosed(id, nextId, incognito, tabRemoved);
            }
        }
    }

    @Override
    protected void tabCreating(int sourceId, boolean isIncognito) {
        if (getActiveLayout() != null
                && !getActiveLayout().isStartingToHide()
                && overlaysHandleTabCreating()
                && getActiveLayout().handlesTabCreating()) {
            // If the current layout in the foreground, let it handle the tab creation animation.
            // This check allows us to switch from the HubLayout to the SimpleAnimationLayout
            // smoothly.
            getActiveLayout().onTabCreating(sourceId);
        } else if (animationsEnabled()) {
            if (!isLayoutVisible(LayoutType.TAB_SWITCHER)) {
                if (getActiveLayout() != null && getActiveLayout().isStartingToHide()) {
                    setNextLayout(mNewTabAnimationLayout, true);
                    // The method Layout#doneHiding() will automatically show the next layout.
                    getActiveLayout().doneHiding();
                } else {
                    startShowing(mNewTabAnimationLayout, false);
                }
            }
            if (getActiveLayout() != null) {
                getActiveLayout().onTabCreating(sourceId);
            }
        }
    }

    /** @return Whether the {@link SceneOverlay}s handle tab creation. */
    private boolean overlaysHandleTabCreating() {
        Layout layout = getActiveLayout();
        if (layout == null
                || layout.getLayoutTabsToRender() == null
                || layout.getLayoutTabsToRender().length != 1) {
            return false;
        }
        for (int i = 0; i < mSceneOverlays.size(); i++) {
            if (!mSceneOverlays.get(i).isSceneOverlayTreeShowing()) continue;
            if (mSceneOverlays.get(i).handlesTabCreating()) {
                // Prevent animation from happening if the overlay handles creation.
                startHiding();
                doneHiding();
                return true;
            }
        }
        return false;
    }

    // Vivaldi: Update the {@link SceneOverlay} according to the tab strip setting.
    private void updateGlobalSceneOverlay() {
        for (int i = 0; i < 2; i++) addSceneOverlay(mTabStrips.get(i));
        if (getTabModelSelector() != null)
            tabModelSwitched(getTabModelSelector().isIncognitoSelected());
    }

    @Override
    protected void tabModelSwitched(boolean incognito) {
        super.tabModelSwitched(incognito);
        getTabModelSelector().commitAllTabClosures();
    }

    /** Vivaldi **/
    @Override
    public StripLayoutHelperManager getStripLayoutHelperManager() {
        // We always return our main strip here.
        return mTabStrips.get(0);
    }
}
