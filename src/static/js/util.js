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
