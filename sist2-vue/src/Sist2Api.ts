import axios from "axios";
import {ext, strUnescape, lum} from "./util";
import Sist2Query from "@/Sist2ElasticsearchQuery";
import store from "@/store";

export interface EsTag {
    id: string
    count: number
    color: string | undefined
    isLeaf: boolean
}

export interface Tag {
    style: string
    text: string
    rawText: string
    fg: string
    bg: string
    userTag: boolean
}

export interface Index {
    name: string
    version: string
    id: string
    idPrefix: string
    timestamp: number
}

export interface EsHit {
    _index: string
    _id: string
    _score: number
    _type: string
    _tags: Tag[]
    _seq: number
    _source: {
        path: string
        size: number
        mime: string
        name: string
        extension: string
        index: string
        _depth: number
        mtime: number
        videoc: string
        audioc: string
        parent: string
        width: number
        height: number
        duration: number
        tag: string[]
        checksum: string
        thumbnail: string
    }
    _props: {
        isSubDocument: boolean
        isImage: boolean
        isGif: boolean
        isVideo: boolean
        isPlayableVideo: boolean
        isPlayableImage: boolean
        isAudio: boolean
        hasThumbnail: boolean
        hasVidPreview: boolean
        imageAspectRatio: number
        /** Number of thumbnails available */
        tnNum: number
    }
    highlight: {
        name: string[] | undefined,
        content: string[] | undefined,
    }
}

function getIdPrefix(indices: Index[], id: string): string {
    for (let i = 4; i < 32; i++) {
        const prefix = id.slice(0, i);

        if (indices.filter(idx => idx.id.slice(0, i) == prefix).length == 1) {
            return prefix;
        }
    }

    return id;
}

export interface EsResult {
    took: number

    hits: {
        // TODO: ES 6.X ?
        total: {
            value: number
        }
        hits: EsHit[]
    }

    aggregations: any
}

class Sist2Api {

    private readonly baseUrl: string
    private sist2Info: any
    private queryfunc: () => EsResult;

    constructor(baseUrl: string) {
        this.baseUrl = baseUrl;
    }

    init(queryFunc: () => EsResult) {
        this.queryfunc = queryFunc;
    }

    backend() {
        return this.sist2Info.searchBackend;
    }

    getSist2Info(): Promise<any> {
        return axios.get(`${this.baseUrl}i`).then(resp => {
            const indices = resp.data.indices as Index[];

            resp.data.indices = indices.map(idx => {
                return {
                    id: idx.id,
                    name: idx.name,
                    timestamp: idx.timestamp,
                    version: idx.version,
                    idPrefix: getIdPrefix(indices, idx.id)
                } as Index;
            });

            this.sist2Info = resp.data;

            return resp.data;
        })
    }

    setHitProps(hit: EsHit): void {
        hit["_props"] = {} as any;

        const mimeCategory = hit._source.mime == null ? null : hit._source.mime.split("/")[0];

        if ("parent" in hit._source) {
            hit._props.isSubDocument = true;
        }

        if ("thumbnail" in hit._source) {
            hit._props.hasThumbnail = true;

            if (Number.isNaN(Number(hit._source.thumbnail))) {
                // Backwards compatibility
                hit._props.tnNum = 1;
                hit._props.hasVidPreview = false;
            } else {
                hit._props.tnNum = Number(hit._source.thumbnail);
                hit._props.hasVidPreview = hit._props.tnNum > 1;
            }
        }

        switch (mimeCategory) {
            case "image":
                if (hit._source.videoc === "gif") {
                    hit._props.isGif = true;
                } else {
                    hit._props.isImage = true;
                }
                if ("width" in hit._source && !hit._props.isSubDocument && hit._source.videoc !== "tiff"
                    && hit._source.videoc !== "raw" && hit._source.videoc !== "ppm") {
                    hit._props.isPlayableImage = true;
                }
                if ("width" in hit._source && "height" in hit._source) {
                    hit._props.imageAspectRatio = hit._source.width / hit._source.height;
                }
                break;
            case "video":
                if ("videoc" in hit._source) {
                    hit._props.isVideo = true;
                }
                if (hit._props.isVideo) {
                    const videoc = hit._source.videoc;
                    const mime = hit._source.mime;

                    hit._props.isPlayableVideo = mime != null &&
                        mime.startsWith("video/") &&
                        !hit._props.isSubDocument &&
                        hit._source.extension !== "mkv" &&
                        hit._source.extension !== "avi" &&
                        hit._source.extension !== "mov" &&
                        videoc !== "hevc" &&
                        videoc !== "mpeg1video" &&
                        videoc !== "mpeg2video" &&
                        videoc !== "wmv3";
                }
                break;
            case "audio":
                if ("audioc" in hit._source && !hit._props.isSubDocument) {
                    hit._props.isAudio = true;
                }
                break;
        }
    }

