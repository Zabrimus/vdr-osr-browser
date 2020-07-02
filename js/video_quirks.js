window.start_video_quirk = function() {
  // Tatort
  if (window.location.href.search("hbbtv-tatort.daserste.de") >= 0) {
      var appscreen = document.getElementById('appscreen');
      if (appscreen) {
          appscreen.style.visibility = "hidden";
      }

      var playerbar = document.getElementsByClassName('playerbar');
      if (playerbar && playerbar.length > 0) {
          playerbar[0].style.visibility = "visible";
      }
  }
}

window.stop_video_quirk = function() {
  // Tatort
  if (window.location.href.search("hbbtv-tatort.daserste.de") >= 0) {
      var appscreen = document.getElementById('appscreen');
      if (appscreen) {
        appscreen.style.visibility = "inherit";
      }

      var playerbar = document.getElementsByClassName('playerbar');
      if (playerbar && playerbar.length > 0) {
          playerbar[0].style.visibility = null;
      }
  }
}
