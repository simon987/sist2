<template>
  <div class="doc-card" :class="{'sub-document': doc._props.isSubDocument}" :style="`width: ${width}px`"
       @click="$store.commit('busTnTouchStart', null)">
    <b-card
        no-body
        img-top
    >
      <!-- Info modal-->
      <DocInfoModal :show="showInfo" :doc="doc" @close="showInfo = false"></DocInfoModal>

      <ContentDiv :doc="doc"></ContentDiv>

      <!-- Thumbnail-->
      <FullThumbnail :doc="doc" :small-badge="smallBadge" @onThumbnailClick="onThumbnailClick()"></FullThumbnail>

      <!-- Audio player-->
      <audio v-if="doc._props.isAudio" ref="audio" preload="none" class="audio-fit fit" controls
             :type="doc._source.mime"
             :src="`f/${doc._id}`"
             @play="onAudioPlay()"></audio>

      <b-card-body class="padding-03">

        <!-- Title line -->
        <div style="display: flex">
          <span class="info-icon" @click="onInfoClick()"></span>
          <DocFileTitle :doc="doc"></DocFileTitle>
        </div>

        <!-- Featured line -->
        <div style="display: flex">
          <FeaturedFieldsLine :doc="doc"></FeaturedFieldsLine>
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
import FullThumbnail from "@/components/FullThumbnail";
import FeaturedFieldsLine from "@/components/FeaturedFieldsLine";


export default {
  components: {FeaturedFieldsLine, FullThumbnail, ContentDiv, DocInfoModal, DocFileTitle, TagContainer},
  props: ["doc", "width"],
  data() {
    return {
      ext: ext,
      showInfo: false,
    }
  },
  computed: {
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
  },
}
</script>
<style>
.fit {
  display: block;
  min-width: 64px;
  max-width: 100%;
  /*max-height: 400px;*/
  margin: 0 auto 0;
  width: auto;
  height: auto;
}

.audio-fit {
  height: 39px;
  vertical-align: bottom;
  display: inline;
  width: 100%;
}
</style>

<style scoped>

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

.doc-card {
  padding-left: 3px;
  padding-right: 3px;
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