import Vue from "vue";
import Vuex from "vuex";

Vue.use(Vuex);

function saveBrowserSettings(state) {
    const settings = {
        jobDesktopNotificationMap: state.jobDesktopNotificationMap
    };
    localStorage.setItem("sist2-admin-settings", JSON.stringify(settings));

    console.log("SAVED");
    console.log(settings);
}

export default new Vuex.Store({
    state: {
        sist2AdminInfo: null,
        jobDesktopNotificationMap: {}
    },
    mutations: {
        setSist2AdminInfo: (state, payload) => state.sist2AdminInfo = payload,
        setJobDesktopNotificationMap: (state, payload) => state.jobDesktopNotificationMap = payload,
    },
    actions: {
        notify: async ({state}, notification) => {

            if (!state.jobDesktopNotificationMap[notification.job]) {
                console.log("pass");
                return;
            }

            new Notification(notification.messageString.replace("$JOB$", notification.job));
        },
        setJobDesktopNotification: async ({state}, {job, enabled}) => {

            if (enabled === true) {
                const permission = await Notification.requestPermission()

                if (permission !== "granted") {
                    return false;
                }
            }

            state.jobDesktopNotificationMap[job] = enabled;
            saveBrowserSettings(state);

            return true;
        },
        loadBrowserSettings({commit}) {
            const settingString = localStorage.getItem("sist2-admin-settings");

            if (!settingString) {
                return;
            }

            const settings = JSON.parse(settingString);

            commit("setJobDesktopNotificationMap", settings["jobDesktopNotificationMap"]);
        }
    },
    modules: {}
})
