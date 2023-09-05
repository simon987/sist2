<template>
  <div v-if="$store.state.optUseDatePicker">
    <b-row>
      <b-col sm="6">
        <b-form-datepicker
            value-as-date
            :date-format-options="{ year: 'numeric', month: '2-digit', day: '2-digit' }"
            :locale="$store.state.optLang"
            class="mb-2"
            :value="dateMin" @input="setDateMin"></b-form-datepicker>
      </b-col>
      <b-col sm="6">
        <b-form-datepicker
            value-as-date
            :date-format-options="{ year: 'numeric', month: '2-digit', day: '2-digit' }"
            :locale="$store.state.optLang"
            class="mb-2"
            :value="dateMax" @input="setDateMax"></b-form-datepicker>
      </b-col>
    </b-row>
  </div>
  <div v-else>
    <b-row>
      <b-col style="height: 70px;">
        <div id="dateSlider"></div>
      </b-col>
    </b-row>
  </div>
</template>

<script>
import noUiSlider from 'nouislider';
import 'nouislider/dist/nouislider.css';
import {humanDate, mergeTooltips} from "@/util";

export default {
  name: "DateSlider",
  methods: {
    setDateMin(val) {
      const epochDate = Math.ceil(+val / 1000);
      this.$store.commit("setDateMin", epochDate);
    },
    setDateMax(val) {
      const epochDate = Math.ceil(+val / 1000);
      this.$store.commit("setDateMax", epochDate);
    },
  },
  computed: {
    dateMin() {
      const dateMin = this.$store.state.dateMin ? this.$store.state.dateMin : this.$store.state.dateBoundsMin;
      return new Date(dateMin * 1000)
    },
    dateMax() {
      const dateMax = this.$store.state.dateMax ? this.$store.state.dateMax : this.$store.state.dateBoundsMax;
      return new Date(dateMax * 1000)
    }
  },
  mounted() {
    this.$store.subscribe((mutation) => {
      if (mutation.type === "setDateBoundsMax") {
        const elem = document.getElementById("dateSlider");

        if (elem === null) {
          // Using b-form-datepicker, skip initialisation of slider
          return
        }

        if (elem.children.length > 0) {
          return;
        }

        const dateMax = this.$store.state.dateBoundsMax;
        const dateMin = this.$store.state.dateBoundsMin;

        const slider = noUiSlider.create(elem, {
          start: [
            this.$store.state.dateMin ? this.$store.state.dateMin : dateMin,
            this.$store.state.dateMax ? this.$store.state.dateMax : dateMax
          ],

          tooltips: [true, true],
          behaviour: "drag-tap",
          connect: true,
          range: {
            "min": dateMin,
            "max": dateMax,
          },
          format: {
            to: x => humanDate(x),
            from: x => x
          }
        });

        mergeTooltips(elem, 10, " - ", true)

        elem.querySelectorAll('.noUi-connect')[0].classList.add("slider-color0")

        slider.on("set", (values, handle, unencoded) => {
          if (handle === 0) {
            this.$store.commit("setDateMin", unencoded[0] === dateMin ? undefined : Math.round(unencoded[0]));
          } else {
            this.$store.commit("setDateMax", unencoded[1] >= dateMax ? undefined : Math.round(unencoded[1]));
          }
        });
      }
    });
  }
}
</script>

<style>
#dateSlider {
  margin-top: 34px;
  height: 12px;
}
</style>
