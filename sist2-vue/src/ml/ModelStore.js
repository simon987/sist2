class ModelStore {

    _ok;
    _db;
    _resolve;
    _loadingPromise;

    constructor() {
        const request = window.indexedDB.open("ModelStore", 1);

        request.onerror = () => {
            this._ok = false;
        }

        request.onupgradeneeded = event => {
            const db = event.target.result;
            db.createObjectStore("models");
        }

        request.onsuccess = () => {
            this._ok = true;
            this._db = request.result;

            this._resolve();
        }

        this._loadingPromise = new Promise(resolve => this._resolve = resolve);
    }

    async get(key) {
        await this._loadingPromise;

        const req = this._db.transaction(["models"], "readwrite")
            .objectStore("models")
            .get(key);

        return new Promise(resolve => {
            req.onsuccess = event => {
                resolve(event.target.result);
            };
            req.onerror = event => {
                console.log("ERROR:");
                console.log(event);
                resolve(null);
            };
        });
    }

    async set(key, val) {
        await this._loadingPromise;

        const req = this._db.transaction(["models"], "readwrite")
            .objectStore("models")
            .put(val, key);

        return new Promise(resolve => {
            req.onsuccess = () => {
                resolve(true);
            };
            req.onerror = () => {
                resolve(false);
            };
        });
    }
}

export default new ModelStore();