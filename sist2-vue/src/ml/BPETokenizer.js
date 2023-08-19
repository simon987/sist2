const inf = Number.POSITIVE_INFINITY;
const START_TOK = 49406;
const END_TOK = 49407;

function min(array, key) {
    return array
        .reduce((a, b) => (key(a, b) ? b : a))
}

class TupleSet extends Set {
    add(elem) {
        return super.add(elem.join("`"));
    }

    has(elem) {
        return super.has(elem.join("`"));
    }

    toList() {
        return [...this].map(x => x.split("`"))
    }
}

export class BPETokenizer {

    _encoder = null;
    _bpeRanks = null;

    constructor(encoder, bpeRanks) {
        this._encoder = encoder;
        this._bpeRanks = bpeRanks;
    }

    getPairs(word) {
        const pairs = new TupleSet();

        let prevChar = word[0];
        for (let i = 1; i < word.length; i++) {
            pairs.add([prevChar, word[i]])
            prevChar = word[i];
        }

        return pairs.toList();
    }

    bpe(token) {
        let word = [...token];
        word[word.length - 1] += "</w>";
        let pairs = this.getPairs(word)

        if (pairs.length === 0) {
            return token + "</w>"
        }

        while (true) {
            const bigram = min(pairs, (a, b) => {
                return (this._bpeRanks[a.join("`")] ?? inf) > (this._bpeRanks[b.join("`") ?? inf])
            });

            if (this._bpeRanks[bigram.join("`")] === undefined) {
                break;
            }

            const [first, second] = bigram;
            let newWord = [];
            let i = 0;

            while (i < word.length) {
                const j = word.indexOf(first, i);
                if (j === -1) {
                    newWord.push(...word.slice(i));
                    break;
                } else {
                    newWord.push(...word.slice(i, j));
                    i = j;
                }

                if (word[i] === first && i < word.length - 1 && word[i + 1] === second) {
                    newWord.push(first + second);
                    i += 2;
                } else {
                    newWord.push(word[i]);
                    i += 1;
                }
            }

            word = [...newWord]
            if (word.length === 1) {
                break;
            } else {
                pairs = this.getPairs(word);
            }
        }

        return word.join(" ");
    }

    encode(text) {
        let bpeTokens = [];
        text = text.trim();
        text = text.replaceAll(/\s+/g, " ");

        text
            .match(/<\|startoftext\|>|<\|endoftext\|>|'s|'t|'re|'ve|'m|'ll|'d|[a-zA-Z0-9]+/ig)
            .forEach(token => {
                bpeTokens.push(...this.bpe(token).split(" ").map(t => this._encoder[t]));
            });

        bpeTokens.unshift(START_TOK);
        bpeTokens = bpeTokens.slice(0, 76);
        bpeTokens.push(END_TOK);
        while (bpeTokens.length < 77) {
            bpeTokens.push(0);
        }

        return bpeTokens;
    }
}
