import BertTokenizer from "@/ml/BertTokenizer";
import * as tf from "@tensorflow/tfjs";
import axios from "axios";

export default class BertNerModel {
    vocabUrl;
    modelUrl;

    id2label;
    _tokenizer;
    _model;
    inputSize = 128;

    _previousWordId = null;

    constructor(vocabUrl, modelUrl, id2label) {
        this.vocabUrl = vocabUrl;
        this.modelUrl = modelUrl;
        this.id2label = id2label;
    }

    async init(onProgress) {
        await Promise.all([this.loadTokenizer(), this.loadModel(onProgress)]);
    }

    async loadTokenizer() {
        const vocab = (await axios.get(this.vocabUrl)).data;
        this._tokenizer = new BertTokenizer(vocab);
    }

    async loadModel(onProgress) {
        this._model = await tf.loadGraphModel(this.modelUrl, {onProgress});
    }

    alignLabels(labels, wordIds, words) {
        const result = [];

        for (let i = 0; i < this.inputSize; i++) {
            const label = labels[i];
            const wordId = wordIds[i];

            if (wordId === -1) {
                continue;
            }
            if (wordId === this._previousWordId) {
                continue;
            }

            result.push({
                word: words[wordId].text, wordIndex: words[wordId].index, label: label
            });
            this._previousWordId = wordId;
        }

        return result;
    }

    async predict(text, callback) {
        this._previousWordId = null;
        const encoded = this._tokenizer.encodeText(text, this.inputSize)

        for (let chunk of encoded.inputChunks) {
            const rawResult = tf.tidy(() => this._model.execute({
                input_ids: tf.tensor2d(chunk.inputIds, [1, this.inputSize], "int32"),
                token_type_ids: tf.tensor2d(chunk.segmentIds, [1, this.inputSize], "int32"),
                attention_mask: tf.tensor2d(chunk.inputMask, [1, this.inputSize], "int32"),
            }));

            const labelIds = await tf.argMax(rawResult, -1);
            const labelIdsArray = await labelIds.array();
            const labels = labelIdsArray[0].map(id => this.id2label[id]);
            rawResult.dispose()

            callback(this.alignLabels(labels, chunk.wordIds, encoded.words))
        }
    }
}