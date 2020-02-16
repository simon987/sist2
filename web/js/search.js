const SIZE = 40;
let mimeMap = [];
let tagMap = [];
let mimeTree;
let tagTree;

let searchBar = document.getElementById("searchBar");
let pathBar = document.getElementById("pathBar");
let lastDoc = null;
let reachedEnd = false;
let docCount = 0;
let coolingDown = false;
let searchBusy = true;
let selectedIndices = [];

let mode;
if (localStorage.getItem("mode") === null) {
    mode = "grid";
} else {
    mode = localStorage.getItem("mode")
}

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

jQuery["jsonPost"] = function (url, data) {
    return jQuery.ajax({
        url: url,
        type: "post",
        data: JSON.stringify(data),
        contentType: "application/json"
    }).fail(err => {
        showEsError();
        console.log(err);
    });
};

window.onload = () => {
    $("#theme").on("click", () => {
        if (!document.cookie.includes("sist")) {
            document.cookie = "sist=dark";
        } else {
            document.cookie = "sist=; Max-Age=-99999999;";
        }
        window.location.reload();
    })
};

function toggleFuzzy() {
    searchDebounced();
}

$.jsonPost("i").then(resp => {

    const urlIndices = (new URLSearchParams(location.search)).get("i");

    resp["indices"].forEach(idx => {
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
});

function getDocumentInfo(id) {
    return $.getJSON("d/" + id).fail(e => {
        console.log(e);
        showEsError();
    })
}

function handleTreeClick(tree) {
    return (event, node, handler) => {
        event.preventTreeDefault();

        if (node.id === "any") {
            if (!node.itree.state.checked) {
                tree.deselect();
            }
        } else {
            tree.node("any").deselect();
        }

        handler();
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
    resp["aggregations"]["mimeTypes"]["buckets"].forEach(bucket => {
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
    mimeTree.on("node.click", handleTreeClick(mimeTree));
    mimeTree.deselect();
    mimeTree.node("any").select();
});

function leafTag(tag) {
    const tokens = tag.split(".");
    return tokens[tokens.length - 1]
}

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
        data: tagMap
    });
    new InspireTreeDOM(tagTree, {
        target: '#tagTree'
    });
    tagTree.on("node.click", handleTreeClick(tagTree));
    tagTree.node("any").select();
    searchBusy = false;
});

function addTag(map, tag, id, count) {
    let tags = tag.split("#")[0].split(".");

    let child = {
        id: id,
        text: tags.length !== 1 ? tags[0] : `${tags[0]} (${count})`,
        children: []
    };

    let found = false;
    map.forEach(node => {
        if (node.text === child.text) {
            found = true;
            if (tags.length !== 1) {
                addTag(node.children, tags.slice(1).join("."), id, count);
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

new autoComplete({
    selector: '#pathBar',
    minChars: 1,
    delay: 400,
    renderItem: function (item) {
        return '<div class="autocomplete-suggestion" data-val="' + item + '">' + item + '</div>';
    },
    source: async function (term, suggest) {
        term = term.toLowerCase();

        const choices = await getPathChoices();

        let matches = [];
        for (let i = 0; i < choices.length; i++) {
            if (~choices[i].toLowerCase().indexOf(term)) {
                matches.push(choices[i]);
            }
        }
        suggest(matches);
    },
    onSelect: function () {
        searchDebounced();
    }
});

function insertHits(resultContainer, hits) {
    for (let i = 0; i < hits.length; i++) {

        if (mode === "grid") {
            resultContainer.appendChild(createDocCard(hits[i]));
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
            selectedNodes.push(selected[i].id);
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

    if ($("#fuzzyToggle").prop("checked")) {
        fields.push("content.nGram");
        fields.push("name.nGram^3");
    }

    let path = pathBar.value.replace(/\/$/, "").toLowerCase(); //remove trailing slashes
    if (path !== "") {
        filters.push([{term: {path: path}}])
    }
    let mimeTypes = getSelectedNodes(mimeTree);
    if (!mimeTypes.includes("any")) {
        filters.push([{terms: {"mime": mimeTypes}}]);
    }

    let tags = getSelectedNodes(tagTree);
    if (!tags.includes("any")) {
        filters.push([{terms: {"tag": tags}}]);
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
        "sort": [
            {"_score": {"order": "desc"}},
            {"_tie": {"order": "asc"}}
        ],
        highlight: {
            pre_tags: ["<mark>"],
            post_tags: ["</mark>"],
            fields: {
                content: {},
                // "content.nGram": {},
                name: {},
                "name.nGram": {},
                font_name: {},
            }
        },
        aggs:
            {
                total_size: {"sum": {"field": "size"}},
                total_count: {"value_count": {"field": "size"}}
            },
        size: SIZE,
    };

    if (after) {
        q.search_after = [after["_score"], after["_id"]];
    }

    $.jsonPost("es", q).then(searchResult => {
        let hits = searchResult["hits"]["hits"];
        if (hits) {
            lastDoc = hits[hits.length - 1];
        }

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

        window.setTimeout(() => {
            $(".sp").SmartPhoto({animationSpeed: 0, swipeTopToClose: true, showAnimation: false, forceInterval: 50});
        }, 100);

        if (!after) {
            docCount = 0;
        }
        reachedEnd = hits.length !== SIZE;
        insertHits(resultContainer, hits);
        searchBusy = false;
    });
}

let size_min = 0;
let size_max = 10000000000000;

let searchDebounced = _.debounce(function () {
    coolingDown = false;
    search()
}, 500);

searchBar.addEventListener("keyup", searchDebounced);

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

window.onkeyup = function(e) {
    if (e.key === "/" || e.key === "Escape") {
        const bar = document.getElementById("searchBar");
        bar.scrollIntoView();
        bar.focus();
    }
    console.log(e)
};

//Suggest
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
    })
}

