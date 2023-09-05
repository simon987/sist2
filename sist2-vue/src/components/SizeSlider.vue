<template>
  <div id="sizeSlider"></div>
</template>

<script>
import noUiSlider from 'nouislider';
import 'nouislider/dist/nouislider.css';
import {humanFileSize, mergeTooltips} from "@/util";

export default {
  name: "SizeSlider",
  mounted() {
    const elem = document.getElementById("sizeSlider");

    const slider = noUiSlider.create(elem, {
      start: [
        this.$store.state.sizeMin ? this.$store.state.sizeMin : 0,
        this.$store.state.sizeMax ? this.$store.state.sizeMax: 1000 * 1000 * 50000
      ],

      tooltips: [true, true],
      behaviour: "drag-tap",
      connect: true,
      range: {
        'min': 0,
        "10%": 1000 * 1000,
        "20%": 1000 * 1000 * 10,
        "50%": 1000 * 1000 * 5000,
        "max": 1000 * 1000 * 50000,
      },
      format: {
        to: x => x >= 1000 * 1000 * 50000 ? "50G+" : humanFileSize(Math.round(x)),
        from: x => x
      }
    });

    mergeTooltips(elem, 10, " - ")

    elem.querySelectorAll('.noUi-connect')[0].classList.add("slider-color0")

    slider.on("set", (values, handle, unencoded) => {

      if (handle === 0) {
        this.$store.commit("setSizeMin", unencoded[0] === 0 ? undefined : Math.round(unencoded[0]))
      } else {
        this.$store.commit("setSizeMax", unencoded[1] >= 1000 * 1000 * 50000 ? undefined : Math.round(unencoded[1]))
      }
    });
  }
}
</script>

<style>
#sizeSlider {
  margin-top: 34px;
  height: 12px;
}

.slider-color0 {
  background: #2196f3;
  box-shadow: none;
}

.theme-black .slider-color0 {
  background: #00bcd4;
}

.noUi-horizontal .noUi-handle {
  width: 2px;
  height: 18px;
  right: -1px;
  top: -3px;
  border: none;
  background-color: #1976d2;
  box-shadow: none;
  cursor: ew-resize;
  border-radius: 0;
}

.theme-black .noUi-horizontal .noUi-handle {
  background-color: #2168ac;
}

.noUi-handle:before {
  top: -6px;
  left: 3px;

  width: 10px;
  height: 28px;
  background-color: transparent;
}

.noUi-handle:after {
  top: -6px;
  left: -10px;

  width: 10px;
  height: 28px;
  background-color: transparent;
}

.noUi-draggable {
  cursor: move;
}

.noUi-tooltip {
  color: #fff;
  font-size: 13px;
  line-height: 1.333;
  text-shadow: none;
  padding: 0 2px;
  background: #2196F3;
  border-radius: 4px;
  border: none;
}

.theme-black .noUi-tooltip {
  background: #00bcd4;
}

.theme-black .noUi-connects {
  background-color: #37474f;
}

.noUi-horizontal .noUi-origin > .noUi-tooltip {
  bottom: 7px;
}

.noUi-target {
  background-color: #e1e4e9;
  border: none;
  box-shadow: none;
}
</style>
