import axios from "axios";
import {ext, strUnescape, lum} from "./util";

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

    private baseUrl: string

    constructor(baseUrl: string) {
        this.baseUrl = baseUrl;
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

    getMimeTypes(query = undefined) {
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
            const mimeMap: any[] = [];
            const buckets = resp["aggregations"]["mimeTypes"]["buckets"];

            buckets.sort((a: any, b: any) => a.key > b.key).forEach((bucket: any) => {
                const tmp = bucket["key"].split("/");
                const category = tmp[0];
                const mime = tmp[1];

                let category_exists = false;

                const child = {
                    "id": bucket["key"],
                    "text": `${mime} (${bucket["doc_count"]})`
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
        });
    }

    _createEsTag(tag: string, count: number): EsTag {
        const tokens = tag.split(".");

        if (/.*\.#[0-9a-f]{6}/.test(tag)) {
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

    getTags() {
        return this.esQuery({
            aggs: {
                tags: {
                    terms: {
                        field: "tag",
                        size: 10000
                    }
                }
            },
            size: 0,
        }).then(resp => {
            const seen = new Set();

            const tags = resp["aggregations"]["tags"]["buckets"]
                .sort((a: any, b: any) => a["key"].localeCompare(b["key"]))
                .map((bucket: any) => this._createEsTag(bucket["key"], bucket["doc_count"]));

            // Remove duplicates (same tag with different color)
            return tags.filter((t: EsTag) => {
                if (seen.has(t.id)) {
                    return false;
                }
                seen.add(t.id);
                return true;
            });
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
}

export default new Sist2Api("");