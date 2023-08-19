<template>
    <b-progress v-if="loading" striped animated value="100"></b-progress>
    <b-card v-else>
        <b-card-title>
            {{ $route.params.name }}
            {{ $t("script") }}
        </b-card-title>

        <div class="mb-3">
            <b-button variant="danger" @click="deleteScript()">{{ $t("delete") }}</b-button>
        </div>

        <b-card>
            <h5>{{ $t("testScript") }}</h5>

            <b-row>
                <b-col cols="11">
                    <JobSelect @change="onJobSelect($event)"></JobSelect>
                </b-col>
                <b-col cols="1">
                    <b-button :disabled="!selectedTestJob" variant="primary" @click="testScript()">{{ $t("test") }}
                    </b-button>
                </b-col>
            </b-row>

        </b-card>
        <br/>

        <label>{{ $t("scriptType") }}</label>
        <b-form-select :options="['git', 'simple']" v-model="script.type" @change="update()"></b-form-select>

        <template v-if="script.type === 'git'">
            <label>{{ $t("gitRepository") }}</label>
            <b-form-input v-model="script.git_repository" placeholder="https://github.com/example/example.git"
                          @change="update()"></b-form-input>

            <label>{{ $t("extraArgs") }}</label>
            <b-form-input v-model="script.extra_args" @change="update()" class="text-monospace"></b-form-input>
        </template>

        <template v-if="script.type === 'simple'">

            <label>{{ $t("scriptCode") }}</label>
            <p>Find sist2-python documentation <a href="https://sist2-python.readthedocs.io/" target="_blank">here</a></p>
            <b-textarea rows="15" class="text-monospace" v-model="script.script" @change="update()" spellcheck="false"></b-textarea>
        </template>

        <template v-if="script.type === 'local'">
            <!-- TODO-->
        </template>


    </b-card>
</template>

<script>

import Sist2AdminApi from "@/Sist2AdminApi";
import JobOptions from "@/components/JobOptions.vue";
import JobCheckboxGroup from "@/components/JobCheckboxGroup.vue";
import JobSelect from "@/components/JobSelect.vue";

export default {
    name: "UserScript",
    components: {JobSelect, JobCheckboxGroup, JobOptions},
    data() {
        return {
            loading: true,
            script: null,
            selectedTestJob: null
        }
    },
    methods: {
        update() {
            Sist2AdminApi.updateUserScript(this.name, this.script);
        },
        onJobSelect(job) {
            this.selectedTestJob = job;
        },
        deleteScript() {
            Sist2AdminApi.deleteUserScript(this.name)
                .then(() => {
                    this.$router.push("/");
                })
                .catch(err => {
                    this.$bvToast.toast("Cannot delete user script " +
                        "because it is referenced by a job", {
                        title: "Error",
                        variant: "danger",
                        toaster: "b-toaster-bottom-right"
                    });
                })
        },
        testScript() {
            Sist2AdminApi.testUserScript(this.name, this.selectedTestJob)
                .then(() => {
                    this.$bvToast.toast(this.$t("runJobConfirmation"), {
                        title: this.$t("runJobConfirmationTitle"),
                        variant: "success",
                        toaster: "b-toaster-bottom-right"
                    });
                })
        }
    },
    mounted() {
        Sist2AdminApi.getUserScript(this.name).then(resp => {
            this.script = resp.data;
            this.loading = false;
        });
    },
    computed: {
        name() {
            return this.$route.params.name;
        },
    },
}
</script>