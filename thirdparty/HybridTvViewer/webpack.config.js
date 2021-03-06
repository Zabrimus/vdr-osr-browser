const path = require("path");

var options = {
  mode: process.env.NODE_ENV || "development",
  entry: {
    hbbtv_polyfill: path.join(__dirname, "src", "js", "hbbtv-polyfill", "index.js")
  },
  output: {
    path: path.join(__dirname, "build"),
    filename: (chunkData) => {
      if (chunkData.chunk.name === 'hbbtv_polyfill') { return 'hbbtv_polyfill.js'; }
      else { return "[name].bundle.js"; }
    }
  },
  module: {
  },
  devtool: 'source-map',
  plugins: [
  ]
};

module.exports = options;
