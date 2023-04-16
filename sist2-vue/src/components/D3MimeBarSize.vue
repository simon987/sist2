<template>
  <div class="graph">
    <svg id="agg-mime-size"></svg>
  </div>
</template>

<script>
import * as d3 from "d3";
import Sist2Api from "@/Sist2Api";

const formatSI = d3.format("~s");
const barHeight = 20;
const ordinalColor = d3.scaleOrdinal(d3.schemeCategory10);

function mimeBarSize(data, svg, fillOpacity, title) {

  const margin = {
    top: 50,
    right: 0,
    bottom: 10,
    left: Math.max(
        d3.max(data.sort((a, b) => b.count - a.count).slice(0, 15), d => d.mime.length) * 6,
        d3.max(data.sort((a, b) => b.size - a.size).slice(0, 15), d => d.mime.length) * 6,
    )
  };

  data.forEach(d => {
    d.name = d.mime;
    d.value = Number(d.size);
  });
  data = data.sort((a, b) => b.value - a.value).slice(0, 15);

  const width = 550;
  const height = Math.ceil((data.length + 0.1) * barHeight) + margin.top + margin.bottom;

  svg.selectAll("*").remove();
  svg.attr("viewBox", [0, 0, width, height]);

  const y = d3.scaleBand()
      .domain(d3.range(data.length))
      .rangeRound([margin.top, height - margin.bottom]);

  const x = d3.scaleLinear()
      .domain([0, d3.max(data, d => d.value)])
      .range([margin.left, width - margin.right]);

  svg.append("g")
      .attr("fill-opacity", fillOpacity)
      .selectAll("rect")
      .data(data)
      .join("rect")
      .attr("fill", d => ordinalColor(d.name))
      .attr("x", x(0))
      .attr("y", (d, i) => y(i))
      .attr("width", d => x(d.value) - x(0))
      .attr("height", y.bandwidth())
      .append("title")
      .text(d => formatSI(d.value));

  svg.append("g")
      .attr("transform", `translate(0,${margin.top})`)
      .call(d3.axisTop(x).ticks(width / 80, data.format).tickFormat(formatSI))
      .call(g => g.select(".domain").remove());

  svg.append("g")
      .attr("transform", `translate(${margin.left},0)`)
      .call(d3.axisLeft(y).tickFormat(i => data[i].name).tickSizeOuter(0));

  svg.append("text")
      .attr("x", (width / 2))
      .attr("y", (margin.top / 2))
      .attr("text-anchor", "middle")
      .style("font-size", "16px")
      .text(title);
}

export default {
  name: "D3MimeBarSize",
  props: ["indexId"],
  mounted() {
    this.update(this.indexId);
  },
  watch: {
    indexId: function () {
      this.update(this.indexId);
    },
  },
  methods: {
    update(indexId) {
      const mimeSvgSize = d3.select("#agg-mime-size");
      const fillOpacity = this.$store.state.optTheme === "black" ? 0.9 : 0.6;

      d3.json(Sist2Api.getMimeStat(indexId)).then(tabularData => {
        mimeBarSize(tabularData.slice(), mimeSvgSize, fillOpacity, this.$t("d3.mimeSize"));
      });
    }
  }
}
</script>