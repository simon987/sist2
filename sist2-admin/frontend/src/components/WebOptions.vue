<template>
    <div>
        <h4>{{ $t("webOptions.title") }}</h4>
        <b-card>
            <label>{{ $t("webOptions.lang") }}</label>
            <b-form-select v-model="options.lang" :options="['en', 'fr', 'zh-CN', 'pl', 'de']"
                           @change="update()"></b-form-select>

            <label>{{ $t("webOptions.bind") }}</label>
            <b-form-input v-model="options.bind" @change="update()"></b-form-input>

            <label>{{ $t("webOptions.tagline") }}</label>
            <b-form-textarea v-model="options.tagline" @change="update()"></b-form-textarea>

            <label>{{ $t("webOptions.auth") }}</label>
            <b-form-input v-model="options.auth" @change="update()"></b-form-input>

            <label>{{ $t("webOptions.tagAuth") }}</label>
            <b-form-input v-model="options.tag_auth" @change="update()"></b-form-input>
        </b-card>

        <br>
        <h4>Auth0 options</h4>
        <b-card>
            <label>{{ $t("webOptions.auth0Audience") }}</label>
            <b-form-input v-model="options.auth0_audience" @change="update()"></b-form-input>

            <label>{{ $t("webOptions.auth0Domain") }}</label>
            <b-form-input v-model="options.auth0_domain" @change="update()"></b-form-input>

            <label>{{ $t("webOptions.auth0ClientId") }}</label>
            <b-form-input v-model="options.auth0_client_id" @change="update()"></b-form-input>

            <label>{{ $t("webOptions.auth0PublicKey") }}</label>
            <b-textarea rows="10" v-model="options.auth0_public_key" @change="update()"></b-textarea>
        </b-card>
    </div>
</template>

<script>

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
            this.$emit("change", this.options);
        },
    }
}
</script>

<style scoped>

</style>
