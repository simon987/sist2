#!/usr/bin/env bash

export NODE_OPTIONS=--openssl-legacy-provider

./node_modules/@vue/cli-service/bin/vue-cli-service.js build --watch