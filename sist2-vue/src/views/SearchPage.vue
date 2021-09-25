<template>
  <div class="container">
    <Lightbox></Lightbox>
    <HelpDialog :show="showHelp" @close="showHelp = false"></HelpDialog>

    <b-card v-if="uiLoading">
      <Preloader></Preloader>
    </b-card>

    <b-card v-show="!uiLoading" id="search-panel">
      <SearchBar @show-help="showHelp=true"></SearchBar>
      <b-row>
        <b-col style="height: 70px;" sm="6">
          <SizeSlider></SizeSlider>
        </b-col>
        <b-col>
          <PathTree @search="search(true)"></PathTree>
        </b-col>
      </b-row>
      <b-row>
        <b-col sm="6">
          <b-row>
            <b-col style="height: 70px;">
              <DateSlider></DateSlider>
            </b-col>
          </b-row>
          <b-row>
            <b-col>
              <IndexPicker></IndexPicker>
            </b-col>
          </b-row>
        </b-col>
        <b-col>
          <b-tabs>
            <b-tab :title="$t('mimeTypes')">
              <MimePicker></MimePicker>
            </b-tab>
            <b-tab :title="$t('tags')">
              <TagPicker></TagPicker>
            </b-tab>
          </b-tabs>
        </b-col>
      </b-row>
    </b-card>

    <Preloader v-if="searchBusy && docs.length === 0" class="mt-3"></Preloader>

    <div v-else-if="docs.length > 0">
      <ResultsCard></ResultsCard>

      <DocCardWall v-if="optDisplay==='grid'" :docs="docs" :append="appendFunc"></DocCardWall>
      <DocList v-else :docs="docs" :append="appendFunc"></DocList>
    </div>
  </div>
</template>

<script lang="ts">
import Preloader from "@/components/Preloader.vue";
import {mapGetters, mapMutations} from "vuex";
import sist2 from "../Sist2Api";
import Sist2Api, {EsHit, EsResult} from "../Sist2Api";
import SearchBar from "@/components/SearchBar.vue";
import IndexPicker from "@/components/IndexPicker.vue";
import Vue from "vue";
import Sist2Query from "@/Sist2Query";
import _debounce from "lodash/debounce";
import DocCardWall from "@/components/DocCardWall.vue";
import Lightbox from "@/components/Lightbox.vue";
import LightboxCaption from "@/components/LightboxCaption.vue";
import MimePicker from "../components/MimePicker.vue";
import ResultsCard from "@/components/ResultsCard.vue";
import PathTree from "@/components/PathTree.vue";
import SizeSlider from "@/components/SizeSlider.vue";
import DateSlider from "@/components/DateSlider.vue";
import TagPicker from "@/components/TagPicker.vue";
import DocList from "@/components/DocList.vue";
import HelpDialog from "@/components/HelpDialog.vue";


