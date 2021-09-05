<template>
  <div class="lightbox-caption">
    <p>
      <b>{{
          `[${$store.getters.indices.find(i => i.id === hit._source.index).name}]`
        }}</b>{{ `/${hit._source.path}/${hit._source.name}${ext(hit)}` }}
    </p>
    <p style="margin-top: -1em">
      <span v-if="hit._source.width">{{ `${hit._source.width}x${hit._source.height}`}}</span>
      {{ ` (${humanFileSize(hit._source.size)})` }}
    </p>
  </div>
</template>

<script>
import {ext, humanFileSize} from "@/util";

export default {
  name: "LightboxCaption",
  props: ["hit"],
  methods: {
    humanFileSize: humanFileSize,
    ext: ext
  }
}
</script>