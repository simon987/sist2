<template>
  <svg class="file-icon" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 50 50">
    <path
        fill="currentColor"
        d="M 7 2 L 7 48 L 43 48 L 43 14.59375 L 42.71875 14.28125 L 30.71875 2.28125 L 30.40625 2 Z M 9 4 L 29 4 L 29 16 L 41 16 L 41 46 L 9 46 Z M 31 5.4375 L 39.5625 14 L 31 14 Z"/>
  </svg>
</template>

<script>
export default {
  name: "FileIcon"
}
</script>

<style scoped>
.file-icon {
  position: absolute;
  max-height: 100%;
  max-width: 100%;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
}

.theme-black .file-icon {
  color: #ffffff50;
}

.theme-light .file-icon {
  color: #000000a0;
}
</style>