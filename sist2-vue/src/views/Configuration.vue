<template>
  <!--  <div :style="{width: `${$store.getters.optContainerWidth}px`}"-->
  <div
      v-if="!configLoading"
      style="margin-left: auto; margin-right: auto;" class="container">

    <b-card>
      <b-card-title>
        <GearIcon></GearIcon>
        {{ $t("config") }}
      </b-card-title>
      <p>{{ $t("configDescription") }}</p>

      <b-card-body>
        <h4>{{ $t("displayOptions") }}</h4>

        <b-card>

          <label><LanguageIcon/><span style="vertical-align: middle">&nbsp;{{ $t("opt.lang") }}</span></label>
          <b-form-select :options="langOptions" :value="optLang" @input="setOptLang"></b-form-select>

          <label>{{ $t("opt.theme") }}</label>
          <b-form-select :options="themeOptions" :value="optTheme" @input="setOptTheme"></b-form-select>

          <label>{{ $t("opt.displayMode") }}</label>
          <b-form-select :options="displayModeOptions" :value="optDisplay" @input="setOptDisplay"></b-form-select>

          <label>{{ $t("opt.columns") }}</label>
          <b-form-select :options="columnsOptions" :value="optColumns" @input="setOptColumns"></b-form-select>

          <div style="height: 10px"></div>

          <b-form-checkbox :checked="optLightboxLoadOnlyCurrent" @input="setOptLightboxLoadOnlyCurrent">
            {{ $t("opt.lightboxLoadOnlyCurrent") }}
          </b-form-checkbox>

          <b-form-checkbox :checked="optHideLegacy" @input="setOptHideLegacy">
            {{ $t("opt.hideLegacy") }}
          </b-form-checkbox>

          <b-form-checkbox :checked="optUpdateMimeMap" @input="setOptUpdateMimeMap">
            {{ $t("opt.updateMimeMap") }}
          </b-form-checkbox>

          <b-form-checkbox :checked="optUseDatePicker" @input="setOptUseDatePicker">
            {{ $t("opt.useDatePicker") }}
          </b-form-checkbox>

          <b-form-checkbox :checked="optSimpleLightbox" @input="setOptSimpleLightbox">{{
              $t("opt.simpleLightbox")
            }}
          </b-form-checkbox>
        </b-card>

        <br/>
        <h4>{{ $t("searchOptions") }}</h4>
        <b-card>
          <b-form-checkbox :checked="optHideDuplicates" @input="setOptHideDuplicates">{{
              $t("opt.hideDuplicates")
            }}
          </b-form-checkbox>

          <b-form-checkbox :checked="optHighlight" @input="setOptHighlight">{{ $t("opt.highlight") }}</b-form-checkbox>
          <b-form-checkbox :checked="optTagOrOperator" @input="setOptTagOrOperator">{{
              $t("opt.tagOrOperator")
            }}
          </b-form-checkbox>
          <b-form-checkbox :checked="optFuzzy" @input="setOptFuzzy">{{ $t("opt.fuzzy") }}</b-form-checkbox>
          <b-form-checkbox :checked="optSearchInPath" @input="setOptSearchInPath">{{
              $t("opt.searchInPath")
            }}
          </b-form-checkbox>
          <b-form-checkbox :checked="optSuggestPath" @input="setOptSuggestPath">{{
              $t("opt.suggestPath")
            }}
          </b-form-checkbox>

          <br/>
          <label>{{ $t("opt.fragmentSize") }}</label>
          <b-form-input :value="optFragmentSize" step="10" type="number" min="0"
                        @input="setOptFragmentSize"></b-form-input>

          <label>{{ $t("opt.resultSize") }}</label>
          <b-form-input :value="optResultSize" type="number" min="10"
                        @input="setOptResultSize"></b-form-input>

          <label>{{ $t("opt.queryMode") }}</label>
          <b-form-select :options="queryModeOptions" :value="optQueryMode" @input="setOptQueryMode"></b-form-select>

          <label>{{ $t("opt.slideDuration") }}</label>
          <b-form-input :value="optLightboxSlideDuration" type="number" min="1"
                        @input="setOptLightboxSlideDuration"></b-form-input>

          <label>{{ $t("opt.vidPreviewInterval") }}</label>
          <b-form-input :value="optVidPreviewInterval" type="number" min="50"
                        @input="setOptVidPreviewInterval"></b-form-input>
        </b-card>

        <h4 class="mt-3">{{ $t("treemapOptions") }}</h4>
        <b-card>
          <label>{{ $t("opt.treemapType") }}</label>
          <b-form-select :value="optTreemapType" :options="treemapTypeOptions"
                         @input="setOptTreemapType"></b-form-select>

          <label>{{ $t("opt.treemapTiling") }}</label>
          <b-form-select :value="optTreemapTiling" :options="treemapTilingOptions"
                         @input="setOptTreemapTiling"></b-form-select>

          <label>{{ $t("opt.treemapColorGroupingDepth") }}</label>
          <b-form-input :value="optTreemapColorGroupingDepth" type="number" min="1"
                        @input="setOptTreemapColorGroupingDepth"></b-form-input>

          <label>{{ $t("opt.treemapSize") }}</label>
          <b-form-select :value="optTreemapSize" :options="treemapSizeOptions"
                         @input="setOptTreemapSize"></b-form-select>

          <template v-if="$store.getters.optTreemapSize === 'custom'">
            <!-- TODO Width/Height input -->
            <b-form-input type="number" min="0" step="10"></b-form-input>
            <b-form-input type="number" min="0" step="10"></b-form-input>
          </template>

          <label>{{ $t("opt.treemapColor") }}</label>
          <b-form-select :value="optTreemapColor" :options="treemapColorOptions"
                         @input="setOptTreemapColor"></b-form-select>
        </b-card>

        <b-button variant="danger" class="mt-4" @click="onResetClick()">{{ $t("configReset") }}</b-button>
      </b-card-body>
    </b-card>

    <b-card v-if="loading" class="mt-4">
      <Preloader></Preloader>
    </b-card>
    <DebugInfo v-else></DebugInfo>
  </div>
