<template>
  <div id="mimeTree"></div>
</template>

<script>
import InspireTree from "inspire-tree";
import InspireTreeDOM from "inspire-tree-dom";

import "inspire-tree-dom/dist/inspire-tree-light.min.css";
import {getSelectedTreeNodes} from "@/util";

export default {
  name: "MimePicker",
  data() {
    return {
      mimeTree: null,
    }
  },
  mounted() {
    this.$store.subscribe((mutation) => {
      if (mutation.type === "setUiMimeMap") {
        const mimeMap = mutation.payload.slice();

        const elem = document.getElementById("mimeTree");
        console.log(elem);

        this.mimeTree = new InspireTree({
          selection: {
            mode: 'checkbox'
          },
          data: mimeMap
        });
        new InspireTreeDOM(this.mimeTree, {
          target: '#mimeTree'
        });
        this.mimeTree.on("node.state.changed", this.handleTreeClick);
        this.mimeTree.deselect();

        if (this.$store.state._onLoadSelectedMimeTypes.length > 0) {
          this.$store.state._onLoadSelectedMimeTypes.forEach(mime => {
            this.mimeTree.node(mime).select();
          });
        }
      }
    });
  },
  methods: {
    handleTreeClick(node, e) {
      if (e === "indeterminate" || e === "collapsed" || e === 'rendered' || e === "focused") {
        return;
      }

      this.$store.commit("setSelectedMimeTypes", getSelectedTreeNodes(this.mimeTree));
    },
  }
}
</script>

<style scoped>
#mimeTree {
  max-height: 350px;
  overflow: auto;
}
</style>