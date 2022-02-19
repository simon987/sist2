<template>
  <div class="thumbnail-progress-bar" :style="{width: `${percentProgress}%`}"></div>
</template>

<script>
export default {
  name: "ThumbnailProgressBar",
  props: ["doc", "progress"],
  computed: {
    percentProgress() {
      return Math.min(Math.max(this.progress * 100, 0), 100);
    }
  }
}
</script>

<style scoped>

.thumbnail-progress-bar {
  position: absolute;
  left: 0;
  bottom: 0;

  height: 4px;
  background: #2196f3AA;

  z-index: 9;
}

.theme-black .thumbnail-progress-bar {
  background: rgba(0, 188, 212, 0.95);
}

.sub-document .thumbnail-progress-bar {
  max-width: calc(100% - 8px);
  left: 4px;
}


</style>