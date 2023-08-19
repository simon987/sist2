import {zip, chunk} from "underscore";
import {toInt64} from "@/ml/mlUtils";

const UNK_INDEX = 100;
const CLS_INDEX = 101;
const SEP_INDEX = 102;
const CONTINUING_SUBWORD_PREFIX = "##";

function isWhitespace(ch) {
    return /\s/.test(ch);
}

function isInvalid(ch) {
    return (ch.charCodeAt(0) === 0 || ch.charCodeAt(0) === 0xfffd);
}

const punctuations = '[~`!@#$%^&*(){}[];:"\'<,.>?/\\|-_+=';

/** To judge whether it's a punctuation. */
function isPunctuation(ch) {
    return punctuations.indexOf(ch) !== -1;
}

export default class BertTokenizer {
    vocab;

    constructor(vocab) {
        this.vocab = vocab;
    }

    tokenize(text) {
        const charOriginalIndex = [];
        const cleanedText = this.cleanText(text, charOriginalIndex);
        const origTokens = cleanedText.split(' ');

        let charCount = 0;
        const tokens = origTokens.map((token) => {
            token = token.toLowerCase();
            const tokens = this.runSplitOnPunctuation(token, charCount, charOriginalIndex);
            charCount += token.length + 1;
            return tokens;
        });

        let flattenTokens = [];
        for (let index = 0; index < tokens.length; index++) {
            flattenTokens = flattenTokens.concat(tokens[index]);
        }
        return flattenTokens;
    }

    /* Performs invalid character removal and whitespace cleanup on text. */
    cleanText(text, charOriginalIndex) {
        text = text.replace(/\?/g, "").trim();

        const stringBuilder = [];
        let originalCharIndex = 0;
        let newCharIndex = 0;

        for (const ch of text) {
            // Skip the characters that cannot be used.
            if (isInvalid(ch)) {
                originalCharIndex += ch.length;
                continue;
            }
            if (isWhitespace(ch)) {
                if (stringBuilder.length > 0 && stringBuilder[stringBuilder.length - 1] !== ' ') {
                    stringBuilder.push(' ');
                    charOriginalIndex[newCharIndex] = originalCharIndex;
                    originalCharIndex += ch.length;
                } else {
                    originalCharIndex += ch.length;
                    continue;
                }
            } else {
                stringBuilder.push(ch);
                charOriginalIndex[newCharIndex] = originalCharIndex;
                originalCharIndex += ch.length;
            }
            newCharIndex++;
        }
        return stringBuilder.join('');
    }

    /* Splits punctuation on a piece of text. */
    runSplitOnPunctuation(text, count, charOriginalIndex) {
        const tokens = [];
        let startNewWord = true;
        for (const ch of text) {
            if (isPunctuation(ch)) {
                tokens.push({text: ch, index: charOriginalIndex[count]});
                count += ch.length;
                startNewWord = true;
            } else {
                if (startNewWord) {
                    tokens.push({text: '', index: charOriginalIndex[count]});
                    startNewWord = false;
                }
                tokens[tokens.length - 1].text += ch;
                count += ch.length;
            }
        }
        return tokens;
    }

    encode(words) {
        let outputTokens = [];
        const wordIds = [];

        for (let i = 0; i < words.length; i++) {
            let chars = [...words[i].text];

            let isUnknown = false;
            let start = 0;
            let subTokens = [];

            while (start < chars.length) {
                let end = chars.length;
                let currentSubstring = null;
                while (start < end) {
                    let substr = chars.slice(start, end).join('');

                    if (start > 0) {
                        substr = CONTINUING_SUBWORD_PREFIX + substr;
                    }
                    if (this.vocab.includes(substr)) {
                        currentSubstring = this.vocab.indexOf(substr);
                        break;
                    }

                    --end;
                }
                if (currentSubstring == null) {
                    isUnknown = true;
                    break;
                }
                subTokens.push(currentSubstring);
                start = end;
            }

            if (isUnknown) {
                outputTokens.push(UNK_INDEX);
                wordIds.push(i);
            } else {
                subTokens.forEach(tok => {
                    outputTokens.push(tok);
                    wordIds.push(i)
                });
            }
        }

        return {tokens: outputTokens, wordIds};
    }

    encodeText(inputText, inputSize) {

        const tokenized = this.tokenize(inputText);
        const encoded = this.encode(tokenized);

        const encodedTokenChunks = chunk(encoded.tokens, inputSize - 2);
        const encodedWordIdChunks = chunk(encoded.wordIds, inputSize - 2);

        const chunks = [];

        zip(encodedTokenChunks, encodedWordIdChunks).forEach(([tokens, wordIds]) => {
            const inputIds = [CLS_INDEX, ...tokens, SEP_INDEX];
            const segmentIds = Array(inputIds.length).fill(0);
            const inputMask = Array(inputIds.length).fill(1);
            wordIds = [-1, ...wordIds, -1];

            while (inputIds.length < inputSize) {
                inputIds.push(0);
                inputMask.push(0);
                segmentIds.push(0);
                wordIds.push(-1);
            }

            chunks.push({inputIds, inputMask, segmentIds, wordIds})
        });

        return {
            inputChunks: chunks,
            words: tokenized
        };
    }
}