</template>

<script>
import {mapActions, mapGetters, mapMutations} from "vuex";
import DebugInfo from "@/components/DebugInfo.vue";
import Preloader from "@/components/Preloader.vue";
import sist2 from "@/Sist2Api";
import GearIcon from "@/components/icons/GearIcon.vue";
import LanguageIcon from "@/components/icons/LanguageIcon";

export default {
  components: {LanguageIcon, GearIcon, DebugInfo, Preloader},
  data() {
    return {
      loading: true,
      configLoading: false,
      langOptions: [
        {value: "en", text: this.$t("lang.en")},
        {value: "fr", text: this.$t("lang.fr")},
        {value: "zh-CN", text: this.$t("lang.zh-CN")},
      ],
      queryModeOptions: [
        {value: "simple", text: this.$t("queryMode.simple")},
        {value: "advanced", text: this.$t("queryMode.advanced")}
      ],
      displayModeOptions: [
        {value: "grid", text: this.$t("displayMode.grid")},
        {value: "list", text: this.$t("displayMode.list")}
      ],
      columnsOptions: [
        {value: "auto", text: this.$t("columns.auto")},
        {value: 1, text: "1"},
        {value: 2, text: "2"},
        {value: 3, text: "3"},
        {value: 4, text: "4"},
        {value: 5, text: "5"},
        {value: 6, text: "6"},
        {value: 7, text: "7"},
        {value: 8, text: "8"},
        {value: 9, text: "9"},
        {value: 10, text: "10"},
        {value: 11, text: "11"},
        {value: 12, text: "12"},
      ],
      treemapTypeOptions: [
        {value: "cascaded", text: this.$t("treemapType.cascaded")},
        {value: "flat", text: this.$t("treemapType.flat")}
      ],
      treemapTilingOptions: [
        {value: "binary", text: this.$t("treemapTiling.binary")},
        {value: "squarify", text: this.$t("treemapTiling.squarify")},
        {value: "slice", text: this.$t("treemapTiling.slice")},
        {value: "dice", text: this.$t("treemapTiling.dice")},
        {value: "sliceDice", text: this.$t("treemapTiling.sliceDice")},
      ],
      treemapSizeOptions: [
        {value: "small", text: this.$t("treemapSize.small")},
        {value: "medium", text: this.$t("treemapSize.medium")},
        {value: "large", text: this.$t("treemapSize.large")},
        {value: "x-large", text: this.$t("treemapSize.xLarge")},
        {value: "xx-large", text: this.$t("treemapSize.xxLarge")},
        // {value: "custom", text: this.$t("treemapSize.custom")},
      ],
      treemapColorOptions: [
        {value: "PuBuGn", text: "Purple-Blue-Green"},
        {value: "PuRd", text: "Purple-Red"},
        {value: "PuBu", text: "Purple-Blue"},
        {value: "YlOrBr", text: "Yellow-Orange-Brown"},
        {value: "YlOrRd", text: "Yellow-Orange-Red"},
        {value: "YlGn", text: "Yellow-Green"},
        {value: "YlGnBu", text: "Yellow-Green-Blue"},
        {value: "Plasma", text: "Plasma"},
        {value: "Magma", text: "Magma"},
        {value: "Inferno", text: "Inferno"},
        {value: "Viridis", text: "Viridis"},
        {value: "Turbo", text: "Turbo"},
      ],
      themeOptions: [
        {value: "light", text: this.$t("theme.light")},
        {value: "black", text: this.$t("theme.black")}
      ]

    }
  },
  computed: {
    ...mapGetters([
      "optTheme",
      "optDisplay",
      "optColumns",
      "optHighlight",
      "optFuzzy",
      "optSearchInPath",
      "optSuggestPath",
      "optFragmentSize",
      "optQueryMode",
      "optTreemapType",
      "optTreemapTiling",
      "optTreemapColorGroupingDepth",
      "optTreemapColor",
      "optTreemapSize",
      "optLightboxLoadOnlyCurrent",
      "optLightboxSlideDuration",
      "optResultSize",
      "optTagOrOperator",
      "optLang",
      "optHideDuplicates",
      "optHideLegacy",
      "optUpdateMimeMap",
      "optUseDatePicker",
      "optVidPreviewInterval",
      "optSimpleLightbox",
    ]),
    clientWidth() {
      return window.innerWidth;
    }
  },
  mounted() {
    sist2.getSist2Info().then(data => {
      this.setSist2Info(data);
      this.loading = false;
    });

    this.$store.subscribe((mutation) => {
      if (mutation.type.startsWith("setOpt")) {
        this.$store.dispatch("updateConfiguration");
      }
    });
  },
  methods: {
    ...mapActions({
      setSist2Info: "setSist2Info",
    }),
    ...mapMutations([
      "setOptTheme",
      "setOptDisplay",
      "setOptColumns",
      "setOptHighlight",
      "setOptFuzzy",
      "setOptSearchInPath",
      "setOptSuggestPath",
      "setOptFragmentSize",
      "setOptQueryMode",
      "setOptTreemapType",
      "setOptTreemapTiling",
      "setOptTreemapColorGroupingDepth",
      "setOptTreemapColor",
      "setOptTreemapSize",
      "setOptLightboxLoadOnlyCurrent",
      "setOptLightboxSlideDuration",
      "setOptResultSize",
      "setOptTagOrOperator",
      "setOptLang",
      "setOptHideDuplicates",
      "setOptHideLegacy",
      "setOptUpdateMimeMap",
      "setOptUseDatePicker",
      "setOptVidPreviewInterval",
      "setOptSimpleLightbox",
    ]),
    onResetClick() {
      localStorage.removeItem("sist2_configuration");
      window.location.reload();
    }
  },
}
</script>

<style>
.shrink {
  flex-grow: inherit;
}
</style>