export default Vue.extend({
  components: {
    HelpDialog,
    DocList,
    TagPicker,
    DateSlider,
    SizeSlider, PathTree, ResultsCard, MimePicker, Lightbox, DocCardWall, IndexPicker, SearchBar, Preloader
  },
  data: () => ({
    loading: false,
    uiLoading: true,
    search: undefined as any,
    docs: [] as EsHit[],
    docIds: new Set(),
    docChecksums: new Set(),
    searchBusy: false,
    Sist2Query: Sist2Query,
    showHelp: false
  }),
  computed: {
    ...mapGetters(["indices", "optDisplay"]),
  },
  mounted() {
    this.search = _debounce(async (clear: boolean) => {
      if (clear) {
        await this.clearResults();
      }

      await this.searchNow(Sist2Query.searchQuery());

    }, 350, {leading: false});

    Sist2Api.getMimeTypes().then(mimeMap => {
      this.$store.commit("setUiMimeMap", mimeMap);
    });

    this.$store.dispatch("loadFromArgs", this.$route).then(() => {
      this.$store.subscribe(() => this.$store.dispatch("updateArgs", this.$router));
      this.$store.subscribe((mutation) => {
        if ([
          "setSizeMin", "setSizeMax", "setDateMin", "setDateMax", "setSearchText", "setPathText",
          "setSortMode", "setOptHighlight", "setOptFragmentSize", "setFuzzy", "setSize", "setSelectedIndices",
          "setSelectedMimeTypes", "setSelectedTags", "setOptQueryMode", "setOptSearchInPath",
        ].includes(mutation.type)) {
          if (this.searchBusy) {
            return;
          }

          this.search(true);
        }
      });
    });


    this.getDateRange().then((range: { min: number, max: number }) => {
      this.setDateBoundsMin(range.min);
      this.setDateBoundsMax(range.max);

      sist2.getSist2Info().then(data => {
        this.setSist2Info(data);
        this.setIndices(data.indices);
        this.uiLoading = false;

        this.search(true);
      }).catch(() => {
        this.showErrorToast();
      });
    });
  },
  methods: {
    ...mapMutations({
      setSist2Info: "setSist2Info",
      setIndices: "setIndices",
      setDateBoundsMin: "setDateBoundsMin",
      setDateBoundsMax: "setDateBoundsMax",
      setTags: "setTags",
    }),
    showErrorToast() {
      this.$bvToast.toast(
          this.$t("toast.esConnErr"),
          {
            title: this.$t("toast.esConnErrTitle"),
            noAutoHide: true,
            toaster: "b-toaster-bottom-right",
            headerClass: "toast-header-error",
            bodyClass: "toast-body-error",
          });
    },
    showSyntaxErrorToast: function (): void {
      this.$bvToast.toast(
          this.$t("toast.esQueryErr"),
          {
            title: this.$t("toast.esQueryErrTitle"),
            noAutoHide: true,
            toaster: "b-toaster-bottom-right",
            headerClass: "toast-header-warning",
            bodyClass: "toast-body-warning",
          });
    },
    async searchNow(q: any) {
      this.searchBusy = true;
      await this.$store.dispatch("incrementQuerySequence");

      Sist2Api.esQuery(q).then(async (resp: EsResult) => {
        await this.handleSearch(resp);
        this.searchBusy = false;
      }).catch(err => {
        if (err.response.status === 500 && this.$store.state.optQueryMode === "advanced") {
          this.showSyntaxErrorToast();
        } else {
          this.showErrorToast();
        }
      });
    },
    async clearResults() {
      this.docs = [];
      this.docIds.clear();
      this.docChecksums.clear();
      await this.$store.dispatch("clearResults");
      this.$store.commit("setUiReachedScrollEnd", false);
    },
    async handleSearch(resp: EsResult) {
      if (resp.hits.hits.length == 0) {
        this.$store.commit("setUiReachedScrollEnd", true);
      }

      resp.hits.hits = resp.hits.hits.filter(hit => !this.docIds.has(hit._id));

      if (this.$store.state.optHideDuplicates) {
        resp.hits.hits = resp.hits.hits.filter(hit => {

          if (!("checksum" in hit._source)) {
              return true;
          }

          const isDupe = !this.docChecksums.has(hit._source.checksum);
          this.docChecksums.add(hit._source.checksum);
          return isDupe;
        });
      }

      for (const hit of resp.hits.hits) {
        if (hit._props.isPlayableImage || hit._props.isPlayableVideo) {
          hit._seq = await this.$store.dispatch("getKeySequence");
          this.$store.commit("addLightboxSource", {
            source: `f/${hit._id}`,
            thumbnail: hit._props.hasThumbnail
                ? `t/${hit._source.index}/${hit._id}`
                : null,
            caption: {
              component: LightboxCaption,
              props: {hit: hit}
            },
            type: hit._props.isVideo ? "video" : "image"
          });
        }
      }

      await this.$store.dispatch("remountLightbox");
      this.$store.commit("setLastQueryResult", resp);

      this.docs.push(...resp.hits.hits);
    },
    getDateRange(): Promise<{ min: number, max: number }> {
      return sist2.esQuery({
        // TODO: filter current selected indices
        aggs: {
          dateMin: {min: {field: "mtime"}},
          dateMax: {max: {field: "mtime"}},
        },
        size: 0
      }).then(res => {
        return {
          min: res.aggregations.dateMin.value,
          max: res.aggregations.dateMax.value,
        }
      })
    },
    appendFunc() {
      if (!this.$store.state.uiReachedScrollEnd && this.search && !this.searchBusy) {
        this.searchNow(Sist2Query.searchQuery());
      }
    }
  },
  beforeRouteUpdate(to, from, next) {
    if (this.$store.state.uiLightboxIsOpen) {
      this.$store.commit("_setUiShowLightbox", false);
      next(false);
    } else {
      next();
    }
  },
})
</script>

<style>

#search-panel {
  box-shadow: 0 .125rem .25rem rgba(0, 0, 0, .08) !important;
  border-radius: 0;
  border: none;
}

.toast-header-error, .toast-body-error {
  background: #a94442;
  color: #f2dede !important;
}

.toast-header-error {
  color: #fff !important;
  border-bottom: none;
  margin-bottom: -1em;
}

.toast-header-error .close {
  text-shadow: none;
}

.toast-header-warning, .toast-body-warning {
  background: #FF8F00;
  color: #FFF3E0 !important;
}
</style>