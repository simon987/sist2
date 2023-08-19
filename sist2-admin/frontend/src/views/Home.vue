<template>
    <div>
        <b-tabs content-class="mt-3" v-model="tab" @input="onTabChange($event)">
            <b-tab :title="$t('backendTab')">

                <b-card>
                    <b-card-title>{{ $t("searchBackends") }}</b-card-title>

                    <b-row>
                        <b-col>
                            <b-input v-model="newBackendName" :placeholder="$t('newBackendName')"></b-input>
                        </b-col>
                        <b-col>
                            <b-button variant="primary" @click="createBackend()"
                                      :disabled="!backendNameValid(newBackendName)">
                                {{ $t("create") }}
                            </b-button>
                        </b-col>
                    </b-row>

                    <hr/>

                    <b-progress v-if="backendsLoading" striped animated value="100"></b-progress>
                    <b-list-group v-else>
                        <SearchBackendListItem v-for="backend in backends"
                                               :key="backend.name" :backend="backend"></SearchBackendListItem>
                    </b-list-group>
                </b-card>

                <br/>

                <b-card>
                    <b-card-title>{{ $t("jobs") }}</b-card-title>
                    <b-row>
                        <b-col>
                            <b-input id="new-job" v-model="newJobName" :placeholder="$t('newJobName')"></b-input>
                            <b-popover
                                    :show.sync="showHelp"
                                    target="new-job"
                                    placement="top"
                                    triggers="manual"
                                    variant="primary"
                                    :content="$t('newJobHelp')"
                            ></b-popover>
                        </b-col>
                        <b-col>
                            <b-button variant="primary" @click="createJob()" :disabled="!jobNameValid(newJobName)">
                                {{ $t("create") }}
                            </b-button>
                        </b-col>
                    </b-row>

                    <hr/>

                    <b-progress v-if="jobsLoading" striped animated value="100"></b-progress>
                    <b-list-group v-else>
                        <JobListItem v-for="job in jobs" :key="job.name" :job="job"></JobListItem>
                    </b-list-group>
                </b-card>
            </b-tab>
            <b-tab :title="$t('scripts')">

                <b-progress v-if="scriptsLoading" striped animated value="100"></b-progress>
                <b-card v-else>
                    <b-card-title>{{ $t("scripts") }}</b-card-title>

                    <label>Select template</label>
                    <b-form-radio-group stacked :options="scriptTemplates" v-model="scriptTemplate"></b-form-radio-group>
                    <br>

                    <b-row>
                        <b-col>
                            <b-form-input v-model="newScriptName" :disabled="!scriptTemplate" :placeholder="$t('newScriptName')"></b-form-input>
                        </b-col>
                        <b-col>
                            <b-button variant="primary" @click="createScript()"
                                      :disabled="!scriptNameValid(newScriptName)">
                                {{ $t("create") }}
                            </b-button>
                        </b-col>
                    </b-row>

                    <hr/>

                    <b-list-group>
                        <UserScriptListItem v-for="script in scripts"
                                            :key="script.name" :script="script"></UserScriptListItem>
                    </b-list-group>

                </b-card>
            </b-tab>
            <b-tab :title="$t('frontendTab')">
                <b-card>

                    <b-card-title>{{ $t("frontends") }}</b-card-title>

                    <b-row>
                        <b-col>
                            <b-input v-model="newFrontendName" :placeholder="$t('newFrontendName')"></b-input>
                        </b-col>
                        <b-col>
                            <b-button variant="primary" @click="createFrontend()"
                                      :disabled="!frontendNameValid(newFrontendName)">
                                {{ $t("create") }}
                            </b-button>
                        </b-col>
                    </b-row>

                    <hr/>

                    <b-progress v-if="frontendsLoading" striped animated value="100"></b-progress>
                    <b-list-group v-else>
                        <FrontendListItem v-for="frontend in frontends"
                                          :key="frontend.name" :frontend="frontend"></FrontendListItem>
                    </b-list-group>

                </b-card>
            </b-tab>
        </b-tabs>
    </div>
</template>

<script>
import JobListItem from "@/components/JobListItem";
import {formatBindAddress} from "@/util";
import Sist2AdminApi from "@/Sist2AdminApi";
import FrontendListItem from "@/components/FrontendListItem";
import SearchBackendListItem from "@/components/SearchBackendListItem.vue";
import UserScriptListItem from "@/components/UserScriptListItem.vue";

export default {
    name: "Jobs",
    components: {UserScriptListItem, SearchBackendListItem, JobListItem, FrontendListItem},
    data() {
        return {
            jobsLoading: true,
            newJobName: "",
            jobs: [],

            frontendsLoading: true,
            frontends: [],
            formatBindAddress,
            newFrontendName: "",

            backends: [],
            backendsLoading: true,
            newBackendName: "",

            scripts: [],
            scriptTemplates: [],
            newScriptName: "",
            scriptTemplate: null,
            scriptsLoading: true,

            showHelp: false,
            tab: 0
        }
    },
    mounted() {
        this.loading = true;
        if (this.$route.params.tab) {
            console.log("mounted " + this.$route.params.tab)
            window.setTimeout(() => {
                this.tab = Math.round(Number(this.$route.params.tab));
            }, 1)
        }
        this.reload();
    },
    methods: {
        jobNameValid(name) {
            if (this.jobs.some(job => job.name === name)) {
                return false;
            }

            return /^[a-zA-Z0-9-_,.; ]+$/.test(name);
        },
        frontendNameValid(name) {
            if (this.frontends.some(frontend => frontend.name === name)) {
                return false;
            }

            return /^[a-zA-Z0-9-_,.; ]+$/.test(name);
        },
        backendNameValid(name) {
            if (this.backends.some(backend => backend.name === name)) {
                return false;
            }

            return /^[a-zA-Z0-9-_,.; ]+$/.test(name);
        },
        scriptNameValid(name) {
            if (this.scripts.some(script => script.name === name)) {
                return false;
            }
            if (name.length > 16) {
                return false;
            }

            return /^[a-zA-Z0-9-_,.; ]+$/.test(name);
        },
        reload() {
            Sist2AdminApi.getJobs().then(resp => {
                this.jobs = resp.data;
                this.jobsLoading = false;
                this.showHelp = this.jobs.length === 0;
            });
            Sist2AdminApi.getFrontends().then(resp => {
                this.frontends = resp.data;
                this.frontendsLoading = false;
            });
            Sist2AdminApi.getSearchBackends().then(resp => {
                this.backends = resp.data;
                this.backendsLoading = false;
            })
            Sist2AdminApi.getUserScripts().then(resp => {
                this.scripts = resp.data;
                this.scriptTemplates = this.$store.state.sist2AdminInfo.user_script_templates;
                this.scriptsLoading = false;
            })
        },
        createJob() {
            Sist2AdminApi.createJob(this.newJobName).then(this.reload);
        },
        createFrontend() {
            Sist2AdminApi.createFrontend(this.newFrontendName).then(this.reload)
        },
        createBackend() {
            Sist2AdminApi.createBackend(this.newBackendName).then(this.reload);
        },
        createScript() {
            Sist2AdminApi.createUserScript(this.newScriptName, this.scriptTemplate).then(this.reload)
        },
        onTabChange(tab) {
            if (this.$route.params.tab != tab) {
                this.$router.push({params: {tab: tab}})
            }
        }
    }
}
</script>
