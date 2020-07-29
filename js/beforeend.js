setTimeout(function() {
  console.log('hbbtv-polyfill: The last script called....');

  window.HBBTV_VIDEOHANDLER.initialize();
  window.cefVideoSize();
}, 1000);

