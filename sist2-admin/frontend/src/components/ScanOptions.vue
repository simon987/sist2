<template>
  <div>
    <label>{{ $t("scanOptions.path") }}</label>
    <b-form-input v-model="options.path" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.threads") }}</label>
    <b-form-input type="number" min="1" v-model="options.threads" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.memThrottle") }}</label>
    <b-form-input type="number" min="0" v-model="options.mem_throttle" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.thumbnailQuality") }}</label>
    <b-form-input type="number" min="1" max="31" v-model="options.thumbnail_quality" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.thumbnailCount") }}</label>
    <b-form-input type="number" min="0" max="1000" v-model="options.thumbnail_count" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.thumbnailSize") }}</label>
    <b-form-input type="number" min="100" v-model="options.thumbnail_size" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.contentSize") }}</label>
    <b-form-input type="number" min="0" v-model="options.content_size" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.rewriteUrl") }}</label>
    <b-form-input v-model="options.rewrite_url" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.depth") }}</label>
    <b-form-input type="number" min="0" v-model="options.depth" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.archive") }}</label>
    <b-form-select :options="['skip', 'list', 'shallow', 'recurse']" v-model="options.archive"
                   @change="update()"></b-form-select>

    <label>{{ $t("scanOptions.archivePassphrase") }}</label>
    <b-form-input v-model="options.archive_passphrase" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.ocrLang") }}</label>
    <b-alert variant="danger" show v-if="selectedOcrLangs.length === 0 && !disableOcrLang">{{ $t("scanOptions.ocrLangAlert") }}</b-alert>
    <b-checkbox-group :disabled="disableOcrLang" v-model="selectedOcrLangs" @input="onOcrLangChange">
      <b-checkbox v-for="lang in ocrLangs" :key="lang" :value="lang">{{ lang }}</b-checkbox>
    </b-checkbox-group>

    <!--    <b-form-input readonly v-model="options.ocr_lang" @change="update()"></b-form-input>-->

    <div style="height: 10px"></div>

    <b-form-checkbox v-model="options.ocr_images" @change="update()">
      {{ $t("scanOptions.ocrImages") }}
    </b-form-checkbox>

    <b-form-checkbox v-model="options.ocr_ebooks" @change="update()">
      {{ $t("scanOptions.ocrEbooks") }}
    </b-form-checkbox>

    <label>{{ $t("scanOptions.exclude") }}</label>
    <b-form-input v-model="options.exclude" @change="update()"
                  :placeholder="$t('scanOptions.excludePlaceholder')"></b-form-input>

    <div style="height: 10px"></div>

    <b-form-checkbox v-model="options.fast" @change="update()">
      {{ $t("scanOptions.fast") }}
    </b-form-checkbox>

    <b-form-checkbox v-model="options.checksums" @change="update()">
      {{ $t("scanOptions.checksums") }}
    </b-form-checkbox>

    <b-form-checkbox v-model="options.read_subtitles" @change="update()">
      {{ $t("scanOptions.readSubtitles") }}
    </b-form-checkbox>

    <label>{{ $t("scanOptions.memBuffer") }}</label>
    <b-form-input type="number" min="0" v-model="options.mem_buffer" @change="update()"></b-form-input>

    <label>{{ $t("scanOptions.treemapThreshold") }}</label>
    <b-form-input type="number" min="0" v-model="options.treemap_threshold" @change="update()"></b-form-input>
  </div>
</template>

<script>

export default {
  name: "ScanOptions",
  props: ["options"],
  data() {
    return {
      disableOcrLang: false,
      selectedOcrLangs: []
    }
  },
  computed: {
    ocrLangs() {
      return this.$store.state.sist2AdminInfo?.tesseract_langs || [];
    }
  },
  methods: {
    onOcrLangChange() {
      this.options.ocr_lang = this.selectedOcrLangs.join("+");
    },
    update() {
      this.disableOcrLang = this.options.ocr_images === false && this.options.ocr_ebooks === false;
      this.$emit("change", this.options);
    },
  },
  mounted() {
    this.disableOcrLang = this.options.ocr_images === false && this.options.ocr_ebooks === false;
    this.selectedOcrLangs = this.options.ocr_lang ? this.options.ocr_lang.split("+") : [];
  }
}
</script>