const TerserPlugin = require("terser-webpack-plugin");

module.exports = {
    filenameHashing: false,
    productionSourceMap: false,
    publicPath: "./",
    pages: {
        index: {
            entry: "src/main.js"
        }
    },
    configureWebpack: config => {
        config.optimization.minimizer = [new TerserPlugin({
            terserOptions: {
                compress: {
                    passes: 2,
                    module: true,
                    hoist_funs: true,
                    // https://github.com/microsoft/onnxruntime/issues/16984
                    unused: false,
                },
                mangle: true,
            }
        })]
    }
}