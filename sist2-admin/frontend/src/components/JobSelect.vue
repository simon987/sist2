<template>
    <b-progress v-if="loading" striped animated value="100"></b-progress>
    <span v-else-if="jobs.length === 0"></span>
    <b-form-select v-else :options="jobs" text-field="name" value-field="name"
                   @change="$emit('change', $event)" :value="$t('selectJob')"></b-form-select>
</template>

<script>
import Sist2AdminApi from "@/Sist2AdminApi";

export default {
    name: "JobSelect",
    mounted() {
        Sist2AdminApi.getJobs().then(resp => {
            this._jobs = resp.data;
            this.loading = false;
        });
    },
    computed: {
        jobs() {
            return [
                {name: this.$t("selectJob"), disabled: true},
                ...this._jobs.filter(job => job.index_path)
            ]
        }
    },
    data() {
        return {
            loading: true,
            _jobs: null
        }
    }
}
</script>