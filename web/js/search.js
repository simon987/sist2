const SIZE = 40;
let mimeMap = [];
let tagMap = [];
let mimeTree;
let tagTree;

let searchBar = document.getElementById("searchBar");
let pathBar = document.getElementById("pathBar");
let scroll_id = null;
let docCount = 0;
let coolingDown = false;
let searchBusy = true;
let selectedIndices = [];

jQuery["jsonPost"] = function (url, data) {
    return jQuery.ajax({
        url: url,
        type: "post",
        data: JSON.stringify(data),
        contentType: "application/json"
    }).fail(err => {
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
    resp["indices"].forEach(idx => {
        const opt = $("<option>")
            .attr("value", idx.id)
            .append(idx.name);
        if (!idx.name.includes("(nsfw)")) {
            opt.attr("selected", !idx.name.includes("(nsfw)"));
            selectedIndices.push(idx.id);
        }
        $("#indices").append(opt);
    });
});

function handleTreeClick (tree) {
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
    mimeTree.select();
    mimeTree.node("any").deselect();
});

function leafTag(tag) {
    const tokens = tag.split(".");
    return tokens[tokens.length-1]
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
        resultContainer.appendChild(createDocCard(hits[i]));
        docCount++;
    }
}

window.addEventListener("scroll", function () {
    if (!coolingDown && !searchBusy) {
        let threshold = 400;

        if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - threshold) {
            coolingDown = true;
            doScroll();
        }
    }
});

function doScroll() {
    $.get("scroll", {scroll_id: scroll_id})
        .then(searchResult => {
            let searchResults = document.getElementById("searchResults");
            let hits = searchResult["hits"]["hits"];

            //Page indicator
            let pageIndicator = makePageIndicator(searchResult);
            searchResults.appendChild(pageIndicator);

            //Result container
            let resultContainer = makeResultContainer();
            searchResults.appendChild(resultContainer);

            insertHits(resultContainer, hits);

            if (hits.length === SIZE) {
                coolingDown = false;
            }
        })
        .fail(() => {
            window.location.reload();
        })
}

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

function search() {
    if (searchBusy) {
        return;
    }
    searchBusy = true;

    //Clear old search results
    let searchResults = document.getElementById("searchResults");
    while (searchResults.firstChild) {
        searchResults.removeChild(searchResults.firstChild);
    }

    const preload = makePreloader();
    searchResults.appendChild(preload);

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

    $.jsonPost("es?scroll=1", {
        "_source": {
            excludes: ["content"]
        },
        query: {
            bool: {
                [condition]: {
                    multi_match: {
                        query: query,
                        type: "most_fields",
                        fields: fields,
                        operator: "and"
                    }
                },
                filter: filters
            }
        },
        sort: [
            "_score"
        ],
        highlight: {
            pre_tags: ["<mark>"],
            post_tags: ["</mark>"],
            fields: {
                content: {},
                name: {},
                "name.nGram": {},
                font_name: {},
            }
        },
        aggs: {
            total_size: {"sum": {"field": "size"}}
        },
        size: SIZE,
    }).then(searchResult => {
        scroll_id = searchResult["_scroll_id"];

        preload.remove();
        //Search stats
        searchResults.appendChild(makeStatsCard(searchResult));

        //Setup page
        let resultContainer = makeResultContainer();
        searchResults.appendChild(resultContainer);

        docCount = 0;
        insertHits(resultContainer, searchResult["hits"]["hits"]);

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

