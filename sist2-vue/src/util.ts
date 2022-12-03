import {EsHit} from "@/Sist2Api";

export function ext(hit: EsHit) {
    return srcExt(hit._source)
}

export function srcExt(src) {
    return Object.prototype.hasOwnProperty.call(src, "extension")
        && src["extension"] !== "" ? "." + src["extension"] : "";
}

export function strUnescape(str: string): string {
    let result = "";

    for (let i = 0; i < str.length; i++) {
        const c = str[i];
        const next = str[i + 1];

        if (c === "]") {
            if (next === "]") {
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

const thresh = 1000;
const units = ["k", "M", "G", "T", "P", "E", "Z", "Y"];

export function humanFileSize(bytes: number): string {
    if (bytes === 0) {
        return "0 B"
    }

    if (Math.abs(bytes) < thresh) {
        return bytes + ' B';
    }
    let u = -1;
    do {
        bytes /= thresh;
        ++u;
    } while (Math.abs(bytes) >= thresh && u < units.length - 1);

    return bytes.toFixed(1) + units[u];
}

export function humanTime(sec_num: number): string {
    sec_num = Math.floor(sec_num);
    const hours = Math.floor(sec_num / 3600);
    const minutes = Math.floor((sec_num - (hours * 3600)) / 60);
    const seconds = sec_num - (hours * 3600) - (minutes * 60);

    return `${hours < 10 ? "0" : ""}${hours}:${minutes < 10 ? "0" : ""}${minutes}:${seconds < 10 ? "0" : ""}${seconds}`;
}

export function humanDate(numMilis: number): string {
    const date = (new Date(numMilis * 1000));
    return date.getUTCFullYear() + "-" + ("0" + (date.getUTCMonth() + 1)).slice(-2) + "-" + ("0" + date.getUTCDate()).slice(-2)

}

export function lum(c: string) {
    c = c.substring(1);
    const rgb = parseInt(c, 16);
    const r = (rgb >> 16) & 0xff;
    const g = (rgb >> 8) & 0xff;
    const b = (rgb >> 0) & 0xff;

    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}


export function getSelectedTreeNodes(tree: any) {
    const selectedNodes = new Set();

    const selected = tree.selected();

    for (let i = 0; i < selected.length; i++) {

        if (selected[i].id === "any") {
            return ["any"]
        }

        //Only get children
        if (selected[i].text.indexOf("(") !== -1) {
            if (selected[i].values) {
                selectedNodes.add(selected[i].values.slice(-1)[0]);
            } else {
                selectedNodes.add(selected[i].id);
            }
        }
    }

    return Array.from(selectedNodes);
}

export function getTreeNodeAttributes(tree: any) {
    const nodes = tree.selectable();
    const attributes = {};

    for (let i = 0; i < nodes.length; i++) {

        let id = null;

        if (nodes[i].text.indexOf("(") !== -1 && nodes[i].values) {
            id = nodes[i].values.slice(-1)[0];
        } else {
            id = nodes[i].id
        }

        attributes[id] = {
            checked: nodes[i].itree.state.checked,
            collapsed: nodes[i].itree.state.collapsed,
        }
    }

    return attributes;
}


export function serializeMimes(mimes: string[]): string | undefined {
    if (mimes.length == 0) {
        return undefined;
    }
    return mimes.map(mime => compressMime(mime)).join("");
}

export function deserializeMimes(mimeString: string): string[] {
    return mimeString
        .replaceAll(/([IVATUF])/g, "$$$&")
        .split("$")
        .map(mime => decompressMime(mime))
        .slice(1) // Ignore the first (empty) token
}

export function compressMime(mime: string): string {
    return mime
        .replace("image/", "I")
        .replace("video/", "V")
        .replace("application/", "A")
        .replace("text/", "T")
        .replace("audio/", "U")
        .replace("font/", "F")
        .replace("+", ",")
        .replace("x-", "X")
}

export function decompressMime(mime: string): string {
    return mime
        .replace("I", "image/")
        .replace("V", "video/")
        .replace("A", "application/")
        .replace("T", "text/")
        .replace("U", "audio/")
        .replace("F", "font/")
        .replace(",", "+")
        .replace("X", "x-")
}

export function randomSeed(): number {
    return Math.round(Math.random() * 100000);
}