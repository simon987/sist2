const SIZE = 60;
let mimeMap = [];
let tagMap = [];
let mimeTree;
let tagTree;

let searchBar = document.getElementById("searchBar");
let pathBar = document.getElementById("pathBar");
let tagBar = document.getElementById("tagBar");
let currentDocToTag = null;
let currentTagCallback = null;
let lastDoc = null;
let reachedEnd = false;
let docCount = 0;
let coolingDown = false;
let searchBusy = true;
let selectedIndices = [];
let indexMap = {};

let size_min = 0;
let size_max = 10000000000000;

let date_min = null;
let date_max = null;

SORT_MODES = {
    score: {
        text: "Relevance",
        mode: [
            {_score: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: hit => hit["_score"]
    },
    date_asc: {
        text: "Date (Ascending)", mode: [
            {mtime: {order: "asc"}},
            {_tie: {order: "asc"}}
        ],
        key: hit => hit["_source"]["mtime"]
    },
    date_desc: {
        text: "Date (Descending)", mode: [
            {mtime: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: hit => hit["_source"]["mtime"]
    },
    size_asc: {
        text: "Size (Ascending)", mode: [
            {size: {order: "asc"}},
            {_tie: {order: "asc"}}
        ],
        key: hit => hit["_source"]["size"]
    },
    size_desc: {
        text: "Size (Descending)", mode: [
            {size: {order: "desc"}},
            {_tie: {order: "asc"}}
        ],
        key: hit => hit["_source"]["size"]
    },
};

function showEsError() {
    $.toast({
        heading: "Elasticsearch connection error",
        text: "sist2 web module encountered an error while connecting " +
            "to Elasticsearch. See server logs for more information.",
        stack: false,
        bgColor: "#a94442",
        textColor: "#f2dede",
        position: 'bottom-right',
        hideAfter: false
    });
}

window.onload = () => {
    CONF.load();
    new autoComplete({
        selector: '#pathBar',
        minChars: 1,
        delay: 400,
        renderItem: function (item) {
            return '<div class="autocomplete-suggestion" data-val="' + item + '">' + item + '</div>';
        },
        source: async function (term, suggest) {

            if (!CONF.options.suggestPath) {
                return []
            }

            term = term.toLowerCase();

            const choices = await getPathChoices();

            let matches = [];
            for (let i = 0; i < choices.length; i++) {
                if (~choices[i].toLowerCase().indexOf(term)) {
                    matches.push(choices[i]);
                }
            }
            suggest(matches.sort());
        },
        onSelect: function () {
            searchDebounced();
        }
    });
    searchBar.addEventListener("keyup", searchDebounced);
    pathBar.addEventListener("keyup", e => {
        if (e.key === "Enter") {
            searchDebounced();
        }
    });
    new autoComplete({
        selector: '#tagBar',
        minChars: 1,
        delay: 200,
        renderItem: function (item) {
            return '<div class="autocomplete-suggestion" data-val="' + item + '">' + item.split("#")[0] + '</div>';
        },
        source: async function (term, suggest) {
            term = term.toLowerCase();

            const choices = await getTagChoices();

            let matches = [];
            for (let i = 0; i < choices.length; i++) {
                if (~choices[i].toLowerCase().indexOf(term)) {
                    matches.push(choices[i]);
                }
            }
            suggest(matches.sort());
        },
        onSelect: function (e, item) {
            const name = item.split("#")[0];
            const color = "#" + item.split("#")[1];
            $("#tag-color").val(color);
            $("#tag-color").trigger("keyup", color);
            tagBar.value = name;
            e.preventDefault();
        }
    });
    [tagBar, document.getElementById("tag-color")].forEach(elem => {
        elem.addEventListener("keyup", e => {
            if (e.key === "Enter" && tagBar.value.length > 0) {
                const tag = tagBar.value + document.getElementById("tag-color").value;
                saveTag(tag, currentDocToTag).then(() => currentTagCallback(tag));
            }
        });
    })
    $("#tag-color").colorpicker({
        format: "hex",
        sliders: {
            saturation: {
                selector: '.colorpicker-saturation',
                callLeft: 'setSaturationRatio',
                callTop: 'setValueRatio'
            },
            hue: {
                selector: '.colorpicker-hue',
                maxLeft: 0,
                callLeft: false,
                callTop: 'setHueRatio'
            }
        }
    });
};

function saveTag(tag, hit) {
    const relPath = hit["_source"]["path"] + (hit["_source"]["path"] ? "/" : "") + hit["_source"]["name"] + ext(hit);

    return $.jsonPost("/tag/" + hit["_source"]["index"], {
        delete: false,
        name: tag,
        doc_id: hit["_id"],
        relpath: relPath
    }).then(() => {
        tagBar.blur();
        $("#tagModal").modal("hide");
        $.toast({
            heading: "Tag added",
            text: "Tag saved to index storage and updated in ElasticSearch",
            stack: 3,
            bgColor: "#00a4bc",
            textColor: "#fff",
            position: 'bottom-right',
            hideAfter: 3000,
            loaderBg: "#08c7e8",
        });
    })
}

function deleteTag(tag, hit) {
    const relPath = hit["_source"]["path"] + "/" + hit["_source"]["name"] + ext(hit);

    return $.jsonPost("/tag/" + hit["_source"]["index"], {
        delete: true,
        name: tag,
        doc_id: hit["_id"],
        relpath: relPath
    }).then(() => {
        $.toast({
            heading: "Tag deleted",
            text: "Tag deleted index storage and updated in ElasticSearch",
            stack: 3,
            bgColor: "#00a4bc",
            textColor: "#fff",
            position: 'bottom-right',
            hideAfter: 3000,
            loaderBg: "#08c7e8",
        });
    })
}

function toggleFuzzy() {
    searchDebounced();
}

$.get("i").then(resp => {

    const urlIndices = (new URLSearchParams(location.search)).get("i");
    resp["indices"].forEach(idx => {
        indexMap[idx.id] = idx.name;
        const opt = $("<option>")
            .attr("value", idx.id)
            .append(idx.name);

        if (urlIndices) {
            if (urlIndices.split(",").indexOf(idx.name) !== -1) {
                opt.attr("selected", true);
                selectedIndices.push(idx.id);
            }
        } else if (!idx.name.includes("(nsfw)")) {
            opt.attr("selected", true);
            selectedIndices.push(idx.id);
        }
        $("#indices").append(opt);
    });

    createPathTree("#pathTree");
});

function getDocumentInfo(id) {
    return $.getJSON("d/" + id).fail(showEsError)
}

function handleTreeClick(tree) {
    return (node, e) => {
        if (e !== "checked") {
            return
        }

        if (node.id === "any") {
            if (!node.itree.state.checked) {
                tree.deselectDeep();
            }
        } else {
            tree.node("any").deselect();
        }

        searchDebounced();
    }
}

$.jsonPost("es", {
    aggs: {
        mimeTypes: {
            terms: {
                field: "mime",
                size: 10000
            }
        }
    },
    size: 0,
}).then(resp => {
    resp["aggregations"]["mimeTypes"]["buckets"].sort((a, b) => a.key > b.key).forEach(bucket => {
        let tmp = bucket["key"].split("/");
        let category = tmp[0];
        let mime = tmp[1];

        let category_exists = false;

        let child = {
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
            mimeMap.push({"text": category, children: [child]});
        }
    });
    mimeMap.push({"text": "All", "id": "any"});

    mimeTree = new InspireTree({
        selection: {
            mode: 'checkbox'
        },
        data: mimeMap
    });
    new InspireTreeDOM(mimeTree, {
        target: '#mimeTree'
    });
    mimeTree.on("node.state.changed", handleTreeClick(mimeTree));
    mimeTree.deselect();
    mimeTree.node("any").select();
});

// Tags tree
$.jsonPost("es", {
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
    resp["aggregations"]["tags"]["buckets"]
        .sort((a, b) => a["key"].localeCompare(b["key"]))
        .forEach(bucket => {
            addTag(tagMap, bucket["key"], bucket["key"], bucket["doc_count"])
        });

    tagMap.push({"text": "All", "id": "any"});
    tagTree = new InspireTree({
        selection: {
            mode: 'checkbox'
        },
        checkbox: {
            autoCheckChildren: false
        },
        data: tagMap
    });
    new InspireTreeDOM(tagTree, {
        target: '#tagTree'
    });
    tagTree.on("node.state.changed", handleTreeClick(tagTree));
    tagTree.node("any").select();
    searchBusy = false;
});

function addTag(map, tag, id, count) {
    // let tags = tag.split("#")[0].split(".");
    let tags = tag.split(".");

    let child = {
        id: id,
        values: [id],
        count: count,
        text: tags.length !== 1 ? tags[0] : `${tags[0].split("#")[0]} (${count})`,
        name: tags[0],
        children: [],
        isLeaf: tags.length === 1,
        //Overwrite base functions
        blur: function () {
        },
        select: function () {
            this.state("selected", true);
            return this.check()
        },
        deselect: function () {
            this.state("selected", false);
            return this.uncheck()
        },
        uncheck: function () {
            if (!this.isLeaf) {
                return;
            }

            baseStateChange('checked', false, 'unchecked', this, false);
            this.state('indeterminate', false);

            if (this.hasParent()) {
                this.getParent().refreshIndeterminateState();
            }

            this._tree.end();
            return this;
        },
        check: function () {
            if (!this.isLeaf) {
                return;
            }

            baseStateChange('checked', true, 'checked', this, false);

            if (this.hasParent()) {
                this.getParent().refreshIndeterminateState();
            }

            this._tree.end();
            return this;
        }
    };

    let found = false;
    map.forEach(node => {
        if (node.name.split("#")[0] === child.name.split("#")[0]) {
            found = true;
            if (tags.length !== 1) {
                addTag(node.children, tags.slice(1).join("."), id, count);
            } else {
                // Same name, different color
                node.count += count;
                node.text = `${tags[0].split("#")[0]} (${node.count})`;
                node.values.push(id);
            }
        }
    });
    if (!found) {
        if (tags.length !== 1) {
            addTag(child.children, tags.slice(1).join("."), id, count);
            map.push(child);
        } else {
            map.push(child);
        }
    }
}

function insertHits(resultContainer, hits) {
    for (let i = 0; i < hits.length; i++) {

        if (CONF.options.display === "grid") {
            resultContainer._brick.append(createDocCard(hits[i]));
        } else {
            resultContainer.appendChild(createDocLine(hits[i]));
        }
        docCount++;
    }
}

window.addEventListener("scroll", function () {
    if (!searchBusy) {
        let threshold = 400;

        if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - threshold) {
            if (!reachedEnd) {
                coolingDown = true;
                search(lastDoc);
            }
        }
    }
});

function getSelectedNodes(tree) {
    let selectedNodes = [];

    let selected = tree.selected();

    for (let i = 0; i < selected.length; i++) {

        if (selected[i].id === "any") {
            return ["any"]
        }

        //Only get children
        if (selected[i].text.indexOf("(") !== -1) {
            if (selected[i].values) {
                selectedNodes.push(selected[i].values);
            } else {
                selectedNodes.push(selected[i].id);
            }
        }
    }

    return selectedNodes
}

function search(after = null) {
    lastDoc = null;

    if (searchBusy) {
        return;
    }
    searchBusy = true;

    let searchResults = document.getElementById("searchResults");
    //Clear old search results
    let preload;
    if (!after) {
        while (searchResults.firstChild) {
            searchResults.removeChild(searchResults.firstChild);
        }
        preload = makePreloader();
        searchResults.appendChild(preload);
    }

    let query = searchBar.value;
    let empty = query === "";
    let condition = empty ? "should" : "must";
    let filters = [
        {range: {size: {gte: size_min, lte: size_max}}},
        {terms: {index: selectedIndices}}
    ];
    let fields = [
        "name^8",
        "content^3",
        "album^8", "artist^8", "title^8", "genre^2", "album_artist^8",
        "font_name^6"
    ];

    if (CONF.options.searchInPath) {
        fields.push("path.text^5");
    }

    if ($("#fuzzyToggle").prop("checked")) {
        fields.push("content.nGram");
        if (CONF.options.searchInPath) {
            fields.push("path.nGram");
        }
        fields.push("name.nGram^3");
    }

    let path = pathBar.value.replace(/\/$/, "").toLowerCase(); //remove trailing slashes
    if (path !== "") {
        filters.push({term: {path: path}})
    }
    let mimeTypes = getSelectedNodes(mimeTree);
    if (!mimeTypes.includes("any")) {
        filters.push({terms: {"mime": mimeTypes}});
    }

    let tags = getSelectedNodes(tagTree);
    if (!tags.includes("any")) {
        tags.forEach(tagGroup => {
            filters.push({terms: {"tag": tagGroup}})
        })
    }

    if (date_min && date_max) {
        filters.push({range: {mtime: {gte: date_min, lte: date_max}}})
    } else if (date_min) {
        filters.push({range: {mtime: {gte: date_min}}})
    } else if (date_max) {
        filters.push({range: {mtime: {lte: date_max}}})
    }

    let q = {
        "_source": {
            excludes: ["content", "_tie"]
        },
        query: {
            bool: {
                [condition]: {
                    simple_query_string: {
                        query: query,
                        fields: fields,
                        default_operator: "and"
                    }
                },
                filter: filters
            }
        },
        "sort": SORT_MODES[CONF.options.sort].mode,
        aggs:
            {
                total_size: {"sum": {"field": "size"}},
                total_count: {"value_count": {"field": "size"}}
            },
        size: SIZE,
    };

    if (after) {
        q.search_after = [SORT_MODES[CONF.options.sort].key(after), after["_id"]];
    }

    if (CONF.options.highlight) {
        q.highlight = {
            pre_tags: ["<mark>"],
            post_tags: ["</mark>"],
            fragment_size: CONF.options.fragmentSize,
            number_of_fragments: 1,
            order: "score",
            fields: {
                content: {},
                // "content.nGram": {},
                name: {},
                "name.nGram": {},
                font_name: {},
            }
        };
        if (CONF.options.searchInPath) {
            q.highlight.fields["path.text"] = {};
            q.highlight.fields["path.nGram"] = {};
        }
    }

    $.jsonPost("es", q).then(searchResult => {
        let hits = searchResult["hits"]["hits"];
        if (hits) {
            lastDoc = hits[hits.length - 1];
        }

        hits.forEach(hit => {
            hit["_source"]["name"] = strUnescape(hit["_source"]["name"]);
            hit["_source"]["path"] = strUnescape(hit["_source"]["path"]);
        });

        if (!after) {
            preload.remove();
            searchResults.appendChild(makeStatsCard(searchResult));
        } else {
            let pageIndicator = makePageIndicator(searchResult);
            searchResults.appendChild(pageIndicator);
        }

        //Setup page
        let resultContainer = makeResultContainer();
        searchResults.appendChild(resultContainer);

        if (CONF.options.display === "grid") {
            resultContainer._brick = new Bricklayer(resultContainer);
        }

        if (!after) {
            docCount = 0;
        }
        reachedEnd = hits.length !== SIZE;
        insertHits(resultContainer, hits);
        searchBusy = false;
    });
}


let searchDebounced = _.debounce(function () {
    coolingDown = false;
    search()
}, 500);


//Size slider
$("#sizeSlider").ionRangeSlider({
    type: "double",
    grid: false,
    force_edges: true,
    min: 0,
    max: 3684.03149864,
    from: 0,
    to: 3684.03149864,
    min_interval: 5,
    drag_interval: true,
    prettify: function (num) {

        if (num === 0) {
            return "0 B"
        } else if (num >= 3684) {
            return humanFileSize(num * num * num) + "+";
        }

        return humanFileSize(num * num * num)
    },
    onChange: function (e) {
        size_min = (e.from * e.from * e.from);
        size_max = (e.to * e.to * e.to);

        if (e.to >= 3684) {
            size_max = 10000000000000;
        }

        searchDebounced();
    }
});

//Date slider
$.jsonPost("es", {
    aggs: {
        date_min: {min: {field: "mtime"}},
        date_max: {max: {field: "mtime"}},
    },
    size: 0
}).then(resp => {
    $("#dateSlider").ionRangeSlider({
        type: "double",
        grid: false,
        force_edges: true,
        min: resp["aggregations"]["date_min"]["value"],
        max: resp["aggregations"]["date_max"]["value"],
        from: resp["aggregations"]["date_min"]["value"],
        to: (Date.now() / 1000),
        min_interval: 3600 * 24 * 7,
        step: 3600 * 24,
        drag_interval: true,
        prettify: function (num) {
            let date = (new Date(num * 1000));
            return date.getUTCFullYear() + "-" + ("0" + (date.getUTCMonth() + 1)).slice(-2) + "-" + ("0" + date.getUTCDate()).slice(-2)
        },
        onFinish: function (e) {
            date_min = e.from === e.min ? null : e.from;
            date_max = e.to === e.max ? null : e.to;
            searchDebounced();
        }
    });
})

function updateIndices() {
    let selected = $('#indices').find('option:selected');
    selectedIndices = [];
    $(selected).each(function () {
        selectedIndices.push($(this).val());
    });

    searchDebounced();
}

document.getElementById("indices").addEventListener("change", updateIndices);
updateIndices();

window.onkeyup = function (e) {
    if (e.key === "/" || e.key === "Escape") {
        const bar = document.getElementById("searchBar");
        bar.scrollIntoView();
        bar.focus();
    }
};

function getNextDepth(node) {
    let q = {
        query: {
            bool: {
                filter: [
                    {term: {index: node.index}},
                    {range: {_depth: {gte: node.depth + 1, lte: node.depth + 3}}},
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

    if (node.depth > 0) {
        q.query.bool.must = {
            prefix: {
                path: node.id,
            }
        };
    }

    return $.jsonPost("es", q).then(resp => {
        const buckets = resp["aggregations"]["paths"]["buckets"];
        if (!buckets) {
            return false;
        }

        const paths = [];

        return buckets
            .filter(bucket => bucket.key.length > node.id.length || node.id.startsWith("/"))
            .sort((a, b) => a.key > b.key)
            .map(bucket => {

                if (paths.some(n => bucket.key.startsWith(n))) {
                    return null;
                }

                const name = node.id.startsWith("/") ? bucket.key : bucket.key.slice(node.id.length + 1);

                paths.push(bucket.key);

                return {
                    id: bucket.key,
                    text: `${name}/ (${bucket.doc_count})`,
                    depth: node.depth + 1,
                    index: node.index,
                    values: [bucket.key],
                    children: true,
                }
            }).filter(x => x !== null)
    })
}

function handlePathTreeClick(tree) {
    return (event, node, handler) => {

        if (node.depth !== 0) {
            $("#pathBar").val(node.id);
            $("#pathTreeModal").modal("hide");
            searchDebounced();
        }

        handler();
    }
}

function createPathTree(target) {
    let pathTree = new InspireTree({
        data: function (node, resolve, reject) {
            return getNextDepth(node);
        },
        sort: "text"
    });

    selectedIndices.forEach(index => {
        pathTree.addNode({
            id: "/" + index,
            values: ["/" + index],
            text: `/[${indexMap[index]}]`,
            index: index,
            depth: 0,
            children: true
        })
    });

    new InspireTreeDOM(pathTree, {
        target: target
    });

    pathTree.on("node.click", handlePathTreeClick(pathTree));
}

function getPathChoices() {
    return new Promise(getPaths => {
        $.jsonPost("es", {
            suggest: {
                path: {
                    prefix: pathBar.value,
                    completion: {
                        field: "suggest-path",
                        skip_duplicates: true,
                        size: 10000
                    }
                }
            }
        }).then(resp => getPaths(resp["suggest"]["path"][0]["options"].map(opt => opt["_source"]["path"])));
    });
}


function getTagChoices() {
    return new Promise(getPaths => {
        $.jsonPost("es", {
            suggest: {
                tag: {
                    prefix: tagBar.value,
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
                    const t = tag.split("#")[0];
                    if (!result.find(x => x.split("#")[0] === t)) {
                        result.push(tag);
                    }
                });
            });
            getPaths(result);
        });
    });
}
