import axios from "axios";

class Sist2AdminApi {

    constructor() {
        this.baseUrl = window.location.protocol + "//" + window.location.host;
    }

    getJobs() {
        return axios.get(`${this.baseUrl}/api/job`);
    }

    getFrontends() {
        return axios.get(`${this.baseUrl}/api/frontend`);
    }

    getTasks() {
        return axios.get(`${this.baseUrl}/api/task`);
    }

    killTask(taskId) {
        return axios.post(`${this.baseUrl}/api/task/${taskId}/kill`)
    }

    getTaskHistory() {
        return axios.get(`${this.baseUrl}/api/task/history`);
    }

    /**
     * @param {string} name
     */
    getJob(name) {
        return axios.get(`${this.baseUrl}/api/job/${name}`);
    }

    getSearchBackend(name) {
        return axios.get(`${this.baseUrl}/api/search_backend/${name}`);
    }

    updateSearchBackend(name, data) {
        return axios.put(`${this.baseUrl}/api/search_backend/${name}`, data);
    }

    getSearchBackends() {
        return axios.get(`${this.baseUrl}/api/search_backend`);
    }

    deleteBackend(name) {
        return axios.delete(`${this.baseUrl}/api/search_backend/${name}`)
    }

    createBackend(name) {
        return axios.post(`${this.baseUrl}/api/search_backend/${name}`);
    }

    getFrontend(name) {
        return axios.get(`${this.baseUrl}/api/frontend/${name}`);
    }

    /**
     * @param {string} name
     */
    startFrontend(name) {
        return axios.post(`${this.baseUrl}/api/frontend/${name}/start`);
    }

    /**
     * @param {string} name
     */
    stopFrontend(name) {
        return axios.post(`${this.baseUrl}/api/frontend/${name}/stop`);
    }

    /**
     * @param {string} name
     * @param job
     */
    updateJob(name, job) {
        return axios.put(`${this.baseUrl}/api/job/${name}`, job);
    }

    /**
     * @param {string} name
     * @param frontend
     */
    updateFrontend(name, frontend) {
        return axios.put(`${this.baseUrl}/api/frontend/${name}`, frontend);
    }

    /**
     * @param {string} name
     */
    runJob(name) {
        return axios.get(`${this.baseUrl}/api/job/${name}/run`);
    }

    /**
     * @param {string} name
     */
    deleteJob(name) {
        return axios.delete(`${this.baseUrl}/api/job/${name}`);
    }

    /**
     * @param {string} name
     */
    deleteFrontend(name) {
        return axios.delete(`${this.baseUrl}/api/frontend/${name}`);
    }

    /**
     * @param {string} name
     */
    createJob(name) {
        return axios.post(`${this.baseUrl}/api/job/${name}`);
    }

    /**
     * @param {string} name
     */
    createFrontend(name) {
        return axios.post(`${this.baseUrl}/api/frontend/${name}`);
    }

    pingEs(url, insecure) {
        return axios.get(`${this.baseUrl}/api/ping_es`, {params: {url, insecure}});
    }

    getSist2AdminInfo() {
        return axios.get(`${this.baseUrl}/api`);
    }

    getLogsToDelete(jobName, n) {
        return axios.get(`${this.baseUrl}/api/job/${jobName}/logs_to_delete`, {
            params: {n: n}
        });
    }

    deleteTaskLogs(taskId) {
        return axios.post(`${this.baseUrl}/api/task/${taskId}/delete_logs`);
    }

    getUserScripts() {
        return axios.get(`${this.baseUrl}/api/user_script`);
    }

    getUserScript(name) {
        return axios.get(`${this.baseUrl}/api/user_script/${name}`);
    }

    createUserScript(name, template) {
        return axios.post(`${this.baseUrl}/api/user_script/${name}`, null, {
            params: {
                template: template
            }
        });
    }

    updateUserScript(name, data) {
        return axios.put(`${this.baseUrl}/api/user_script/${name}`, data);
    }

    deleteUserScript(name) {
        return axios.delete(`${this.baseUrl}/api/user_script/${name}`);
    }

    testUserScript(name, job) {
        return axios.get(`${this.baseUrl}/api/user_script/${name}/run`, {
            params: {
                job: job
            }
        });
    }
}

export default new Sist2AdminApi()