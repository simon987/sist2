<template>
    <div>
        <b-card class="mb-2">
            <AnalyzedContentSpan v-for="span of legend" :key="span.id" :span="span"
                                 class="mr-2"></AnalyzedContentSpan>
        </b-card>
        <div class="content-div">
            <AnalyzedContentSpan v-for="span of mergedSpans" :key="span.id" :span="span"></AnalyzedContentSpan>
        </div>
    </div>
</template>

<script>


import AnalyzedContentSpan from "@/components/AnalyzedContentSpan.vue";
import ModelsRepo from "@/ml/modelsRepo";

export default {
    name: "AnalyzedContentSpanContainer",
    components: {AnalyzedContentSpan},
    props: ["spans", "text"],
    computed: {
        legend() {
            return Object.entries(ModelsRepo.data[this.$store.state.nerModel.name].legend)
                .map(([label, name]) => ({
                    text: name,
                    id: label,
                    label: label
                }));
        },
        mergedSpans() {
            const spans = this.spans;

            const merged = [];

            let lastLabel = null;
            let fixSpace = false;
            for (let i = 0; i < spans.length; i++) {

                if (spans[i].label !== lastLabel) {
                    let start = spans[i].wordIndex;
                    const nextSpan = spans.slice(i + 1).find(s => s.label !== spans[i].label)
                    let end = nextSpan ? nextSpan.wordIndex : undefined;

                    if (end !== undefined && this.text[end - 1] === " ") {
                        end -= 1;
                        fixSpace = true;
                    }

                    merged.push({
                        text: this.text.slice(start, end),
                        label: spans[i].label,
                        id: spans[i].wordIndex
                    });

                    if (fixSpace) {
                        merged.push({
                            text: " ",
                            label: "O",
                            id: end
                        });
                        fixSpace = false;
                    }
                    lastLabel = spans[i].label;
                }
            }

            return merged;
        },
    },
}
</script>

<style scoped></style>
