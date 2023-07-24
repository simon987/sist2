<template>
    <div>
        <b-progress v-if="modelLoading" :value="modelLoadingProgress" max="1" class="mb-1" variant="warning"
                    show-progress>
        </b-progress>
        <b-input-group>
            <b-form-input :value="embeddingText"
                          :placeholder="$t('embeddingsSearchPlaceholder')"
                          @input="onInput($event)"
                          :disabled="modelLoading"
            ></b-form-input>

            <!-- TODO: dropdown of available models-->
            <!--            <template #prepend>-->
            <!--                <b-input-group-text>-->
            <!--                    <b-form-checkbox :checked="fuzzy" title="Toggle fuzzy searching" @change="setFuzzy($event)">-->
            <!--                        {{ $t("searchBar.fuzzy") }}-->
            <!--                    </b-form-checkbox>-->
            <!--                </b-input-group-text>-->
            <!--            </template>-->
            <template #append>
                <b-input-group-text>
                    <MLIcon></MLIcon>
                </b-input-group-text>
            </template>
        </b-input-group>
    </div>
</template>

<script>
import {mapGetters, mapMutations} from "vuex";
import {CLIPTransformerModel} from "@/ml/CLIPTransformerModel"
import _debounce from "lodash/debounce";
import MLIcon from "@/components/icons/MlIcon.vue";

export default {
    components: {MLIcon},
    data() {
        return {
            modelLoading: false,
            modelLoadingProgress: 0,
            modelLoaded: false,
            model: null
        }
    },
    computed: {
        ...mapGetters({
            optQueryMode: "optQueryMode",
            embeddingText: "embeddingText",
            fuzzy: "fuzzy",
        }),
    },
    mounted() {
        this.onInput = _debounce(this._onInput, 300, {leading: false});
    },
    methods: {
        ...mapMutations({
            setEmbeddingText: "setEmbeddingText",
            setEmbedding: "setEmbedding",
            setEmbeddingModel: "setEmbeddingsModel",
        }),
        async loadModel() {
            this.modelLoading = true;
            this.model = new CLIPTransformerModel(
                // TODO: add a config for this (?)
                "https://github.com/simon987/sist2-models/raw/main/clip/models/clip-vit-base-patch32-q8.onnx",
                "https://github.com/simon987/sist2-models/raw/main/clip/models/tokenizer.json",
            );

            await this.model.init(async progress => {
                this.modelLoadingProgress = progress;
            });
            this.modelLoading = false;
            this.modelLoaded = true;
        },
        async _onInput(text) {
            if (!this.modelLoaded) {
                await this.loadModel();
                this.setEmbeddingModel(1); // TODO
            }

            if (text.length === 0) {
                this.setEmbeddingText("");
                this.setEmbedding(null);
                return;
            }

            const embeddings = await this.model.predict(text);

            this.setEmbeddingText(text);
            this.setEmbedding(embeddings);
        },
        mounted() {
        }
    }
}
</script>
<style>
</style>