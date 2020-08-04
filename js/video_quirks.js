window.start_video_quirk = function() {
  console.log("Start Window Quirk: " + window.location.href);

  // Tatort
  if (window.location.href.search("hbbtv-tatort.daserste.de") >= 0) {
      let appscreen = document.getElementById('appscreen');
      if (appscreen) {
          appscreen.style.visibility = "hidden";
      }

      let playerbar = document.getElementsByClassName('playerbar');
      if (playerbar && playerbar.length > 0) {
          playerbar[0].style.visibility = "visible";
      }
  }

  // ARD replay
  if (window.location.href.search("itv.ard.de/replay/index.php") >= 0) {
      document.body.setAttribute('style', 'background-color: transparent !important');
  }

  // ARD replay dyn
  if (window.location.href.search("itv.ard.de/replay/dyn/index.php") >= 0) {
  }

  // Pro 7
  if (window.location.href.search("hbbtv.redbutton.de/extern/redorbit/hbbtv/apps/mediathek/v3/web/p7de/home/p7de")) {
      document.body.setAttribute('style', 'background-color: transparent !important');

      let screen = document.getElementById('screen');
      if (screen) {
          screen.style.visibility = "hidden";
      }

      let slides = document.getElementsByClassName('slider')[0];
      if (slides) {
          slides.style.display = 'none';
      }
  }
}

window.stop_video_quirk = function() {
  // Tatort
  if (window.location.href.search("hbbtv-tatort.daserste.de") >= 0) {
      let appscreen = document.getElementById('appscreen');
      if (appscreen) {
          appscreen.style.visibility = "inherit";
      }

      let playerbar = document.getElementsByClassName('playerbar');
      if (playerbar && playerbar.length > 0) {
          playerbar[0].style.visibility = null;
      }
  }

  // ARD replay
  if (window.location.href.search("itv.ard.de/replay/index.php") >= 0) {
      document.body.setAttribute('style', 'background-color: black');
  }

  // ARD replay dyn
  if (window.location.href.search("itv.ard.de/replay/dyn/index.php") >= 0) {
  }

  // Pro 7
  if (window.location.href.search("hbbtv.redbutton.de/extern/redorbit/hbbtv/apps/mediathek/v3/web/p7de/home/p7de")) {
      document.body.setAttribute('style', 'background-color: #2c2c2c');
      let screen = document.getElementById('screen');
      if (screen) {
          screen.style.visibility = "inherit";
      }

      let slides = document.getElementsByClassName('slider')[0];
      if (slides) {
        slides.style.display = 'block';
      }
  }

}
