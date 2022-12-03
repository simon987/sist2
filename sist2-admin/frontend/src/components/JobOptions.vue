<template>
  <div>
    <b-form-checkbox :checked="desktopNotificationsEnabled" @change="updateNotifications($event)">
      {{ $t("jobOptions.desktopNotifications") }}
    </b-form-checkbox>

    <b-form-checkbox v-model="job.schedule_enabled" @change="update()">
      {{ $t("jobOptions.scheduleEnabled") }}
    </b-form-checkbox>

    <label>{{ $t("jobOptions.cron") }}</label>
    <b-form-input class="text-monospace" :state="cronValid" v-model="job.cron_expression" :disabled="!job.schedule_enabled" @change="update()"></b-form-input>
  </div>
</template>

<script>

export default {
  name: "JobOptions",
  props: ["job"],
  data() {
    return {
      cronValid: undefined
    }
  },
  computed: {
    desktopNotificationsEnabled() {
      return this.$store.state.jobDesktopNotificationMap[this.job.name];
    }
  },
  methods: {
    updateNotifications(value) {
      this.$store.dispatch("setJobDesktopNotification", {
        job: this.job.name,
        enabled: value
      })
    },
    update() {
      if (this.job.schedule_enabled) {
        this.cronValid = /((((\d+,)+\d+|(\d+([/-])\d+)|\d+|\*) ?){5,7})/.test(this.job.cron_expression);
      } else {
        this.cronValid = undefined;
      }

      if (this.cronValid !== false) {
        this.$emit("change", this.job);
      }
    },
  },
}
</script>