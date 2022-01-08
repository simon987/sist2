import Vue from "vue"
import Vuex from "vuex"
import VueRouter, {Route} from "vue-router";
import {EsHit, EsResult, EsTag, Index, Tag} from "@/Sist2Api";
import {deserializeMimes, serializeMimes} from "@/util";

Vue.use(Vuex)

export default new Vuex.Store({
    state: {
        seed: 0,
        indices: [] as Index[],
        tags: [] as EsTag[],
        sist2Info: null as any,

        sizeMin: undefined,
        sizeMax: undefined,
        dateBoundsMin: null,
        dateBoundsMax: null,
        dateMin: undefined,
        dateMax: undefined,
        searchText: "",
        pathText: "",
        sortMode: "score",

        fuzzy: false,
        size: 60,

        optLang: "en",
        optLangIsDefault: true,
        optHideDuplicates: true,
        optTheme: "light",
        optDisplay: "grid",

        optHighlight: true,
        optTagOrOperator: false,
        optFuzzy: true,
        optFragmentSize: 200,
        optQueryMode: "simple",
        optSearchInPath: false,
        optColumns: "auto",
        optSuggestPath: true,
        optTreemapType: "cascaded",
        optTreemapTiling: "squarify",
        optTreemapColorGroupingDepth: 3,
        optTreemapSize: "medium",
        optTreemapColor: "PuBuGn",
        optLightboxLoadOnlyCurrent: false,
        optLightboxSlideDuration: 15,
        optHideLegacy: false,

        _onLoadSelectedIndices: [] as string[],
        _onLoadSelectedMimeTypes: [] as string[],
        _onLoadSelectedTags: [] as string[],
        selectedIndices: [] as Index[],
        selectedMimeTypes: [] as string[],
        selectedTags: [] as string[],

        lastQueryResults: null,

        keySequence: 0,
        querySequence: 0,

        uiTagHover: null as Tag | null,
        uiLightboxIsOpen: false,
        uiShowLightbox: false,
        uiLightboxSources: [] as string[],
        uiLightboxThumbs: [] as string[],
        uiLightboxCaptions: [] as any[],
        uiLightboxTypes: [] as string[],
        uiLightboxKey: 0,
        uiLightboxSlide: 0,
        uiReachedScrollEnd: false,

        uiMimeMap: [] as any[]
    },
    mutations: {
        setUiReachedScrollEnd: (state, val) => state.uiReachedScrollEnd = val,
        setTags: (state, val) => state.tags = val,
        setPathText: (state, val) => state.pathText = val,
        setSizeMin: (state, val) => state.sizeMin = val,
        setSizeMax: (state, val) => state.sizeMax = val,
        setSist2Info: (state, val) => state.sist2Info = val,
        setSeed: (state, val) => state.seed = val,
        setOptHideDuplicates: (state, val) => state.optHideDuplicates = val,
        setOptLang: (state, val) => {
            state.optLang = val;
            state.optLangIsDefault = false;
        },
        setSortMode: (state, val) => state.sortMode = val,
        setIndices: (state, val) => {
            state.indices = val;

            if (state._onLoadSelectedIndices.length > 0) {

                state.selectedIndices = val.filter(
                    (idx: Index) => state._onLoadSelectedIndices.some(prefix => idx.id.startsWith(prefix))
                );
            } else {
                state.selectedIndices = val;
            }
        },
        setDateMin: (state, val) => state.dateMin = val,
        setDateMax: (state, val) => state.dateMax = val,
        setDateBoundsMin: (state, val) => state.dateBoundsMin = val,
        setDateBoundsMax: (state, val) => state.dateBoundsMax = val,
        setSearchText: (state, val) => state.searchText = val,
        setFuzzy: (state, val) => state.fuzzy = val,
        setLastQueryResult: (state, val) => state.lastQueryResults = val,
        _setOnLoadSelectedIndices: (state, val) => state._onLoadSelectedIndices = val,
        _setOnLoadSelectedMimeTypes: (state, val) => state._onLoadSelectedMimeTypes = val,
        _setOnLoadSelectedTags: (state, val) => state._onLoadSelectedTags = val,
        setSelectedIndices: (state, val) => state.selectedIndices = val,
        setSelectedMimeTypes: (state, val) => state.selectedMimeTypes = val,
        setSelectedTags: (state, val) => state.selectedTags = val,
        setUiTagHover: (state, val: Tag | null) => state.uiTagHover = val,
        setUiLightboxIsOpen: (state, val: boolean) => state.uiLightboxIsOpen = val,
        _setUiShowLightbox: (state, val: boolean) => state.uiShowLightbox = val,
        setUiLightboxKey: (state, val: number) => state.uiLightboxKey = val,
        _setKeySequence: (state, val: number) => state.keySequence = val,
        _setQuerySequence: (state, val: number) => state.querySequence = val,
        addLightboxSource: (state, {source, thumbnail, caption, type}) => {
            state.uiLightboxSources.push(source);
            state.uiLightboxThumbs.push(thumbnail);
            state.uiLightboxCaptions.push(caption);
            state.uiLightboxTypes.push(type);
        },
        setUiLightboxSlide: (state, val: number) => state.uiLightboxSlide = val,

        setUiLightboxSources: (state, val) => state.uiLightboxSources = val,
        setUiLightboxThumbs: (state, val) => state.uiLightboxThumbs = val,
        setUiLightboxTypes: (state, val) => state.uiLightboxTypes = val,
        setUiLightboxCaptions: (state, val) => state.uiLightboxCaptions = val,

        setOptTheme: (state, val) => state.optTheme = val,
        setOptDisplay: (state, val) => state.optDisplay = val,
        setOptColumns: (state, val) => state.optColumns = val,
        setOptHighlight: (state, val) => state.optHighlight = val,
        setOptFuzzy: (state, val) => state.fuzzy = val,
        setOptSearchInPath: (state, val) => state.optSearchInPath = val,
        setOptSuggestPath: (state, val) => state.optSuggestPath = val,
        setOptFragmentSize: (state, val) => state.optFragmentSize = val,
        setOptQueryMode: (state, val) => state.optQueryMode = val,
        setOptResultSize: (state, val) => state.size = val,
        setOptTagOrOperator: (state, val) => state.optTagOrOperator = val,

        setOptTreemapType: (state, val) => state.optTreemapType = val,
        setOptTreemapTiling: (state, val) => state.optTreemapTiling = val,
        setOptTreemapColorGroupingDepth: (state, val) => state.optTreemapColorGroupingDepth = val,
        setOptTreemapSize: (state, val) => state.optTreemapSize = val,
        setOptTreemapColor: (state, val) => state.optTreemapColor = val,
        setOptHideLegacy: (state, val) => state.optHideLegacy = val,

        setOptLightboxLoadOnlyCurrent: (state, val) => state.optLightboxLoadOnlyCurrent = val,
        setOptLightboxSlideDuration: (state, val) => state.optLightboxSlideDuration = val,

        setUiMimeMap: (state, val) => state.uiMimeMap = val,

        busUpdateWallItems: () => {
            // noop
        },
        busUpdateTags: () => {
            // noop
        },
    },
    actions: {
        setSist2Info: (store, val) => {
            store.commit("setSist2Info", val);

            if (store.state.optLangIsDefault) {
                store.commit("setOptLang", val.lang);
            }
        },
        loadFromArgs({commit}, route: Route) {

            if (route.query.q) {
                commit("setSearchText", route.query.q);
            }

            if (route.query.fuzzy !== undefined) {
                commit("setFuzzy", true);
            }

            if (route.query.i) {
                commit("_setOnLoadSelectedIndices", Array.isArray(route.query.i) ? route.query.i : [route.query.i,]);
            }

            if (route.query.dMin) {
                commit("setDateMin", Number(route.query.dMin))
            }

            if (route.query.dMax) {
                commit("setDateMax", Number(route.query.dMax))
            }

            if (route.query.sMin) {
                commit("setSizeMin", Number(route.query.sMin))
            }

            if (route.query.sMax) {
                commit("setSizeMax", Number(route.query.sMax))
            }

            if (route.query.path) {
                commit("setPathText", route.query.path)
            }

            if (route.query.m) {
                commit("_setOnLoadSelectedMimeTypes", deserializeMimes(route.query.m as string));
            }

            if (route.query.t) {
                commit("_setOnLoadSelectedTags", (route.query.t as string).split(","));
            }

            if (route.query.sort) {
                commit("setSortMode", route.query.sort);
                commit("setSeed", Number(route.query.seed));
            }
        },
        async updateArgs({state}, router: VueRouter) {
            await router.push({
                query: {
                    q: state.searchText.trim() ? state.searchText.trim().replace(/\s+/g, " ") : undefined,
                    fuzzy: state.fuzzy ? null : undefined,
                    i: state.selectedIndices ? state.selectedIndices.map((idx: Index) => idx.idPrefix) : undefined,
                    dMin: state.dateMin,
                    dMax: state.dateMax,
                    sMin: state.sizeMin,
                    sMax: state.sizeMax,
                    path: state.pathText ? state.pathText : undefined,
                    m: serializeMimes(state.selectedMimeTypes),
                    t: state.selectedTags.length == 0 ? undefined : state.selectedTags.join(","),
                    sort: state.sortMode === "score" ? undefined : state.sortMode,
                    seed: state.sortMode === "random" ? state.seed.toString() : undefined
                }
            }).catch(() => {
                // ignore
            });
        },
        updateConfiguration({state}) {
            const conf = {} as any;

            Object.keys(state).forEach((key) => {
                if (key.startsWith("opt")) {
                    conf[key] = (state as any)[key];
                }
            });

            localStorage.setItem("sist2_configuration", JSON.stringify(conf));
        },
        loadConfiguration({state}) {
            const confString = localStorage.getItem("sist2_configuration");
            if (confString) {
                const conf = JSON.parse(confString);

                Object.keys(state).forEach((key) => {
                    if (key.startsWith("opt")) {
                        (state as any)[key] = conf[key];
                    }
                });
            }
        },
        setSelectedIndices: ({commit}, val) => commit("setSelectedIndices", val),
        getKeySequence({commit, state}) {
            const val = state.keySequence;
            commit("_setKeySequence", val + 1);

            return val
        },
        incrementQuerySequence({commit, state}) {
            const val = state.querySequence;
            commit("_setQuerySequence", val + 1);

            return val
        },
        remountLightbox({commit, state}) {
            // Change key to an arbitrary number to force the lightbox to remount
            commit("setUiLightboxKey", state.uiLightboxKey + 1);
        },
        showLightbox({commit, state}) {
            commit("_setUiShowLightbox", !state.uiShowLightbox);
        },
        clearResults({commit}) {
            commit("setLastQueryResult", null);
            commit("_setKeySequence", 0);
            commit("_setUiShowLightbox", false);
            commit("setUiLightboxSources", []);
            commit("setUiLightboxThumbs", []);
            commit("setUiLightboxTypes", []);
            commit("setUiLightboxCaptions", []);
            commit("setUiLightboxKey", 0);
        }
    },
    modules: {},
    getters: {
        seed: (state) => state.seed,
        getPathText: (state) => state.pathText,
        indices: state => state.indices,
        sist2Info: state => state.sist2Info,
        indexMap: state => {
            const map = {} as any;
            state.indices.forEach(idx => map[idx.id] = idx);
            return map;
        },
        selectedIndices: (state) => state.selectedIndices,
        _onLoadSelectedIndices: (state) => state._onLoadSelectedIndices,
        selectedMimeTypes: (state) => state.selectedMimeTypes,
        selectedTags: (state) => state.selectedTags,
        dateMin: state => state.dateMin,
        dateMax: state => state.dateMax,
        sizeMin: state => state.sizeMin,
        sizeMax: state => state.sizeMax,
        searchText: state => state.searchText,
        pathText: state => state.pathText,
        fuzzy: state => state.fuzzy,
        size: state => state.size,
        sortMode: state => state.sortMode,
        lastQueryResult: state => state.lastQueryResults,
        lastDoc: function (state): EsHit | null {
            if (state.lastQueryResults == null) {
                return null;
            }

            return (state.lastQueryResults as unknown as EsResult).hits.hits.slice(-1)[0];
        },
        uiTagHover: state => state.uiTagHover,
        uiShowLightbox: state => state.uiShowLightbox,
        uiLightboxSources: state => state.uiLightboxSources,
        uiLightboxThumbs: state => state.uiLightboxThumbs,
        uiLightboxCaptions: state => state.uiLightboxCaptions,
        uiLightboxTypes: state => state.uiLightboxTypes,
        uiLightboxKey: state => state.uiLightboxKey,
        uiLightboxSlide: state => state.uiLightboxSlide,

        optHideDuplicates: state => state.optHideDuplicates,
        optLang: state => state.optLang,
        optTheme: state => state.optTheme,
        optDisplay: state => state.optDisplay,
        optColumns: state => state.optColumns,
        optHighlight: state => state.optHighlight,
        optTagOrOperator: state => state.optTagOrOperator,
        optFuzzy: state => state.optFuzzy,
        optSearchInPath: state => state.optSearchInPath,
        optSuggestPath: state => state.optSuggestPath,
        optFragmentSize: state => state.optFragmentSize,
        optQueryMode: state => state.optQueryMode,
        optTreemapType: state => state.optTreemapType,
        optTreemapTiling: state => state.optTreemapTiling,
        optTreemapSize: state => state.optTreemapSize,
        optTreemapColorGroupingDepth: state => state.optTreemapColorGroupingDepth,
        optTreemapColor: state => state.optTreemapColor,
        optLightboxLoadOnlyCurrent: state => state.optLightboxLoadOnlyCurrent,
        optLightboxSlideDuration: state => state.optLightboxSlideDuration,
        optResultSize: state => state.size,
        optHideLegacy: state => state.optHideLegacy,
    }
})