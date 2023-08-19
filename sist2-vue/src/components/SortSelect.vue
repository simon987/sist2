<template>
  <b-dropdown variant="primary" :disabled="$store.getters.embedding !== null">
    <b-dropdown-item :class="{'dropdown-active': sort === 'score'}" @click="onSelect('score')">{{
        $t("sort.relevance")
      }}
    </b-dropdown-item>
    <b-dropdown-item :class="{'dropdown-active': sort === 'dateAsc'}" @click="onSelect('dateAsc')">{{
        $t("sort.dateAsc")
      }}
    </b-dropdown-item>
    <b-dropdown-item :class="{'dropdown-active': sort === 'dateDesc'}" @click="onSelect('dateDesc')">
      {{ $t("sort.dateDesc") }}
    </b-dropdown-item>
    <b-dropdown-item :class="{'dropdown-active': sort === 'sizeAsc'}" @click="onSelect('sizeAsc')">{{
        $t("sort.sizeAsc")
      }}
    </b-dropdown-item>
    <b-dropdown-item :class="{'dropdown-active': sort === 'sizeDesc'}" @click="onSelect('sizeDesc')">
      {{ $t("sort.sizeDesc") }}
    </b-dropdown-item>

    <b-dropdown-item :class="{'dropdown-active': sort === 'nameDesc'}" @click="onSelect('nameDesc')">
      {{ $t("sort.nameDesc") }}
    </b-dropdown-item>

    <b-dropdown-item :class="{'dropdown-active': sort === 'nameAsc'}" @click="onSelect('nameAsc')">
      {{ $t("sort.nameAsc") }}
    </b-dropdown-item>

    <b-dropdown-item :class="{'dropdown-active': sort === 'random'}" @click="onSelect('random')">
      {{ $t("sort.random") }}
    </b-dropdown-item>

    <template #button-content>
      <svg aria-hidden="true" width="20px" height="20px" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 512">
        <path
            fill="currentColor"
            d="M41 288h238c21.4 0 32.1 25.9 17 41L177 448c-9.4 9.4-24.6 9.4-33.9 0L24 329c-15.1-15.1-4.4-41 17-41zm255-105L177 64c-9.4-9.4-24.6-9.4-33.9 0L24 183c-15.1 15.1-4.4 41 17 41h238c21.4 0 32.1-25.9 17-41z"></path>
      </svg>
    </template>
  </b-dropdown>
</template>

<script>
import {randomSeed} from "@/util";

export default {
  name: "SortSelect",
  computed: {
    sort() {
      return this.$store.state.sortMode;
    }
  },
  methods: {
    onSelect(sortMode) {
      if (sortMode === "random") {
        this.$store.commit("setSeed", randomSeed());
      }
      this.$store.commit("setSortMode", sortMode);
    }
  },
}
</script>

<style>
.dropdown-active a {
  font-weight: bold;
}
</style>