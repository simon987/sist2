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
import {humanDate} from "@/util";
import {mergeTooltips} from "@/util-js";

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

        // Compute slider bound dates (seconds since EPOCH)
        let bDateMin = this.$store.state.dateBoundsMin;
        let bDateMax = this.$store.state.dateBoundsMax;
        if (!bDateMin || !bDateMax || bDateMax <= bDateMin) {
          // if initialization of $store.state values outside this function failed
          let today = new Date()
          let todayP1 = new Date();
          todayP1.setFullYear(today.getFullYear()+1);
          let todayM20 = new Date()
          todayM20.setFullYear(today.getFullYear()-20);
          bDateMax = todayP1.getTime()/1000; // today plus 1 year
          bDateMin = todayM20.getTime()/1000; // today minus 20 year
        }

        // Compute initail slider dates (seconds since EPOCH)
        let cDateMin = this.$store.state.dateMin ? this.$store.state.dateMin : bDateMin;
        let cDateMax = this.$store.state.dateMax ? this.$store.state.dateMax : bDateMax;
        if (cDateMax <= cDateMin) {
          // if initialization out of $store.state values outside this function failed
          cDateMin = bDateMin;
          cDateMax = bDateMax;
        }

        const slider = noUiSlider.create(elem, {
          start: [ cDateMin, cDateMax],
          tooltips: [true, true],
          behaviour: "drag-tap",
          connect: true,
          range: {
            "min": bDateMin,
            "max": bDateMax,
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
            this.$store.commit("setDateMin", unencoded[0] === bDateMin ? undefined : Math.round(unencoded[0]));
          } else {
            this.$store.commit("setDateMax", unencoded[1] >= bDateMax ? undefined : Math.round(unencoded[1]));
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
