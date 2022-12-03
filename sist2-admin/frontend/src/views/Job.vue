<template>
  <b-card>
    <b-card-title>
      [{{ getName() }}]
      {{ $t("jobTitle") }}
    </b-card-title>

    <div class="mb-3">
      <b-button class="mr-1" variant="primary" @click="runJob()">{{ $t("runNow") }}</b-button>
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

      <h4>{{ $t("scanOptions.title") }}</h4>
      <b-card>
        <ScanOptions :options="job.scan_options" @change="update()"></ScanOptions>
      </b-card>

      <br/>

      <h4>{{ $t("indexOptions.title") }}</h4>
      <b-card>
        <IndexOptions :options="job.index_options" @change="update()"></IndexOptions>
      </b-card>

    </b-card-body>

  </b-card>
</template>

<script>
import ScanOptions from "@/components/ScanOptions";
import Sist2AdminApi from "@/Sist2AdminApi";
import IndexOptions from "@/components/IndexOptions";
import JobOptions from "@/components/JobOptions";

export default {
  name: "Job",
  components: {
    IndexOptions,
    ScanOptions,
    JobOptions
  },
  data() {
    return {
      loading: true,
      job: null
    }
  },
  methods: {
    getName() {
      return this.$route.params.name;
    },
    update() {
      Sist2AdminApi.updateJob(this.getName(), this.job);
    },
    runJob() {
      Sist2AdminApi.runJob(this.getName()).then(() => {
        this.$bvToast.toast(this.$t("runJobConfirmation"), {
          title: this.$t("runJobConfirmationTitle"),
          variant: "success",
          toaster: "b-toaster-bottom-right"
        });
      });
    },
    deleteJob() {
      Sist2AdminApi.deleteJob(this.getName()).then(() => {
        this.$router.push("/");
      })
    }
  },
  mounted() {
    Sist2AdminApi.getJob(this.getName()).then(resp => {
      this.loading = false;
      this.job = resp.data;
    })
  }
}
</script>