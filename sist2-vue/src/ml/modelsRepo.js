import axios from "axios";

class ModelsRepo {
    _repositories;
    data = {};

    async init(repositories) {
        this._repositories = repositories;

        const data = await Promise.all(this._repositories.map(this._loadRepository));

        data.forEach(models => {
            models.forEach(model => {
                this.data[model.name] = model;
            })
        });
    }

    async _loadRepository(repository) {
        const data = (await axios.get(repository)).data;
        data.forEach(model => {
            model["modelUrl"] = new URL(model["modelPath"], repository).href;
            model["vocabUrl"] = new URL(model["vocabPath"], repository).href;
        });
        return data;
    }

    getOptions() {
        return Object.values(this.data).map(model => ({
            text: `${model.name} (${Math.round(model.size / (1024*1024))}MB)`,
            value: model.name
        }));
    }

    getDefaultModel() {
        if (Object.values(this.data).length === 0) {
            return null;
        }
        return Object.values(this.data).find(model => model.default).name;
    }
}

export default new ModelsRepo();