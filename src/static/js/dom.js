/**
 * Enable gif loading on hover
 */
function gifOver(thumbnail, hit) {
    let callee = arguments.callee;

    thumbnail.addEventListener("mouseover", function () {

        thumbnail.mouseStayedOver = true;

        window.setTimeout(function () {
            if (thumbnail.mouseStayedOver) {
                thumbnail.removeEventListener('mouseover', callee, false);

                //Load gif
                thumbnail.setAttribute("src", "f/" + hit["_id"]);
            }
        }, 600);

    });

    thumbnail.addEventListener("mouseout", function () {
        //Reset timer
        thumbnail.mouseStayedOver = false;
        thumbnail.setAttribute("src", `t/${hit["_source"]["index"]}/${hit["_id"]}`);
    })
}

function getContentHighlight(hit) {
    const re = RegExp(/<mark>/g);

    const sortByMathCount = (a, b) => {
        return b.match(re).length - a.match(re).length;
    };

    if (hit.hasOwnProperty("highlight")) {
        if (hit["highlight"].hasOwnProperty("content")) {
            return hit["highlight"]["content"].sort(sortByMathCount)[0];

        } else if (hit["highlight"].hasOwnProperty("content.nGram")) {
            return hit["highlight"]["content.nGram"].sort(sortByMathCount)[0];
        }
    }

    return undefined;
}

function applyNameToTitle(hit, title, extension) {
    if (hit.hasOwnProperty("highlight")) {
        if (hit["highlight"].hasOwnProperty("name")) {
            title.insertAdjacentHTML('afterbegin', hit["highlight"]["name"] + extension);
            return;
        } else if (hit["highlight"].hasOwnProperty("name.nGram")) {
            title.insertAdjacentHTML('afterbegin', hit["highlight"]["name.nGram"] + extension);
            return;
        }
    }

    title.appendChild(document.createTextNode(hit["_source"]["name"] + extension));
}

function addVidSrc(url, mime, video) {
    let vidSource = document.createElement("source");
    vidSource.setAttribute("src", url);
    if (video.canPlayType(mime)) {
        vidSource.setAttribute("type", mime);
    } else {
        vidSource.setAttribute("type", "video/webm");
    }
    video.appendChild(vidSource);
}

function shouldPlayVideo(hit) {
    const videoc = hit["_source"]["videoc"];
    return videoc !== "hevc" && videoc !== "mpeg2video" && videoc !== "wmv3";
}

function shouldDisplayRawImage(hit) {
    return hit["_source"]["mime"] && hit["_source"]["mime"].startsWith("image/") && hit["_source"]["videoc"] !== "tiff";
}

function makePlaceholder(w, h, small) {
    let calc;
    if (small) {
        calc = w > h
            ? (64 / w / h) >= 100
                ? (64 * w / h)
                : 64
            : 64;
    } else {
        calc = w > h
            ? (175 / w / h) >= 272
                ? (175 * w / h)
                : 175
            : 175;
    }

    const el = document.createElement("div");
    el.setAttribute("style", `height: ${calc}px`);
    return el;
}

function ext(hit) {
    return hit["_source"].hasOwnProperty("extension") && hit["_source"]["extension"] !== "" ? "." + hit["_source"]["extension"] : "";
}

function makeTitle(hit) {
    let title = document.createElement("div");
    title.setAttribute("class", "file-title");
    let extension = ext(hit);

    applyNameToTitle(hit, title, extension);

    title.setAttribute("title", hit["_source"]["path"] + "/" + hit["_source"]["name"] + extension);
    return title;
}

