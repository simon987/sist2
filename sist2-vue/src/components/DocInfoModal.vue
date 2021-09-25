<template>
  <b-modal :visible="show" size="lg" :hide-footer="true" static lazy @close="$emit('close')" @hide="$emit('close')"
  >
    <template #modal-title>
      <h5 class="modal-title" :title="doc._source.name + ext(doc)">{{ doc._source.name + ext(doc) }}</h5>
    </template>

    <img v-if="doc._props.hasThumbnail" :src="`t/${doc._source.index}/${doc._id}`" alt="" class="fit card-img-top">

    <InfoTable :doc="doc"></InfoTable>

    <LazyContentDiv :doc-id="doc._id"></LazyContentDiv>
  </b-modal>
</template>

<script>
import {ext} from "@/util";
import InfoTable from "@/components/InfoTable";
import LazyContentDiv from "@/components/LazyContentDiv";

export default {
  name: "DocInfoModal",
  components: {LazyContentDiv, InfoTable},
  props: ["doc", "show"],
  methods: {
    ext: ext,
  }
}
</script>

<style scoped>

</style>