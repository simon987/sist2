<template>
  <div>
    <label>{{ $t("indexOptions.threads") }}</label>
    <b-form-input v-model="options.threads" type="number" min="1" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.esUrl") }}</label>
    <b-alert :variant="esTestOk ? 'success' : 'danger'" :show="showEsTestAlert" class="mt-1">
      {{ esTestMessage }}
    </b-alert>
    <b-input-group>
      <b-form-input v-model="options.es_url" @change="update()"></b-form-input>
      <b-input-group-append>
        <b-button variant="outline-primary" @click="testEs()">{{ $t("test") }}</b-button>
      </b-input-group-append>
    </b-input-group>

    <label>{{ $t("indexOptions.esIndex") }}</label>
    <b-form-input v-model="options.es_index" @change="update()"></b-form-input>

    <br>
    <b-form-checkbox v-model="options.es_insecure_ssl" :disabled="!options.es_url.startsWith('https')" @change="update()">
      {{ $t("webOptions.esInsecure") }}
    </b-form-checkbox>

    <label>{{ $t("indexOptions.batchSize") }}</label>
    <b-form-input v-model="options.batch_size" type="number" min="1" @change="update()"></b-form-input>

    <label>{{ $t("indexOptions.script") }}</label>
    <b-form-textarea v-model="options.script" rows="6" @change="update()"></b-form-textarea>
  </div>

</template>

<script>
import sist2AdminApi from "@/Sist2AdminApi";

export default {
  name: "IndexOptions",
  props: ["options"],
  data() {
    return {
      showEsTestAlert: false,
      esTestOk: false,
      esTestMessage: "",
    }
  },
  methods: {
    update() {
      this.$emit("change", this.options);
    },
    testEs() {
      sist2AdminApi.pingEs(this.options.es_url, this.options.es_insecure_ssl).then((resp) => {
        this.showEsTestAlert = true;
        this.esTestOk = resp.data.ok;
        this.esTestMessage = resp.data.message;
      });
    }
  },
}
</script>

<style scoped>

</style>