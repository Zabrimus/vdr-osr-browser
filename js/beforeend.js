setTimeout(function() {
  console.log('hbbtv-polyfill: The last script called....');

  var videoexists = document.getElementById('hbbtv-polyfill-video-player');
  if (typeof videoexists === 'undefined' || videoexists === null) {
      console.log('hbbtv-polyfill: The last script: Video does not exists....');
      window.HBBTV_VIDEOHANDLER.initialize();
  }

  window.cefVideoSize();
}, 1000);

