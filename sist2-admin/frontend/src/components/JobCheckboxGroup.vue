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
          <b-form-checkbox :disabled="job.status !== 'indexed'" :value="job.name">[{{ job.name }}]</b-form-checkbox>
          <br/>
        </div>
      </b-form-checkbox-group>
      <div v-else>
        <span class="text-muted">{{ $t('jobOptions.noJobAvailable') }}</span>
        &nbsp;<router-link to="/">{{$t("create")}}</router-link>
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
      this.jobs = resp.data;
      this.loading = false;
    });
  },
  data() {
    return {
      loading: true,
    }
  }
}
</script>