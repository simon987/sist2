<template>
  <div v-if="doc._props.hasThumbnail" class="img-wrapper" @mouseenter="onTnEnter()" @mouseleave="onTnLeave()">
    <div v-if="doc._props.isAudio" class="card-img-overlay" :class="{'small-badge': smallBadge}">
      <span class="badge badge-resolution">{{ humanTime(doc._source.duration) }}</span>
    </div>

    <div
        v-if="doc._props.isImage && !hover && doc._props.tnW / doc._props.tnH < 5"
        class="card-img-overlay"
        :class="{'small-badge': smallBadge}">
      <span class="badge badge-resolution">{{ `${doc._source.width}x${doc._source.height}` }}</span>
    </div>

    <div v-if="(doc._props.isVideo || doc._props.isGif) && doc._source.duration > 0 && !hover"
         class="card-img-overlay"
         :class="{'small-badge': smallBadge}">
      <span class="badge badge-resolution">{{ humanTime(doc._source.duration) }}</span>
    </div>

    <div v-if="doc._props.isPlayableVideo" class="play">
      <svg viewBox="0 0 494.942 494.942" xmlns="http://www.w3.org/2000/svg">
        <path d="m35.353 0 424.236 247.471-424.236 247.471z"/>
      </svg>
    </div>

    <img ref="tn"
         v-if="doc._props.isPlayableImage || doc._props.isPlayableVideo"
         :src="(doc._props.isGif && hover) ? `f/${doc._id}` : `t/${doc._source.index}/${doc._id}`"
         alt=""
         :style="{height: (doc._props.isGif && hover) ? `${tnHeight()}px` : undefined}"
         class="pointer fit card-img-top" @click="onThumbnailClick()">
    <img v-else :src="`t/${doc._source.index}/${doc._id}`" alt=""
         class="fit card-img-top">
  </div>
</template>

<script>
import {humanTime} from "@/util";

export default {
  name: "FullThumbnail",
  props: ["doc", "smallBadge"],
  data() {
    return {
      hover: false
    }
  },
  methods: {
    humanTime: humanTime,
    onThumbnailClick() {
      this.$emit("onThumbnailClick");
    },
    tnHeight() {
      return this.$refs.tn.height;
    },
    onTnEnter() {
      this.hover = true;
    },
    onTnLeave() {
      this.hover = false;
    },
  }
}
</script>

<style scoped>
.img-wrapper {
  position: relative;
}

.img-wrapper:hover svg {
  fill: rgba(0, 0, 0, 1);
}

.card-img-top {
  border-top-left-radius: 0;
  border-top-right-radius: 0;
}

.play {
  position: absolute;
  width: 25px;
  height: 25px;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);
  pointer-events: none;
}

.play svg {
  fill: rgba(0, 0, 0, 0.7);
}

.badge-resolution {
  color: #212529;
  background-color: #FFC107;
}

.card-img-overlay {
  pointer-events: none;
  padding: 0.75rem;
  bottom: unset;
  top: 0;
  left: unset;
  right: unset;
}

.small-badge {
  padding: 1px 3px;
  font-size: 70%;
}

</style>