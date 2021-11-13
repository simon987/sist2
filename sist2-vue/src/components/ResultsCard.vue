<template>
  <b-card v-if="lastResultsLoaded" id="results">
    <span>{{ hitCount }} {{ hitCount === 1 ? $t("hit") : $t("hits") }}</span>

    <div style="float: right">
      <b-button v-b-toggle.collapse-1 variant="primary" class="not-mobile">{{ $t("details") }}</b-button>

      <template v-if="hitCount !== 0">
        <SortSelect class="ml-2"></SortSelect>

        <DisplayModeToggle class="ml-2"></DisplayModeToggle>
      </template>
    </div>

    <b-collapse id="collapse-1" class="pt-2" style="clear:both;">
      <b-card>
        <b-table :items="tableItems" small borderless thead-class="hidden" class="mb-0"></b-table>
      </b-card>
    </b-collapse>
  </b-card>
</template>

<script lang="ts">
import {EsResult} from "@/Sist2Api";
import Vue from "vue";
import {humanFileSize} from "@/util";
import DisplayModeToggle from "@/components/DisplayModeToggle.vue";
import SortSelect from "@/components/SortSelect.vue";

export default Vue.extend({
  name: "ResultsCard",
  components: {SortSelect, DisplayModeToggle},
  computed: {
    lastResultsLoaded() {
      return this.$store.state.lastQueryResults != null;
    },
    hitCount() {
      return (this.$store.state.lastQueryResults as EsResult).aggregations.total_count.value;
    },
    tableItems() {
      const items = [];


      items.push({key: this.$t("queryTime"), value: this.took()});
      items.push({key: this.$t("totalSize"), value: this.totalSize()});

      return items;
    }
  },
  methods: {
    took() {
      return (this.$store.state.lastQueryResults as EsResult).took + "ms";
    },
    totalSize() {
      return humanFileSize((this.$store.state.lastQueryResults as EsResult).aggregations.total_size.value);
    },
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