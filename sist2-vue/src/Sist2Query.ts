import store from "./store";
import {EsHit, Index} from "@/Sist2Api";

const SORT_MODES = {
    score: {
        mode: [
            {_score: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._score
    },
    random: {
        mode: [
            {_score: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._score
    },
    dateAsc: {
        mode: [
            {mtime: {order: "asc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._source.mtime
    },
    dateDesc: {
        mode: [
            {mtime: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._source.mtime
    },
    sizeAsc: {
        mode: [
            {size: {order: "asc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._source.size
    },
    sizeDesc: {
        mode: [
            {size: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._source.size
    },
    nameAsc: {
        mode: [
            {name: {order: "asc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._source.name
    },
    nameDesc: {
        mode: [
            {name: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: (hit: EsHit) => hit._source.name
    }
} as any;

interface SortMode {
    text: string
    mode: any[]
    key: (hit: EsHit) => any
}


class Sist2Query {

    searchQuery(blankSearch: boolean = false): any {

        const getters = store.getters;

        const searchText = getters.searchText;
        const pathText = getters.pathText;
        const empty = searchText === "";
        const sizeMin = getters.sizeMin;
        const sizeMax = getters.sizeMax;
        const dateMin = getters.dateMin;
        const dateMax = getters.dateMax;
        const fuzzy = getters.fuzzy;
        const size = getters.size;
        const after = getters.lastDoc;
        const selectedIndexIds = getters.selectedIndices.map((idx: Index) => idx.id)
        const selectedMimeTypes = getters.selectedMimeTypes;
        const selectedTags = getters.selectedTags;

        const legacyES = store.state.sist2Info.esVersionLegacy;

        const filters = [
            {terms: {index: selectedIndexIds}}
        ] as any[];

        const fields = [
            "name^8",
            "content^3",
            "album^8", "artist^8", "title^8", "genre^2", "album_artist^8",
            "font_name^6"
        ];

        if (getters.optSearchInPath) {
            fields.push("path.text^5");
        }

        if (fuzzy) {
            fields.push("content.nGram");
            if (getters.optSearchInPath) {
                fields.push("path.nGram");
            }
            fields.push("name.nGram^3");
        }

        if (!blankSearch) {
            if (sizeMin && sizeMax) {
                filters.push({range: {size: {gte: sizeMin, lte: sizeMax}}})
            } else if (sizeMin) {
                filters.push({range: {size: {gte: sizeMin}}})
            } else if (sizeMax) {
                filters.push({range: {size: {lte: sizeMax}}})
            }

            if (dateMin && dateMax) {
                filters.push({range: {mtime: {gte: dateMin, lte: dateMax}}})
            } else if (dateMin) {
                filters.push({range: {mtime: {gte: dateMin}}})
            } else if (dateMax) {
                filters.push({range: {mtime: {lte: dateMax}}})
            }

            const path = pathText.replace(/\/$/, "").toLowerCase(); //remove trailing slashes

            if (path !== "") {
                filters.push({term: {path: path}})
            }

            if (selectedMimeTypes.length > 0) {
                filters.push({terms: {"mime": selectedMimeTypes}});
            }

            if (selectedTags.length > 0) {
                if (getters.optTagOrOperator) {
                    filters.push({terms: {"tag": selectedTags}});
                } else {
                    selectedTags.forEach((tag: string) => filters.push({term: {"tag": tag}}));
                }
            }
        }

        let query;
        if (getters.optQueryMode === "simple") {
            query = {
                simple_query_string: {
                    query: searchText,
                    fields: fields,
                    default_operator: "and"
                }
            }
        } else {
            query = {
                query_string: {
                    query: searchText,
                    default_field: "name",
                    default_operator: "and"
                }
            }
        }

        const q = {
            _source: {
                excludes: ["content", "_tie"]
            },
            query: {
                bool: {
                    filter: filters,
                }
            },
            sort: SORT_MODES[getters.sortMode].mode,
            aggs:
                {
                    total_size: {"sum": {"field": "size"}},
                    total_count: {"value_count": {"field": "size"}}
                },
            size: size,
        } as any;

        if (!empty && !blankSearch) {
            q.query.bool.must = query;
        }

        if (after) {
            q.search_after = [SORT_MODES[getters.sortMode].key(after), after["_id"]];
        }

        if (getters.optHighlight) {
            q.highlight = {
                pre_tags: ["<mark>"],
                post_tags: ["</mark>"],
                fragment_size: getters.optFragmentSize,
                number_of_fragments: 1,
                order: "score",
                fields: {
                    content: {},
                    name: {},
                    "name.nGram": {},
                    "content.nGram": {},
                    font_name: {},
                }
            };

            if (!legacyES) {
                q.highlight.max_analyzed_offset = 999_999;
            }

            if (getters.optSearchInPath) {
                q.highlight.fields["path.text"] = {};
                q.highlight.fields["path.nGram"] = {};
            }
        }

        if (getters.sortMode === "random") {
            q.query = {
                function_score: {
                    query: {
                        bool: {
                            must: filters,
                        }
                    },
                    functions: [
                        {
                            random_score: {
                                seed: getters.seed,
                                field: "_seq_no",
                            },
                            weight: 1000
                        }
                    ],
                    boost_mode: "sum"
                }
            }

            if (!empty && !blankSearch) {
                q.query.function_score.query.bool.must.push(query);
            }
        }

        return q;
    }
}

export default new Sist2Query();