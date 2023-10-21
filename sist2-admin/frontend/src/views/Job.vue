<template>
    <b-card>
        <b-card-title>
            [{{ getName() }}]
            {{ $t("jobTitle") }}
        </b-card-title>

        <div class="mb-3">

            <b-dropdown
                    split
                    split-variant="primary"
                    variant="primary"
                    :text="$t('runNow')"
                    class="mr-1"
                    :disabled="!valid"
                    @click="runJob()"
            >
                <b-dropdown-item href="#" @click="runJob(true)">{{ $t("runNowFull") }}</b-dropdown-item>
            </b-dropdown>

            <b-button variant="danger" @click="deleteJob()">{{ $t("delete") }}</b-button>
        </div>

        <div v-if="job">
            {{ $t("status") }}: <code>{{ job.status }}</code>
        </div>

        <b-progress v-if="loading" striped animated value="100"></b-progress>
        <b-card-body v-else>

            <h4>{{ $t("jobOptions.title") }}</h4>
            <b-card>
                <JobOptions :job="job" @change="update"></JobOptions>
            </b-card>

            <br/>

            <h4>{{ $t("backendOptions.title") }}</h4>
            <b-card>
                <b-alert v-if="!valid" variant="warning" show>{{ $t("jobOptions.noBackendError") }}</b-alert>
                <SearchBackendSelect :value="job.index_options.search_backend"
                                     @change="onBackendSelect($event)"></SearchBackendSelect>
            </b-card>
            <br/>

            <h4>{{ $t("scriptOptions") }}</h4>
            <b-card>
                <UserScriptPicker :selected-scripts="job.user_scripts"
                                  @change="onScriptChange($event)"></UserScriptPicker>
            </b-card>

            <br/>

            <h4>{{ $t("scanOptions.title") }}</h4>
            <b-card>
                <ScanOptions :options="job.scan_options" @change="update()"></ScanOptions>
            </b-card>

        </b-card-body>

    </b-card>
</template>

<script>
import ScanOptions from "@/components/ScanOptions";
import Sist2AdminApi from "@/Sist2AdminApi";
import JobOptions from "@/components/JobOptions";
import SearchBackendSelect from "@/components/SearchBackendSelect.vue";
import UserScriptPicker from "@/components/UserScriptPicker.vue";

export default {
    name: "Job",
    components: {
        UserScriptPicker,
        SearchBackendSelect,
        ScanOptions,
        JobOptions
    },
    data() {
        return {
            loading: true,
            job: null,
            console: console
        }
    },
    methods: {
        getName() {
            return this.$route.params.name;
        },
        update() {
            Sist2AdminApi.updateJob(this.getName(), this.job);
        },
        runJob(full = false) {
            Sist2AdminApi.runJob(this.getName(), full).then(() => {
                this.$bvToast.toast(this.$t("runJobConfirmation"), {
                    title: this.$t("runJobConfirmationTitle"),
                    variant: "success",
                    toaster: "b-toaster-bottom-right"
                });
            });
        },
        deleteJob() {
            Sist2AdminApi.deleteJob(this.getName())
                .then(() => {
                    this.$router.push("/");
                })
                .catch(err => {
                    this.$bvToast.toast("Cannot delete job " +
                        "because it is referenced by a frontend", {
                        title: "Error",
                        variant: "danger",
                        toaster: "b-toaster-bottom-right"
                    });
                })
        },
        onBackendSelect(backend) {
            this.job.index_options.search_backend = backend;
            this.update();
        },
        onScriptChange(scripts) {
            this.job.user_scripts = scripts;
            this.update();
        }
    },
    mounted() {
        Sist2AdminApi.getJob(this.getName()).then(resp => {
            this.loading = false;
            this.job = resp.data;
        })
    },
    computed: {
        valid() {
            return this.job?.index_options.search_backend != null;
        }
    }
}
</script>