function getTags(hit, mimeCategory) {

    let tags = [];
    switch (mimeCategory) {
        case "video":
        case "image":
            if (hit["_source"].hasOwnProperty("videoc") && hit["_source"]["videoc"]) {
                const formatTag = document.createElement("span");
                formatTag.setAttribute("class", "badge badge-pill badge-video");
                formatTag.appendChild(document.createTextNode(hit["_source"]["videoc"].replace(" ", "")));
                tags.push(formatTag);
            }
            break;
        case "audio": {
            if (hit["_source"].hasOwnProperty("audioc") && hit["_source"]["audioc"]) {
                let formatTag = document.createElement("span");
                formatTag.setAttribute("class", "badge badge-pill badge-audio");
                formatTag.appendChild(document.createTextNode(hit["_source"]["audioc"]));
                tags.push(formatTag);
            }
        }
            break;
    }
    // User tags
    if (hit["_source"].hasOwnProperty("tag")) {
        hit["_source"]["tag"].forEach(tag => {
            const userTag = document.createElement("span");
            userTag.setAttribute("class", "badge badge-pill badge-user");

            const tokens = tag.split("#");

            if (tokens.length > 1) {
                const bg = "#" + tokens[1];
                const fg = lum(tokens[1]) > 40 ? "#000" : "#fff";
                userTag.setAttribute("style", `background-color: ${bg}; color: ${fg}`);
            }

            const name = tokens[0].split(".")[tokens[0].split(".").length - 1];
            userTag.appendChild(document.createTextNode(name));
            tags.push(userTag);
        })
    }

    return tags
}

function infoButtonCb(hit) {
    return () => {
        getDocumentInfo(hit["_id"]).then(doc => {
            $("#modal-title").text(doc["name"] + ext(hit));

            const tbody = $("<tbody>");
            $("#modal-body").empty()
                .append($("<table class='table table-sm'>")
                    .append($("<thead>")
                        .append($("<tr>")
                            .append($("<th>").text("Field"))
                            .append($("<th>").text("Value"))
                        )
                    )
                    .append(tbody)
                );

            const displayFields = new Set([
                "mime", "size", "mtime", "path", "title", "width", "height", "duration", "audioc", "videoc",
                "bitrate", "artist", "album", "album_artist", "genre", "title", "font_name", "tag"
            ]);
            Object.keys(doc)
                .filter(key => key.startsWith("_keyword.") || key.startsWith("_text.") || displayFields.has(key) || key.startsWith("exif_"))
                .forEach(key => {
                    tbody.append($("<tr>")
                        .append($("<td>").text(key))
                        .append($("<td>").text(doc[key]))
                    );
                });
            if (doc.hasOwnProperty("content") && doc["content"]) {
                $("#modal-body").append($("<div class='content-div'>").text(doc["content"]))
            }

            $("#modal").modal();
        });
    }
}

