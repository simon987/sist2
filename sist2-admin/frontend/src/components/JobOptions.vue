<template>
    <div>
        <b-form-checkbox :checked="desktopNotificationsEnabled" @change="updateNotifications($event)">
            {{ $t("jobOptions.desktopNotifications") }}
        </b-form-checkbox>

        <b-form-checkbox v-model="job.schedule_enabled" @change="update()">
            {{ $t("jobOptions.scheduleEnabled") }}
        </b-form-checkbox>

        <label>{{ $t("jobOptions.cron") }}</label>
        <b-form-input class="text-monospace" :state="cronValid" v-model="job.cron_expression"
                      :disabled="!job.schedule_enabled" @change="update()"></b-form-input>

        <label>{{ $t("jobOptions.keepNLogs") }}</label>
        <b-input-group>
            <b-form-input type="number" v-model="job.keep_last_n_logs" @change="update()"></b-form-input>
            <b-input-group-append>
                <b-button variant="danger" @click="onDeleteNowClick()">{{ $t("jobOptions.deleteNow") }}</b-button>
            </b-input-group-append>
        </b-input-group>

    </div>
</template>

<script>

import Sist2AdminApi from "@/Sist2AdminApi";

export default {
    name: "JobOptions",
    props: ["job"],
    data() {
        return {
            cronValid: undefined,
            logsToDelete: null
        }
    },
    computed: {
        desktopNotificationsEnabled() {
            return this.$store.state.jobDesktopNotificationMap[this.job.name];
        }
    },
    mounted() {
        this.cronValid = this.checkCron(this.job.cron_expression)
    },
    methods: {
        checkCron(expression) {
            return /((((\d+,)+\d+|(\d+([/-])\d+)|\d+|\*) ?){5,7})/.test(expression);
        },
        updateNotifications(value) {
            this.$store.dispatch("setJobDesktopNotification", {
                job: this.job.name,
                enabled: value
            });
        },
        update() {
            if (this.job.schedule_enabled) {
                this.cronValid = this.checkCron(this.job.cron_expression);
            } else {
                this.cronValid = undefined;
            }

            if (this.cronValid !== false) {
                this.$emit("change", this.job);
            }
        },
        onDeleteNowClick() {
            Sist2AdminApi.getLogsToDelete(this.job.name, this.job.keep_last_n_logs).then(resp => {
                const toDelete = resp.data;
                const message = `Delete ${toDelete.length} log files?`;

                this.$bvModal.msgBoxConfirm(message, {
                    title: this.$t("confirmation"),
                    size: "sm",
                    buttonSize: "sm",
                    okVariant: "danger",
                    okTitle: this.$t("delete"),
                    cancelTitle: this.$t("cancel"),
                    footerClass: "p-2",
                    hideHeaderClose: false,
                    centered: true
                }).then(value => {
                    if (value) {
                        toDelete.forEach(row => {
                            Sist2AdminApi.deleteTaskLogs(row["id"]);
                        });
                    }
                });
            })
        }
    },
}
</script>