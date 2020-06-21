/**
 * OIPF 
 * Release 1 Specification
 * Volume 5 - Declarative Application Environment 
 * 7.14.1 The CEA 2014 A/V Control embedded object
 */

// import dashjs file --> we want it sync so don't pull from cdn ->  downside is we need a copy in repo TODO: fetch latest in build process
import { MediaPlayer } from "dashjs";

const PLAY_STATES = {
    stopped: 0,
    playing: 1,
    paused: 2,
    connecting: 3,
    buffering: 4,
    finished: 5,
    error: 6,
};

function scanChildrenForPlayer(items) {
    Array.from(items).forEach(function (item) {
        if ('VIDEO' === item.tagName) {
            player = item;
        } else if (item.children) {
            scanChildrenForPlayer(item.children);
        }
    });
}

export class OipfAVControlMapper {

    /**
     * object tag with media type
     * @param {*} node 
     */
    constructor(node, isDashVideo) {
        this.avControlObject = node;
        this.isDashVideo = isDashVideo;

        const originalDataAttribute = this.avControlObject.data;
        // let video playback fail. Modern browsers don't support any handling of media events and methods on <object type="" data=""> tags
        // setting data to unknown url will cause a GET 40x.. found no better solution yet to disable playback
        this.avControlObject.data = "client://movie/fail";
        this.videoElement = document.createElement('video'); // setup artificial video tag
        this.videoElement.setAttribute('id', 'hbbtv-polyfill-video-player');
        this.videoElement.setAttribute('autoplay', ''); // setting src will start the video and send an event

        // the opaque video only has size 16x16. To prevent image scaling set the container size to these values
        this.videoElement.setAttribute('style', 'top:0px; left:0px; width:16px; height:16px;');
        // this.videoElement.setAttribute('style', 'top:0px; left:0px; width:100%; height:100%;');

        // interval to simulate rewind functionality
        this.rewindInterval;

        this.dashPlayer;
        // user dash.js to init player
        if (this.isDashVideo) {
            this.dashPlayer = MediaPlayer().create();
            this.dashPlayer.initialize(this.videoElement, originalDataAttribute, true);
        } else {
            var target = document.getElementById("video");

            if (!target) {
                target = document.getElementById("videocontainer");
            }

            if (target) {
                var width = target.getAttribute("width");
                var height = target.getAttribute("height");

                if (width && height) {
                    var position = target.getBoundingClientRect();
                    var x = position.x;
                    var y = position.y;


                    signalCef("VIDEO_SIZE: " + width + "," + height + "," + x + "," + y);
                }
            }

            signalCef("VIDEO_URL:" + originalDataAttribute);

            // this.videoElement.src = originalDataAttribute;
            // copy object data url to html5 video tag src attribute ...
            this.videoElement.src = "client://movie/transparent.webm";
        }

        this.mapAvControlToHtml5Video();
        this.watchAvControlObjectMutations(this.avControlObject);
        this.watchAvVideoElementAttributeMutations(this.videoElement);
        this.registerEmbeddedVideoPlayerEvents(this.avControlObject, this.videoElement);
        this.avControlObject.appendChild(this.videoElement);
        this.avControlObject.playTime = this.videoElement.duration * 1000;
        // ANSI CTA-2014-B - 5.7.1.f - 5
        this.avControlObject.error = -1;
    }

