<template>
  <div class="content-div" v-html="content()" v-if="content()"></div>
</template>

<script>
export default {
  name: "ContentDiv",
  props: ["doc"],
  methods: {
    content() {
      if (!this.doc.highlight) {
        return null;
      }

      if (this.doc.highlight["content.nGram"]) {
        return this.doc.highlight["content.nGram"][0];
      }
      if (this.doc.highlight.content) {
        return this.doc.highlight.content[0];
      }
    }
  }
}
</script>

<style scoped>

</style>