import BertTokenizer from "@/ml/BertTokenizer";
import axios from "axios";
import {chunk as _chunk} from "underscore";
import * as ort from "onnxruntime-web";
import {argMax, downloadToBuffer, ORT_WASM_PATHS} from "@/ml/mlUtils";

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
        ort.env.wasm.wasmPaths = ORT_WASM_PATHS;
        const buf = await downloadToBuffer(this.modelUrl, onProgress);

        this._model = await ort.InferenceSession.create(buf.buffer, {executionProviders: ["wasm"]});
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
        const encoded = this._tokenizer.encodeText(text, this.inputSize);

        let i = 0;
        for (let chunk of encoded.inputChunks) {

            const results = await this._model.run({
                input_ids: new ort.Tensor("int32", chunk.inputIds, [1, this.inputSize]),
                token_type_ids: new ort.Tensor("int32", chunk.segmentIds, [1, this.inputSize]),
                attention_mask: new ort.Tensor("int32", chunk.inputMask, [1, this.inputSize]),
            });

            const labelIds = _chunk(results["output"].data, this.id2label.length).map(argMax);
            const labels = labelIds.map(id => this.id2label[id]);

            callback(this.alignLabels(labels, chunk.wordIds, encoded.words));

            i += 1;

            // give browser some time to repaint
            if (i % 2 === 0) {
                await new Promise(resolve => setTimeout(resolve, 0));
            }
        }
    }
}