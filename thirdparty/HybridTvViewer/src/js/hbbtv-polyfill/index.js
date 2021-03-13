import { keyEventInit } from "./keyevent-init.js";
import { hbbtvFn } from "./hbbtv.js";
import { VideoHandler } from "./hbb-video-handler.js";

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

    window.cefStopVideo = function() {
        const videoPlayer = document.getElementById("hbbtv-polyfill-video-player");
        if (typeof videoPlayer !== 'undefined' && videoPlayer !== null) {
            if (!videoPlayer.ended) {
                if (typeof videoPlayer.stop !== 'undefined') {
                    videoPlayer.stop();
                } else {
                    videoPlayer.seek(videoPlayer.duration);
                }
            }
        }
    }

    window.cefVideoSize = function() {
        let broadcastObject;

        // find the existing video container
        let videoContainer_1 = document.getElementById("vidcontainer");
        let videoContainer_2 = document.getElementById("videocontainer");
        let videoContainer_3 = document.getElementsByClassName("video-container");

        let videoContainer = videoContainer_1;
        if (videoContainer === null || typeof videoContainer === 'undefined') {
            videoContainer = videoContainer_2;
        }

        if (videoContainer === null || typeof videoContainer === 'undefined') {
            if (videoContainer_3.length > 0) {
                videoContainer = videoContainer_3[0];
            }
        }

        // calculate size only if a broadcast player exists, otherwise set video to fullscreen
        let isBroadcast = false;
        if (videoContainer !== null && typeof videoContainer !== 'undefined') {
            let containerChild = videoContainer.childNodes;
            for (let i = 0; i < containerChild.length; i++) {
                if (containerChild[i].tagName === 'object' && containerChild[i].getAttribute('type') === 'video/broadcast') {
                    isBroadcast = true;
                    broadcastObject = containerChild[i];
                }
            }
        }

        // try to find the video element
        let video = document.getElementById("video");

        if (video === null || typeof video === 'undefined') {
            let videos = document.getElementsByTagName('video');
            if (videos.length > 0) {
                video = videos[0];
            }
        }

        let videoPlayer = document.getElementById("hbbtv-polyfill-video-player");

        if (video !== null && typeof video !== 'undefined' && videoPlayer !== null && typeof videoPlayer !== 'undefined') {
            // copy size attributes from videocontainer to video
            videoPlayer.style.width = video.style.width;
            videoPlayer.style.height = video.style.height;
            videoPlayer.style.left = video.style.left;
            videoPlayer.style.top = video.style.top;
            videoPlayer.style.position = video.style.position;
        }

        let position;

        // use either the video element or the broadcast object
        if (video !== null && typeof video !== 'undefined') {
            position = video.getClientRects()[0];
        } else if (broadcastObject !== null && typeof broadcastObject !== 'undefined') {
            position = broadcastObject.getClientRects()[0];
        }

        if (!isBroadcast) {
            // no video/broadcast => size is always fullscreen
            signalCef("VIDEO_SIZE: 1280,720,0,0");
            return;
        }

        if (position !== null) {
            signalCef("VIDEO_SIZE: " + Math.ceil(position.width) + "," + Math.ceil(position.height) + "," + Math.ceil(position.x) + "," + Math.ceil(position.y));
        }
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

            // return cefOldXHROpen.call(this, method, url, async, user, password);
            return cefOldXHROpen.call(this, method, url, true, user, password);
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
