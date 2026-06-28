// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.search_engines.settings;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Context;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.AccessibilityDelegate;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.TextView;

import androidx.annotation.IntDef;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.JavaExceptionReporter;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.base.version_info.VersionInfo;
import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.regional_capabilities.RegionalCapabilitiesServiceFactory;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.browser_ui.widget.containment.ContainerStyle;
import org.chromium.components.browser_ui.widget.containment.ContainmentItemController;
import org.chromium.components.browser_ui.widget.containment.ContainmentViewStyler;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.omnibox.OmniboxFeatures;
import org.chromium.components.regional_capabilities.RegionalCapabilitiesService;
import org.chromium.components.search_engines.ChoiceMadeLocation;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.url.GURL;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;

// Vivaldi
import android.os.Bundle;
import android.annotation.SuppressLint;
import android.graphics.BitmapFactory;
import android.widget.PopupMenu;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import java.net.MalformedURLException;
import java.net.URL;

import org.chromium.base.Log;
import org.chromium.build.BuildConfig;
import org.chromium.ui.widget.Toast;

/** A custom adapter for listing search engines. */
@NullMarked
public class SearchEngineAdapter extends BaseAdapter
        implements TemplateUrlService.LoadListener,
                TemplateUrlService.TemplateUrlServiceObserver,
                OnClickListener, /*Vivaldi*/ View.OnLongClickListener /*Vivaldi end*/  {

    public static final int MAX_RECENT_ENGINE_NUM = 3;
    public static final long MAX_DISPLAY_TIME_SPAN_MS = DateUtils.DAY_IN_MILLIS * 2;
    private static final Runnable NO_OP = () -> {};

    private static final int VIEW_TYPE_COUNT = 3;

    @VisibleForTesting
    @IntDef({ViewType.ITEM, ViewType.DIVIDER, ViewType.SITE_SEARCH_SETTINGS})
    @Retention(RetentionPolicy.SOURCE)
    @interface ViewType {
        int ITEM = 0;
        int DIVIDER = 1;
        int SITE_SEARCH_SETTINGS = 2;
    }

    /**
     * Type for source of search engine. This is needed because if a custom search engine is set as
     * default, it will be moved to the prepopulated list.
     */
    @IntDef({
        TemplateUrlSourceType.DEFAULT,
        TemplateUrlSourceType.PREPOPULATED,
        TemplateUrlSourceType.RECENT
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface TemplateUrlSourceType {
        int DEFAULT = 0;
        int PREPOPULATED = 1;
        int RECENT = 2;
    }

    /** The current context. */
    private final Context mContext;

    private final Profile mProfile;

    private final Runnable mSiteSearchClickHandler;

    /** The layout inflater to use for the custom views. */
    private final LayoutInflater mLayoutInflater;

    /** The large icon bridge to get icons for search engines. */
    private LargeIconBridge mLargeIconBridge;

    /** The list of prepopulated and default search engines. */
    private List<TemplateUrl> mPrepopulatedSearchEngines = new ArrayList<>();

    /** The list of recently visited search engines. */
    private List<TemplateUrl> mRecentSearchEngines = new ArrayList<>();

    /** Cache for storing fetched search icon bitmaps. */
    private final Map<GURL, Bitmap> mIconCache = new HashMap<>();

    /**
     * The position (index into mPrepopulatedSearchEngines) of the currently selected search engine.
     * Can be -1 if current search engine is managed and set to something other than the
     * pre-populated values.
     */
    private int mSelectedSearchEnginePosition = -1;

    /** The position of the default search engine before user's action. */
    private int mInitialEnginePosition = -1;

    private boolean mHasLoadObserver;

    private boolean mIsLocationPermissionChanged;

    private @MonotonicNonNull Runnable mDisableAutoSwitchRunnable;
    private final ContainmentItemController mContainmentItemController;

    // Vivaldi
    long mCurrentDefaultSearchEngine;
    static final String EDIT_SEARCH_ENGINE_FRAGMENT =
            "org.vivaldi.browser.preferences.search_engines."
            + "VivaldiCustomSearchEnginePreference";
    /**
     * Construct a SearchEngineAdapter.
     *
     * @param context The current context.
     * @param profile The Profile associated with these settings.
     * @param siteSearchClickHandler Used for "Manage search engines and site search". Only
     *     avaliable when OmniboxFeatures.sOmniboxSiteSearch is enabled.
     */
    public SearchEngineAdapter(
            Context context, Profile profile, @Nullable Runnable siteSearchClickHandler) {
        mContext = context;
        mProfile = profile;
        mLayoutInflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mContainmentItemController = new ContainmentItemController(mContext);

        if (OmniboxFeatures.sOmniboxSiteSearch.isEnabled() && siteSearchClickHandler != null) {
            this.mSiteSearchClickHandler = siteSearchClickHandler;
        } else {
            this.mSiteSearchClickHandler = NO_OP;
        }
    }

    /** Start the adapter to gather the available search engines and listen for updates. */
    @Initializer
    public void start() {
        mLargeIconBridge = createLargeIconBridge();
        refreshData();
        TemplateUrlServiceFactory.getForProfile(mProfile).addObserver(this);
    }

    /** Stop the adapter from listening for future search engine updates. */
    public void stop() {
        mLargeIconBridge.destroy();
        if (mHasLoadObserver) {
            TemplateUrlServiceFactory.getForProfile(mProfile).unregisterLoadListener(this);
            mHasLoadObserver = false;
        }

        // Vivaldi Note: Don't query the profile when the native pointer is already destroyed.
        // Ref:VAB-11592
        if (mProfile.getNativeBrowserContextPointer() > 0) // Vivaldi End
        TemplateUrlServiceFactory.getForProfile(mProfile).removeObserver(this);
    }

    String getValueForTesting() {
        return Integer.toString(mSelectedSearchEnginePosition);
    }

    String setValueForTesting(String value) {
        return searchEngineSelected(Integer.parseInt(value));
    }

    String getKeywordForTesting(int index) {
        return toKeyword(index);
    }

    // Can be overridden in tests.
    @VisibleForTesting
    LargeIconBridge createLargeIconBridge() {
        return new LargeIconBridge(mProfile);
    }

    /** Initialize the search engine list. */
    private void refreshData() {
        // Vivaldi Note: Don't query the profile when the native pointer is already destroyed.
        // Ref:VAB-11612
        if (mProfile.getNativeBrowserContextPointer() == 0) return;// Vivaldi End
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(mProfile);
        if (!templateUrlService.isLoaded()) {
            mHasLoadObserver = true;
            templateUrlService.registerLoadListener(this);
            templateUrlService.load();
            return; // Flow continues in onTemplateUrlServiceLoaded below.
        }

        RegionalCapabilitiesService regionalCapabilities =
                RegionalCapabilitiesServiceFactory.getForProfile(mProfile);
        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();

        // Note: DSE may be null if explicitly blocked by policy.
        @Nullable TemplateUrl defaultSearchEngineTemplateUrl;

        if (mProfile.isOffTheRecord()) { // Start Vivaldi
            defaultSearchEngineTemplateUrl =
                    templateUrlService.vivaldiGetDefaultSearchEngine(
                            TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_PRIVATE);
        } else { // else from Vivaldi
            defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();

        } // End Vivaldi

        // Vivaldi
        boolean defaultSearchEngineChanged = false;
        if (defaultSearchEngineTemplateUrl != null
                && mCurrentDefaultSearchEngine != defaultSearchEngineTemplateUrl.getNativePtr()) {
            mCurrentDefaultSearchEngine = defaultSearchEngineTemplateUrl.getNativePtr();
            defaultSearchEngineChanged = true;
        } // End Vivaldi

        assert defaultSearchEngineTemplateUrl != null;
        // In Vivaldi, we get everything sorted on the native side.
        if (!BuildConfig.IS_VIVALDI)
        sortAndFilterUnnecessaryTemplateUrl(
                templateUrls,
                defaultSearchEngineTemplateUrl,
                regionalCapabilities.isInEeaCountry());
        boolean forceRefresh = mIsLocationPermissionChanged;
        mIsLocationPermissionChanged = false;
        // Vivaldi
        if (!defaultSearchEngineChanged && !didSearchEnginesChange(templateUrls)) {
            if (forceRefresh) notifyDataSetChanged();
            return;
        }

        mPrepopulatedSearchEngines = new ArrayList<>();
        mRecentSearchEngines = new ArrayList<>();

        for (int i = 0; i < templateUrls.size(); i++) {
            // Vivaldi NOTE(jarle@vivaldi.com): Vivaldi for desktop does not have the concept of
            // recent search engines, so don't populate the mRecentSearchEngines list.
            if (BuildConfig.IS_VIVALDI) {
                // Note (nagamani@vivaldi.com): We hide the starter pack search engines from normal
                // search engine list. Ref: VAB-11558
                if (templateUrls.get(i).getStarterPackId() == 0)
                    mPrepopulatedSearchEngines.add(templateUrls.get(i));
            } else { // Vivaldi End
            TemplateUrl templateUrl = templateUrls.get(i);
            if (getSearchEngineSourceType(templateUrl, defaultSearchEngineTemplateUrl)
                    == TemplateUrlSourceType.RECENT) {
                mRecentSearchEngines.add(templateUrl);
            } else {
                mPrepopulatedSearchEngines.add(templateUrl);
            }
            } // Vivaldi
        }

        // Convert the TemplateUrl index into an index of mSearchEngines.
        mSelectedSearchEnginePosition = -1;
        for (int i = 0; i < mPrepopulatedSearchEngines.size(); ++i) {
            if (Objects.equals(mPrepopulatedSearchEngines.get(i), defaultSearchEngineTemplateUrl)) {
                mSelectedSearchEnginePosition = i;
            }
        }

        for (int i = 0; i < mRecentSearchEngines.size(); ++i) {
            if (Objects.equals(mRecentSearchEngines.get(i), defaultSearchEngineTemplateUrl)) {
                // Add one to offset the title for the recent search engine list.
                mSelectedSearchEnginePosition = i + computeStartIndexForRecentSearchEngines();
            }
        }

        if (mSelectedSearchEnginePosition == -1) {
            if (defaultSearchEngineTemplateUrl != null) {
                mRecentSearchEngines.add(defaultSearchEngineTemplateUrl);
                mSelectedSearchEnginePosition = mRecentSearchEngines.size() - 1;
            }

            // Vivaldi NOTE(jarle@vivaldi.com): This is not so serious for Vivaldi,
            // we allow editing the template URLs. No need to throw an exception.
            if (BuildConfig.IS_VIVALDI)
                Log.w("SearchEngineAdapter",
                        "Default search engine is not found in available search engines");
            else
            if (VersionInfo.isOfficialBuild()) {
                // TODO(crbug.com/437052188): address exceptions linked to search engine choice
                // program and remove the diagnostics logic.
                // It's very likely this is impacting users who have selected a search engine in a
                // country where SEC program is in effect and have moved/relocated.
                // If true, these engines should not be suppressed/removed, but appended to recents.
                var knownEngines = new StringBuilder(" ");
                for (var engine : mPrepopulatedSearchEngines) {
                    knownEngines.append(engine.getShortName()).append(", ");
                }
                for (var engine : mRecentSearchEngines) {
                    knownEngines.append(engine.getShortName()).append(", ");
                }
                var report =
                        new IllegalStateException(
                                String.format(
                                        Locale.ROOT,
                                        "Default search engine is not found in available search"
                                                + " engines: DSE is valid=%b (%s), is managed=%b,"
                                                + " known=%d [%s]",
                                        defaultSearchEngineTemplateUrl != null,
                                        defaultSearchEngineTemplateUrl != null
                                                ? defaultSearchEngineTemplateUrl.getShortName()
                                                : "<null>",
                                        TemplateUrlServiceFactory.getForProfile(mProfile)
                                                .isDefaultSearchManaged(),
                                        mPrepopulatedSearchEngines.size()
                                                + mRecentSearchEngines.size(),
                                        knownEngines.toString()));
                JavaExceptionReporter.reportException(report);
            }
        }

        mInitialEnginePosition = mSelectedSearchEnginePosition;

        notifyDataSetChanged();
    }

    @VisibleForTesting
    public static void sortAndFilterUnnecessaryTemplateUrl(
            List<TemplateUrl> templateUrls,
            @Nullable TemplateUrl defaultSearchEngine,
            boolean isEeaChoiceCountry) {
        // In the EEA and when the new settings design is shown, we want to avoid re-sorting, to
        // stick to the order of prepopulated engines provided by the service.
        boolean sortPrepopulatedEngines = !isEeaChoiceCountry;
        templateUrls.sort(templateUrlsComparatorWith(defaultSearchEngine, sortPrepopulatedEngines));

        int recentEngineNum = 0;
        long displayTime = System.currentTimeMillis() - MAX_DISPLAY_TIME_SPAN_MS;
        Iterator<TemplateUrl> iterator = templateUrls.iterator();
        // Recently visited search engines is disabled as site search can set more advance settings.
        boolean shouldShowRecentSearchEngines = !OmniboxFeatures.sOmniboxSiteSearch.isEnabled();
        while (iterator.hasNext()) {
            TemplateUrl templateUrl = iterator.next();
            if (getSearchEngineSourceType(templateUrl, defaultSearchEngine)
                    != TemplateUrlSourceType.RECENT) {
                continue;
            }
            if (shouldShowRecentSearchEngines
                    && recentEngineNum < MAX_RECENT_ENGINE_NUM
                    && templateUrl.getLastVisitedTime() > displayTime) {
                recentEngineNum++;
            } else {
                iterator.remove();
            }
        }
    }

    /**
     * Returns a {@link Comparator} for {@link TemplateUrl}, that will properly sort items based on
     * the current user selections.
     */
    private static Comparator<TemplateUrl> templateUrlsComparatorWith(
            @Nullable TemplateUrl defaultSearchEngine, boolean sortPrepopulatedEngines) {
        return (TemplateUrl templateUrl1, TemplateUrl templateUrl2) -> {
            // Don't change the order for duplicates.
            if (templateUrl1.getNativePtr() == templateUrl2.getNativePtr()) {
                return 0;
            }

            // Prepopulated engines go first and are sorted by prepopulatedID.
            if (templateUrl1.getIsPrepopulated() && templateUrl2.getIsPrepopulated()) {
                if (sortPrepopulatedEngines) {
                    // Reorder the prepopulated engines by prepopulated ID.
                    return templateUrl1.getPrepopulatedId() - templateUrl2.getPrepopulatedId();
                } else {
                    // Don't reorder the prepopulated engines among themselves. They have
                    // been ordered in a specific way by the service.
                    return 0;
                }
            } else if (templateUrl1.getIsPrepopulated()) {
                return -1;
            } else if (templateUrl2.getIsPrepopulated()) {
                return 1;
            }

            // A custom DSE should be displayed right after the prepopulated ones.
            if (Objects.equals(templateUrl1, defaultSearchEngine)) {
                return -1;
            } else if (Objects.equals(templateUrl2, defaultSearchEngine)) {
                return 1;
            }

            // Fallback: just sort by visit recency.
            return Long.compare(
                    templateUrl2.getLastVisitedTime(), templateUrl1.getLastVisitedTime());
        };
    }

    private static @TemplateUrlSourceType int getSearchEngineSourceType(
            TemplateUrl templateUrl, @Nullable TemplateUrl defaultSearchEngine) {
        if (templateUrl.getIsPrepopulated()) {
            return TemplateUrlSourceType.PREPOPULATED;
        } else if (defaultSearchEngine != null
                && templateUrl.getNativePtr() == defaultSearchEngine.getNativePtr()) {
            return TemplateUrlSourceType.DEFAULT;
        } else {
            return TemplateUrlSourceType.RECENT;
        }
    }

    private static boolean containsTemplateUrl(
            List<TemplateUrl> templateUrls, TemplateUrl targetTemplateUrl) {
        for (int i = 0; i < templateUrls.size(); i++) {
            TemplateUrl templateUrl = templateUrls.get(i);
            // Explicitly excluding TemplateUrlSourceType and Index as they might change if a search
            // engine is set as default.
            if (templateUrl.getIsPrepopulated() == targetTemplateUrl.getIsPrepopulated()
                    && TextUtils.equals(templateUrl.getKeyword(), targetTemplateUrl.getKeyword())
                    && TextUtils.equals(
                            templateUrl.getShortName(), targetTemplateUrl.getShortName())) {
                return true;
            }
        }
        return false;
    }

    private boolean didSearchEnginesChange(List<TemplateUrl> templateUrls) {
        if (templateUrls.size()
                != mPrepopulatedSearchEngines.size() + mRecentSearchEngines.size()) {
            return true;
        }
        for (int i = 0; i < templateUrls.size(); i++) {
            TemplateUrl templateUrl = templateUrls.get(i);
            if (!containsTemplateUrl(mPrepopulatedSearchEngines, templateUrl)
                    && !SearchEngineAdapter.containsTemplateUrl(
                            mRecentSearchEngines, templateUrl)) {
                return true;
            }
        }
        return false;
    }

    private String toKeyword(int position) {
        if (position < mPrepopulatedSearchEngines.size()) {
            return mPrepopulatedSearchEngines.get(position).getKeyword();
        } else {
            position -= computeStartIndexForRecentSearchEngines();
            return mRecentSearchEngines.get(position).getKeyword();
        }
    }

    // BaseAdapter:

    @Override
    public int getCount() {
        int size = 0;
        if (mPrepopulatedSearchEngines != null) {
            size += mPrepopulatedSearchEngines.size();
        }
        if (mRecentSearchEngines != null && mRecentSearchEngines.size() != 0) {
            // Account for the header by adding one to the size.
            size += mRecentSearchEngines.size() + 1;
        }
        if (mSiteSearchClickHandler != NO_OP) {
            size += 1;
        }
        return size;
    }

    @Override
    public int getViewTypeCount() {
        return VIEW_TYPE_COUNT;
    }

    @Override
    public @Nullable Object getItem(int pos) {
        if (getItemViewType(pos) == ViewType.SITE_SEARCH_SETTINGS) {
            return null;
        }
        if (pos < mPrepopulatedSearchEngines.size()) {
            return mPrepopulatedSearchEngines.get(pos);
        } else if (pos > mPrepopulatedSearchEngines.size()) {
            pos -= computeStartIndexForRecentSearchEngines();
            return mRecentSearchEngines.get(pos);
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public @ViewType int getItemViewType(int position) {
        if (mSiteSearchClickHandler != NO_OP && position == getCount() - 1) {
            return ViewType.SITE_SEARCH_SETTINGS;
        } else if (position == mPrepopulatedSearchEngines.size()
                && mRecentSearchEngines.size() != 0) {
            return ViewType.DIVIDER;
        } else {
            return ViewType.ITEM;
        }
    }

    @SuppressLint("SetTextI18n") // Vivaldi
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(mProfile);

        View view = convertView;
        int itemViewType = getItemViewType(position);
        if (itemViewType == ViewType.DIVIDER) {
            if (convertView == null && mRecentSearchEngines.size() != 0) {
                view = mLayoutInflater.inflate(R.layout.search_engine_recent_title, parent, false);
            }
            return view;
        }

        if (convertView == null) {
            int layoutId;
            if (itemViewType == ViewType.SITE_SEARCH_SETTINGS) {
                layoutId = R.layout.search_engine_site_search_link;
            } else {
                layoutId = R.layout.search_engine_with_logo;
            }
            view = mLayoutInflater.inflate(layoutId, parent, false);
        }

        View containerView = view.findViewById(R.id.container);

        boolean isTop = position == 0 || getItemViewType(position - 1) == ViewType.DIVIDER;
        boolean isBottom =
                position == getCount() - 1 || getItemViewType(position + 1) == ViewType.DIVIDER;

        ContainerStyle containerStyle =
                mContainmentItemController
                        .createStandardBuilder(isTop, isBottom, /* isSingleLine= */ true)
                        .build();
        ContainmentViewStyler.applyBackgroundStyle(containerView, containerStyle);
        ContainmentViewStyler.applyMargins(containerView, containerStyle);

        if (itemViewType == ViewType.SITE_SEARCH_SETTINGS) {
            view.setOnClickListener(this);
            view.setTag(position);
            return view;
        }

        view.setOnClickListener(this);
        view.setOnLongClickListener(this); // Vivaldi
        view.setTag(position);

        RadioButton radioButton = view.findViewById(R.id.radiobutton);
        final boolean selected = position == mSelectedSearchEnginePosition;
        radioButton.setChecked(selected);

        TextView description = view.findViewById(R.id.name);

        TemplateUrl templateUrl = (TemplateUrl) getItem(position);
        assumeNonNull(templateUrl);
        description.setText(templateUrl.getShortName());

        TextView url = view.findViewById(R.id.url);
        url.setText(templateUrl.getKeyword());
        if (TextUtils.isEmpty(templateUrl.getKeyword())) {
            url.setVisibility(View.GONE);
        }

        ImageView logoView = view.findViewById(R.id.logo);
        GURL faviconUrl =
                new GURL(
                        templateUrlService.getSearchEngineUrlFromTemplateUrl(
                                templateUrl.getKeyword()));

        updateLogo(logoView, templateUrl, faviconUrl);

        radioButton.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        containerView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);

        view.setAccessibilityDelegate(
                new AccessibilityDelegate() {
                    @Override
                    public void onInitializeAccessibilityNodeInfo(
                            View host, AccessibilityNodeInfo info) {
                        super.onInitializeAccessibilityNodeInfo(host, info);
                        info.setSelected(selected);
                    }
                });


        // Vivaldi - Custom search engine changes
        TextView shortcut = view.findViewById(R.id.shortcut);
        if (shortcut != null) { // Vivaldi VAB-9739
        shortcut.setText(templateUrl.getKeyword());
        }
        try {
            URL itemUrl = new URL(templateUrl.getURL());
            if (BuildConfig.IS_VIVALDI && templateUrl.getKeyword() != null)
                url.setText(templateUrl.getKeyword());
            else
            url.setText(itemUrl.getHost());
        } catch (MalformedURLException e) {
            if (templateUrl.getURL().contains("{google:baseURL}"))
                // Vivaldi - Handling the corner case for Google search engine. Ref: VAB-11667
                if (BuildConfig.IS_VIVALDI && templateUrl.getKeyword() != null) {
                    url.setText(templateUrl.getKeyword());
                } else // Vivaldi End
                url.setText("google.com");
        }

        return view;
    }

    private void updateLogo(ImageView logoView, TemplateUrl templateUrl, GURL faviconUrl) {
        SearchEngineIconUtils.updateIcon(
                mContext, logoView, templateUrl, faviconUrl, mLargeIconBridge, mIconCache);
    }

    // TemplateUrlService.LoadListener

    @Override
    public void onTemplateUrlServiceLoaded() {
        TemplateUrlServiceFactory.getForProfile(mProfile).unregisterLoadListener(this);
        mHasLoadObserver = false;
        refreshData();
    }

    @Override
    public void onTemplateURLServiceChanged() {
        refreshData();
    }

    // OnClickListener:

    @Override
    public void onClick(View view) {
        int position = (int) view.getTag();
        if (getItemViewType(position) == ViewType.SITE_SEARCH_SETTINGS) {
            mSiteSearchClickHandler.run();
            return;
        }
        searchEngineSelected(position);
    }

    private String searchEngineSelected(int position) {
        // Record the change in search engine.
        mSelectedSearchEnginePosition = position;

        String keyword = toKeyword(mSelectedSearchEnginePosition);
        if (mProfile.isOffTheRecord()) {
            TemplateUrlServiceFactory.getForProfile(mProfile).setSearchEngine(keyword,
                    TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_PRIVATE);
        } else {
            TemplateUrlServiceFactory.getForProfile(mProfile).setSearchEngine(keyword);
        }

        // If the user has manually set the default search engine, disable auto switching.
        boolean manualSwitch = mSelectedSearchEnginePosition != mInitialEnginePosition;
        if (manualSwitch) {
            RecordUserAction.record("SearchEngine_ManualChange");
            assumeNonNull(mDisableAutoSwitchRunnable);
            mDisableAutoSwitchRunnable.run();
        }
        notifyDataSetChanged();
        return keyword;
    }

    private int computeStartIndexForRecentSearchEngines() {
        // If there are custom search engines to show, add 1 for showing the
        // "Recently visited" header.
        if (mRecentSearchEngines.size() > 0) {
            return mPrepopulatedSearchEngines.size() + 1;
        }
        return mPrepopulatedSearchEngines.size();
    }

    // Vivaldi
    @Override
    public boolean onLongClick(View view) {
        String keyword = toKeyword((int) view.getTag());
        PopupMenu popupMenu = new PopupMenu(view.getContext(), view);
        popupMenu.inflate(R.menu.edit_custom_search_engine);
        popupMenu.setOnMenuItemClickListener(menuItem -> {
            if (menuItem.getItemId() == R.id.edit_search_engine_button) {
                Toast.makeText(mContext,
                        mContext.getString(R.string.vivaldi_custom_search_engine_edit_text),
                        Toast.LENGTH_LONG);
                Fragment fragment =
                        Fragment.instantiate(mContext, EDIT_SEARCH_ENGINE_FRAGMENT, null);
                Bundle bundle = new Bundle();
                bundle.putString("keyword", keyword);
                bundle.putBoolean("isPrivate", mProfile.isIncognitoBranded());
                fragment.setArguments(bundle);
                ((FragmentActivity) mContext)
                        .getSupportFragmentManager()
                        .beginTransaction()
                        .addToBackStack(null)
                        .add(android.R.id.content, fragment)
                        .commit();
                return true;
            } else if (menuItem.getItemId() == R.id.delete_search_engine) {
                TemplateUrlService service = TemplateUrlServiceFactory.getForProfile(mProfile);
                @Nullable
                TemplateUrl defaultSearch = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_MAIN);
                @Nullable
                TemplateUrl defaultSearchPrivate = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_PRIVATE);
                @Nullable
                TemplateUrl defaultSearchField = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_FIELD);
                @Nullable
                TemplateUrl defaultSearchFieldPrivate = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_FIELD_PRIVATE);
                @Nullable
                TemplateUrl defaultSearchSpeedDial = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_SPEED_DIALS);
                @Nullable
                TemplateUrl defaultSearchSpeedDialPrivate = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_SPEED_DIALS_PRIVATE);
                @Nullable
                TemplateUrl defaultSearchImage = service.vivaldiGetDefaultSearchEngine(
                        TemplateUrlService.DefaultSearchType.DEFAULT_SEARCH_IMAGE);
                if ((defaultSearch != null && defaultSearchPrivate != null
                            && defaultSearchField != null && defaultSearchFieldPrivate != null
                            && defaultSearchSpeedDial != null
                            && defaultSearchSpeedDialPrivate != null && defaultSearchImage != null)
                        && (defaultSearch.getKeyword().equals(keyword)
                                || defaultSearchPrivate.getKeyword().equals(keyword)
                                || defaultSearchField.getKeyword().equals(keyword)
                                || defaultSearchFieldPrivate.getKeyword().equals(keyword)
                                || defaultSearchSpeedDial.getKeyword().equals(keyword)
                                || defaultSearchSpeedDialPrivate.getKeyword().equals(keyword)
                                || defaultSearchImage.getKeyword().equals(keyword))) {
                    Toast.makeText(mContext,
                                 mContext.getString(
                                         R.string.vivaldi_custom_search_engine_cannot_remove_text),
                                 Toast.LENGTH_LONG)
                            .show();
                    return true;
                }
                TemplateUrlServiceFactory.getForProfile(mProfile).removeTemplateUrl(keyword);
                Toast.makeText(mContext,
                        String.format(
                                mContext.getString(
                                        R.string.vivaldi_custom_search_engine_query_string_error),
                                keyword),
                        Toast.LENGTH_LONG);
                notifyDataSetChanged();
                return true;
            }
            return false;
        });
        popupMenu.show();
        return true;
    } // End Vivaldi
    void setDisableAutoSwitchRunnable(Runnable runnable) {
        mDisableAutoSwitchRunnable = runnable;
    }
}
