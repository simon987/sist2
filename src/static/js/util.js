/**
 * https://stackoverflow.com/questions/10420352
 */
function humanFileSize(bytes) {
    if (bytes === 0) {
        return "0 B"
    }

    let thresh = 1000;
    if (Math.abs(bytes) < thresh) {
        return bytes + ' B';
    }
    let units = ['k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'];
    let u = -1;
    do {
        bytes /= thresh;
        ++u;
    } while (Math.abs(bytes) >= thresh && u < units.length - 1);

    return bytes.toFixed(1) + units[u];
}

/**
 * https://stackoverflow.com/questions/6312993
 */
function humanTime(sec_num) {
    sec_num = Math.floor(sec_num);
    let hours = Math.floor(sec_num / 3600);
    let minutes = Math.floor((sec_num - (hours * 3600)) / 60);
    let seconds = sec_num - (hours * 3600) - (minutes * 60);

    if (hours < 10) {
        hours = "0" + hours;
    }
    if (minutes < 10) {
        minutes = "0" + minutes;
    }
    if (seconds < 10) {
        seconds = "0" + seconds;
    }
    return hours + ":" + minutes + ":" + seconds;
}

function debounce(func, wait) {
    let timeout;
    return function () {
        let context = this, args = arguments;
        let later = function () {
            timeout = null;
            func.apply(context, args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
        func.apply(context, args);
    };
}

function lum(c) {
    c = c.substring(1);
    let rgb = parseInt(c, 16);
    let r = (rgb >> 16) & 0xff;
    let g = (rgb >> 8) & 0xff;
    let b = (rgb >> 0) & 0xff;

    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

function strUnescape(str) {
    let result = "";

    for (let i = 0; i < str.length; i++) {
        const c = str[i];
        const next = str[i + 1];

        if (c === ']') {
            if (next === ']') {
                result += c;
                i += 1;
            } else {
                result += String.fromCharCode(parseInt(str.slice(i, i + 2), 16));
                i += 2;
            }
        } else {
            result += c;
        }
    }
    return result;
}

const CONF = new Settings();

const _defaults = {
    display: "grid",
    fuzzy: true,
    highlight: true,
    sort: "score",
    searchInPath: false,
    treemapType: "cascaded",
    treemapTiling: "squarify",
    treemapGroupingDepth: 3,
    treemapColor: "PuBuGn",
    treemapSize: "large",
    suggestPath: true,
    fragmentSize: 100,
    columns: 5,
    queryMode: "simple"
};

function loadSettings() {
    CONF.load();

    $("#settingDisplay").val(CONF.options.display);
    $("#settingFuzzy").prop("checked", CONF.options.fuzzy);
    $("#settingHighlight").prop("checked", CONF.options.highlight);
    $("#settingSearchInPath").prop("checked", CONF.options.searchInPath);
    $("#settingTreemapTiling").val(CONF.options.treemapTiling);
    $("#settingTreemapGroupingDepth").val(CONF.options.treemapGroupingDepth);
    $("#settingTreemapColor").val(CONF.options.treemapColor);
    $("#settingTreemapSize").val(CONF.options.treemapSize);
    $("#settingTreemapType").val(CONF.options.treemapType);
    $("#settingSuggestPath").prop("checked", CONF.options.suggestPath);
    $("#settingFragmentSize").val(CONF.options.fragmentSize);
    $("#settingColumns").val(CONF.options.columns);
    $("#settingQueryMode").val(CONF.options.queryMode);
}

function Settings() {
    this.options = {};

    this._onUpdate = function () {
        $("#fuzzyToggle").prop("checked", this.options.fuzzy);
        $("#searchBar").attr("placeholder", this.options.queryMode === "simple" ? "Search" : "Advanced search");
        updateColumnStyle();
    };

    this.load = function () {
        const raw = window.localStorage.getItem("options");
        if (raw === null) {
            this.options = _defaults;
        } else {
            const j = JSON.parse(raw);
            if (!j || Object.keys(_defaults).some(k => !j.hasOwnProperty(k))) {
                this.options = _defaults;
            } else {
                this.options = j;
            }
        }

        this._onUpdate();
    };

    this.save = function () {
        window.localStorage.setItem("options", JSON.stringify(this.options));
        this._onUpdate();
    }
}

function updateSettings() {
    CONF.options.display = $("#settingDisplay").val();
    CONF.options.fuzzy = $("#settingFuzzy").prop("checked");
    CONF.options.highlight = $("#settingHighlight").prop("checked");
    CONF.options.searchInPath = $("#settingSearchInPath").prop("checked");
    CONF.options.treemapTiling = $("#settingTreemapTiling").val();
    CONF.options.treemapGroupingDepth = $("#settingTreemapGroupingDepth").val();
    CONF.options.treemapColor = $("#settingTreemapColor").val();
    CONF.options.treemapSize = $("#settingTreemapSize").val();
    CONF.options.treemapType = $("#settingTreemapType").val();
    CONF.options.suggestPath = $("#settingSuggestPath").prop("checked");
    CONF.options.fragmentSize = $("#settingFragmentSize").val();
    CONF.options.columns = $("#settingColumns").val();
    CONF.options.queryMode = $("#settingQueryMode").val();
    CONF.save();

    if (typeof searchDebounced !== "undefined") {
        searchDebounced();
    }

    if (typeof updateStats !== "undefined") {
        updateStats();
    }

    $.toast({
        heading: "Settings updated",
        text: "Settings saved to browser storage",
        stack: 3,
        bgColor: "#00a4bc",
        textColor: "#fff",
        position: 'bottom-right',
        hideAfter: 3000,
        loaderBg: "#08c7e8",
    });
}

jQuery["jsonPost"] = function (url, data, showError = true) {
    return jQuery.ajax({
        url: url,
        type: "post",
        data: JSON.stringify(data),
        contentType: "application/json"
    }).fail(err => {
        if (showError) {
            showEsError();
        }
        console.log(err);
    });
};

function toggleTheme() {
    if (!document.cookie.includes("sist")) {
        document.cookie = "sist=dark;SameSite=Strict";
    } else {
        document.cookie = "sist=; Max-Age=-99999999;";
    }
    window.location.reload();
}

function updateColumnStyle() {
    const style = document.getElementById("style");
    if (style) {
        style.innerHTML =
            `
@media screen and (min-width: 1500px) {
    .container {
            max-width: 1440px;
        }

    .bricklayer-column-sizer {
            width: ${100 / CONF.options.columns}% !important;
        }

    .bricklayer-column {
            max-width: ${100 / CONF.options.columns}%;
        }
    }
}
        `
    }
}