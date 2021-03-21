import { keyEventInit } from "./keyevent-init.js";
import { hbbtvFn } from "./hbbtv.js";
import { VideoHandler } from "./hbb-video-handler.js";
import {OipfAVControlMapper} from "./a-v-control-embedded-object";

function init() {
    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill: load");

    // convenience method: Javascript to VDR wrapper method, one-way
    window.signalCef = function(command) {
        window.cefQuery({
            request: command,
            persistent: false,
            onSuccess: function(response) {},
            onFailure: function(error_code, error_message) {}
        });
    };

    window.signalVdr = function(command) {
        signalCef('VDR:' + command);
    };

    window.cefChangeUrl = function(uri) {
        signalCef('CHANGE_URL: ' + uri);
    }

    // FIXME: Die Prozedur funktioniert so nicht. Da muss eine andere Lösung her.
    //  Das Ziel: Wenn der Player im VDR detached wird, dann muss das Video gestoppt werden.
    window.cefStopVideo = function() {
        const videoPlayer = document.getElementById("hbbtv-polyfill-video-player");
        if (typeof videoPlayer !== 'undefined' && videoPlayer !== null) {
            if (!videoPlayer.ended) {
                // videoPlayer.currentTime = videoPlayer.duration();
            }
        }
    }

    window.cefVideoSize = function() {
        // search for a video/broadcast object
        let vbo = document.querySelector("[type='video/broadcast']");
        if (vbo) {
            let style = window.getComputedStyle(vbo);
            if (style.visibility !== 'hidden') {
                // search for video with id hbbtv-polyfill-broadcast-player
                let broadcast = document.getElementById('hbbtv-polyfill-broadcast-player');
                if (broadcast !== null && typeof broadcast !== 'undefined') {
                    let br = broadcast.getBoundingClientRect();
                    signalCef("VIDEO_SIZE: " + parseInt(br.width) + "," + parseInt(br.height) + "," + parseInt(br.left) + "," + parseInt(br.top));
                    return;
                }
            }
        }

        // search object element with video type which is visible
        let objects = document.getElementsByTagName('object');
        if (objects.length > 0) {
            // test if objects is visible
            let i;
            for (i = 0; i < objects.length; i++) {
                let objectStyle = window.getComputedStyle(objects[i]);
                if (objectStyle.visibility !== 'hidden') {
                    // check if type is a video type
                    if ((objects[i].type === 'video/mpeg4') ||
                        (objects[i].type === 'video/mp4') ||
                        (objects[i].type === 'audio/mp4') ||
                        (objects[i].type === 'audio/mpeg') ||
                        (objects[i].type === 'video/mpeg')) {

                        let vo = objects[i].nextSibling;
                        if (vo.nodeName === 'video') {
                            // copy attributes
                            vo.style.width = objects[i].style.width;
                            vo.style.height = objects[i].style.height;
                            vo.style.left = objects[i].style.left;
                            vo.style.top = objects[i].style.top;
                            vo.style.position = objects[i].style.position;
                        }
                    }
                }
            }
        }

        // set to fullscreen
        signalCef("VIDEO_SIZE: 1280,720,0,0");
    }

    // intercept XMLHttpRequest
    /* Enable/Disable if ajax module shall be used */
    if (location.href.search("hbbtv.swisstxt.ch") === -1) {
        let cefOldXHROpen = window.XMLHttpRequest.prototype.open;
        window.XMLHttpRequest.prototype.open = function (method, url, async, user, password) {
            // do something with the method, url and etc.
            window._HBBTV_DEBUG_ && console.log("XMLHttpRequest.method: " + method);
            window._HBBTV_DEBUG_ && console.log("XMLHttpRequest.async: " + async);
            window._HBBTV_DEBUG_ && console.log("XMLHttpRequest.url: " + url);

            url = window.cefXmlHttpRequestQuirk(url);

            window._HBBTV_DEBUG_ && console.log("XMLHttpRequest.newurl: " + url);

            this.addEventListener('load', function () {
                // do something with the response text

                // disabled: Too much information
                // window._HBBTV_DEBUG_ && console.log('XMLHttpRequest: url ' + url + ', load: ' + this.responseText);
            });

            /*
            this.addEventListener('readystatechange', function (event) {
                if (this.readyState === 4) {
                    var res = event.target.responseText;
                    Object.defineProperty(this, 'response', {writable: true});
                    Object.defineProperty(this, 'responseText', {writable: true});

                    this.response = this.responseText = res.split("&#034;").join("\\\"");

                    console.log("Res = " + this.responseText);
                }
            });
            */

            return cefOldXHROpen.call(this, method, url, async, user, password);
            // return cefOldXHROpen.call(this, method, url, true, user, password);
        };
    }
    /* Enable/Disable if ajax module shall be used */

    // global helper namespace to simplify testing
    window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {
    };
    window.HBBTV_POLYFILL_NS = {
        ...window.HBBTV_POLYFILL_NS, ...{
            keysetSetValueCount: 0,
            streamEventListeners: [],
        }
    };
    window.HBBTV_POLYFILL_NS.currentChannel = window.HBBTV_POLYFILL_NS.currentChannel || {
        'TYPE_TV': 12,
        'channelType': 12,
        'sid': 1,
        'onid': 1,
        'tsid': 1,
        'name': 'test',
        'ccid': 'ccid:dvbt.0',
        'dsd': ''
    };

    keyEventInit();

    hbbtvFn();

    window.HBBTV_VIDEOHANDLER = new VideoHandler();
    window.HBBTV_VIDEOHANDLER.initialize();

    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill: loaded");
}

if (!document.body) {
    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill: add as event listener, DOMContentLoaded");
    document.addEventListener("DOMContentLoaded", init);
} else {
    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill: call init");
    init();
}
