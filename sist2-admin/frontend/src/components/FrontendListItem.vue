<template>
  <b-list-group-item action :to="`/frontend/${frontend.name}`">

    <div class="d-flex w-100 justify-content-between">
      <h5 class="mb-1" style="display: block">
        {{ frontend.name }}
        <b-badge variant="light">{{ formatBindAddress(frontend.web_options.bind) }}</b-badge>
      </h5>

      <div>
        <b-badge v-if="frontend.running" variant="success">{{$t("online")}}</b-badge>
        <b-badge v-else variant="secondary">{{$t("offline")}}</b-badge>
      </div>
    </div>

  </b-list-group-item>
</template>

<script>
import {formatBindAddress} from "@/util";

export default {
  name: "FrontendListItem",
  props: ["frontend"],
  data() {
    return {
      formatBindAddress
    }
  }
}
</script>