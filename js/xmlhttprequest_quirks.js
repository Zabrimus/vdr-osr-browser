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