function createDocCard(hit) {
    let docCard = document.createElement("div");
    docCard.setAttribute("class", "card");

    let docCardBody = document.createElement("div");
    docCardBody.setAttribute("class", "card-body document");

    //Title
    let title = makeTitle(hit);
    let isSubDocument = false;

    let link = document.createElement("a");
    link.setAttribute("href", "f/" + hit["_id"]);
    link.setAttribute("target", "_blank");
    link.style.maxWidth = "calc(100% - 1.2rem)";
    link.appendChild(title);

    if (hit["_source"].hasOwnProperty("parent")) {
        docCard.classList.add("sub-document");
        isSubDocument = true;
    }

    let tagContainer = document.createElement("div");
    tagContainer.setAttribute("class", "card-text");

    if (hit["_source"].hasOwnProperty("mime") && hit["_source"]["mime"] !== null) {

        let thumbnailOverlay = null;
        let imgWrapper = document.createElement("div");
        imgWrapper.setAttribute("style", "position: relative");

        let mimeCategory = hit["_source"]["mime"].split("/")[0];

        //Thumbnail
        let thumbnail = makeThumbnail(mimeCategory, hit, imgWrapper, false);

        //Thumbnail overlay
        switch (mimeCategory) {

            case "image":
                thumbnailOverlay = document.createElement("div");
                thumbnailOverlay.setAttribute("class", "card-img-overlay");

                //Resolution
                if (hit["_source"].hasOwnProperty("width") && hit["_source"]["width"] > 32 && hit["_source"]["height"] > 32) {
                    let resolutionBadge = document.createElement("span");
                    resolutionBadge.setAttribute("class", "badge badge-resolution");
                    if (hit["_source"].hasOwnProperty("width")) {
                        resolutionBadge.appendChild(document.createTextNode(hit["_source"]["width"] + "x" + hit["_source"]["height"]));
                    }
                    thumbnailOverlay.appendChild(resolutionBadge);
                }

                // Hover
                if (thumbnail && hit["_source"]["videoc"] === "gif" && !isSubDocument) {
                    gifOver(thumbnail, hit);
                }
                break;

            case "video":
                //Duration
                if (hit["_source"].hasOwnProperty("duration")) {
                    thumbnailOverlay = document.createElement("div");
                    thumbnailOverlay.setAttribute("class", "card-img-overlay");
                    const durationBadge = document.createElement("span");
                    durationBadge.setAttribute("class", "badge badge-resolution");
                    durationBadge.appendChild(document.createTextNode(humanTime(hit["_source"]["duration"])));
                    thumbnailOverlay.appendChild(durationBadge);
                }
        }

        // Tags
        let tags = getTags(hit, mimeCategory);
        for (let i = 0; i < tags.length; i++) {
            tagContainer.appendChild(tags[i]);
        }

        //Content
        let contentHl = getContentHighlight(hit);
        if (contentHl !== undefined) {
            const contentDiv = document.createElement("div");
            contentDiv.setAttribute("class", "content-div");
            contentDiv.insertAdjacentHTML('afterbegin', contentHl);
            docCard.appendChild(contentDiv);
        }

        if (thumbnail !== null) {
            docCard.appendChild(imgWrapper);
        }

        //Audio
        if (mimeCategory === "audio" && hit["_source"].hasOwnProperty("audioc") && !isSubDocument) {

            let audio = document.createElement("audio");
            audio.setAttribute("preload", "none");
            audio.setAttribute("class", "audio-fit fit");
            audio.setAttribute("controls", "");
            audio.setAttribute("type", hit["_source"]["mime"]);
            audio.setAttribute("src", "f/" + hit["_id"]);

            docCard.appendChild(audio)
        }

        if (thumbnailOverlay !== null) {
            imgWrapper.appendChild(thumbnailOverlay);
        }
    }

    //Size tag
    let sizeTag = document.createElement("small");
    sizeTag.appendChild(document.createTextNode(humanFileSize(hit["_source"]["size"])));
    sizeTag.setAttribute("class", "text-muted");
    tagContainer.appendChild(sizeTag);

    const titleWrapper = document.createElement("div");
    titleWrapper.style.display = "flex";

    const infoButton = makeInfoButton(hit);

    titleWrapper.appendChild(infoButton);
    titleWrapper.appendChild(link);

    docCardBody.appendChild(titleWrapper);
    docCard.appendChild(docCardBody);

    docCardBody.appendChild(tagContainer);

    return docCard;
}

function makeThumbnail(mimeCategory, hit, imgWrapper, small) {
    let thumbnail;
    let isSubDocument = hit["_source"].hasOwnProperty("parent");

    if (mimeCategory === "video" && shouldPlayVideo(hit) && !isSubDocument) {
        thumbnail = document.createElement("video");
        addVidSrc("f/" + hit["_id"], hit["_source"]["mime"], thumbnail);

        const placeholder = makePlaceholder(hit["_source"]["width"], hit["_source"]["height"], small);
        imgWrapper.appendChild(placeholder);

        if (small) {
            thumbnail.setAttribute("class", "fit-sm");
        } else {
            thumbnail.setAttribute("class", "fit");
        }
        if (small) {
            thumbnail.style.cursor = "pointer";
            thumbnail.title = "Enlarge";
            thumbnail.addEventListener("click", function () {
                imgWrapper.classList.remove("wrapper-sm", "mr-1");
                imgWrapper.parentElement.classList.add("media-expanded");
                thumbnail.setAttribute("class", "fit");
                thumbnail.setAttribute("controls", "");
            });
        } else {
            thumbnail.setAttribute("controls", "");
        }
        thumbnail.setAttribute("preload", "none");
        thumbnail.setAttribute("poster", `t/${hit["_source"]["index"]}/${hit["_id"]}`);
        thumbnail.addEventListener("dblclick", function () {
            thumbnail.setAttribute("controls", "");
            if (thumbnail.webkitRequestFullScreen) {
                thumbnail.webkitRequestFullScreen();
            } else {
                thumbnail.requestFullscreen();
            }
        });
        const poster = new Image();
        poster.src = thumbnail.getAttribute('poster');
        poster.addEventListener("load", function () {
            placeholder.remove();
            imgWrapper.appendChild(thumbnail);
        });
    } else if ((hit["_source"].hasOwnProperty("width") && hit["_source"]["width"] > 32 && hit["_source"]["height"] > 32)
        || hit["_source"]["mime"] === "application/pdf"
        || hit["_source"]["mime"] === "application/epub+zip"
        || hit["_source"]["mime"] === "application/x-cbz"
        || hit["_source"]["mime"] === "application/x-cbr"
        || hit["_source"].hasOwnProperty("font_name")
    ) {
        thumbnail = document.createElement("img");
        if (small) {
            thumbnail.setAttribute("class", "fit-sm");
        } else {
            thumbnail.setAttribute("class", "card-img-top fit");
        }
        thumbnail.setAttribute("src", `t/${hit["_source"]["index"]}/${hit["_id"]}`);

        if (!hit["_source"]["parent"] && shouldDisplayRawImage(hit)) {
            imgWrapper.setAttribute("id", "sp" + hit["_id"]);
            imgWrapper.setAttribute("data-src", `t/${hit["_source"]["index"]}/${hit["_id"]}`);
            imgWrapper.setAttribute("href", `f/${hit["_id"]}`);
            imgWrapper.setAttribute("data-caption", hit["_source"]["path"] + "/" + hit["_source"]["name"] + ext(hit));
            imgWrapper.setAttribute("data-group", "p" + Math.floor(docCount / SIZE));
            imgWrapper.classList.add("sp");
        }

        const placeholder = makePlaceholder(hit["_source"]["width"], hit["_source"]["height"], small);
        imgWrapper.appendChild(placeholder);

        thumbnail.addEventListener("error", () => {
            imgWrapper.remove();
        });
        thumbnail.addEventListener("load", () => {
            placeholder.remove();
            imgWrapper.appendChild(thumbnail);
        });
    }

    return thumbnail;
}

