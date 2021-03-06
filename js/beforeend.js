setTimeout(function() {
    window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: The last script called....');

    // hide all objects
    var objects = document.getElementsByTagName('object');
    for (var i = 0; i < objects.length; i++) {
        objects[i].style.display = 'hidden';
    }

    var videoexists = document.getElementById('hbbtv-polyfill-video-player');
    if (typeof videoexists === 'undefined' || videoexists === null) {
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: The last script: Video does not exists....');
        window.HBBTV_VIDEOHANDLER.initialize();
    }

    window.cefVideoSize();
}, 1000);

