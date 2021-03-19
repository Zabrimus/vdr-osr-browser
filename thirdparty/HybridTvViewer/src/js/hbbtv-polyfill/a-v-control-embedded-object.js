/**
 * OIPF
 * Release 1 Specification
 * Volume 5 - Declarative Application Environment
 * 7.14.1 The CEA 2014 A/V Control embedded object
 */

// import dashjs file --> we want it sync so don't pull from cdn ->  downside is we need a copy in repo TODO: fetch latest in build process
// import { MediaPlayer } from "dashjs";
// import shaka from "shaka-player";

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

// dash.js event handler.
// Used as a replacement for registerVideoPlayerEvents for dash.js.
// In this case use the dash.js events natively.
function handleDashjsEvents(event) {
    switch (event.type) {
        case 'playbackPlaying':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_PLAYING');
            break;

        case 'playbackPaused':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_PAUSED');
            break;

        case 'playbackEnded':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_ENDED');

            signalCef("END_VIDEO");
            break;

        case 'playbackError':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_ENDED');

            signalCef("ERROR_VIDEO");
            break;

        case 'periodSwitchCompleted':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PERIOD_SWITCH_COMPLETED');

            break;

        case 'playbackTimeUpdated':
            // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_TIME_UPDATED');

            /*
            if (videoElement.currentTime > 0) {
                signalCef("TIMEUPDATE: " + videoElement.currentTime * 1000 * 1000);
            }
            */

            break;

        case 'playbackRateChanged':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_RATE_CHANGED');
            break;

        case 'playbackSeeked':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_SEEKED');
            break;

        case 'manifestLoaded':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: MANIFEST_LOADED');
            break;

        case 'playbackStarted':
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill-dashjs: PLAYBACK_STARTED');
            break;
    }
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
        // this.videoElement.setAttribute('style', 'position:absolute');
        this.videoElement.setAttribute('style', 'height:100%; width:100%');

        // TEST
        // this.videoElement.setAttribute('data', originalDataAttribute);

        // interval to simulate rewind functionality
        this.rewindInterval;

        this.dashPlayer;
        // user dash.js to init player
        if (this.isDashVideo) {
            if (window._HBBTV_DASH_PLAYER_ === 'dashjs') {
                /*****************
                 dash-js
                 *****************/
                 this.dashPlayer = dashjs.MediaPlayer().create();
                 this.dashPlayer.initialize(this.videoElement, originalDataAttribute, true);

                 if (window._HBBTV_DEBUG_) {
                    this.dashPlayer.updateSettings({'debug': {'logLevel': dashjs.Debug.LOG_LEVEL_DEBUG}});
                 } else {
                    this.dashPlayer.updateSettings({'debug': {'logLevel': dashjs.Debug.LOG_LEVEL_WARNING}});
                 }

                 // dash.js events:
                 //    BUFFER_EMPTY
                 //    BUFFER_LOADED
                 //    CAN_PLAY
                 //    DYNAMIC_TO_STATIC
                 //    ERROR
                 //    LOG
                 //    MANIFEST_LOADED
                 //    METRIC_ADDED
                 //    METRIC_CHANGED
                 //    METRIC_UPDATED
                 //    METRICS_CHANGED
                 //    PERIOD_SWITCH_COMPLETED
                 //    PERIOD_SWITCH_STARTED
                 //    PLAYBACK_ENDED
                 //    PLAYBACK_ERROR
                 //    PLAYBACK_METADATA_LOADED
                 //    PLAYBACK_PAUSED
                 //    PLAYBACK_PLAYING
                 //    PLAYBACK_PROGRESS
                 //    PLAYBACK_RATE_CHANGED
                 //    PLAYBACK_SEEKED
                 //    PLAYBACK_SEEKING
                 //    PLAYBACK_STARTED
                 //    PLAYBACK_TIME_UPDATED
                 //    PLAYBACK_WAITING
                 //    STREAM_UPDATED
                 //    STREAM_INITIALIZED
                 //    TEXT_TRACK_ADDED
                 //    TEXT_TRACKS_ADDED
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_PLAYING'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_PAUSED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_ENDED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_ERROR'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PERIOD_SWITCH_COMPLETED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_TIME_UPDATED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_RATE_CHANGED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_SEEKED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['MANIFEST_LOADED'], handleDashjsEvents);
                 this.dashPlayer.on(dashjs.MediaPlayer.events['PLAYBACK_STARTED'], handleDashjsEvents);
            } else if (window._HBBTV_DASH_PLAYER_ === 'shaka') {
                shaka.polyfill.installAll();
                // shaka.log.setLevel(shaka.log.Level.DEBUG);
                shaka.log.setLevel(shaka.log.Level.V2);

                // Install built-in polyfills to patch browser incompatibilities.
                const player = new shaka.Player(this.videoElement);
                window.player = player;

                // player.addEventListener('error', onErrorEvent);

                try {
                    player.load(originalDataAttribute);
                    console.log('The video has now been loaded!');
                } catch (error) {
                    // onError is executed if the asynchronous load fails.
                    console.error('Error code', error.code, 'object', error);
                }
            }
        } else {
            this.videoElement.src = originalDataAttribute;
        }

        this.mapAvControlToHtml5Video();
        this.watchAvControlObjectMutations(this.avControlObject);
        this.registerEmbeddedVideoPlayerEvents(this.avControlObject, this.videoElement);


        // this does not work as desired: <object...><video.../></object>
        // it has to be <object/></video>
        if (isDashVideo) {
            this.avControlObject.parentNode.insertBefore(this.videoElement, this.avControlObject.nextSibling);
            // this.avControlObject.appendChild(this.videoElement);
        } else {
            this.avControlObject.parentNode.insertBefore(this.videoElement, this.avControlObject.nextSibling);
            // this.avControlObject.appendChild(this.videoElement);
        }

        this.avControlObject.playTime = this.videoElement.duration * 1000;

        // ANSI CTA-2014-B - 5.7.1.f - 5
        this.avControlObject.error = -1;
    }

    mapAvControlToHtml5Video() {
        clearInterval(this.rewindInterval);
        // ANSI CTA-2014-B
        // 5.7.1.f
        this.avControlObject.play = (speed) => { // number
            window._HBBTV_DEBUG_ && console.log("Im Mapping, Play, speed = " + speed);

            if (speed === 0) {
                // get current video position
                let currentTime = this.videoElement.currentTime;

                signalCef("PAUSE_VIDEO");

                setTimeout(() => {
                    this.videoElement.pause();
                    this.avControlObject.speed = 0;
                }, 0);
            }
            else if (speed > 0) {
                if (speed === 1) {
                    let currentTime = this.videoElement.currentTime;

                    signalCef("PLAY_VIDEO");
                    window.cefVideoSize();
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
            window._HBBTV_DEBUG_ && console.log("Im Mapping, stop");

            signalCef("STOP_VIDEO");

            this.videoElement.pause();
            this.videoElement.currentTime = 0;
            this.avControlObject.playState = 0;
            this.avControlObject.playPosition = 0;
            this.avControlObject.speed = 0;

            return true;
        };
        this.avControlObject.seek = (posInMs) => {
            window._HBBTV_DEBUG_ && console.log("Im Mapping, seek " + posInMs);

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
            } /* else if ((event.attributeName === 'width') || (event.attributeName === 'height')) {
                    this.videoElement.style.width = this.avControlObject.style.width;
                    this.videoElement.style.height = this.avControlObject.style.height;
                    this.videoElement.style.left = this.avControlObject.style.left;
                    this.videoElement.style.top = this.avControlObject.style.top;
            } */
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
        mutationObserver.observe(avControlObject, {
            'subtree': true,
            'childList': true,
            'attributes': true,
            'characterData': true,
            'attributeFilte': ["type"]
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
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange_1', objectElement.playState);
                objectElement.onPlayStateChange(objectElement.playState);
            } else {
                window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayStateChange_2', objectElement.playState);
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = objectElement.playState;
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('ended', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: ended');

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
            // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: timeupdate:' + (videoElement.currentTime * 1000));

            if (videoElement.currentTime > 0) {
                signalCef("TIMEUPDATE: " + videoElement.currentTime * 1000 * 1000);
            }

            var pos = Math.floor(videoElement.currentTime * 1000);
            objectElement.playPostion = pos;
            //objectElement.currentTime = videoElement.currentTime;
            if (objectElement.PlayPositionChanged) {
                objectElement.PlayPositionChanged(pos);
            }
            objectElement.playPosition = pos;
            // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayPositionChanged', pos);
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
                // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: dispatchEvent PlayPositionChanged', pos);
                var playerEvent = new Event('PlayPositionChanged');
                playerEvent.position = pos;
                objectElement.dispatchEvent(playerEvent);
            }
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('durationchange', function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: durationchange');
            objectElement.playTime = videoElement.duration * 1000;
        }, false);

        // FIXME: TEST
        videoElement && videoElement.addEventListener && videoElement.addEventListener('loadeddata', function () {
            // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: loadeddata');
        }, false);

        videoElement && videoElement.addEventListener && videoElement.addEventListener('play', function () {
            // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: play !');
        }, false);
    }
}
