<template>
  <div id="dateSlider"></div>
</template>

<script>
import noUiSlider from 'nouislider';
import 'nouislider/dist/nouislider.css';
import {humanDate} from "@/util";
import {mergeTooltips} from "@/util-js";

export default {
  name: "DateSlider",
  mounted() {
    this.$store.subscribe((mutation) => {
      if (mutation.type === "setDateBoundsMax") {
        const elem = document.getElementById("dateSlider");

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
