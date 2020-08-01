window.cefXmlHttpRequestQuirk = function(uri) {
  // Start 1.
  //    Das Erste + ...

  // The URL contains sid as decimal value, but hex value is needed
  let sid = /(getapp\.php\?sid=)(.*?)(&colid=.*?&query=appid)/.exec(uri);

  if (sid) {
      // change sid[2] to hex
      sid[2] = Number(sid[2]).toString(16);
      return sid[1] + sid[2] + sid[3];
  }
  // End 1.


  // return unchanged URL
  return uri;
}

/*
ah.proxy({
  onRequest: (config, handler) => {
    console.log("[ajaxhook] onRequest: " + config.url);
    config.url = window.cefXmlHttpRequestQuirk(config.url);
    handler.next(config);
  },

  onError: (err, handler) => {
    console.log("[ajaxhook] onError: " + err.type);
    handler.next(err)
  },

  onResponse: (response, handler) => {
    // console.log("[ajaxhook] onResponse: " + response.response);
    console.log("[ajaxhook] onResponse");
    handler.next(response)
  }
})
*/

/*
window.xhook.before(function(request) {
  console.log("window.xhook.before called.");
  request.url = window.cefXmlHttpRequestQuirk(request.url);

  // request.headers['Content-type'] = 'awesome/file';
});

window.xhook.after(function(request, response) {
  console.log("window.xhook.after called.");

  // response.headers['Access-Control-Allow-Origin'] = '*';
});
*/