    mapAvControlToHtml5Video() {
        clearInterval(this.rewindInterval);
        // ANSI CTA-2014-B
        // 5.7.1.f
        this.avControlObject.play = (speed) => { // number
            console.log("Im Mapping, Play, speed = " + speed);

            if (speed === 0) {
                signalCef("PAUSE_VIDEO");

                setTimeout(() => {
                    this.videoElement.pause();
                    this.avControlObject.speed = 0;
                }, 0);
            }
            else if (speed > 0) {
                if (speed === 1) {
                    signalCef("RESUME_VIDEO");
                } else {
                    signalCef("SPEED_VIDEO " + speed);
                }

                // delay play as some code may made some initializations beforehand in same event loop
                setTimeout(() => {
                    this.avControlObject.speed = speed;
                    this.videoElement.playbackRate = speed;
                    this.videoElement.play().catch((e) => {
                        console.error(e.message);
                        this.avControlObject.error = 5;
                        //  this.videoElement.dispatchEvent(new Event('error'));
                    });
                }, 0);
            }
            else if (speed < 0) {
                signalCef("SPEED_VIDEO " + speed);

                this.avControlObject.speed = speed;
                this.videoElement.playbackRate = 1.0;
                this.videoElement.play().catch((e) => {
                    console.error(e.message);
                    this.avControlObject.error = 5;
                    //  this.videoElement.dispatchEvent(new Event('error'));
                });
                this.rewindInterval = setInterval(() => {
                    if (this.videoElement.currentTime < 0.1) {
                        this.videoElement.currentTime = 0;
                        this.videoElement.pause();
                        this.avControlObject.speed = 0;
                        clearInterval(this.rewindInterval);
                    } else {
                        this.videoElement.currentTime = this.videoElement.currentTime -= 0.1;
                    }
                }, 100);
            }
            return true;
        };
        this.avControlObject.stop = () => {
            signalCef("STOP_VIDEO");

            this.videoElement.pause();
            this.videoElement.currentTime = 0;
            this.avControlObject.playState = 0;
            this.avControlObject.playPosition = 0;
            this.avControlObject.speed = 0;
            return true;
        };
        this.avControlObject.seek = (posInMs) => {
            signalCef("SEEK_VIDEO " + posInMs);

            // need seconds HTMLMediaElement.currentTime
            this.videoElement.currentTime = posInMs / 1000;
            this.avControlObject.playPosition = posInMs;
            return true;

        };
    }

    watchAvControlObjectMutations(avControlObject) {
        // if url of control object changed - change url of video object
        const handleDataChanged = (event) => { // MutationRecord
            if (event.attributeName === "data") {
                if (this.avControlObject.data.search("client://movie/fail") < 0) { // prevent infinite data change loop
                    this.videoElement.src = this.avControlObject.data;
                    // let video playback fail. Modern browsers don't support any handling of media events and methods on <object type="" data=""> tags
                    // setting data to unknown url will cause a GET 40x.. found no better soluttion yet to disable playback
                    this.avControlObject.data = "client://movie/fail";
                    this.videoElement.load();
                }
            } else if (event.attributeName === 'width' || event.attributeName === 'height') {
                var target = this.avControlObject;
                var width = target.getAttribute("width");
                var height = target.getAttribute("height");

                var position = target.getBoundingClientRect();
                var x = position.x;
                var y = position.y;

                signalCef("VIDEO_SIZE: " + width + "," + height + "," + x + "," + y);
            }
        };
        const handleMutation = (mutationList, mutationObserver) => {
            mutationList.forEach((mutation) => {
                switch (mutation.type) {
                    case 'childList':
                        break;
                    case 'attributes':
                        handleDataChanged(mutation);
                        /* An attribute value changed on the element in
                           mutation.target; the attribute name is in,.,  
                           mutation.attributeName and its previous value is in
                           mutation.oldValue */
                        break;
                }
            });
        };

        const mutationObserver = new MutationObserver(handleMutation);
        mutationObserver.observe(this.avControlObject, {
            'subtree': true,
            'childList': true,
            'attributes': true,
            'characterData': true
        });
    }

    watchAvVideoElementAttributeMutations(videoElement) {
        const handleMutation = (mutationList, mutationObserver) => {
            mutationList.forEach((mutation) => {
                if (mutation.attributeName === 'src') {
                    var target = mutation.target;
                    var newSrc = target.getAttribute("src");

                    if (newSrc.search("client://movie/transparent.webm") >= 0) {
                        // prevent recursion
                        return;
                    }

                    signalCef("CHANGE_VIDEO_URL:" + newSrc);

                    // overwrite src
                    target.src = "client://movie/transparent.webm";
                }
            });
        };

        const mutationObserver = new MutationObserver(handleMutation);
        mutationObserver.observe(videoElement, {
            'subtree': false,
            'childList': false,
            'attributes': true,
            'characterData': false
        });

    }

