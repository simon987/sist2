<template>
  <VueMultiselect
      multiple
      label="name"
      :value="selectedIndices"
      :options="indices"
      :close-on-select="indices.length <= 1"
      :placeholder="$t('indexPickerPlaceholder')"
      @select="addItem"
      @remove="removeItem">

    <template slot="option" slot-scope="idx">
      <b-row>
        <b-col>
          <span class="mr-1">{{ idx.option.name }}</span>
          <SmallBadge pill :text="idx.option.version"></SmallBadge>
        </b-col>
      </b-row>
      <b-row class="mt-1">
        <b-col>
          <span>{{ formatIdxDate(idx.option.timestamp) }}</span>
        </b-col>
      </b-row>
    </template>

  </VueMultiselect>
</template>

<script lang="ts">
import VueMultiselect from "vue-multiselect"
import SmallBadge from "./SmallBadge.vue"
import {mapActions, mapGetters} from "vuex";
import {Index} from "@/Sist2Api";
import Vue from "vue";
import {format} from "date-fns";

export default Vue.extend({
  components: {
    VueMultiselect,
    SmallBadge
  },
  data() {
    return {
      loading: true
    }
  },
  computed: {
    ...mapGetters([
      "indices", "selectedIndices"
    ]),
  },
  methods: {
    ...mapActions({
      setSelectedIndices: "setSelectedIndices"
    }),
    removeItem(val: Index): void {
      this.setSelectedIndices(this.selectedIndices.filter((item: Index) => item !== val))
    },
    addItem(val: Index): void {
      this.setSelectedIndices([...this.selectedIndices, val])
    },
    formatIdxDate(timestamp: number): string {
      return format(new Date(timestamp * 1000), "yyyy-MM-dd");
    }
  },
})
</script>

<style src="vue-multiselect/dist/vue-multiselect.min.css"></style>

<style>
.multiselect__option {
  padding: 5px 10px;
}

.multiselect__content-wrapper {
  overflow: hidden;
}

.theme-black .multiselect__tags {
  background: #37474F;
  border: 1px solid #616161 !important
}

.theme-black .multiselect__input {
  color: #dbdbdb;
  background: #37474F;
}

.theme-black .multiselect__content-wrapper {
  border: none
}
</style>