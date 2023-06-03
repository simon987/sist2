<template>
    <b-progress v-if="loading" striped animated value="100"></b-progress>
    <div v-else>
        <label>{{$t("backendOptions.searchBackend")}}</label>
        <b-select :options="options" :value="value" @change="$emit('change', $event)"></b-select>
    </div>
</template>

<script>
import Sist2AdminApi from "@/Sist2AdminApi";

export default {
    name: "SearchBackendSelect",
    props: ["value"],
    data() {
        return {
            loading: true,
            backends: null,
        }
    },
    computed: {
        options() {
            return this.backends.map(backend => backend.name)
        }
    },
    mounted() {
        Sist2AdminApi.getSearchBackends().then(resp => {
            this.loading = false;
            this.backends = resp.data
        })
    }
}
</script>

<style scoped>

</style>