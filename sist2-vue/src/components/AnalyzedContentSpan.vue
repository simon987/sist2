<template>
    <span :style="getStyle()">{{span.text}}</span>
</template>

<script>


import ModelsRepo from "@/ml/modelsRepo";

export default {
    name: "AnalyzedContentSpan",
    props: ["span", "text"],
    methods: {
        getStyle() {
            return ModelsRepo.data[this.$store.getters.mlModel.name].labelStyles[this.span.label];
        }
    }
}
</script>

<style scoped></style>