    setHitTags(hit: EsHit): void {
        const tags = [] as Tag[];

        // User tags
        if ("tag" in hit._source) {
            hit._source.tag.forEach(tag => {
                tags.push(this.createUserTag(tag));
            })
        }

        hit._tags = tags;
    }

    createUserTag(tag: string): Tag {
        const tokens = tag.split(".");

        const colorToken = tokens.pop() as string;

        const bg = colorToken;
        const fg = lum(colorToken) > 50 ? "#000" : "#fff";

        return {
            style: "user",
            fg: fg,
            bg: bg,
            text: tokens.join("."),
            rawText: tag,
            userTag: true,
        } as Tag;
    }

    search(): Promise<EsResult> {
        if (this.backend() == "sqlite") {
            return this.ftsQuery(this.queryfunc())
        } else {
            return this.esQuery(this.queryfunc());
        }
    }

    esQuery(query: any): Promise<EsResult> {
        return axios.post(`${this.baseUrl}es`, query).then(resp => {
            const res = resp.data as EsResult;

            if (res.hits?.hits) {
                res.hits.hits.forEach((hit: EsHit) => {
                    hit["_source"]["name"] = strUnescape(hit["_source"]["name"]);
                    hit["_source"]["path"] = strUnescape(hit["_source"]["path"]);

                    this.setHitProps(hit);
                    this.setHitTags(hit);
                });
            }

            return res;
        });
    }

    ftsQuery(query: any): Promise<EsResult> {
        return axios.post(`${this.baseUrl}fts/search`, query).then(resp => {
            const res = resp.data as any;

            if (res.hits.hits) {
                res.hits.hits.forEach(hit => {
                    hit["_source"]["name"] = strUnescape(hit["_source"]["name"]);
                    hit["_source"]["path"] = strUnescape(hit["_source"]["path"]);

                    this.setHitProps(hit);
                    this.setHitTags(hit);

                    if ("highlight" in hit) {
                        hit["highlight"]["name"] = [hit["highlight"]["name"]];
                        hit["highlight"]["content"] = [hit["highlight"]["content"]];
                    }
                });
            }

            return res;
        });
    }

    private getMimeTypesEs(query) {
        const AGGS = {
            mimeTypes: {
                terms: {
                    field: "mime",
                    size: 10000
                }
            }
        };

        if (!query) {
            query = {
                aggs: AGGS,
                size: 0,
            };
        } else {
            query.size = 0;
            query.aggs = AGGS;
        }

        return this.esQuery(query).then(resp => {
            return resp["aggregations"]["mimeTypes"]["buckets"].map(bucket => ({
                mime: bucket.key,
                count: bucket.doc_count
            }));

        });
    }

    private getMimeTypesSqlite(): Promise<[{ mime: string, count: number }]> {
        return axios.get(`${this.baseUrl}fts/mimetypes`)
            .then(resp => {
                return resp.data;
            });
    }

    async getMimeTypes(query = undefined) {
        let buckets;

        if (this.backend() == "sqlite") {
            buckets = await this.getMimeTypesSqlite();
        } else {
            buckets = await this.getMimeTypesEs(query);
        }

        const mimeMap: any[] = [];

        buckets.sort((a: any, b: any) => a.mime > b.mime).forEach((bucket: any) => {
            const tmp = bucket.mime.split("/");
            const category = tmp[0];
            const mime = tmp[1];

            let category_exists = false;

            const child = {
                "id": bucket.mime,
                "text": `${mime} (${bucket.count})`
            };

            mimeMap.forEach(node => {
                if (node.text === category) {
                    node.children.push(child);
                    category_exists = true;
                }
            });

            if (!category_exists) {
                mimeMap.push({text: category, children: [child], id: category});
            }
        })

        mimeMap.forEach(node => {
            if (node.children) {
                node.children.sort((a, b) => a.id.localeCompare(b.id));
            }
        })
        mimeMap.sort((a, b) => a.id.localeCompare(b.id))

        return {buckets, mimeMap};
    }

