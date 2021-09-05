<template>
  <b-navbar>
    <b-navbar-brand v-if="$route.path !== '/'" to="/">
      <Sist2Icon></Sist2Icon>
    </b-navbar-brand>
    <b-navbar-brand v-else href=".">
      <Sist2Icon></Sist2Icon>
    </b-navbar-brand>

    <span class="badge badge-pill version" v-if="$store && $store.state.sist2Info">
      {{ sist2Version() }}<span v-if="isDebug()">-dbg</span>
    </span>

    <span v-if="$store && $store.state.sist2Info" class="tagline" v-html="tagline()"></span>

    <b-button class="ml-auto" to="stats" variant="link">{{ $t("stats") }}</b-button>
    <b-button to="config" variant="link">{{ $t("config") }}</b-button>
  </b-navbar>
</template>

<script>
import Sist2Icon from "@/components/Sist2Icon";
export default {
  name: "NavBar",
  components: {Sist2Icon},
  methods: {
    tagline() {
      return this.$store.state.sist2Info.tagline;
    },
    sist2Version() {
      return this.$store.state.sist2Info.version;
    },
    isDebug() {
      return this.$store.state.sist2Info.debug;
    }
  }
}
</script>

<style scoped>
.navbar {
  box-shadow: 0 0.125rem 0.25rem rgb(0 0 0 / 8%) !important;
  border-radius: 0;
}

.theme-black .navbar {
  background: #546b7a30;
  border-bottom: none;
}

.navbar-brand {
  color: #222 !important;
  font-size: 1.75rem;
  padding: 0;
  font-family: Hack;
}

.navbar-brand:hover {
  color: #000 !important;
}

.version {
  color: #222 !important;
  margin-left: -18px;
  margin-top: -14px;
  font-size: 11px;
  font-family: monospace;
}

.theme-black .version {
  color: #f5f5f5 !important;
}

.theme-black .navbar-brand {
  font-size: 1.75rem;
  padding: 0;
  color: #f5f5f5 !important;
}

.theme-black a:hover, .theme-black .btn:hover {
  color: #fff;
}

.theme-black .navbar span {
  color: #eee;
}

@media (max-width: 650px) {
  .tagline {
    display: none;
  }

  .version {
    display: none;
  }
}

.theme-light .btn-link{
  color: #222;
}
</style>