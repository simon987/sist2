<template>
    <div>

        <b-card v-if="tasks.length > 0">
            <h2>{{ $t("runningTasks") }}</h2>
            <b-list-group>
                <TaskListItem v-for="task in tasks" :key="task.id" :task="task"></TaskListItem>
            </b-list-group>
        </b-card>

        <b-card class="mt-4">

            <b-card-title>{{ $t("taskHistory") }}</b-card-title>

            <br/>

            <b-table
                    id="task-history"
                    :items="historyItems"
                    :fields="historyFields"
                    :current-page="historyCurrentPage"
                    :tbody-tr-class="rowClass"
                    :per-page="10"
            >
                <template #cell(logs)="data">
                    <template v-if="data.item._row.has_logs">
                        <b-button variant="link" size="sm" :to="`/log/${data.item.id}`">
                            {{ $t("view") }}
                        </b-button>
                        /
                        <b-button variant="link" size="sm" @click="deleteLogs(data.item.id)">
                            {{ $t("delete") }}
                        </b-button>
                    </template>
                </template>

                <template #cell(delete)="data">
                </template>

            </b-table>

            <b-pagination limit="20" v-model="historyCurrentPage" :total-rows="historyItems.length"
                          :per-page="10"></b-pagination>

        </b-card>
    </div>
</template>

<script>
import TaskListItem from "@/components/TaskListItem";
import Sist2AdminApi from "@/Sist2AdminApi";
import moment from "moment";

const DAY = 3600 * 24;
const HOUR = 3600;
const MINUTE = 60;

function humanDuration(sec_num) {
    sec_num = sec_num / 1000;
    const days = Math.floor(sec_num / DAY);
    sec_num -= days * DAY;
    const hours = Math.floor(sec_num / HOUR);
    sec_num -= hours * HOUR;
    const minutes = Math.floor(sec_num / MINUTE);
    sec_num -= minutes * MINUTE;
    const seconds = Math.floor(sec_num);

    if (days > 0) {
        return `${days} days ${hours}h ${minutes}m ${seconds}s`;
    }

    if (hours > 0) {
        return `${hours}h ${minutes}m ${seconds}s`;
    }

    if (minutes > 0) {
        return `${minutes}m ${seconds}s`;
    }

    if (seconds > 0) {
        return `${seconds}s`;
    }

    return "<1s";
}

export default {
    name: 'Tasks',
    components: {TaskListItem},
    data() {
        return {
            loading: true,
            tasks: [],
            taskHistory: [],
            timerId: null,
            historyFields: [
                {key: "name", label: this.$t("taskName")},
                {key: "time", label: this.$t("taskStarted")},
                {key: "duration", label: this.$t("taskDuration")},
                {key: "status", label: this.$t("taskStatus")},
                {key: "logs", label: this.$t("logs")},
            ],
            historyCurrentPage: 1,
            historyItems: []
        }
    },
    props: {
        msg: String
    },
    mounted() {
        this.loading = true;
        this.update().then(() => this.loading = false);

        this.timerId = window.setInterval(this.update, 1000);
        this.updateHistory();
    },
    destroyed() {
        if (this.timerId) {
            window.clearInterval(this.timerId);
        }
    },
    methods: {
        rowClass(row) {
            if (row.status === "failed") {
                return "table-danger";
            }
            return null;
        },
        updateHistory() {
            Sist2AdminApi.getTaskHistory().then(resp => {
                this.historyItems = resp.data.map(row => ({
                    id: row.id,
                    name: row.name,
                    duration: this.taskDuration(row),
                    time: moment.utc(row.started).local().format("dd, MMM Do YYYY, HH:mm:ss"),
                    logs: null,
                    status: row.return_code === 0 ? "ok" : "failed",
                    _row: row
                }));
            });
        },
        update() {
            return Sist2AdminApi.getTasks().then(resp => {
                this.tasks = resp.data;
            })
        },
        taskDuration(task) {
            const start = moment.utc(task.started);
            const end = moment.utc(task.ended);

            return humanDuration(end.diff(start))
        },
        deleteLogs(taskId) {
            Sist2AdminApi.deleteTaskLogs(taskId).then(() => {
                this.updateHistory();
            })
        }
    }
}
</script>

<style scoped>
#task-history {
    font-family: monospace;
    font-size: 12px;
}

.btn-link {
    padding: 0;
}
</style>
