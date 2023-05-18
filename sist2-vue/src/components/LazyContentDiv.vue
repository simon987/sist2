<template>
    <Preloader v-if="loading"></Preloader>
    <div v-else-if="content">
        <b-form inline class="my-2" v-if="ModelsRepo.getOptions().length > 0">
            <b-checkbox class="ml-auto mr-2" :checked="optAutoAnalyze"
                        @input="setOptAutoAnalyze($event); $store.dispatch('updateConfiguration')">
                {{ $t("ml.auto") }}
            </b-checkbox>
            <b-button :disabled="mlPredictionsLoading || mlLoading" @click="mlAnalyze" variant="primary"
            >{{ $t("ml.analyzeText") }}
            </b-button>
            <b-select :disabled="mlPredictionsLoading || mlLoading" class="ml-2" v-model="mlModel">
                <b-select-option :value="opt.value" v-for="opt of ModelsRepo.getOptions()">{{ opt.text }}
                </b-select-option>
            </b-select>
        </b-form>

        <b-progress v-if="mlLoading" variant="warning" show-progress :max="1" class="mb-3"
        >
            <b-progress-bar :value="modelLoadingProgress">
                <strong>{{ ((modelLoadingProgress * modelSize) / (1024 * 1024)).toFixed(1) }}MB / {{
                    (modelSize / (1024 * 1024)).toFixed(1)
                    }}MB</strong>
            </b-progress-bar>
        </b-progress>

        <b-progress v-if="mlPredictionsLoading" variant="primary" :value="modelPredictionProgress"
                    :max="content.length" class="mb-3"></b-progress>

        <AnalyzedContentSpansContainer v-if="analyzedContentSpans.length > 0"
                                       :spans="analyzedContentSpans" :text="rawContent"></AnalyzedContentSpansContainer>
        <div v-else class="content-div" v-html="content"></div>
    </div>
</template>

<script>
import Sist2Api from "@/Sist2Api";
import Preloader from "@/components/Preloader";
import Sist2Query from "@/Sist2ElasticsearchQuery";
import store from "@/store";
import BertNerModel from "@/ml/BertNerModel";
import AnalyzedContentSpansContainer from "@/components/AnalyzedContentSpanContainer.vue";
import ModelsRepo from "@/ml/modelsRepo";
import {mapGetters, mapMutations} from "vuex";

export default {
    name: "LazyContentDiv",
    components: {AnalyzedContentSpansContainer, Preloader},
    props: ["docId"],
    data() {
        return {
            ModelsRepo,
            content: "",
            rawContent: "",
            loading: true,
            modelLoadingProgress: 0,
            modelPredictionProgress: 0,
            mlPredictionsLoading: false,
            mlLoading: false,
            mlModel: null,
            analyzedContentSpans: []
        }
    },
    mounted() {

        if (this.$store.getters.optMlDefaultModel) {
            this.mlModel = this.$store.getters.optMlDefaultModel
        } else {
            this.mlModel = ModelsRepo.getDefaultModel();
        }

        Sist2Api
            .getDocument(this.docId, this.$store.state.optHighlight, this.$store.state.fuzzy)
            .then(doc => {
                this.loading = false;

                if (doc) {
                    this.content = this.getContent(doc)
                }

                if (this.optAutoAnalyze) {
                    this.mlAnalyze();
                }
            });
    },
    computed: {
        ...mapGetters(["optAutoAnalyze"]),
        modelSize() {
            const modelData = ModelsRepo.data[this.mlModel];
            if (!modelData) {
                return 0;
            }
            return modelData.size;
        }
    },
    methods: {
        ...mapMutations(["setOptAutoAnalyze"]),
        getContent(doc) {
            this.rawContent = doc._source.content;

            if (!doc.highlight) {
                return doc._source.content;
            }

            if (doc.highlight["content.nGram"]) {
                return doc.highlight["content.nGram"][0];
            }
            if (doc.highlight.content) {
                return doc.highlight.content[0];
            }
        },
        async getMlModel() {
            if (this.$store.getters.mlModel.name !== this.mlModel) {
                this.mlLoading = true;
                this.modelLoadingProgress = 0;
                const modelInfo = ModelsRepo.data[this.mlModel];

                const model = new BertNerModel(
                    modelInfo.vocabUrl,
                    modelInfo.modelUrl,
                    modelInfo.id2label,
                )

                await model.init(progress => this.modelLoadingProgress = progress);
                this.$store.commit("setMlModel", {model, name: this.mlModel});

                this.mlLoading = false;
                return model
            }

            return this.$store.getters.mlModel.model;
        },
        async mlAnalyze() {
            if (!this.content) {
                return;
            }

            const modelInfo = ModelsRepo.data[this.mlModel];
            if (modelInfo === undefined) {
                return;
            }

            this.$store.commit("setOptMlDefaultModel", this.mlModel);
            await this.$store.dispatch("updateConfiguration");

            const model = await this.getMlModel();

            this.analyzedContentSpans = [];

            this.mlPredictionsLoading = true;

            await model.predict(this.rawContent, results => {
                results.forEach(result => result.label = modelInfo.humanLabels[result.label]);
                this.analyzedContentSpans.push(...results);
                this.modelPredictionProgress = results[results.length - 1].wordIndex;
            });
            this.mlPredictionsLoading = false;
        }
    }
}
</script>

<style>
.progress-bar {
    transition: none;
}
</style>