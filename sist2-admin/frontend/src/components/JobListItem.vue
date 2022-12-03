<template>
  <b-list-group-item class="flex-column align-items-start" action :to="`job/${job.name}`">

    <div class="d-flex w-100 justify-content-between">
      <div>
        <h5 class="mb-1">
          {{ job.name }}
        </h5>
      </div>
      <div>
        <b-row>
          <b-col>
            <small v-if="job.last_index_date">
              {{ $t("scanned") }} {{ formatLastIndexDate(job.last_index_date) }}</small>
            <div v-else>&nbsp;</div>
          </b-col>
        </b-row>
        <b-row v-if="job.schedule_enabled">
          <b-col>
            <small><code>{{job.cron_expression }}</code></small>
          </b-col>
        </b-row>
        <b-row v-else>
          <b-col>
            &nbsp;
          </b-col>
        </b-row>

      </div>
    </div>

  </b-list-group-item>
</template>

<script>
import moment from "moment";

export default {
  name: "JobListItem",
  props: ["job"],
  methods: {
    formatLastIndexDate(dateString) {
      if (dateString === null) {
        return "";
      }

      const date = Date.parse(dateString);
      return moment(date).fromNow();
    }
  }
}
</script>

<style scoped>

</style>