    _createEsTag(tag: string, count: number): EsTag {
        const tokens = tag.split(".");

        if (/.*\.#[0-9a-fA-F]{6}/.test(tag)) {
            return {
                id: tokens.slice(0, -1).join("."),
                color: tokens.pop(),
                isLeaf: true,
                count: count
            };
        }

        return {
            id: tag,
            count: count,
            isLeaf: false,
            color: undefined
        };
    }

    private getTagsEs() {
        return this.esQuery({
            aggs: {
                tags: {
                    terms: {
                        field: "tag",
                        size: 65535
                    }
                }
            },
            size: 0,
        }).then(resp => {
            return resp["aggregations"]["tags"]["buckets"]
                .sort((a: any, b: any) => a["key"].localeCompare(b["key"]))
                .map((bucket: any) => this._createEsTag(bucket["key"], bucket["doc_count"]));
        });
    }

    private getTagsSqlite() {
        return axios.get(`${this.baseUrl}/fts/tags`)
            .then(resp => {
                return resp.data.map(tag => this._createEsTag(tag.tag, tag.count))
            });
    }

    async getTags(): Promise<EsTag[]> {
        let tags;
        if (this.backend() == "sqlite") {
            tags = await this.getTagsSqlite();
        } else {
            tags = await this.getTagsEs();
        }

        // Remove duplicates (same tag with different color)
        const seen = new Set();

        return tags.filter((t: EsTag) => {
            if (seen.has(t.id)) {
                return false;
            }
            seen.add(t.id);
            return true;
        });
    }

    saveTag(tag: string, hit: EsHit) {
        return axios.post(`${this.baseUrl}tag/` + hit["_source"]["index"], {
            delete: false,
            name: tag,
            doc_id: hit["_id"]
        });
    }

    deleteTag(tag: string, hit: EsHit) {
        return axios.post(`${this.baseUrl}tag/` + hit["_source"]["index"], {
            delete: true,
            name: tag,
            doc_id: hit["_id"]
        });
    }

    searchPaths(indexId, minDepth, maxDepth, prefix = null) {
        if (this.backend() == "sqlite") {
            return this.searchPathsSqlite(indexId, minDepth, minDepth, prefix);
        } else {
            return this.searchPathsEs(indexId, minDepth, maxDepth, prefix);
        }
    }

    private searchPathsSqlite(indexId, minDepth, maxDepth, prefix) {
        return axios.post(`${this.baseUrl}fts/paths`, {
            indexId, minDepth, maxDepth, prefix
        }).then(resp => {
            return resp.data;
        });
    }

    private searchPathsEs(indexId, minDepth, maxDepth, prefix): Promise<[{ path: string, count: number }]> {

        const query = {
            query: {
                bool: {
                    filter: [
                        {term: {index: indexId}},
                        {range: {_depth: {gte: minDepth, lte: maxDepth}}},
                    ]
                }
            },
            aggs: {
                paths: {
                    terms: {
                        field: "path",
                        size: 10000
                    }
                }
            },
            size: 0
        };

        if (prefix != null) {
            query["query"]["bool"]["must"] = {
                prefix: {
                    path: prefix,
                }
            };
        }

        return this.esQuery(query).then(resp => {
            const buckets = resp["aggregations"]["paths"]["buckets"];

            if (!buckets) {
                return [];
            }

            return buckets
                .map(bucket => ({
                    path: bucket.key,
                    count: bucket.doc_count
                }));
        });
    }

    private getDateRangeSqlite() {
        return axios.get(`${this.baseUrl}fts/dateRange`)
            .then(resp => ({
                min: resp.data.dateMin,
                max: resp.data.dateMax,
            }));
    }

    getDateRange(): Promise<{ min: number, max: number }> {
        if (this.backend() == "sqlite") {
            return this.getDateRangeSqlite();
        } else {
            return this.getDateRangeEs();
        }
    }

