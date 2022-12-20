<template>
  <b-card>
    <b-card-title>
      {{ name }}
      <small style="vertical-align: top">
        <b-badge v-if="!loading && frontend.running" variant="success">{{ $t("online") }}</b-badge>
        <b-badge v-else-if="!loading" variant="secondary">{{ $t("offline") }}</b-badge>
      </small>
    </b-card-title>

    <div class="mb-3" v-if="!loading">
      <b-button class="mr-1" :disabled="frontend.running || !valid" variant="success" @click="start()">{{
          $t("start")
        }}
      </b-button>
      <b-button class="mr-1" :disabled="!frontend.running" variant="danger" @click="stop()">{{
          $t("stop")
        }}
      </b-button>
      <b-button class="mr-1" :disabled="!frontend.running" variant="primary" :href="frontendUrl" target="_blank">
        {{ $t("go") }}
      </b-button>
      <b-button variant="danger" @click="deleteFrontend()">{{ $t("delete") }}</b-button>
    </div>


    <b-progress v-if="loading" striped animated value="100"></b-progress>
    <b-card-body v-else>

      <h4>{{ $t("frontendOptions.title") }}</h4>
      <b-card>
        <b-form-checkbox v-model="frontend.auto_start" @change="update()">
          {{ $t("autoStart") }}
        </b-form-checkbox>

        <label>{{ $t("extraQueryArgs") }}</label>
        <b-form-input v-model="frontend.extra_query_args" @change="update()"></b-form-input>

        <label>{{ $t("customUrl") }}</label>
        <b-form-input v-model="frontend.custom_url" @change="update()" placeholder="http://"></b-form-input>

        <br/>

        <b-alert v-if="!valid" variant="warning" show>{{ $t("frontendOptions.noJobSelectedWarning") }}</b-alert>

        <JobCheckboxGroup :frontend="frontend" @input="update()"></JobCheckboxGroup>
      </b-card>

      <br/>

      <h4>{{ $t("jobOptions.title") }}</h4>
      <b-card>
        <WebOptions :options="frontend.web_options" :frontend-name="$route.params.name" @change="update()"></WebOptions>
      </b-card>
    </b-card-body>

  </b-card>
</template>

<script>

import Sist2AdminApi from "@/Sist2AdminApi";
import JobCheckboxGroup from "@/components/JobCheckboxGroup";
import WebOptions from "@/components/WebOptions";

export default {
  name: 'Frontend',
  components: {JobCheckboxGroup, WebOptions},
  data() {
    return {
      loading: true,
      frontend: null,
    }
  },
  computed: {
    valid() {
      return !this.loading && this.frontend.jobs.length > 0;
    },
    frontendUrl() {
      if (this.frontend.custom_url) {
        return this.frontend.custom_url + this.args;
      }

      if (this.frontend.web_options.bind.startsWith("0.0.0.0")) {
        return window.location.protocol + "//" + window.location.hostname + ":" + this.port + this.args;
      }

      return window.location.protocol + "//" + this.frontend.web_options.bind + this.args;
    },
    name() {
      return this.$route.params.name;
    },
    port() {
      return this.frontend.web_options.bind.split(":")[1]
    },
    args() {
      const args = this.frontend.extra_query_args;
      if (args !== "") {
        return "#" + (args.startsWith("?") ? (args) : ("?" + args));
      }
      return "";
    }
  },
  mounted() {
    Sist2AdminApi.getFrontend(this.name).then(resp => {
      this.frontend = resp.data;
      this.loading = false;
    });
  },
  methods: {
    start() {
      this.frontend.running = true;
      Sist2AdminApi.startFrontend(this.name)
    },
    stop() {
      this.frontend.running = false;
      Sist2AdminApi.stopFrontend(this.name)
    },
    deleteFrontend() {
      Sist2AdminApi.deleteFrontend(this.name).then(() => {
        this.$router.push("/frontends");
      });
    },
    update() {
      Sist2AdminApi.updateFrontend(this.name, this.frontend);
    },
  }
}
</script>