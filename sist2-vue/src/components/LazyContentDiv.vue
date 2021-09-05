<template>
  <Preloader v-if="loading"></Preloader>
  <div v-else-if="content" class="content-div">{{ content }}</div>
</template>

<script>
import Sist2Api from "@/Sist2Api";
import Preloader from "@/components/Preloader";

export default {
  name: "LazyContentDiv",
  components: {Preloader},
  props: ["docId"],
  data() {
    return {
      content: "",
      loading: true
    }
  },
  mounted() {
    Sist2Api.getDocInfo(this.docId).then(src => {
      this.content = src.data.content;
      this.loading = false;
    })
  }
}
</script>

<style scoped>
</style>