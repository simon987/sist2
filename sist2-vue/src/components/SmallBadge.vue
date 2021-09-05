<template>
  <b-badge variant="secondary" :pill="pill">{{ text }}</b-badge>
</template>

<script lang="ts">
import Vue from "vue";

export default Vue.extend({
  props: {
    text: String,
    pill: Boolean
  }
})
</script>

<style scoped>
.badge-pill {
  padding: 0.3em .4em 0.1em;
  border-radius: 6rem;
}
</style>