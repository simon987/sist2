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
          <router-link :to="`/log/${data.item.logs}`">{{ $t("logs") }}</router-link>
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
          time: moment(row.started).format("dd, MMM Do YYYY, HH:mm:ss"),
          logs: row.id,
          status: row.return_code === 0 ? "ok" : "failed"
        }));
      });
    },
    update() {
      return Sist2AdminApi.getTasks().then(resp => {
        this.tasks = resp.data;
      })
    },
    taskDuration(task) {
      const start = moment(task.started);
      const end = moment(task.ended);

      let duration = moment.utc(end.diff(start)).format("HH[h] mm[m] ss[s]");

      duration = duration.replace("00h ", "");
      duration = duration.replace(/^00m /, "");
      duration = duration.replace(/00s/, "<1s");
      duration = duration.replace(/^0/, "");

      return duration;
    }
  }
}
</script>

<style scoped>
#task-history {
  font-family: monospace;
  font-size: 12px;
}
</style>