function makeInfoButton(hit) {
    const infoButton = document.createElement("span");
    infoButton.appendChild(document.createTextNode("🛈"));
    infoButton.setAttribute("class", "info-icon");
    infoButton.addEventListener("click", infoButtonCb(hit));
    return infoButton;
}

function createDocLine(hit) {

    const mime = hit["_source"]["mime"];
    let mimeCategory = mime ? mime.split("/")[0] : null;
    let tags = getTags(hit, mimeCategory);

    let imgWrapper = document.createElement("div");
    imgWrapper.setAttribute("class", "align-self-start mr-1 wrapper-sm");

    let media = document.createElement("div");
    media.setAttribute("class", "media");

    const line = document.createElement("div");
    line.setAttribute("class", "list-group-item flex-column align-items-start");

    if (hit["_source"].hasOwnProperty("parent")) {
        line.classList.add("sub-document");
        isSubDocument = true;
    }

    const infoButton = makeInfoButton(hit);

    const title = makeTitle(hit);

    let link = document.createElement("a");
    link.setAttribute("href", "f/" + hit["_id"]);
    link.setAttribute("target", "_blank");
    link.style.maxWidth = "calc(100% - 1.2rem)";
    link.appendChild(title);

    const titleDiv = document.createElement("div");

    const titleWrapper = document.createElement("div");
    titleWrapper.style.display = "flex";
    titleWrapper.appendChild(infoButton);
    titleWrapper.appendChild(link);

    titleDiv.appendChild(titleWrapper);

    line.appendChild(media);

    let thumbnail = makeThumbnail(mimeCategory, hit, imgWrapper, true);
    if (thumbnail) {
        media.appendChild(imgWrapper);
        titleDiv.style.maxWidth = "calc(100% - 64px)";
    } else {
        titleDiv.style.maxWidth = "100%";
    }
    media.appendChild(titleDiv);

    // Content
    let contentHl = getContentHighlight(hit);
    if (contentHl !== undefined) {
        const contentDiv = document.createElement("div");
        contentDiv.setAttribute("class", "content-div");
        contentDiv.insertAdjacentHTML('afterbegin', contentHl);
        titleDiv.appendChild(contentDiv);
    }

    let pathLine = document.createElement("div");
    pathLine.setAttribute("class", "path-row");

    let path = document.createElement("div");
    path.setAttribute("class", "path-line");
    path.setAttribute("title", hit["_source"]["path"] + "/");
    path.appendChild(document.createTextNode(hit["_source"]["path"] + "/"));

    let tagContainer = document.createElement("div");
    tagContainer.setAttribute("class", "tag-container");

    for (let i = 0; i < tags.length; i++) {
        tagContainer.appendChild(tags[i]);
    }

    //Size tag
    let sizeTag = document.createElement("small");
    sizeTag.appendChild(document.createTextNode(humanFileSize(hit["_source"]["size"])));
    sizeTag.setAttribute("class", "text-muted");
    tagContainer.appendChild(sizeTag);

    titleDiv.appendChild(pathLine);
    pathLine.appendChild(path);
    pathLine.appendChild(tagContainer);

    return line;
}

