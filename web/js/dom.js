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

/**
 *
 * @param hit
 * @returns {Element}
 */
function createDocCard(hit) {
    let docCard = document.createElement("div");
    docCard.setAttribute("class", "card shadow-sm");

    let docCardBody = document.createElement("div");
    docCardBody.setAttribute("class", "card-body document");

    let link = document.createElement("a");
    link.setAttribute("href", "f/" + hit["_id"]);
    link.setAttribute("target", "_blank");

    //Title
    let title = document.createElement("p");
    title.setAttribute("class", "file-title");
    let extension = hit["_source"].hasOwnProperty("extension") && hit["_source"]["extension"] !== "" ? "." + hit["_source"]["extension"] : "";

    applyNameToTitle(hit, title, extension);

    title.setAttribute("title", hit["_source"]["path"] + "/" + hit["_source"]["name"] + extension);
    docCard.appendChild(title);

    let tagContainer = document.createElement("div");
    tagContainer.setAttribute("class", "card-text");

    if (hit["_source"].hasOwnProperty("mime") && hit["_source"]["mime"] !== null) {

        let tags = [];
        let thumbnail = null;
        let thumbnailOverlay = null;
        let imgWrapper = document.createElement("div");
        imgWrapper.setAttribute("style", "position: relative");

        let mimeCategory = hit["_source"]["mime"].split("/")[0];

        //Thumbnail
        if (mimeCategory === "video" && shouldPlayVideo(hit)) {
            thumbnail = document.createElement("video");
            addVidSrc("f/" + hit["_id"], hit["_source"]["mime"], thumbnail);

            thumbnail.setAttribute("class", "fit");
            thumbnail.setAttribute("loop", "");
            thumbnail.setAttribute("controls", "");
            thumbnail.setAttribute("preload", "none");
            thumbnail.setAttribute("poster", `t/${hit["_source"]["index"]}/${hit["_id"]}`);
            thumbnail.addEventListener("dblclick", function () {
                thumbnail.webkitRequestFullScreen();
            });
        } else if ((hit["_source"].hasOwnProperty("width") && hit["_source"]["width"] > 20 && hit["_source"]["height"] > 20)
            || hit["_source"]["mime"] === "application/pdf"
            || hit["_source"].hasOwnProperty("font_name")
        ) {
            thumbnail = document.createElement("img");
            thumbnail.setAttribute("class", "card-img-top fit");
            thumbnail.setAttribute("src", `t/${hit["_source"]["index"]}/${hit["_id"]}`);
        }

        //Thumbnail overlay
        switch (mimeCategory) {

            case "image":
                thumbnailOverlay = document.createElement("div");
                thumbnailOverlay.setAttribute("class", "card-img-overlay");

                //Resolution
                let resolutionBadge = document.createElement("span");
                resolutionBadge.setAttribute("class", "badge badge-resolution");
                if (hit["_source"].hasOwnProperty("width")) {
                    resolutionBadge.appendChild(document.createTextNode(hit["_source"]["width"] + "x" + hit["_source"]["height"]));
                }
                thumbnailOverlay.appendChild(resolutionBadge);

                // Hover
                if (thumbnail && hit["_source"]["videoc"] === "gif") {
                    gifOver(thumbnail, hit);
                }
                break;

            case "video":
                //Duration
                if (hit["_source"].hasOwnProperty("duration")) {
                    thumbnailOverlay = document.createElement("div");
                    thumbnailOverlay.setAttribute("class", "card-img-overlay");
                    let durationBadge = document.createElement("span");
                    durationBadge.setAttribute("class", "badge badge-resolution");
                    durationBadge.appendChild(document.createTextNode(humanTime(hit["_source"]["duration"])));
                    thumbnailOverlay.appendChild(durationBadge);
                }
        }

        //Tags
        switch (mimeCategory) {
            case "video":
            case "image":
                if (hit["_source"].hasOwnProperty("videoc")) {
                    let formatTag = document.createElement("span");
                    formatTag.setAttribute("class", "badge badge-pill badge-video");
                    formatTag.appendChild(document.createTextNode(hit["_source"]["videoc"].replace(" ", "")));
                    tags.push(formatTag);
                }
                break;
            case "audio": {
                if (hit["_source"].hasOwnProperty("audioc")) {
                    let formatTag = document.createElement("span");
                    formatTag.setAttribute("class", "badge badge-pill badge-audio");
                    formatTag.appendChild(document.createTextNode(hit["_source"]["audioc"]));
                    tags.push(formatTag);
                }
            }
                break;
        }

        //Content
        let contentHl = getContentHighlight(hit);
        if (contentHl !== undefined) {
            let contentDiv = document.createElement("div");
            contentDiv.setAttribute("class", "content-div bg-light");
            contentDiv.insertAdjacentHTML('afterbegin', contentHl);
            docCard.appendChild(contentDiv);
        }

        if (thumbnail !== null) {
            imgWrapper.appendChild(thumbnail);
            docCard.appendChild(imgWrapper);
        }

        //Audio
        if (mimeCategory === "audio" && hit["_source"].hasOwnProperty("audioc")) {

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

        for (let i = 0; i < tags.length; i++) {
            tagContainer.appendChild(tags[i]);
        }
    }

    //Size tag
    let sizeTag = document.createElement("small");
    sizeTag.appendChild(document.createTextNode(humanFileSize(hit["_source"]["size"])));
    sizeTag.setAttribute("class", "text-muted");
    tagContainer.appendChild(sizeTag);

    docCardBody.appendChild(link);
    docCard.appendChild(docCardBody);

    link.appendChild(title);
    docCardBody.appendChild(tagContainer);

    return docCard;
}

function makePageIndicator(searchResult) {
    let pageIndicator = document.createElement("div");
    pageIndicator.setAttribute("class", "page-indicator shadow-sm bg-light font-weight-light");
    const totalHits = searchResult["hits"]["total"].hasOwnProperty("value")
        ? searchResult["hits"]["total"]["value"] : searchResult["hits"]["total"];
    pageIndicator.appendChild(document.createTextNode(docCount + " / " + totalHits));
    return pageIndicator;
}


function makeStatsCard(searchResult) {

    let statsCard = document.createElement("div");
    statsCard.setAttribute("class", "card");
    let statsCardBody = document.createElement("div");
    statsCardBody.setAttribute("class", "card-body");

    let stat = document.createElement("p");
    const totalHits = searchResult["hits"]["total"].hasOwnProperty("value")
        ? searchResult["hits"]["total"]["value"] : searchResult["hits"]["total"];
    stat.appendChild(document.createTextNode(totalHits + " results in " + searchResult["took"] + "ms"));
    statsCardBody.appendChild(stat);

    if (totalHits !== 0) {
        let sizeStat = document.createElement("span");
        sizeStat.appendChild(document.createTextNode(humanFileSize(searchResult["aggregations"]["total_size"]["value"])));
        statsCardBody.appendChild(sizeStat);
    }

    statsCard.appendChild(statsCardBody);

    return statsCard;
}

function makeResultContainer() {
    let resultContainer = document.createElement("div");
    resultContainer.setAttribute("class", "card-columns");

    return resultContainer;
}
