<template>
    <b-card>
        <b-card-body>

            <h4 class="mb-3">{{ taskId }} {{ $t("logs") }}</h4>

            <div v-if="$store.state.sist2AdminInfo">
                {{ $t("logFile") }}
                <code>{{ $store.state.sist2AdminInfo.logs_folder }}/sist2-{{ taskId }}.log</code>
                <br/>
                <br/>
            </div>

            <b-row>
                <b-col>
                    <span>{{ $t("logLevel") }}</span>
                    <b-select :options="levels.slice(0, -1)" v-model="logLevel" @input="connect()"></b-select>
                </b-col>
                <b-col>
                    <span>{{ $t("logMode") }}</span>
                    <b-select :options="modeOptions" v-model="mode" @input="connect()"></b-select>
                </b-col>
            </b-row>

            <div id="log-tail-output" class="mt-3 ml-1"></div>

        </b-card-body>
    </b-card>
</template>

<script>

export default {
    name: "Tail",
    data() {
        return {
            logLevel: "DEBUG",
            levels: ["DEBUG", "INFO", "WARNING", "ERROR", "ADMIN", "FATAL"],
            socket: null,
            mode: "follow",
            modeOptions: [
                {
                    "text": this.$t('follow'),
                    "value": "follow"
                },
                {
                    "text": this.$t('wholeFile'),
                    "value": "wholeFile"
                }
            ]
        }
    },
    computed: {
        taskId: function () {
            return this.$route.params.taskId;
        }
    },
    methods: {
        connect() {
            let lineCount = 0;
            const outputElem = document.getElementById("log-tail-output")
            outputElem.replaceChildren();
            if (this.socket !== null) {
                this.socket.close();
            }

            const n = this.mode === "follow" ? 32 : 9999999999;
            if (window.location.protocol === "https:") {
                this.socket = new WebSocket(`wss://${window.location.host}/log/${this.taskId}?n=${n}`);
            } else {
                this.socket = new WebSocket(`ws://${window.location.host}/log/${this.taskId}?n=${n}`);
            }
            this.socket.onopen = () => {
                this.socket.send("Hello from client");
            }

            this.socket.onmessage = e => {
                let message;
                try {
                    message = JSON.parse(e.data);
                } catch {
                    console.error(e.data)
                    return;
                }

                if ("ping" in message) {
                    return;
                }

                if (message.level === undefined) {

                    if ("stderr" in message) {
                        message.level = "ERROR";
                        message.message = message["stderr"];
                    } else if ("stdout" in message) {
                        message.level = "INFO";
                        message.message = message["stdout"];
                    } else {
                        message.level = "ADMIN";
                        message.message = message["sist2-admin"];
                    }
                    message.datetime = ""
                    message.filepath = ""
                }

                if (this.levels.indexOf(message.level) < this.levels.indexOf(this.logLevel)) {
                    return;
                }

                const logLine = `${message.datetime} [${message.level} ${message.filepath}] ${message.message}`;

                const span = document.createElement("span");
                span.setAttribute("class", message.level);
                span.appendChild(document.createTextNode(logLine));

                outputElem.appendChild(span);
                lineCount += 1;

                if (this.mode === "follow" && lineCount >= n) {
                    outputElem.firstChild.remove();
                }
            }
        }
    },
    mounted() {
        this.connect()
    }
}

</script>

<style>
#log-tail-output span {
    display: block;
}

span.DEBUG {
    color: #9E9E9E;
}

span.WARNING {
    color: #FFB300;
}

span.INFO {
    color: #039BE5;
}

span.ERROR {
    color: #F4511E;
}

span.FATAL {
    color: #F4511E;
}

span.ADMIN {
    color: #ee05ff;
}


#log-tail-output {
    font-size: 13px;
    font-family: monospace;

    padding: 6px;
    background-color: #f5f5f5;
    border: 1px solid #ccc;
    border-radius: 4px;
    margin: 3px;
    white-space: pre;
    color: #000;
    overflow-y: hidden;
}
</style>