    private getDateRangeEs() {
        return this.esQuery({
            // TODO: filter current selected indices
            aggs: {
                dateMin: {min: {field: "mtime"}},
                dateMax: {max: {field: "mtime"}},
            },
            size: 0
        }).then(res => {
            const range = {
                min: res.aggregations.dateMin.value / 1000,
                max: res.aggregations.dateMax.value / 1000,
            }

            if (range.min == null) {
                range.min = 0;
                range.max = 1;
            } else if (range.min == range.max) {
                range.max += 1;
            }

            return range;
        });
    }

    private getPathSuggestionsSqlite(text: string) {
        return axios.post(`${this.baseUrl}fts/paths`, {
            prefix: text,
            minDepth: 1,
            maxDepth: 10000
        }).then(resp => {
            return resp.data.map(bucket => bucket.path);
        })
    }

    private getPathSuggestionsEs(text) {
        return this.esQuery({
            suggest: {
                path: {
                    prefix: text,
                    completion: {
                        field: "suggest-path",
                        skip_duplicates: true,
                        size: 10000
                    }
                }
            }
        }).then(resp => {
            return resp["suggest"]["path"][0]["options"]
                .map(opt => opt["_source"]["path"]);
        });
    }

    getPathSuggestions(text: string): Promise<string[]> {
        if (this.backend() == "sqlite") {
            return this.getPathSuggestionsSqlite(text);
        } else {
            return this.getPathSuggestionsEs(text)
        }
    }

    getTreemapStat(indexId: string) {
        return `${this.baseUrl}s/${indexId}/TMAP`;
    }

    getMimeStat(indexId: string) {
        return `${this.baseUrl}s/${indexId}/MAGG`;
    }

    getSizeStat(indexId: string) {
        return `${this.baseUrl}s/${indexId}/SAGG`;
    }

    getDateStat(indexId: string) {
        return `${this.baseUrl}s/${indexId}/DAGG`;
    }

    private getDocumentEs(docId: string, highlight: boolean, fuzzy: boolean) {
        const query = Sist2Query.searchQuery();

        if (highlight) {
            const fields = fuzzy
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

        query.query.bool.must.push({match: {_id: docId}});

        delete query["sort"];
        delete query["aggs"];
        delete query["search_after"];
        delete query.query["function_score"];

        query._source = {
            includes: ["content", "name", "path", "extension"]
        }

        query.size = 1;

        return this.esQuery(query).then(resp => {
            if (resp.hits.hits.length === 1) {
                return resp.hits.hits[0];
            }
            return null;
        });
    }

    private getDocumentSqlite(docId: string): Promise<EsHit> {
        return axios.get(`${this.baseUrl}/fts/d/${docId}`)
            .then(resp => ({
                _source: resp.data
            } as EsHit));
    }

    getDocument(docId: string, highlight: boolean, fuzzy: boolean): Promise<EsHit | null> {
        if (this.backend() == "sqlite") {
            return this.getDocumentSqlite(docId);
        } else {
            return this.getDocumentEs(docId, highlight, fuzzy);
        }
    }

    getTagSuggestions(prefix: string): Promise<string[]> {
        if (this.backend() == "sqlite") {
            return this.getTagSuggestionsSqlite(prefix);
        } else {
            return this.getTagSuggestionsEs(prefix);
        }
    }

    private getTagSuggestionsSqlite(prefix): Promise<string[]> {
        return axios.post(`${this.baseUrl}/fts/suggestTags`, prefix)
            .then(resp => (resp.data));
    }

    private getTagSuggestionsEs(prefix): Promise<string[]> {
        return this.esQuery({
            suggest: {
                tag: {
                    prefix: prefix,
                    completion: {
                        field: "suggest-tag",
                        skip_duplicates: true,
                        size: 10000
                    }
                }
            }
        }).then(resp => {
            const result = [];
            resp["suggest"]["tag"][0]["options"].map(opt => opt["_source"]["tag"]).forEach(tags => {
                tags.forEach(tag => {
                    const t = tag.slice(0, -8);
                    if (!result.find(x => x.slice(0, -8) === t)) {
                        result.push(tag);
                    }
                });
            });
            return result;
        });
    }
}

export default new Sist2Api("");