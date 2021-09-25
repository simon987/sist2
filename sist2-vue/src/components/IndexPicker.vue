<template>
  <div v-if="isMobile">
    <b-form-select
        :value="selectedIndicesIds"
        @change="onSelect($event)"
        :options="indices" multiple :select-size="6" text-field="name"
        value-field="id"></b-form-select>
  </div>
  <div v-else>
    <b-list-group id="index-picker-desktop">
      <b-list-group-item
          v-for="idx in indices"
          @click="toggleIndex(idx)"
          class="d-flex justify-content-between align-items-center list-group-item-action pointer">
        <div class="d-flex">
          <b-checkbox @change="toggleIndex(idx)" :checked="isSelected(idx)"></b-checkbox>
          {{ idx.name }}
          <span class="text-muted timestamp-text ml-2">{{ formatIdxDate(idx.timestamp) }}</span>
        </div>
        <b-badge class="version-badge">v{{ idx.version }}</b-badge>
      </b-list-group-item>
    </b-list-group>
  </div>
</template>

<script lang="ts">
import SmallBadge from "./SmallBadge.vue"
import {mapActions, mapGetters} from "vuex";
import Vue from "vue";
import {format} from "date-fns";

export default Vue.extend({
  components: {
    SmallBadge
  },
  data() {
    return {
      loading: true,
    }
  },
  computed: {
    ...mapGetters([
      "indices", "selectedIndices"
    ]),
    selectedIndicesIds() {
      return this.selectedIndices.map(idx => idx.id)
    },
    isMobile() {
      return window.innerWidth <= 650;
    }
  },
  methods: {
    ...mapActions({
      setSelectedIndices: "setSelectedIndices"
    }),
    onSelect(value) {
      this.setSelectedIndices(this.indices.filter(idx => value.includes(idx.id)));
    },
    formatIdxDate(timestamp: number): string {
      return format(new Date(timestamp * 1000), "yyyy-MM-dd");
    },
    toggleIndex(index) {
      if (this.isSelected(index)) {
        this.setSelectedIndices(this.selectedIndices.filter(idx => idx.id != index.id));
      } else {
        this.setSelectedIndices([index, ...this.selectedIndices]);
      }
    },
    isSelected(index) {
      return this.selectedIndices.find(idx => idx.id == index.id) != null;
    }
  },
})
</script>

<style scoped>
.timestamp-text {
  line-height: 24px;
  font-size: 80%;
}

.version-badge {
  color: #222 !important;
  background: none;
}

.list-group-item {
  padding: 0.2em 0.4em;
}

#index-picker-desktop {
  overflow-y: auto;
  max-height: 132px;
}
</style>