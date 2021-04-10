import { OipfVideoBroadcastMapper } from "./video-broadcast-embedded-object";
import { OipfAVControlMapper } from "./a-v-control-embedded-object";

// append play method to <object> tag
HTMLObjectElement.prototype.play = () => {};

export class VideoHandler {
    constructor() {
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: Construct VideoHandler');

        this.videoObj = undefined;
        this.mutationObserver = undefined;
        this.videoBroadcastEmbeddedObject = undefined;

        this.leftBeforeFullScreen = undefined;
        this.topBeforeFullScreen = undefined;
        this.widthBeforeFullScreen = undefined;
        this.heightBeforeFullScreen = undefined;
    }

    initialize() {
        /* */
        // check at first, if the video object is already injected
        var videoexists = document.getElementById('hbbtv-polyfill-video-player');
        if (typeof videoexists !== 'undefined' && videoexists != null) {
            // FIXME: var x = document.getElementById('hbbtv-polyfill-video-player')
            //  x.getComputedStyle(x).visibility === 'hidden'

            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: VideoHandler already initialized');
            return;
        }
        /* */
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: Init VideoHandler');

        // go through all existing nodes and check if we need to inject the video player emulation
        document.querySelectorAll('*').forEach((node) => {
            this.checkNodeTypeAndInjectVideoMethods(node);
        });

        this.watchAndHandleVideoObjectMutations();
    }

    checkNodeTypeAndInjectVideoMethods(node) {
        let mimeType = node.type;
        if (!node.type) {
            return;  
        } 
        mimeType = mimeType.toLowerCase(); // ensure lower case string comparison

        // check if node is visible
        // dont't do this check on arte. The player is hidden, but we need to insert a video tag
        if (window.location.host !== 'www.arte.tv') {
            let nodeStyle = window.getComputedStyle(node);
            if (nodeStyle.visibility === 'hidden') {
                // node is invisible => no further checks
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: checkNodeTypeAndInjectVideoMethods, node is invisible ...');
                return;
            }
        }

        /* */
        if (document.getElementsByTagName('video').length > 0) {
            // video already injected.

            // on Arte remove the existing video tag, it will be reinserted
            if (window.location.host === 'www.arte.tv') {
                document.getElementsByTagName('video')[0].remove();
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: checkNodeTypeAndInjectVideoMethods, video already injected ...');
                return;
            }
        }
        /* */

        let observeNewVideoElement = false;

        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BROADCAST VIDEO PLAYER ...');
            this.videoBroadcastEmbeddedObject = new OipfVideoBroadcastMapper(node);

            observeNewVideoElement = true;

            // hide all objects
            var objects = document.getElementsByTagName('object');
            for (var i = 0; i < objects.length; i++) {
                objects[i].style.display = 'hidden';
            }
        }
        if (mimeType.lastIndexOf('video/mpeg4', 0) === 0 ||
            mimeType.lastIndexOf('video/mp4', 0) === 0 ||  // h.264 video
            mimeType.lastIndexOf('audio/mp4', 0) === 0 ||  // aac audio
            mimeType.lastIndexOf('audio/mpeg', 0) === 0) { // mp3 audio
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BROADBAND VIDEO PLAYER ...');
            new OipfAVControlMapper(node, false);

            observeNewVideoElement = true;
        }
        // setup mpeg dash player
        if (mimeType.lastIndexOf('application/dash+xml', 0) === 0 || // mpeg-dash
            mimeType.lastIndexOf('video/mpeg', 0) === 0) { // mpeg-ts
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: DASH VIDEO PLAYER ...');
            new OipfAVControlMapper(node, true);

            observeNewVideoElement = true;
        }

        if (observeNewVideoElement) {
            this.watchAndHandleVideoAttributesMutations();

            // add functions
            node.setFullScreen = function(full) {
                if (full) {
                    this.leftBeforeFullScreen = node.style.left;
                    this.topBeforeFullScreen = node.style.top;
                    this.widthBeforeFullScreen = node.style.width;
                    this.heightBeforeFullScreen = node.style.height;

                    node.style.width ='100%';
                    node.style.height ='100%';
                    node.style.top = '0';
                    node.style.left = '0';
                } else {
                    node.style.width = this.widthBeforeFullScreen;
                    node.style.height = this.heightBeforeFullScreen;
                    node.style.top = this.topBeforeFullScreen;
                    node.style.left = this.leftBeforeFullScreen;
                }
            };

            node.bindToCurrentChannel = function() {
            }
        }
    }

    watchAndHandleVideoAttributesMutations() {
        const video = document.getElementById("video");

        if (video === null || typeof video === 'undefined') {
            // no video element, no size
            return;
        }

        const handleVideoAttribute = (mutationList) => {
            mutationList.forEach((mutation) => {
                if (mutation.attributeName === 'style') {
                    window.cefVideoSize();
                }
            });
        };

        this.videoAttributeObserver = new MutationObserver(handleVideoAttribute);
        this.videoAttributeObserver.observe(video, {
            'attributes': true
        });
    }

    watchAndHandleVideoObjectMutations() {

        /**
         * Detect if a new video/broadcast element or other video has been added
         * @param {*} mutation 
         */
        const handleChildAddedRemoved = (mutation) => {
            if (mutation.addedNodes.length > 0) {
                mutation.addedNodes.forEach((node) => {
                    this.checkNodeTypeAndInjectVideoMethods(node);
                });
            } else if (mutation.removedNodes.length > 0) {
                // TODO: handle object removal
            }
        };

        const handleMutation = (mutationList) => {
            mutationList.forEach((mutation) => {
                switch (mutation.type) {
                    case 'childList':
                        handleChildAddedRemoved(mutation);
                        break;
                    case 'attributes':
                        /* An attribute value changed on the element in
                           mutation.target; the attribute name is in
                           mutation.attributeName and its previous value is in
                           mutation.oldValue */
                        break;
                }
            });
        };

        this.mutationObserver = new MutationObserver(handleMutation);
        this.mutationObserver.observe(document.body, {
            'subtree': true,
            'childList': true,
            'attributes': true,
            'characterData': true,
            'attributeFilter': ["type"]
            });
    }
}