    /**
     * Method mapping the internal embbeded player events to the defined object one.
     * @param {object} objectElement A reference on the object tag.
     * @param {=object} optionHtmlPlayer A reference on the embedded html player video tag.
     */
    registerEmbeddedVideoPlayerEvents(objectElement, optionHtmlPlayer) {
        var embbededDocument = objectElement.contentDocument;
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: registerEmbeddedVideoPlayerEvents doc=', embbededDocument);
        // objectElement.onreadystatechange = function() {
        //   console.log("onreadystatechange state=", objectElement.readyState)
        // };

        if (optionHtmlPlayer) {
            this.registerVideoPlayerEvents(objectElement, optionHtmlPlayer); // same events for HTML5 video tag
            return;
        }
        setTimeout(function () {
            var embbededDocument = document.getElementById(objectElement.id);
            embbededDocument = embbededDocument && embbededDocument.contentDocument ? embbededDocument.contentDocument : null;
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: doc=', embbededDocument);
            if (embbededDocument) {
                var items = embbededDocument.body.children, player;

                scanChildrenForPlayer(items);
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: PLAYER:', player);
                if (player) {
                    //clearInterval(watchDogvideoElement);
                    this.registerVideoPlayerEvents(objectElement, player); // same events for HTML5 video tag
                    if (typeof player.getAttribute('autoplay') !== 'undefined') {
                        objectElement.playState = PLAY_STATES.playing;
                        if (objectElement.onPlayStateChange) {
                            objectElement.onPlayStateChange(objectElement.playState);
                        } else {
                            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange', objectElement.playState);
                            var playerEvent = new Event('PlayStateChange');
                            playerEvent.state = objectElement.playState;
                            objectElement.dispatchEvent(playerEvent);
                        }
                    }
                }
            }
        }, 200); // fixme: delay as inner document is not created so quickly by the browser or can take time to start ...
    }

    /**
     * Method mapping the HTML5 player events to the defined object one.
     * @param {object} objectElement A reference on the object tag.
     * @param {object} videoElement A reference on the HTML5 player video tag
     */
    registerVideoPlayerEvents(objectElement, videoElement) {
        videoElement && videoElement.addEventListener && videoElement.addEventListener('playing', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: )))))) play');

            objectElement.playState = PLAY_STATES.playing;
            if (objectElement.onPlayStateChange) {
                objectElement.onPlayStateChange(objectElement.playState);
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange', objectElement.playState);
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = objectElement.playState;
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('pause', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: pause');

            // ANSI CTA-2014-B
            // 5.7.1.f1 
            // bullet 2)
            objectElement.playState = PLAY_STATES.paused;
            if (objectElement.onPlayStateChange) {
                objectElement.onPlayStateChange(objectElement.playState);
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange', objectElement.playState);
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = objectElement.playState;
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('ended', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: ended');

            console.log("ENDED: " + objectElement.playState + " --> " + 5)

            signalCef("END_VIDEO");

            objectElement.playState = 5;
            if (objectElement.onPlayStateChange) {
                objectElement.onPlayStateChange(objectElement.playState);
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange', objectElement.playState);
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = objectElement.playState;
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('error', function (e) {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: error', e.message, e);

            console.log("ENDED: " + objectElement.playState + " --> " + PLAY_STATES.error)

            signalCef("ERROR_VIDEO");

            objectElement.playState = PLAY_STATES.error;
            if (objectElement.onPlayStateChange) {
                objectElement.onPlayStateChange(objectElement.playState);
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange', objectElement.playState);
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = objectElement.playState;
                // ANSI CTA-2014-B - 5.7.1.f1 - 5
                objectElement.error = 0; // 0 - A/V format not supported
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('durationchange', () => {
            objectElement.duration = videoElement.duration;
        }, false);
        videoElement && videoElement.addEventListener && videoElement.addEventListener('timeupdate', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: timeupdate');
            var pos = Math.floor(videoElement.currentTime * 1000);
            objectElement.playPostion = pos;
            //objectElement.currentTime = videoElement.currentTime;
            if (objectElement.PlayPositionChanged) {
                objectElement.PlayPositionChanged(pos);
            }
            objectElement.playPosition = pos;
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayPositionChanged', pos);
            var playerEvent = new Event('PlayPositionChanged');
            playerEvent.position = pos;
            objectElement.dispatchEvent(playerEvent);
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('ratechange', function () {
            window._HBBTV_DEBUG_ && console.log('ratechange');
            var playSpeed = videoElement.playbackRate;

            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlaySpeedChanged', playSpeed);
            var playerEvent = new Event('PlaySpeedChanged');
            playerEvent.speed = playSpeed;
            objectElement.dispatchEvent(playerEvent);

        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('seeked', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: seeked');
            var pos = Math.floor(videoElement.currentTime * 1000);
            if (objectElement.onPlayPositionChanged) {
                objectElement.playPosition = pos;
                objectElement.PlayPositionChanged(pos);
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayPositionChanged', pos);
                var playerEvent = new Event('PlayPositionChanged');
                playerEvent.position = pos;
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('durationchange', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: durationchanged');
            objectElement.playTime = videoElement.duration * 1000;
        }, false);
    }
}
