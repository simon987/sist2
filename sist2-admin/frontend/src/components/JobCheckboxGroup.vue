<template>
    <div>
        <h5>{{ $t("selectJobs") }}</h5>
        <b-progress v-if="loading" striped animated value="100"></b-progress>
        <b-form-group v-else>
            <b-form-checkbox-group
                    v-if="jobs.length > 0"
                    :checked="frontend.jobs"
                    @input="frontend.jobs = $event; $emit('input')"
            >
                <div v-for="job in jobs" :key="job.name">
                    <b-form-checkbox :disabled="job.status !== 'indexed'"
                                     :value="job.name">
                        <template #default><span
                                :title="job.status !== 'indexed' ? $t('jobOptions.notIndexed') : ''"
                        >[{{ job.name }}]</span></template>
                    </b-form-checkbox>
                    <br/>
                </div>
            </b-form-checkbox-group>
            <div v-else>
                <span class="text-muted">{{ $t('jobOptions.noJobAvailable') }}</span>
                <router-link to="/">{{ $t("create") }}</router-link>
            </div>
        </b-form-group>
    </div>
</template>

<script>
import Sist2AdminApi from "@/Sist2AdminApi";

export default {
    name: "JobCheckboxGroup",
    props: ["frontend"],
    mounted() {
        Sist2AdminApi.getJobs().then(resp => {
            this._jobs = resp.data;
            this.loading = false;
        });
    },
    computed: {
        jobs() {
            return this._jobs
                .filter(job => job.index_options.search_backend === this.frontend.web_options.search_backend)
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