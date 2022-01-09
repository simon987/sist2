<template>
  <div id="tagTree"></div>
</template>

<script>
import InspireTree from "inspire-tree";
import InspireTreeDOM from "inspire-tree-dom";

import "inspire-tree-dom/dist/inspire-tree-light.min.css";
import {getSelectedTreeNodes} from "@/util";
import Sist2Api from "@/Sist2Api";

function resetState(node) {
  node._tree.defaultState.forEach(function (val, prop) {
    node.state(prop, val);
  });

  return node;
}

function baseStateChange(prop, value, verb, node, deep) {
  if (node.state(prop) !== value) {
    node._tree.batch();

    if (node._tree.config.nodes.resetStateOnRestore && verb === 'restored') {
      resetState(node);
    }

    node.state(prop, value);

    node._tree.emit('node.' + verb, node, false);

    if (deep && node.hasChildren()) {
      node.children.recurseDown(function (child) {
        baseStateChange(prop, value, verb, child);
      });
    }

    node.markDirty();
    node._tree.end();
  }

  return node;
}

function addTag(map, tag, id, count) {
  const tags = tag.split(".");

  const child = {
    id: id,
    count: count,
    text: tags.length !== 1 ? tags[0] : `${tags[0]} (${count})`,
    name: tags[0],
    children: [],
    // Overwrite base functions
    blur: function () {
      // noop
    },
    select: function () {
      this.state("selected", true);
      return this.check()
    },
    deselect: function () {
      this.state("selected", false);
      return this.uncheck()
    },
    uncheck: function () {
      baseStateChange('checked', false, 'unchecked', this, false);
      this.state('indeterminate', false);

      if (this.hasParent()) {
        this.getParent().refreshIndeterminateState();
      }

      this._tree.end();
      return this;
    },
    check: function () {
      baseStateChange('checked', true, 'checked', this, false);

      if (this.hasParent()) {
        this.getParent().refreshIndeterminateState();
      }

      this._tree.end();
      return this;
    }
  };

  let found = false;
  map.forEach(node => {
    if (node.name === child.name) {
      found = true;
      if (tags.length !== 1) {
        addTag(node.children, tags.slice(1).join("."), id, count);
      } else {
        // Same name, different color
        console.error("FIXME: Duplicate tag?")
        console.trace(node)
      }
    }
  });
  if (!found) {
    if (tags.length !== 1) {
      addTag(child.children, tags.slice(1).join("."), id, count);
      map.push(child);
    } else {
      map.push(child);
    }
  }
}

export default {
  name: "TagPicker",
  data() {
    return {
      tagTree: null,
      loadedFromArgs: false,
    }
  },
  mounted() {
    this.$store.subscribe((mutation) => {
      if (mutation.type === "setUiMimeMap" && this.tagTree === null) {
        this.initializeTree();
        this.updateTree();
      } else if (mutation.type === "busUpdateTags") {
        window.setTimeout(this.updateTree, 2000);
      }
    });
  },
  methods: {
    initializeTree() {
      const tagMap = [];
      this.tagTree = new InspireTree({
        selection: {
          mode: "checkbox",
          autoDeselect: false,
        },
        checkbox: {
          autoCheckChildren: false,
        },
        data: tagMap
      });
      new InspireTreeDOM(this.tagTree, {
        target: '#tagTree'
      });
      this.tagTree.on("node.state.changed", this.handleTreeClick);
    },
    updateTree() {
      // TODO: remember which tags are selected and restore?
      const tagMap = [];
      Sist2Api.getTags().then(tags => {
        tags.forEach(tag => addTag(tagMap, tag.id, tag.id, tag.count));
        this.tagTree.removeAll();
        this.tagTree.addNodes(tagMap);

        if (this.$store.state._onLoadSelectedTags.length > 0 && !this.loadedFromArgs) {
          this.$store.state._onLoadSelectedTags.forEach(mime => {
            this.tagTree.node(mime).select();
            this.loadedFromArgs = true;
          });
        }
      });
    },
    handleTreeClick(node, e) {
      if (e === "indeterminate" || e === "collapsed" || e === 'rendered' || e === "focused") {
        return;
      }

      this.$store.commit("setSelectedTags", getSelectedTreeNodes(this.tagTree));
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
<style>
.inspire-tree .focused>.wholerow {
  border: none;
}
</style>