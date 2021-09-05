<template>
  <div>
    <h4>[{{ index.name }}]</h4>
    <b-table :items="tableItems" small borderless responsive="md" thead-class="hidden" class="mb-0"></b-table>
  </div>
</template>

<script>
import {humanDate} from "@/util";

export default {
  name: "IndexDebugInfo",
  props: ["index"],
  computed: {
    tableItems() {
      return [
        {key: this.$t("name"), value: this.index.name},
        {key: this.$t("id"), value: this.index.id},
        {key: this.$t("indexVersion"), value: this.index.version},
        {key: this.$t("rewriteUrl"), value: this.index.rewriteUrl},
        {key: this.$t("timestamp"), value: humanDate(this.index.timestamp)},
      ]
    }
  }
}
</script>

<style scoped>

</style>