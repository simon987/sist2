<template>
  <div>
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

    <b-form-checkbox v-model="options.es_insecure_ssl" :disabled="!this.options.es_url.startsWith('https')" @change="update()">
      {{ $t("webOptions.esInsecure") }}
    </b-form-checkbox>

    <label>{{ $t("webOptions.esIndex") }}</label>
    <b-form-input v-model="options.es_index" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.lang") }}</label>
    <b-form-select v-model="options.lang" :options="['en', 'fr', 'zh-CN']" @change="update()"></b-form-select>

    <label>{{ $t("webOptions.bind") }}</label>
    <b-form-input v-model="options.bind" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.tagline") }}</label>
    <b-form-textarea v-model="options.tagline" @change="update()"></b-form-textarea>

    <label>{{ $t("webOptions.auth") }}</label>
    <b-form-input v-model="options.auth" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.tagAuth") }}</label>
    <b-form-input v-model="options.tag_auth" @change="update()"></b-form-input>

    <br>
    <h5>Auth0 options</h5>
    <label>{{ $t("webOptions.auth0Audience") }}</label>
    <b-form-input v-model="options.auth0_audience" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.auth0Domain") }}</label>
    <b-form-input v-model="options.auth0_domain" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.auth0ClientId") }}</label>
    <b-form-input v-model="options.auth0_client_id" @change="update()"></b-form-input>

    <label>{{ $t("webOptions.auth0PublicKey") }}</label>
    <b-textarea rows="10" v-model="options.auth0_public_key" @change="update()"></b-textarea>



  </div>
</template>

<script>

import sist2AdminApi from "@/Sist2AdminApi";

export default {
  name: "WebOptions",
  props: ["options", "frontendName"],
  data() {
    return {
      showEsTestAlert: false,
      esTestOk: false,
      esTestMessage: "",
    }
  },
  methods: {
    update() {
      if (!this.options.es_url.startsWith("https")) {
        this.options.es_insecure_ssl = false;
      }

      this.$emit("change", this.options);
    },
    testEs() {
      sist2AdminApi.pingEs(this.options.es_url, this.options.es_insecure_ssl).then((resp) => {
        this.showEsTestAlert = true;
        this.esTestOk = resp.data.ok;
        this.esTestMessage = resp.data.message;
      });
    }
  }
}
</script>

<style scoped>

</style>
