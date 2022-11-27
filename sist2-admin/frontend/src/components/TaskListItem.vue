<template>
  <b-list-group-item>
    <b-row style="height: 50px">
      <b-col><h5>{{ task.display_name }}</h5></b-col>
      <b-col class="shrink">
        <router-link class="btn btn-link" :to="`/log/${task.id}`">{{ $t("logs") }}</router-link>
      </b-col>
      <b-col class="shrink">
        <b-btn variant="link" @click="killTask(task.id)">{{ $t("kill") }}</b-btn>
      </b-col>
    </b-row>

    <b-row>
      <b-col>
        <b-progress :max="task.progress.count">
          <b-progress-bar :value="task.progress.done" :label-html="label" :striped="!task.progress.waiting"/>
        </b-progress>
      </b-col>
    </b-row>

  </b-list-group-item>
</template>

<script>
import sist2AdminApi from "@/Sist2AdminApi";

export default {
  name: "TaskListItem",
  props: ["task"],
  computed: {
    label() {

      const count = this.task.progress.count;
      const done = this.task.progress.done;

      return `<span>${done}/${count}</span>`
    }
  },
  methods: {
    killTask(taskId) {
      sist2AdminApi.killTask(taskId).then(() => {
        this.$bvToast.toast(this.$t("killConfirmation"), {
          title: this.$t("killConfirmationTitle"),
          variant: "success",
          toaster: "b-toaster-bottom-right"
        });
      });
    }
  }
}
</script>

<style scoped>
.shrink {
  flex-grow: inherit;
}
</style>