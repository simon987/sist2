<template>
  <Preloader v-if="loading"></Preloader>
  <div v-else-if="content" class="content-div" v-html="content"></div>
</template>

<script>
import Sist2Api from "@/Sist2Api";
import Preloader from "@/components/Preloader";
import Sist2Query from "@/Sist2Query";
import store from "@/store";

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
    const query = Sist2Query.searchQuery();

    if (this.$store.state.optHighlight) {

      const fields = this.$store.state.fuzzy
          ? {"content.nGram": {}}
          : {content: {}};

      query.highlight = {
        pre_tags: ["<mark>"],
        post_tags: ["</mark>"],
        number_of_fragments: 0,
        fields,
      };

      if (!store.state.sist2Info.esVersionLegacy) {
        query.highlight.max_analyzed_offset = 999_999;
      }
    }

    if ("function_score" in query.query) {
      query.query = query.query.function_score.query;
    }

    if (!("must" in query.query.bool)) {
      query.query.bool.must = [];
    } else if (!Array.isArray(query.query.bool.must)) {
      query.query.bool.must = [query.query.bool.must];
    }

    query.query.bool.must.push({match: {_id: this.docId}});

    delete query["sort"];
    delete query["aggs"];
    delete query["search_after"];
    delete query.query["function_score"];

    query._source = {
      includes: ["content", "name", "path", "extension"]
    }

    query.size = 1;

    Sist2Api.esQuery(query).then(resp => {
      this.loading = false;
      if (resp.hits.hits.length === 1) {
        this.content = this.getContent(resp.hits.hits[0]);
      } else {
        console.log("FIXME: could not get content")
        console.log(resp)
      }
    });
  },
  methods: {
    getContent(doc) {
      if (!doc.highlight) {
        return doc._source.content;
      }

      if (doc.highlight["content.nGram"]) {
        return doc.highlight["content.nGram"][0];
      }
      if (doc.highlight.content) {
        return doc.highlight.content[0];
      }
    }
  }
}
</script>

<style scoped>
</style>