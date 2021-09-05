<template>
  <div class="doc-card" :class="{'sub-document': doc._props.isSubDocument}" :style="`width: ${width}px`">
    <b-card
        no-body
        img-top
    >
      <!-- Info modal-->
      <DocInfoModal :show="showInfo" :doc="doc" @close="showInfo = false"></DocInfoModal>

      <ContentDiv :doc="doc"></ContentDiv>

      <!-- Thumbnail-->
      <div v-if="doc._props.hasThumbnail" class="img-wrapper" @mouseenter="onTnEnter()" @mouseleave="onTnLeave()">
        <div v-if="doc._props.isAudio" class="card-img-overlay" :class="{'small-badge': smallBadge}">
          <span class="badge badge-resolution">{{ humanTime(doc._source.duration) }}</span>
        </div>

        <div v-if="doc._props.isImage && !hover" class="card-img-overlay" :class="{'small-badge': smallBadge}">
          <span class="badge badge-resolution">{{ `${doc._source.width}x${doc._source.height}` }}</span>
        </div>

        <div v-if="(doc._props.isVideo || doc._props.isGif) && doc._source.duration > 0 && !hover" class="card-img-overlay"
             :class="{'small-badge': smallBadge}">
          <span class="badge badge-resolution">{{ humanTime(doc._source.duration) }}</span>
        </div>

        <div v-if="doc._props.isPlayableVideo" class="play">
          <svg viewBox="0 0 494.942 494.942" xmlns="http://www.w3.org/2000/svg">
            <path d="m35.353 0 424.236 247.471-424.236 247.471z"/>
          </svg>
        </div>

        <img v-if="doc._props.isPlayableImage || doc._props.isPlayableVideo"
             :src="(doc._props.isGif && hover) ? `f/${doc._id}` : `t/${doc._source.index}/${doc._id}`"
             alt=""
             class="pointer fit card-img-top" @click="onThumbnailClick()">
        <img v-else :src="`t/${doc._source.index}/${doc._id}`" alt=""
             class="fit card-img-top">
      </div>

      <!-- Audio player-->
      <audio v-if="doc._props.isAudio" ref="audio" preload="none" class="audio-fit fit" controls :type="doc._source.mime"
             :src="`f/${doc._id}`"
             @play="onAudioPlay()"></audio>

      <b-card-body class="padding-03">

        <!-- Title line -->
        <div style="display: flex">
          <span class="info-icon" @click="onInfoClick()"></span>
          <DocFileTitle :doc="doc"></DocFileTitle>
        </div>

        <!-- Tags -->
        <div class="card-text">
          <TagContainer :hit="doc"></TagContainer>
        </div>
      </b-card-body>
    </b-card>
  </div>
</template>

<script>
import {ext, humanFileSize, humanTime} from "@/util";
import TagContainer from "@/components/TagContainer.vue";
import DocFileTitle from "@/components/DocFileTitle.vue";
import DocInfoModal from "@/components/DocInfoModal.vue";
import ContentDiv from "@/components/ContentDiv.vue";


export default {
  components: {ContentDiv, DocInfoModal, DocFileTitle, TagContainer},
  props: ["doc", "width"],
  data() {
    return {
      ext: ext,
      showInfo: false,
      hover: false
    }
  },
  computed: {
    placeHolderStyle() {

      const tokens = this.doc._source.thumbnail.split(",");
      const w = Number(tokens[0]);
      const h = Number(tokens[1]);

      const MAX_HEIGHT = 400;

      return {
        height: `${Math.min((h / w) * this.width, MAX_HEIGHT)}px`,
      }
    },
    smallBadge() {
      return this.width < 150;
    }
  },
  methods: {
    humanFileSize: humanFileSize,
    humanTime: humanTime,
    onInfoClick() {
      this.showInfo = true;
    },
    async onThumbnailClick() {
      this.$store.commit("setUiLightboxSlide", this.doc._seq);
      await this.$store.dispatch("showLightbox");
    },
    onAudioPlay() {
      document.getElementsByTagName("audio").forEach((el) => {
        if (el !== this.$refs["audio"]) {
          el.pause();
        }
      });
    },
    onTnEnter() {
      this.hover = true;
    },
    onTnLeave() {
      this.hover = false;
    }
  },
}
</script>
<style>
.img-wrapper {
  position: relative;
}

.img-wrapper:hover svg {
  fill: rgba(0, 0, 0, 1);
}

.pointer {
  cursor: pointer;
}

.fit {
  display: block;
  min-width: 64px;
  max-width: 100%;
  /*max-height: 400px;*/
  margin: 0 auto 0;
  width: auto;
  height: auto;
}
</style>

<style scoped>

.card-img-top {
  border-top-left-radius: 0;
  border-top-right-radius: 0;
}

.padding-03 {
  padding: 0.3rem;
}

.card {
  margin-top: 1em;
  margin-left: 0;
  margin-right: 0;
  box-shadow: 0 .125rem .25rem rgba(0, 0, 0, .08) !important;
  border-radius: 0;
  border: none;
}

.card-body {
  padding: 0.3rem;
}

.thumbnail-placeholder {

}

.card-img-overlay {
  pointer-events: none;
  padding: 0.75rem;
  bottom: unset;
  top: 0;
  left: unset;
  right: unset;
}

.badge-resolution {
  color: #212529;
  background-color: #FFC107;
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

.doc-card {
  padding-left: 3px;
  padding-right: 3px;
}

.small-badge {
  padding: 1px 3px;
  font-size: 70%;
}

.audio-fit {
  height: 39px;
  vertical-align: bottom;
  display: inline;
  width: 100%;
}

.sub-document .card {
  background: #AB47BC1F !important;
}

.theme-black .sub-document .card {
  background: #37474F !important;
}

.sub-document .fit {
  padding: 4px 4px 0 4px;
}
</style>