function makePreloader() {
    const elem = document.createElement("div");
    elem.setAttribute("class", "progress");
    const bar = document.createElement("div");
    bar.setAttribute("class", "progress-bar progress-bar-striped progress-bar-animated");
    bar.setAttribute("style", "width: 100%");
    elem.appendChild(bar);

    return elem;
}

function makePageIndicator(searchResult) {
    let pageIndicator = document.createElement("div");
    pageIndicator.setAttribute("class", "page-indicator font-weight-light");
    const totalHits = searchResult["aggregations"]["total_count"]["value"];
    pageIndicator.appendChild(document.createTextNode(docCount + " / " + totalHits));
    return pageIndicator;
}


function makeStatsCard(searchResult) {

    let statsCard = document.createElement("div");
    statsCard.setAttribute("class", "card stat");
    let statsCardBody = document.createElement("div");
    statsCardBody.setAttribute("class", "card-body");

    // Stats
    let stat = document.createElement("span");
    const totalHits = searchResult["aggregations"]["total_count"]["value"];
    stat.appendChild(document.createTextNode(totalHits + " results in " + searchResult["took"] + "ms"));

    statsCardBody.appendChild(stat);

    // Display mode
    const resultMode = document.createElement("div");
    resultMode.setAttribute("class", "btn-group btn-group-toggle");
    resultMode.setAttribute("data-toggle", "buttons");
    resultMode.style.cssFloat = "right";

    const listMode = document.createElement("label");
    listMode.setAttribute("class", "btn btn-primary");
    listMode.setAttribute("title", "List mode");
    listMode.innerHTML = '<svg width="20px" height="20px" role="img" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path fill="currentColor" d="M80 368H16a16 16 0 0 0-16 16v64a16 16 0 0 0 16 16h64a16 16 0 0 0 16-16v-64a16 16 0 0 0-16-16zm0-320H16A16 16 0 0 0 0 64v64a16 16 0 0 0 16 16h64a16 16 0 0 0 16-16V64a16 16 0 0 0-16-16zm0 160H16a16 16 0 0 0-16 16v64a16 16 0 0 0 16 16h64a16 16 0 0 0 16-16v-64a16 16 0 0 0-16-16zm416 176H176a16 16 0 0 0-16 16v32a16 16 0 0 0 16 16h320a16 16 0 0 0 16-16v-32a16 16 0 0 0-16-16zm0-320H176a16 16 0 0 0-16 16v32a16 16 0 0 0 16 16h320a16 16 0 0 0 16-16V80a16 16 0 0 0-16-16zm0 160H176a16 16 0 0 0-16 16v32a16 16 0 0 0 16 16h320a16 16 0 0 0 16-16v-32a16 16 0 0 0-16-16z"></path></svg>';

    const gridMode = document.createElement("label");
    gridMode.setAttribute("class", "btn btn-primary");
    gridMode.setAttribute("title", "Grid mode");
    gridMode.innerHTML = '<svg width="20px" height="20px" role="img" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path fill="currentColor" d="M149.333 56v80c0 13.255-10.745 24-24 24H24c-13.255 0-24-10.745-24-24V56c0-13.255 10.745-24 24-24h101.333c13.255 0 24 10.745 24 24zm181.334 240v-80c0-13.255-10.745-24-24-24H205.333c-13.255 0-24 10.745-24 24v80c0 13.255 10.745 24 24 24h101.333c13.256 0 24.001-10.745 24.001-24zm32-240v80c0 13.255 10.745 24 24 24H488c13.255 0 24-10.745 24-24V56c0-13.255-10.745-24-24-24H386.667c-13.255 0-24 10.745-24 24zm-32 80V56c0-13.255-10.745-24-24-24H205.333c-13.255 0-24 10.745-24 24v80c0 13.255 10.745 24 24 24h101.333c13.256 0 24.001-10.745 24.001-24zm-205.334 56H24c-13.255 0-24 10.745-24 24v80c0 13.255 10.745 24 24 24h101.333c13.255 0 24-10.745 24-24v-80c0-13.255-10.745-24-24-24zM0 376v80c0 13.255 10.745 24 24 24h101.333c13.255 0 24-10.745 24-24v-80c0-13.255-10.745-24-24-24H24c-13.255 0-24 10.745-24 24zm386.667-56H488c13.255 0 24-10.745 24-24v-80c0-13.255-10.745-24-24-24H386.667c-13.255 0-24 10.745-24 24v80c0 13.255 10.745 24 24 24zm0 160H488c13.255 0 24-10.745 24-24v-80c0-13.255-10.745-24-24-24H386.667c-13.255 0-24 10.745-24 24v80c0 13.255 10.745 24 24 24zM181.333 376v80c0 13.255 10.745 24 24 24h101.333c13.255 0 24-10.745 24-24v-80c0-13.255-10.745-24-24-24H205.333c-13.255 0-24 10.745-24 24z"></path></svg>';

    resultMode.appendChild(gridMode);
    resultMode.appendChild(listMode);

    if (CONF.options.display === "grid") {
        gridMode.classList.add("active")
    } else {
        listMode.classList.add("active")
    }

    gridMode.addEventListener("click", () => {
        CONF.options.display = "grid";
        CONF.save();
        searchDebounced();
    });
    listMode.addEventListener("click", () => {
        CONF.options.display = "list";
        CONF.save();
        searchDebounced();
    });

    statsCardBody.appendChild(resultMode);

    // Sort mode
    const sortMode = document.createElement("div");
    sortMode.setAttribute("class", "dropdown");
    sortMode.style.cssFloat = "right";
    sortMode.style.marginRight = "10px";

    const sortModeBtn = document.createElement("button");
    sortModeBtn.setAttribute("class", "btn btn-md btn-primary dropdown-toggle");
    sortModeBtn.setAttribute("id", "sortModeBtn");
    sortModeBtn.setAttribute("type", "button");
    sortModeBtn.setAttribute("data-toggle", "dropdown");
    sortModeBtn.setAttribute("aria-haspopup", "true");
    sortModeBtn.setAttribute("aria-expanded", "false");
    sortModeBtn.setAttribute("title", "Sort options");
    sortModeBtn.innerHTML = '<svg aria-hidden="true" width="20px" height="20px" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 512"><path fill="currentColor" d="M41 288h238c21.4 0 32.1 25.9 17 41L177 448c-9.4 9.4-24.6 9.4-33.9 0L24 329c-15.1-15.1-4.4-41 17-41zm255-105L177 64c-9.4-9.4-24.6-9.4-33.9 0L24 183c-15.1 15.1-4.4 41 17 41h238c21.4 0 32.1-25.9 17-41z"></path></svg>';

    const sortModeMenu = document.createElement("div");
    sortModeMenu.setAttribute("class", "dropdown-menu");
    sortModeMenu.setAttribute("aria-labelledby", "sortModeBtn");

    Object.keys(SORT_MODES).forEach(mode => {
        const item = document.createElement("div");
        item.setAttribute("class", "dropdown-item");
        item.appendChild(document.createTextNode(SORT_MODES[mode].text));
        sortModeMenu.appendChild(item);

        item.onclick = function() {
            CONF.options.sort = mode;
            CONF.save();
            searchDebounced();
        };
    });

    sortMode.appendChild(sortModeBtn);
    sortMode.appendChild(sortModeMenu);

    statsCardBody.appendChild(sortMode);

    if (totalHits !== 0) {
        let sizeStat = document.createElement("div");
        sizeStat.appendChild(document.createTextNode(humanFileSize(searchResult["aggregations"]["total_size"]["value"])));
        statsCardBody.appendChild(sizeStat);
    }

    statsCard.appendChild(statsCardBody);

    return statsCard;
}

function makeResultContainer() {
    let resultContainer = document.createElement("div");

    if (CONF.options.display === "grid") {
        resultContainer.setAttribute("class", "bricklayer");
    } else {
        resultContainer.setAttribute("class", "list-group");
    }
    return resultContainer;
}
