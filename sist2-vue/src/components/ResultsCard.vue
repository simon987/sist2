<template>
    <b-card v-if="lastResultsLoaded" id="results">
        <span>{{ hitCount }} {{ hitCount === 1 ? $t("hit") : $t("hits") }}</span>

        <div style="float: right">
            <b-button v-b-toggle.collapse-1 variant="primary" class="not-mobile" @click="onToggle()">{{
                $t("details")
                }}
            </b-button>

            <template v-if="hitCount !== 0">
                <SortSelect class="ml-2"></SortSelect>

                <DisplayModeToggle class="ml-2"></DisplayModeToggle>
            </template>
        </div>

        <b-collapse id="collapse-1" class="pt-2" style="clear:both;">
            <b-card>
                <b-table :items="tableItems" small borderless thead-class="hidden" class="mb-0"></b-table>

                <template v-if="!$store.state.uiSqliteMode">

                    <br/>
                    <h4>
                        {{ $t("mimeTypes") }}
                        <b-button size="sm" variant="primary" class="float-right" @click="onCopyClick">
                            <ClipboardIcon/>
                        </b-button>
                    </h4>
                    <Preloader v-if="$store.state.uiDetailsMimeAgg == null"></Preloader>
                    <b-table
                            v-else
                            sort-by="doc_count"
                            :sort-desc="true"
                            thead-class="hidden"
                            bordered
                            :items="$store.state.uiDetailsMimeAgg" small class="mb-0"
                    ></b-table>
                </template>
            </b-card>
        </b-collapse>
    </b-card>
</template>

<script>
import Vue from "vue";
import {humanFileSize} from "@/util";
import DisplayModeToggle from "@/components/DisplayModeToggle.vue";
import SortSelect from "@/components/SortSelect.vue";
import Preloader from "@/components/Preloader.vue";
import Sist2Query from "@/Sist2ElasticsearchQuery";
import ClipboardIcon from "@/components/icons/ClipboardIcon.vue";
import Sist2Api from "@/Sist2Api";

export default Vue.extend({
    name: "ResultsCard",
    components: {ClipboardIcon, Preloader, SortSelect, DisplayModeToggle},
    created() {

    },
    computed: {
        lastResultsLoaded() {
            return this.$store.state.lastQueryResults != null;
        },
        hitCount() {
            return (this.$store.state.firstQueryResults).aggregations.total_count.value;
        },
        tableItems() {
            const items = [];

            if (!this.$store.state.uiSqliteMode) {
                items.push({key: this.$t("queryTime"), value: this.took()});
            }
            items.push({key: this.$t("totalSize"), value: this.totalSize()});

            return items;
        }
    },
    methods: {
        took() {
            return (this.$store.state.lastQueryResults).took + "ms";
        },
        totalSize() {
            return humanFileSize((this.$store.state.firstQueryResults).aggregations.total_size.value);
        },
        onToggle() {
            const show = !document.getElementById("collapse-1").classList.contains("show");
            this.$store.commit("setUiShowDetails", show);

            if (this.$store.state.uiSqliteMode) {
                return;
            }

            if (show && this.$store.state.uiDetailsMimeAgg == null && !this.$store.state.optUpdateMimeMap) {
                // Mime aggs are not updated automatically, update now
                this.forceUpdateMimeAgg();
            }
        },
        onCopyClick() {
            let tsvString = "";
            this.$store.state.uiDetailsMimeAgg.slice().sort((a, b) => b["doc_count"] - a["doc_count"]).forEach(row => {
                tsvString += `${row["key"]}\t${row["doc_count"]}\n`;
            });

            navigator.clipboard.writeText(tsvString);

            this.$bvToast.toast(
                this.$t("toast.copiedToClipboard"),
                {
                    title: null,
                    noAutoHide: false,
                    toaster: "b-toaster-bottom-right",
                    headerClass: "hidden",
                    bodyClass: "toast-body-info",
                });
        },
        forceUpdateMimeAgg() {
            const query = Sist2Query.searchQuery();
            Sist2Api.getMimeTypes(query).then(({buckets}) => {
                this.$store.commit("setUiDetailsMimeAgg", buckets);
            });
        }
    },
});

</script>

<style>
#results {
    margin-top: 1em;

    box-shadow: 0 .125rem .25rem rgba(0, 0, 0, .08) !important;
    border-radius: 0;
    border: none;
}

#results .card-body {
    padding: 0.7em 1.25em;
}

.hidden {
    display: none;
}
</style>