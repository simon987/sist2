<template>
    <div>

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
    }
}
</script>

<style scoped>

</style>
