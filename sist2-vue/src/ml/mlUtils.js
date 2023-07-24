export async function downloadToBuffer(url, onProgress) {
    const resp = await fetch(url);

    const contentLength = +resp.headers.get("Content-Length");
    const buf = new Uint8ClampedArray(contentLength);
    const reader = resp.body.getReader();
    let cursor = 0;

    if (onProgress) {
        onProgress(0);
    }

    while (true) {
        const {done, value} = await reader.read();

        if (done) {
            break;
        }

        console.log(`Sending ${value.length} bytes into ${buf.length} at offset ${cursor} (${buf.length - cursor} free)`)
        buf.set(value, cursor);
        cursor += value.length;

        if (onProgress) {
            onProgress(cursor / contentLength);
        }
    }

    return buf;
}

export function argMax(array) {
    return array
        .map((x, i) => [x, i])
        .reduce((r, a) => (a[0] > r[0] ? a : r))[1];
}

export function toInt64(array) {
    return new BigInt64Array(array.map(BigInt));
}

export const ORT_WASM_PATHS = {
    "ort-wasm-simd.wasm": "https://cdn.jsdelivr.net/npm/onnxruntime-web@1.15.1/dist/ort-wasm-simd.wasm",
    "ort-wasm.wasm": "https://cdn.jsdelivr.net/npm/onnxruntime-web@1.15.1/dist/ort-wasm.wasm",
    "ort-wasm-simd-threaded.wasm": "https://cdn.jsdelivr.net/npm/onnxruntime-web@1.15.1/dist/ort-wasm-simd-threaded.wasm",
    "ort-wasm-threaded.wasm": "https://cdn.jsdelivr.net/npm/onnxruntime-web@1.15.1/dist/ort-wasm-threaded.wasm",
}