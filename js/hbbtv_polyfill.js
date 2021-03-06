/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, { enumerable: true, get: getter });
/******/ 		}
/******/ 	};
/******/
/******/ 	// define __esModule on exports
/******/ 	__webpack_require__.r = function(exports) {
/******/ 		if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 			Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 		}
/******/ 		Object.defineProperty(exports, '__esModule', { value: true });
/******/ 	};
/******/
/******/ 	// create a fake namespace object
/******/ 	// mode & 1: value is a module id, require it
/******/ 	// mode & 2: merge all properties of value into the ns
/******/ 	// mode & 4: return value when already ns object
/******/ 	// mode & 8|1: behave like require
/******/ 	__webpack_require__.t = function(value, mode) {
/******/ 		if(mode & 1) value = __webpack_require__(value);
/******/ 		if(mode & 8) return value;
/******/ 		if((mode & 4) && typeof value === 'object' && value && value.__esModule) return value;
/******/ 		var ns = Object.create(null);
/******/ 		__webpack_require__.r(ns);
/******/ 		Object.defineProperty(ns, 'default', { enumerable: true, value: value });
/******/ 		if(mode & 2 && typeof value != 'string') for(var key in value) __webpack_require__.d(ns, key, function(key) { return value[key]; }.bind(null, key));
/******/ 		return ns;
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = "./src/js/hbbtv-polyfill/index.js");
/******/ })
/************************************************************************/
/******/ ({

/***/ "./src/js/hbbtv-polyfill/a-v-control-embedded-object.js":
/*!**************************************************************!*\
  !*** ./src/js/hbbtv-polyfill/a-v-control-embedded-object.js ***!
  \**************************************************************/
/*! exports provided: OipfAVControlMapper */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "OipfAVControlMapper", function() { return OipfAVControlMapper; });
/**
 * OIPF 
 * Release 1 Specification
 * Volume 5 - Declarative Application Environment 
 * 7.14.1 The CEA 2014 A/V Control embedded object
 */

// import dashjs file --> we want it sync so don't pull from cdn ->  downside is we need a copy in repo TODO: fetch latest in build process
// import { MediaPlayer } from "dashjs";

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

/*
Copied from mpd-parser/src/utils/time.js
https://github.com/videojs/mpd-parser/blob/master/src/utils/time.js
*/
function ParseDuration(str) {
    const SECONDS_IN_YEAR = 365 * 24 * 60 * 60;
    const SECONDS_IN_MONTH = 30 * 24 * 60 * 60;
    const SECONDS_IN_DAY = 24 * 60 * 60;
    const SECONDS_IN_HOUR = 60 * 60;
    const SECONDS_IN_MIN = 60;

    // P10Y10M10DT10H10M10.1S
    const durationRegex =
      /P(?:(\d*)Y)?(?:(\d*)M)?(?:(\d*)D)?(?:T(?:(\d*)H)?(?:(\d*)M)?(?:([\d.]*)S)?)?/;
    const match = durationRegex.exec(str);

    if (!match) {
        return 0;
    }

    const [year, month, day, hour, minute, second] = match.slice(1);

    return (parseFloat(year || 0) * SECONDS_IN_YEAR +
      parseFloat(month || 0) * SECONDS_IN_MONTH +
      parseFloat(day || 0) * SECONDS_IN_DAY +
      parseFloat(hour || 0) * SECONDS_IN_HOUR +
      parseFloat(minute || 0) * SECONDS_IN_MIN +
      parseFloat(second || 0));
}

function ParseDate(str) {
    // Date format without timezone according to ISO 8601
    // YYY-MM-DDThh:mm:ss.ssssss
    const dateRegex = /^\d+-\d+-\d+T\d+:\d+:\d+(\.\d+)?$/;

    // If the date string does not specifiy a timezone, we must specifiy UTC. This is
    // expressed by ending with 'Z'
    if (dateRegex.test(str)) {
        str += 'Z';
    }

    return Date.parse(str) / 1000;
};
// end Copied from mpd-parser/src/utils/time.js

// https://davidwalsh.name/convert-xml-json
function xmlToJson(xml) {
    // Create the return object
    var obj = {};

    if (xml.nodeType === 1) { // element
        // do attributes
        if (xml.attributes.length > 0) {
            obj["_attributes"] = {};
            for (var j = 0; j < xml.attributes.length; j++) {
                var attribute = xml.attributes.item(j);
                obj["_attributes"][attribute.nodeName] = attribute.nodeValue;
            }
        }
    } else if (xml.nodeType === 3) { // text
        obj = xml.nodeValue;
    }

    // do children
    if (xml.hasChildNodes()) {
        for(var i = 0; i < xml.childNodes.length; i++) {
            var item = xml.childNodes.item(i);
            var nodeName = item.nodeName;
            if (typeof(obj[nodeName]) == "undefined") {
                obj[nodeName] = xmlToJson(item);
            } else {
                if (typeof(obj[nodeName].push) == "undefined") {
                    var old = obj[nodeName];
                    obj[nodeName] = [];
                    obj[nodeName].push(old);
                }
                obj[nodeName].push(xmlToJson(item));
            }
        }
    }
    return obj;
}

function getPrefixForMimeType(rep, adaptionSet) {
    var prefix;
    var mimetype;

    if (typeof adaptionSet._attributes.mimeType !== "undefined") {
        mimetype = adaptionSet._attributes.mimeType;
    } else {
        mimetype = rep._attributes.mimeType;
    }

    if (mimetype === 'video' || mimetype === 'video/mp4') {
        prefix = "V";
    } else if (mimetype === 'audio' || mimetype === 'audio/mp4') {
        prefix = "A";
    }

    return prefix;
}

function getMediaInit(rep, stattributes, repid) {
    var _st;
    if (typeof rep.SegmentTemplate !== 'undefined') {
        _st = rep.SegmentTemplate._attributes;
    } else {
        _st = rep._attributes;
    }

    var media;
    if (typeof _st.media !== "undefined") {
        media = _st.media;
    } else {
        media = stattributes.media;
    }

    if (typeof repid !== "undefined") {
        media = media.replace("$RepresentationID$", repid);
    }

    var init;
    if (typeof _st.initialization !== "undefined") {
        init = _st.initialization;
    } else {
        init = stattributes.initialization;
    }

    return {"media":media, "init":init};
}

function GetAndParseMpd(uri) {
    // get the mpd file
    var request = new XMLHttpRequest();

    request.open("GET", uri);
    request.addEventListener('load', function(event) {
        if (request.status >= 200 && request.status < 300) {
            let xml = request.responseText;

            var parsedXml = new window.DOMParser().parseFromString(xml, "text/xml");
            var parsedJson = xmlToJson(parsedXml);

            var baseUrl = new URL('.', uri).href;
            if (typeof parsedJson.MPD.BaseURL !== 'undefined') {
                baseUrl = parsedJson.MPD.BaseURL["#text"];
            }

            signalCef("DASH:BA:0:" + baseUrl);

            var _now = Date.now() / 1000.0;
            var _availabilityStartTime = ParseDate(parsedJson.MPD._attributes.availabilityStartTime);
            var _timeShiftBufferDepth = ParseDuration(parsedJson.MPD._attributes.timeShiftBufferDepth);
            var _periodStart = ParseDuration(parsedJson.MPD.Period._attributes.start);
            var _adaptionSet = parsedJson.MPD.Period.AdaptationSet;

            var _repidx = 0;

            for (var i in _adaptionSet) {
                var _stAttributes = _adaptionSet[i].SegmentTemplate._attributes;
                var _timescale = Number(_stAttributes.timescale);
                var _duration = Number(_stAttributes.duration);
                var _startNumber = Number(_stAttributes.startNumber);

                var duration = _duration / _timescale;
                var firstseg = Math.floor(_startNumber + (_now - _availabilityStartTime - _timeShiftBufferDepth) / duration);
                var lastseg = Math.floor((_now - _availabilityStartTime) / duration + _startNumber - 1);
                var startseg = Math.floor(lastseg - _periodStart / duration - 1);

                var _representation = _adaptionSet[i].Representation;

                if (Array.isArray(_representation)) {
                    for (var k in _representation) {
                        var _rep = _representation[k];

                        var width = (typeof _rep._attributes.width == 'undefined') ? 0 : _rep._attributes.width;
                        var height = (typeof _rep._attributes.height == 'undefined') ? 0 : _rep._attributes.height;
                        var bandwidth = _rep._attributes.bandwidth;

                        var repid = _rep._attributes.id;
                        var prefix = getPrefixForMimeType(_rep, _adaptionSet[i]);
                        var mediainit = getMediaInit(_rep, _stAttributes, repid);
                        var media = mediainit.media;
                        var init = mediainit.init;

                        signalCef("DASH:" + prefix + "C:" + _repidx + ":" + duration + ":" + firstseg + ":" + lastseg + ":" + startseg);
                        signalCef("DASH:" + prefix + "D:" + _repidx + ":" + width + ":" + height + ":" + bandwidth);
                        signalCef("DASH:" + prefix + "I:" + _repidx + ":" + init);
                        signalCef("DASH:" + prefix + "M:" + _repidx + ":" + media);

                        _repidx = _repidx + 1;
                    }
                } else {
                    var _rep = _representation;

                    var width = (typeof _rep._attributes.width == 'undefined') ? 0 : _rep._attributes.width;
                    var height = (typeof _rep._attributes.height == 'undefined') ? 0 : _rep._attributes.height;
                    var bandwidth = _rep._attributes.bandwidth;

                    var repid = _rep._attributes.id;
                    var prefix = getPrefixForMimeType(_rep, _adaptionSet[i]);
                    var mediainit = getMediaInit(_rep, _stAttributes, repid);
                    var media = mediainit.media;
                    var init = mediainit.init;

                    signalCef("DASH:" + prefix + "C:" + _repidx + ":" + duration + ":" + firstseg + ":" + lastseg + ":" + startseg);
                    signalCef("DASH:" + prefix + "D:" + _repidx + ":" + width + ":" + height + ":" + bandwidth);
                    signalCef("DASH:" + prefix + "I:" + _repidx + ":" + init);
                    signalCef("DASH:" + prefix + "M:" + _repidx + ":" + media);

                    _repidx = _repidx + 1;
                }
            }
        } else {
            console.warn(request.statusText, request.responseText);
        }
    });
    request.send();
}

class OipfAVControlMapper {

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
        // this.videoElement.setAttribute('style', 'top:0px; left:0px; width:16px; height:16px;');
        this.videoElement.setAttribute('style', 'top:0px; left:0px; width:100%; height:100%;');

        // interval to simulate rewind functionality
        this.rewindInterval;

        // user dash.js to init player
        if (this.isDashVideo) {
            // dash player
            // this.dashPlayer = MediaPlayer().create();
            // this.dashPlayer.initialize(this.videoElement, originalDataAttribute, true);

            // get the mpd file
            signalCef("CLEAR_DASH");
            GetAndParseMpd(originalDataAttribute);
        } else {
            if (originalDataAttribute.length <= 0) {
                // do nothing, there exists no video file
                window._HBBTV_DEBUG_ && console.log("originalDataAttribute is empty, ignore video request");
            } else {
                // signal video URL and set the timestamp of the transparent video
                let d = new Date();
                let n = d.getTime();
                signalCef("VIDEO_URL:" + String(n) + ":" + originalDataAttribute);

                // this.videoElement.src = originalDataAttribute;
                // copy object data url to html5 video tag src attribute ...
                this.videoElement.src = "client://movie/transparent_" + String(n) + ".webm";
            }
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
            window._HBBTV_DEBUG_ && console.log("Im Mapping, Play, speed = " + speed);

            if (speed === 0) {
                // get current video position
                let currentTime = this.videoElement.currentTime;
                signalCef("PAUSE_VIDEO: " + currentTime);

                setTimeout(() => {
                    this.videoElement.pause();
                    this.avControlObject.speed = 0;
                }, 0);
            }
            else if (speed > 0) {
                if (speed === 1) {
                    let currentTime = this.videoElement.currentTime;
                    signalCef("RESUME_VIDEO: " + currentTime);
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
            } else if (event.attributeName === "style") {
                window.cefVideoSize();
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

                    if (newSrc.search("client://movie/transparent") >= 0) {
                        // prevent recursion
                        return;
                    }

                    // get the mpd file
                    var isMpd = newSrc.endsWith(".mpd");
                    signalCef("CLEAR_DASH");
                    if (isMpd) {
                        GetAndParseMpd(newSrc);
                    }

                    let d = new Date();
                    let n = d.getTime();
                    signalCef("CHANGE_VIDEO_URL:" + String(n) + ":" + newSrc);

                    // overwrite src
                    target.src = "client://movie/transparent_" + String(n) + ".webm";
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

            window._HBBTV_DEBUG_ && console.log("ENDED: " + objectElement.playState + " --> " + 5)

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

            window._HBBTV_DEBUG_ && console.log("ENDED: " + objectElement.playState + " --> " + PLAY_STATES.error)

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
            // window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: timeupdate');
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
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: durationchanged');
            objectElement.playTime = videoElement.duration * 1000;
            window._HBBTV_DEBUG_ && console.log('========> durationchanged ' + videoElement.duration * 1000);
        }, false);
    }
}


/***/ }),

/***/ "./src/js/hbbtv-polyfill/hbb-video-handler.js":
/*!****************************************************!*\
  !*** ./src/js/hbbtv-polyfill/hbb-video-handler.js ***!
  \****************************************************/
/*! exports provided: VideoHandler */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "VideoHandler", function() { return VideoHandler; });
/* harmony import */ var _video_broadcast_embedded_object__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./video-broadcast-embedded-object */ "./src/js/hbbtv-polyfill/video-broadcast-embedded-object.js");
/* harmony import */ var _a_v_control_embedded_object__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./a-v-control-embedded-object */ "./src/js/hbbtv-polyfill/a-v-control-embedded-object.js");



// append play method to <object> tag
HTMLObjectElement.prototype.play = () => {};

class VideoHandler {
    constructor() {
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: Construct VideoHandler');

        this.videoObj = undefined;
        this.mutationObserver = undefined;
        this.videoBroadcastEmbeddedObject = undefined;
    }

    initialize() {
        // check at first, if the video object is already injected
        var videoexists = document.getElementById('hbbtv-polyfill-video-player');
        if (typeof videoexists !== 'undefined' && videoexists != null) {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: VideoHandler already initialized');
            return;
        }

        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: Init VideoHandler');

        // go through all existing nodes and check if we need to inject the video player emulation
        document.querySelectorAll('*').forEach((node) => {
            this.checkNodeTypeAndInjectVideoMethods(node);
        });

        this.watchAndHandleVideoObjectMutations();
    }

    checkNodeTypeAndInjectVideoMethods(node) {
        let mimeType = node.type;
        if (!node.type){
            return;  
        } 
        mimeType = mimeType.toLowerCase(); // ensure lower case string comparison

        if (node.getElementsByTagName('video').length > 0) {
            // video already injected.
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: checkNodeTypeAndInjectVideoMethods, video already injected ...');
            return;
        }

        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BROADCAST VIDEO PLAYER ...');
            this.videoBroadcastEmbeddedObject = new _video_broadcast_embedded_object__WEBPACK_IMPORTED_MODULE_0__["OipfVideoBroadcastMapper"](node);
        }
        if (mimeType.lastIndexOf('video/mpeg4', 0) === 0 ||
            mimeType.lastIndexOf('video/mp4', 0) === 0 ||  // h.264 video
            mimeType.lastIndexOf('audio/mp4', 0) === 0 ||  // aac audio
            mimeType.lastIndexOf('audio/mpeg', 0) === 0) { // mp3 audio
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BROADBAND VIDEO PLAYER ...');
            new _a_v_control_embedded_object__WEBPACK_IMPORTED_MODULE_1__["OipfAVControlMapper"](node);
        }
        // setup mpeg dash player
        if(mimeType.lastIndexOf('application/dash+xml', 0) === 0){
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: DASH VIDEO PLAYER ...');
            // new OipfAVControlMapper(node, true);
            // node.type = "video/mp4";
            // node.data = "client://movie/transparent-full.webm";
            new _a_v_control_embedded_object__WEBPACK_IMPORTED_MODULE_1__["OipfAVControlMapper"](node, true);
        }
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


/***/ }),

/***/ "./src/js/hbbtv-polyfill/hbbtv.js":
/*!****************************************!*\
  !*** ./src/js/hbbtv-polyfill/hbbtv.js ***!
  \****************************************/
/*! exports provided: hbbtvFn */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "hbbtvFn", function() { return hbbtvFn; });
/*******************************************************************************

    HybridTvViewer - a browser extension to open HbbTV,CE-HTML,BML,OHTV webpages
    instead of downloading them. A mere simulator is also provided.

    Copyright (C) 2015 Karl Rousseau

    MIT License:
    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    See the MIT License for more details: http://opensource.org/licenses/MIT

    HomePage: https://github.com/karl-rousseau/HybridTvViewer
*/
/** global Application, oipfObjectFactory, oipfApplicationManager, oipfConfiguration, oipfCapabilities */

const hbbtvFn = function () {
    window.oipf = window.oipf || {};

    // 7.1 Object factory API ------------------------------------------------------

    window.oipfObjectFactory = window.oipfObjectFactory || {};

    window.oipfObjectFactory.isObjectSupported = function (mimeType) {
        window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: isObjectSupported(' + mimeType + ') ...');
        return mimeType === 'video/broadcast' ||
            mimeType === 'video/mpeg' ||
            mimeType === 'application/oipfApplicationManager' ||
            mimeType === 'application/oipfCapabilities' ||
            mimeType === 'application/oipfConfiguration' ||
            mimeType === 'application/oipfDrmAgent' ||
            mimeType === 'application/oipfParentalControlManager' ||
            mimeType === 'application/oipfSearchManager';
    };
    window.oipfObjectFactory.createVideoBroadcastObject = function () {
        window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createVideoBroadcastObject() ...');
        return class VideoBroadcastObject {
            bindToCurrentChannel() { }
            setChannel() { }
            onblur(evt) { }
            onfocus(evt) { }
            addEventListener(eventName, callback, useCapture) { window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createVideoBroadcastObject / addEventListener'); }
            removeEventListener(eventName, callback, useCapture) { window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createVideoBroadcastObject / addEventListener'); }
            onPlayStateChange(evt) { }
            onPlaySpeedChanged(evt) { }
            onPlaySpeedsArrayChanged(evt) { }
            onPlayPositionChanged(evt) { }
            onFullScreenChange(evt) { }
            onParentalRatingChange(evt) { }
            onParentalRatingError(evt) { }
            onDRMRightsError(evt) { }
        };
    };
    window.oipfObjectFactory.createVideoMpegObject = function () {
        window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createVideoMpegObject() ...');
        return class VideoMpegObject {
            onblur(evt) { }
            onfocus(evt) { }
            addEventListener(eventName, callback, useCapture) { window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createVideoBroadcastObject / addEventListener'); }
            removeEventListener(eventName, callback, useCapture) { window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createVideoBroadcastObject / addEventListener'); }
            onPlayStateChange(evt) { }
            onPlaySpeedChanged(evt) { }
            onPlaySpeedsArrayChanged(evt) { }
            onPlayPositionChanged(evt) { }
            onFullScreenChange(evt) { }
            onParentalRatingChange(evt) { }
            onParentalRatingError(evt) { }
            onDRMRightsError(evt) { }
        };
        //return new VideoMpegObject();
    };
    window.oipfObjectFactory.onLowMemory = function () {
        // FIXME: see when we can generate this event (maybe inside the Web Inspector panel)
    };

    // 7.2.1  The application/oipfApplicationManager embedded object ---------------

    window.oipfApplicationManager = window.oipfApplicationManager || {};
    Object.defineProperties(window.Document.prototype, {
        '_application': {
            value: undefined,
            writable: true,
            enumerable: false
        }
    });

    window.oipfApplicationManager.onLowMemory = function () {
        // FIXME: see when we can generate this event
    };

    window.oipfApplicationManager.getOwnerApplication = function (document) {
        return window.Document._application || (window.Document._application = new window.Application(document));
    };



    // OIPF 7.2.2  The Application class ------------------------------------------------

    window.Application = {};

    function Application(doc) {
        this._document = doc;
    }
    Application.prototype.visible = undefined;
    Application.prototype.privateData = {};
    Application.prototype.privateData.keyset = {};
    var keyset = Application.prototype.privateData.keyset;
    keyset.RED = 0x1;
    keyset.GREEN = 0x2;
    keyset.YELLOW = 0x4;
    keyset.BLUE = 0x8;
    keyset.NAVIGATION = 0x10;
    keyset.VCR = 0x20;
    keyset.SCROLL = 0x40;
    keyset.INFO = 0x80;
    keyset.NUMERIC = 0x100;
    keyset.ALPHA = 0x200;
    keyset.OTHER = 0x400;
    keyset.value = 0x1f;
    keyset.setValue = function (value) {
        keyset.value = value;
        // add HBBTV_POLYFILL_NS namespace and count setValue calls
        window.HBBTV_POLYFILL_NS.keysetSetValueCount++;
    };
    Object.defineProperties(Application.prototype.privateData, {
        '_document': {
            value: null,
            writable: true,
            enumerable: false
        }
    });
    Object.defineProperties(Application.prototype.privateData, {
        'currentChannel': {
            enumerable: true,
            get: function currentChannel() {
                return window.HBBTV_POLYFILL_NS.currentChannel;
                // var currentCcid = window.oipf.getCurrentTVChannel().ccid; // FIXME: ccid is platform-dependent
                // return window.oipf.channelList.getChannel(currentCcid) || {};
            }
        }
    });
    Application.prototype.privateData.getFreeMem = function () {
        return ((window.performance && window.performance.memory.usedJSHeapSize) || 0);
    };

    Application.prototype.show = function () {
        if (this._document) {
            this._document.body.style.visibility = 'visible';
            return (this._visible = true);
        }
        return false;
    };

    Application.prototype.hide = function () {
        if (this._document) {
            this._document.body.style.visibility = 'hidden';
            this._visible = false;
            return true;
        }
        return false;
    };

    Application.prototype.createApplication = function (uri, createChild) {
        window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: createApplication: ' + uri);

        var newLocation = uri;

        if (uri.startsWith("dvb://current.ait")) {
            var app;

            app = /dvb:\/\/current\.ait\/(.*)\.(.*?)([\?#].*)/.exec(uri);
            if (app == undefined) {
                app = /dvb:\/\/current\.ait\/(.*)\.(.*)/.exec(uri);
            }

            if (app) {
                let appid = ('0' + app[2].toLocaleUpperCase()).slice(-2);
                let newurl = window._HBBTV_APPURL_.get(appid);

                if (newurl) {
                    newLocation = newurl + (app[3] ? app[3] : "");
                }
            }
        }

        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: createApplication: ' + uri + " -> " + newLocation);

        signalCef("CREATE_APP: " + window.location.href);

        window.cefChangeUrl(newLocation);
    };

    Application.prototype.destroyApplication = function () {
        window._HBBTV_DEBUG__ && console.log('hbbtv-polyfill: destroyApplication');

        delete this._applicationUrl;

        // destroy the whole page and set a flag
        document.body.innerText = "";
        document.head.innerText = "";

        signalCef("DESTROY_APP");
    };

    window.Application = Application;

    // 7.3.1  The application/oipfConfiguration embedded object --------------------
    window.oipfConfiguration = window.oipfConfiguration || {};

    oipfConfiguration.configuration = {};
    /* We don't have a localStorage
    oipfConfiguration.configuration.preferredAudioLanguage = window.localStorage.getItem('tvViewer_country') || 'ENG';
    oipfConfiguration.configuration.preferredSubtitleLanguage = window.localStorage.getItem('tvViewer_country') || 'ENG,FRA';
    oipfConfiguration.configuration.preferredUILanguage = window.localStorage.getItem('tvViewer_country') || 'ENG,FRA';
    oipfConfiguration.configuration.countryId = window.localStorage.getItem('tvViewer_country') || 'ENG';
     */
    oipfConfiguration.configuration.preferredAudioLanguage = 'DEU';
    oipfConfiguration.configuration.preferredSubtitleLanguage = 'DEU';
    oipfConfiguration.configuration.preferredUILanguage = 'DEU';
    oipfConfiguration.configuration.countryId = 'DEU';

    //oipfConfiguration.configuration.regionId = 0;
    //oipfConfiguration.localSystem = {};
    oipfConfiguration.getText = function (key) {

    };
    oipfConfiguration.setText = function (key, value) {
    };

    // 7.15.3 The application/oipfCapabilities embedded object ---------------------
    window.oipfCapabilities = window.oipfCapabilities || {};
    // we don't have a local storage
    // var storedCapabilities = window.localStorage.getItem('tvViewer_capabilities'); // FIXME: use tvViewer_caps object
    var storedCapabilities = null;
    var currentCapabilities = storedCapabilities ||
        '<profilelist>' +
        '<ui_profile name="OITF_HD_UIPROF+META_SI+META_EIT+TRICKMODE+RTSP+AVCAD+DRM+DVB_T">' +
        '<ext>' +
        '<colorkeys>true</colorkeys>' +
        '<video_broadcast type="ID_DVB_T" scaling="arbitrary" minSize="0">true</video_broadcast>' +
        '<parentalcontrol schemes="dvb-si">true</parentalcontrol>' +
        '</ext>' +
        '<drm DRMSystemID="urn:dvb:casystemid:19219">TS MP4</drm>' +
        '<drm DRMSystemID="urn:dvb:casystemid:1664" protectionGateways="ci+">TS</drm>' +
        '</ui_profile>' +
        '<audio_profile name=\"MPEG1_L3\" type=\"audio/mpeg\"/>' +
        '<audio_profile name=\"HEAAC\" type=\"audio/mp4\"/>' +
        '<video_profile name=\"TS_AVC_SD_25_HEAAC\" type=\"video/mpeg\"/>' +
        '<video_profile name=\"TS_AVC_HD_25_HEAAC\" type=\"video/mpeg\"/>' +
        '<video_profile name=\"MP4_AVC_SD_25_HEAAC\" type=\"video/mp4\"/>' +
        '<video_profile name=\"MP4_AVC_HD_25_HEAAC\" type=\"video/mp4\"/>' +
        '<video_profile name=\"MP4_AVC_SD_25_HEAAC\" type=\"video/mp4\" transport=\"dash\"/>' +
        '<video_profile name=\"MP4_AVC_HD_25_HEAAC\" type=\"video/mp4\" transport=\"dash\"/>' +
        '</profilelist>';
    var videoProfiles = currentCapabilities.split('video_profile');
    window.oipfCapabilities.xmlCapabilities = (new window.DOMParser()).parseFromString(currentCapabilities, 'text/xml');
    window.oipfCapabilities.extraSDVideoDecodes = videoProfiles.length > 1 ? videoProfiles.slice(1).join().split('_SD_').slice(1).length : 0;
    window.oipfCapabilities.extraHDVideoDecodes = videoProfiles.length > 1 ? videoProfiles.slice(1).join().split('_HD_').slice(1).length : 0;
    window.oipfCapabilities.hasCapability = function (capability) {
        return !!~new window.XMLSerializer().serializeToString(window.oipfCapabilities.xmlCapabilities).indexOf(capability.toString() || '??');
    };

    // 7.4.3 The application/oipfDownloadManager embedded object (+DL) -------------


    // 7.6.1 The application/oipfDrmAgent embedded object (+DRM and CI+) -----------


    // 7.9.1 The application/oipfParentalControlManager embedded object ------------


    // 7.10.1 The application/oipfRecordingScheduler embedded object (+PVR) --------


    // 7.12.1 The application/oipfSearchManager embedded object --------------------


    // 7.13.1 The video/broadcast embedded object ----------------------------------


    // 7.13.9 Extensions to video/broadcast for channel list -----------------------


    // 7.13.6 Extensions to video/broadcast for DRM rights errors ------------------


    // 7.13.21 Extensions to video/broadcast for synchronization -------------------


    // 7.16.2.4 DVB-SI extensions to Programme -------------------------------------


    // 7.16.5 Extensions for playback of selected media components -----------------

    // Detect existing hbb objects and append oipf functionality

    // inject properties of source into target
    function mixin(source, target) {
        for (var prop in source) {
            if (source.hasOwnProperty(prop)) {
                target[prop] = source[prop];
            }
        }
    }
    // Introspection: looking for existing objects ...
    var objects = document.getElementsByTagName("object");
    for (var i = 0; i < objects.length; i++) {
        var oipfPluginObject = objects.item(i);
        var sType = oipfPluginObject.getAttribute("type");
        if (sType === "application/oipfApplicationManager") {
            mixin(window.oipfApplicationManager, oipfPluginObject);
            oipfPluginObject.style.visibility = "hidden";
        } else if (sType === "application/oipfConfiguration") {
            mixin(window.oipfConfiguration, oipfPluginObject);
            oipfPluginObject.style.visibility = "hidden";
        } else if (sType === "oipfCapabilities") {
            mixin(window.oipfCapabilities, oipfPluginObject);
            oipfPluginObject.style.visibility = "hidden";
            break;
        }
    }
};


/***/ }),

/***/ "./src/js/hbbtv-polyfill/index.js":
/*!****************************************!*\
  !*** ./src/js/hbbtv-polyfill/index.js ***!
  \****************************************/
/*! no exports provided */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony import */ var _keyevent_init_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./keyevent-init.js */ "./src/js/hbbtv-polyfill/keyevent-init.js");
/* harmony import */ var _hbbtv_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./hbbtv.js */ "./src/js/hbbtv-polyfill/hbbtv.js");
/* harmony import */ var _hbb_video_handler_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./hbb-video-handler.js */ "./src/js/hbbtv-polyfill/hbb-video-handler.js");




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

    window.cefVideoSize = function() {
        var video = document.getElementById("video");
        var videocontainer = document.getElementById("videocontainer");
        var videoplayer = document.getElementById("hbbtv-polyfill-video-player");
        var playerobject = document.getElementById("playerObject");

        var target = null;
        var position;
        var maxwidth = 0, maxheight = 0;

        // --------------------------------------------------
        // quirks:
        // ignore videocontainer on hbbtv.daserste.de
        // --------------------------------------------------
        if (document.location.href.search('hbbtv.daserste.de') > 0) {
            videocontainer = null;
        }

        if (typeof video !== 'undefined' && video !== null) {
            position = video.getBoundingClientRect();
            if (position.width > 0 && position.height > 0) {
                target = video;
                maxwidth = position.width;
                maxheight = position.height;
            }

            window._HBBTV_DEBUG_ && console.log("===> VIDEO: "+ position.width + "," + position.height + "," + position.x + "," + position.y);
        }

        if (typeof videocontainer !== 'undefined' && videocontainer !== null) {
            position = videocontainer.getBoundingClientRect();

            if (position.width > 0 && position.height > 0 && position.width >= maxwidth && position.height >= maxheight) {
                target = videocontainer;
                maxwidth = position.width;
                maxheight = position.height;
            }

            window._HBBTV_DEBUG_ && console.log("===> VIDEOCONTAINER: "+ position.width + "," + position.height + "," + position.x + "," + position.y);
        }

        if (typeof videoplayer !== 'undefined' && videoplayer !== null) {
            position = videoplayer.getBoundingClientRect();

            if (position.width === 0 && position.height === 0) {
                // use parent element
                videoplayer = videoplayer.parentElement;
                position = videoplayer.getBoundingClientRect();
            }

            if (position.width > 0 && position.height > 0 && position.width >= maxwidth && position.height >= maxheight) {
                target = videoplayer;
                maxwidth = position.width;
                maxheight = position.height;
            }

            window._HBBTV_DEBUG_ && console.log("===> VIDEOPLAYER: "+ position.width + "," + position.height + "," + position.x + "," + position.y);
        }

        if (typeof playerobject !== 'undefined' && playerobject !== null) {
            position = playerobject.getBoundingClientRect();

            if (position.width > 0 && position.height > 0 && position.width >= maxwidth && position.height >= maxheight) {
                target = playerobject;
            }

            window._HBBTV_DEBUG_ && console.log("===> PLAYEROBJECT: "+ position.width + "," + position.height + "," + position.x + "," + position.y);
        }

        if (target) {
            var position = target.getBoundingClientRect();
            var x = parseInt(position.x, 10);
            var y = parseInt(position.y, 10);
            var width = parseInt(position.width, 10);
            var height = parseInt(position.height, 10);

            window.process_video_quirk(position, target);

            signalCef("VIDEO_SIZE: " + width + "," + height + "," + x + "," + y);

            if (width === 1280 && height === 720) {
                var overlay = document.getElementById('_video_color_overlay_');
                if (overlay) {
                    overlay.style.visibility = "hidden";
                }
            } else {
                var overlay = document.getElementById('_video_color_overlay_');
                if (overlay) {
                    overlay.style.visibility = "visible";
                    overlay.style.left = x.toString() + "px";
                    overlay.style.top = y.toString() + "px";
                    overlay.style.width = width.toString() + "px";
                    overlay.style.height = height.toString() + "px";
                    overlay.style.backgroundColor = "rgb(254, 46, 154)";
                }
            }
        } else {
            // no video tag found -> fullscreen
            signalCef("VIDEO_SIZE: " + 1280 + "," + 720 + "," + 0 + "," + 0);

            var overlay = document.getElementById('_video_color_overlay_');
            if (overlay) {
                overlay.style.visibility = "hidden";
            }
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

            return cefOldXHROpen.call(this, method, url, async, user, password);
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

    Object(_keyevent_init_js__WEBPACK_IMPORTED_MODULE_0__["keyEventInit"])();

    Object(_hbbtv_js__WEBPACK_IMPORTED_MODULE_1__["hbbtvFn"])();

    window.HBBTV_VIDEOHANDLER = new _hbb_video_handler_js__WEBPACK_IMPORTED_MODULE_2__["VideoHandler"]();
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


/***/ }),

/***/ "./src/js/hbbtv-polyfill/keyevent-init.js":
/*!************************************************!*\
  !*** ./src/js/hbbtv-polyfill/keyevent-init.js ***!
  \************************************************/
/*! exports provided: keyEventInit */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "keyEventInit", function() { return keyEventInit; });
/* harmony import */ var _shared_pc_keycodes_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ../shared/pc-keycodes.js */ "./src/js/shared/pc-keycodes.js");



const keyEventInit = function () {
    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill: keyEventInit");

    window.KeyEvent = window.KeyEvent || {}; // defining default global KeyEvent as defined in CEA-HTML 2014 specs
    window.KeyEvent.VK_LEFT = (typeof window.KeyEvent.VK_LEFT !== 'undefined' ? window.KeyEvent.VK_LEFT : 0x25);
    window.KeyEvent.VK_UP = (typeof window.KeyEvent.VK_UP !== 'undefined' ? window.KeyEvent.VK_UP : 0x26);
    window.KeyEvent.VK_RIGHT = (typeof window.KeyEvent.VK_RIGHT !== 'undefined' ? window.KeyEvent.VK_RIGHT : 0x27);
    window.KeyEvent.VK_DOWN = (typeof window.KeyEvent.VK_DOWN !== 'undefined' ? window.KeyEvent.VK_DOWN : 0x28);
    window.KeyEvent.VK_ENTER = (typeof window.KeyEvent.VK_ENTER !== 'undefined' ? window.KeyEvent.VK_ENTER : 13);
    window.KeyEvent.VK_BACK = (typeof window.KeyEvent.VK_BACK !== 'undefined' ? window.KeyEvent.VK_BACK : 461);

    window.KeyEvent.VK_RED = (typeof window.KeyEvent.VK_RED !== 'undefined' ? window.KeyEvent.VK_RED : 403);
    window.KeyEvent.VK_GREEN = (typeof window.KeyEvent.VK_GREEN !== 'undefined' ? window.KeyEvent.VK_GREEN : 404);
    window.KeyEvent.VK_YELLOW = (typeof window.KeyEvent.VK_YELLOW !== 'undefined' ? window.KeyEvent.VK_YELLOW : 405);
    window.KeyEvent.VK_BLUE = (typeof window.KeyEvent.VK_BLUE !== 'undefined' ? window.KeyEvent.VK_BLUE : 406);

    window.KeyEvent.VK_PLAY = (typeof window.KeyEvent.VK_PLAY !== 'undefined' ? window.KeyEvent.VK_PLAY : 0x50);
    window.KeyEvent.VK_PAUSE = (typeof window.KeyEvent.VK_PAUSE !== 'undefined' ? window.KeyEvent.VK_PAUSE : 0x51);
    window.KeyEvent.VK_STOP = (typeof window.KeyEvent.VK_STOP !== 'undefined' ? window.KeyEvent.VK_STOP : 0x53);

    window.KeyEvent.VK_FAST_FWD = (typeof window.KeyEvent.VK_FAST_FWD !== 'undefined' ? window.KeyEvent.VK_FAST_FWD : 0x46);
    window.KeyEvent.VK_REWIND = (typeof window.KeyEvent.VK_REWIND !== 'undefined' ? window.KeyEvent.VK_REWIND : 0x52);

    window.KeyEvent.VK_PAGE_UP = (typeof window.KeyEvent.VK_PAGE_UP !== 'undefined' ? window.KeyEvent.VK_PAGE_UP : 0x21);
    window.KeyEvent.VK_PAGE_DOWN = (typeof window.KeyEvent.VK_PAGE_DOWN !== 'undefined' ? window.KeyEvent.VK_PAGE_DOWN : 0x22);

    window.KeyEvent.VK_0 = (typeof window.KeyEvent.VK_0 !== 'undefined' ? window.KeyEvent.VK_0 : 0x30);
    window.KeyEvent.VK_1 = (typeof window.KeyEvent.VK_1 !== 'undefined' ? window.KeyEvent.VK_1 : 0x31);
    window.KeyEvent.VK_2 = (typeof window.KeyEvent.VK_2 !== 'undefined' ? window.KeyEvent.VK_2 : 0x32);
    window.KeyEvent.VK_3 = (typeof window.KeyEvent.VK_3 !== 'undefined' ? window.KeyEvent.VK_3 : 0x33);
    window.KeyEvent.VK_4 = (typeof window.KeyEvent.VK_4 !== 'undefined' ? window.KeyEvent.VK_4 : 0x34);
    window.KeyEvent.VK_5 = (typeof window.KeyEvent.VK_5 !== 'undefined' ? window.KeyEvent.VK_5 : 0x35);
    window.KeyEvent.VK_6 = (typeof window.KeyEvent.VK_6 !== 'undefined' ? window.KeyEvent.VK_6 : 0x36);
    window.KeyEvent.VK_7 = (typeof window.KeyEvent.VK_7 !== 'undefined' ? window.KeyEvent.VK_7 : 0x37);
    window.KeyEvent.VK_8 = (typeof window.KeyEvent.VK_8 !== 'undefined' ? window.KeyEvent.VK_8 : 0x38);
    window.KeyEvent.VK_9 = (typeof window.KeyEvent.VK_9 !== 'undefined' ? window.KeyEvent.VK_9 : 0x39);

    // patches for some CE-HTML apps ...

    window.VK_LEFT = (typeof window.VK_LEFT !== 'undefined' ? window.VK_LEFT : 0x25);
    window.VK_UP = (typeof window.VK_UP !== 'undefined' ? window.VK_UP : 0x26);
    window.VK_RIGHT = (typeof window.VK_RIGHT !== 'undefined' ? window.VK_RIGHT : 0x27);
    window.VK_DOWN = (typeof window.VK_DOWN !== 'undefined' ? window.VK_DOWN : 0x28);
    window.VK_ENTER = (typeof window.VK_ENTER !== 'undefined' ? window.VK_ENTER : 13);
    window.VK_BACK = (typeof window.VK_BACK !== 'undefined' ? window.VK_BACK : 461);

    window.VK_RED = (typeof window.VK_RED !== 'undefined' ? window.VK_RED : 403);
    window.VK_GREEN = (typeof window.VK_GREEN !== 'undefined' ? window.VK_GREEN : 404);
    window.VK_YELLOW = (typeof window.VK_YELLOW !== 'undefined' ? window.VK_YELLOW : 405);
    window.VK_BLUE = (typeof window.VK_BLUE !== 'undefined' ? window.VK_BLUE : 406);

    window.VK_PLAY = (typeof window.VK_PLAY !== 'undefined' ? window.VK_PLAY : 0x50);
    window.VK_PAUSE = (typeof window.VK_PAUSE !== 'undefined' ? window.VK_PAUSE : 0x51);
    window.VK_STOP = (typeof window.VK_STOP !== 'undefined' ? window.VK_STOP : 0x53);

    window.VK_FAST_FWD = (typeof window.VK_FAST_FWD !== 'undefined' ? window.VK_FAST_FWD : 0x46);
    window.VK_REWIND = (typeof window.VK_REWIND !== 'undefined' ? window.VK_REWIND : 0x52);

    window.VK_PAGE_UP = (typeof window.VK_PAGE_UP !== 'undefined' ? window.VK_PAGE_UP : 0x21);
    window.VK_PAGE_DOWN = (typeof window.VK_PAGE_DOWN !== 'undefined' ? window.VK_PAGE_DOWN : 0x22);

    window.VK_0 = (typeof window.VK_0 !== 'undefined' ? window.VK_0 : 0x30);
    window.VK_1 = (typeof window.VK_1 !== 'undefined' ? window.VK_1 : 0x31);
    window.VK_2 = (typeof window.VK_2 !== 'undefined' ? window.VK_2 : 0x32);
    window.VK_3 = (typeof window.VK_3 !== 'undefined' ? window.VK_3 : 0x33);
    window.VK_4 = (typeof window.VK_4 !== 'undefined' ? window.VK_4 : 0x34);
    window.VK_5 = (typeof window.VK_5 !== 'undefined' ? window.VK_5 : 0x35);
    window.VK_6 = (typeof window.VK_6 !== 'undefined' ? window.VK_6 : 0x36);
    window.VK_7 = (typeof window.VK_7 !== 'undefined' ? window.VK_7 : 0x37);
    window.VK_8 = (typeof window.VK_8 !== 'undefined' ? window.VK_8 : 0x38);
    window.VK_9 = (typeof window.VK_9 !== 'undefined' ? window.VK_9 : 0x39);



    window.addEventListener(
        "keydown",
        (evt) => {
            window._HBBTV_DEBUG_ &&  console.log("hbbtv-polyfill: browser keydown " + evt.keyCode, "internal", evt.detail && evt.detail.hbbInternal === true);
            if (evt.detail && evt.detail.hbbInternal === true) {
                return;
            }

            evt.preventDefault();
            evt.stopPropagation();
            var keyCode = evt.which || evt.keyCode;
            if (keyCode === 82) {
                // 'r' key on PC
                doKeyPress("VK_RED");
            } else if (keyCode === 71) {
                // 'g' key on PC
                doKeyPress("VK_GREEN");
            } else if (keyCode === 89) {
                // 'y' key on PC
                doKeyPress("VK_YELLOW");
            } else if (keyCode === 66) {
                // 'b' key on PC
                doKeyPress("VK_BLUE");
            } else if (keyCode === 37) {
                doKeyPress("VK_LEFT");
            } else if (keyCode === 38) {
                doKeyPress("VK_UP");
            } else if (keyCode === 39) {
                doKeyPress("VK_RIGHT");
            } else if (keyCode === 40) {
                // DOWN on PC
                doKeyPress("VK_DOWN");
            } else if (keyCode === 13) {
                // ENTER on PC
                doKeyPress("VK_ENTER");
            } else if (keyCode === 8) {
                // Backspace on PC
                doKeyPress("VK_BACK");
            } else if (keyCode === 96) {
                doKeyPress("VK_0");
            } else if (keyCode === 97) {
                doKeyPress("VK_1");
            } else if (keyCode === 98) {
                doKeyPress("VK_2");
            } else if (keyCode === 99) {
                doKeyPress("VK_3");
            } else if (keyCode === 100) {
                doKeyPress("VK_4");
            } else if (keyCode === 101) {
                doKeyPress("VK_5");
            } else if (keyCode === 102) {
                doKeyPress("VK_6");
            } else if (keyCode === 103) {
                doKeyPress("VK_7");
            } else if (keyCode === 104) {
                doKeyPress("VK_8");
            } else if (keyCode === 105) {
                doKeyPress("VK_9");
            } else if (keyCode === 116) {
                // F5 in browser
                window.location.reload();
            } else if (keyCode === _shared_pc_keycodes_js__WEBPACK_IMPORTED_MODULE_0__["PC_KEYCODES"].k) {
                doKeyPress("VK_PAUSE");
            } else if (keyCode === _shared_pc_keycodes_js__WEBPACK_IMPORTED_MODULE_0__["PC_KEYCODES"].j) {
                doKeyPress("VK_PLAY");
            } else if (keyCode === _shared_pc_keycodes_js__WEBPACK_IMPORTED_MODULE_0__["PC_KEYCODES"].h) {
                doKeyPress("VK_REWIND");
            } else if (keyCode === _shared_pc_keycodes_js__WEBPACK_IMPORTED_MODULE_0__["PC_KEYCODES"].l) {
                doKeyPress("VK_STOP");
            } else if (keyCode === _shared_pc_keycodes_js__WEBPACK_IMPORTED_MODULE_0__["PC_KEYCODES"].oe) {
                doKeyPress("VK_FAST_FWD");
            }
        },
        true
    );

    function doKeyPress(keyCode) {
        // document.activeElement.dispatchEvent(new Event('focus'));

        // create and dispatch the event (keydown)
        var keyDownEvent = new KeyboardEvent("keydown", {
            bubbles: true,
            cancelable: true,
        });

        delete keyDownEvent.keyCode;
        Object.defineProperty(keyDownEvent, "keyCode", { "value": window[keyCode] });
        Object.defineProperty(keyDownEvent, "detail", { "value": { hbbInternal: true } });

        document.dispatchEvent(keyDownEvent);

        // create and dispatch the event (keypress)
        var keypressEvent = new KeyboardEvent("keypress", {
            bubbles: true,
            cancelable: true,
        });
        delete keypressEvent.keyCode;
        Object.defineProperty(keypressEvent, "keyCode", { "value": window[keyCode] });
        Object.defineProperty(keypressEvent, "detail", { "value": { hbbInternal: true } });

        document.dispatchEvent(keypressEvent);

        // create and dispatch the event (keyup)
        var keyUpEvent = new KeyboardEvent("keyup", {
          bubbles: true,
          cancelable: true,
        });

        delete keyUpEvent.keyCode;
        Object.defineProperty(keyUpEvent, "keyCode", { "value": window[keyCode] });
        Object.defineProperty(keyUpEvent, "detail", { "value": { hbbInternal: true } });

        document.dispatchEvent(keyUpEvent);

        /*
        $(document).ready(function() {
          setTimeout(function() { document.activeElement.focus() }, 20);
        });
        */

        /*
        setTimeout(function() { document.activeElement.focus() }, 20);
        */

        /*
        setTimeout(function() {
            let el = document.activeElement;
            if (typeof el !== 'undefined') {
              el.blur();
              el.focus();
            }
        }, 3000);
        */
    };

    window.cefKeyPress = function(keyCode) {
       // special handling if possible (use a whitelist, because not all sites works really good with this hack)
       // whitelist
       // http://www.tagesschau.de/itv/hbbtv/hbbtv.cehtml

       var href = document.location.href;
       var next;

       if (href.search("www.tagesschau.de") !== -1) {
         if (keyCode === 'VK_LEFT' || keyCode === 'VK_UP') {
           next = findNextFocusableElement(true, document.activeElement);
         } else if (keyCode === 'VK_RIGHT' || keyCode === 'VK_DOWN') {
           next = findNextFocusableElement(false, document.activeElement);
         }
       }

       if (typeof next !== 'undefined') {
          // TODO: Something is still wrong here. Focus, Click and what else?
          next.focus();
          next.click();
       } else {
         doKeyPress(keyCode);
       }
    };

};


/***/ }),

/***/ "./src/js/hbbtv-polyfill/video-broadcast-embedded-object.js":
/*!******************************************************************!*\
  !*** ./src/js/hbbtv-polyfill/video-broadcast-embedded-object.js ***!
  \******************************************************************/
/*! exports provided: OipfVideoBroadcastMapper */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "OipfVideoBroadcastMapper", function() { return OipfVideoBroadcastMapper; });
/**
 * OIPF 
 * Release 1 Specification
 * Volume 5 - Declarative Application Environment 
 * 7.13.1 The video/broadcast embedded object
 */

class OipfVideoBroadcastMapper {
    constructor(node) { // the vide/broadcast object tag
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: Create video to oipf object mapper.');

        this.oipfPluginObject = node;
        this.videoTag = undefined;
        this.injectBroadcastVideoMethods(this.oipfPluginObject);
    }
    injectBroadcastVideoMethods(oipfPluginObject) {
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: injectBroadcastVideoMethods, length ' + oipfPluginObject.children.length);

        var isVideoPlayerAlreadyAdded = oipfPluginObject.children.length > 0;
        if (!isVideoPlayerAlreadyAdded) {
            this.videoTag = document.createElement('video');
            this.videoTag.setAttribute('id', 'hbbtv-polyfill-video-player');
            this.videoTag.setAttribute('autoplay', ''); // note: call to bindToCurrentChannel() or play() is doing it
            this.videoTag.setAttribute('loop', '');
            this.videoTag.setAttribute('style', 'top:0px; left:0px; width:100%; height:100%;');
            // this.videoTag.src = "http://cdn.smartclip.net/assets/atv/video/Caminandes1_720p.mp4";
            this.videoTag.src = "client://movie/transparent-full.webm";
            oipfPluginObject.appendChild(this.videoTag);
            oipfPluginObject.playState = 2;
            window._HBBTV_DEBUG_ &&  console.info('hbbtv-polyfill: BROADCAST VIDEO PLAYER ... ADDED');

            window.cefVideoSize();
        }

        // inject OIPF methods ...

        //injectBroadcastVideoMethods(oipfPluginObject);
        var currentChannel = window.HBBTV_POLYFILL_NS.currentChannel;
        oipfPluginObject.currentChannel = currentChannel;
        oipfPluginObject.createChannelObject = function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BroadcastVideo createChannelObject() ...');
        };
        oipfPluginObject.bindToCurrentChannel = function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BroadcastVideo bindToCurrentChannel() ...');
            var player = document.getElementById('hbbtv-polyfill-video-player');
            if (player) {
                player.onerror = function (e) {
                    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill:", e);
                };
                window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill: now play");
                player.play().catch((e) => {
                    window._HBBTV_DEBUG_ && console.log("hbbtv-polyfill:", e, e.message, player.src);
                });
                oipfPluginObject.playState = 2;
                window.cefVideoSize();

                // TODO: If there is no channel currently being presented, the OITF SHALL dispatch an event to the onPlayStateChange listener(s) whereby the state parameter is given value 0 (“ unrealized ")
            }
            return; // TODO: must return a Channel object
        };

        oipfPluginObject.setChannel = function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BroadcastVideo setChannel() ...');
        };
        oipfPluginObject.prevChannel = function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BroadcastVideo prevChannel() ...');
            return currentChannel;
        };
        oipfPluginObject.nextChannel = function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BroadcastVideo nextChannel() ...');
            return currentChannel;
        };
        oipfPluginObject.release = function () {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: BroadcastVideo release() ...2');
            var player = document.getElementById('hbbtv-polyfill-video-player');
            if (player) {
                player.pause();
                player.parentNode.removeChild(player);
            }
        };
        function ChannelConfig() {
        }
        ChannelConfig.prototype.channelList = {};
        ChannelConfig.prototype.channelList._list = [];
        ChannelConfig.prototype.channelList._list.push(currentChannel);
        Object.defineProperties(ChannelConfig.prototype.channelList, {
            'length': {
                enumerable: true,
                get: function length() {
                    return window.oipf.ChannelConfig.channelList._list.length;
                }
            }
        });
        ChannelConfig.prototype.channelList.item = function (index) {
            return window.oipf.ChannelConfig.channelList._list[index];
        };
        ChannelConfig.prototype.channelList.getChannel = function (ccid) {
            var channels = window.oipf.ChannelConfig.channelList._list;
            for (var channelIdx in channels) {
                if (channels.hasOwnProperty(channelIdx)) {
                    var channel = channels[channelIdx];
                    if (ccid === channel.ccid) {
                        return channel;
                    }
                }
            }
            return null;
        };
        ChannelConfig.prototype.channelList.getChannelByTriplet = function (onid, tsid, sid, nid) {
            var channels = window.oipf.ChannelConfig.channelList._list;
            for (var channelIdx in channels) {
                if (channels.hasOwnProperty(channelIdx)) {

                    var channel = channels[channelIdx];
                    if (onid === channel.onid &&
                        tsid === channel.tsid &&
                        sid === channel.sid) {
                        return channel;
                    }
                }
            }
            return null;
        };
        window.oipf.ChannelConfig = new ChannelConfig();
        oipfPluginObject.getChannelConfig = {}; // OIPF 7.13.9 getChannelConfig
        Object.defineProperties(oipfPluginObject, {
            'getChannelConfig': {
                value: function () {
                    return window.oipf.ChannelConfig;
                },
                enumerable: true,
                writable: false
            }
        });
        oipfPluginObject.programmes = [];
        oipfPluginObject.programmes.push({ name: 'Event 1, umlaut \u00e4', channelId: 'ccid:dvbt.0', duration: 600, startTime: Date.now() / 1000, description: 'EIT present event is under construction' });
        oipfPluginObject.programmes.push({ name: 'Event 2, umlaut \u00f6', channelId: 'ccid:dvbt.0', duration: 300, startTime: Date.now() / 1000 + 600, description: 'EIT following event is under construction' });
        Object.defineProperty(oipfPluginObject, 'COMPONENT_TYPE_VIDEO', { value: 0, enumerable: true });
        Object.defineProperty(oipfPluginObject, 'COMPONENT_TYPE_AUDIO', { value: 1, enumerable: true });
        Object.defineProperty(oipfPluginObject, 'COMPONENT_TYPE_SUBTITLE', { value: 2, enumerable: true });
        class AVComponent {
            constructor() {
                this.COMPONENT_TYPE_VIDEO = 0;
                this.COMPONENT_TYPE_AUDIO = 1;
                this.COMPONENT_TYPE_SUBTITLE = 2;
                this.componentTag = 0;
                this.pid = undefined;
                this.type = undefined;
                this.encoding = 'DVB-SUBT';
                this.encrypted = false;
            }
        }
        class AVVideoComponent extends AVComponent {
            constructor() {
                super();
                this.type = this.COMPONENT_TYPE_VIDEO;
                this.aspectRatio = 1.78;
            }
        }
        class AVAudioComponent extends AVComponent {
            constructor() {
                super();
                this.type = this.COMPONENT_TYPE_AUDIO;
                this.language = 'eng';
                this.audioDescription = false;
                this.audioChannels = 2;
            }
        }
        class AVSubtitleComponent extends AVComponent {
            constructor() {
                super();
                this.type = this.COMPONENT_TYPE_SUBTITLE;
                this.language = 'deu';
                this.hearingImpaired = false;
            }
        }
        class AVComponentCollection extends Array {
            constructor(num) {
                super(num);
            }
            item(idx) {
                return idx < this.length ? this[idx] : [];
            }
        }
        oipfPluginObject.getComponents = (function (type) {
            return [
                type === this.COMPONENT_TYPE_VIDEO ? new AVVideoComponent() :
                    type === this.COMPONENT_TYPE_AUDIO ? new AVAudioComponent() :
                        type === this.COMPONENT_TYPE_SUBTITLE ? new AVSubtitleComponent() : null
            ];
        }).bind(oipfPluginObject);
        // TODO: read those values from a message to the extension (+ using a dedicated worker to retrieve those values from the TS file inside broadcast_url form field)
        oipfPluginObject.getCurrentActiveComponents = (function () { return [new AVVideoComponent(), new AVAudioComponent(), new AVSubtitleComponent()]; }).bind(oipfPluginObject);
        oipfPluginObject.selectComponent = (function (cpt) { return true; }).bind(oipfPluginObject);
        oipfPluginObject.unselectComponent = (function (cpt) { return true; }).bind(oipfPluginObject);
        oipfPluginObject.setFullScreen = (function (state) {
            this.onFullScreenChange(state);
            var player = this.children.length > 0 ? this.children[0] : undefined;
            if (player && state) {
                player.style.width = '100%'; player.style.height = '100%';
            }
        }).bind(oipfPluginObject);
        oipfPluginObject.onFullScreenChange = function () {
        };
        oipfPluginObject.onChannelChangeError = function (channel, error) {
        };
        oipfPluginObject.onChannelChangeSucceeded = function (channel) {
        };
        // use custom namespace to track and trigger registered streamevents
        oipfPluginObject.addStreamEventListener = function (url, eventName, listener) {
            window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: register listener -', eventName);
            window.HBBTV_POLYFILL_NS.streamEventListeners.push({ url, eventName, listener });
        };
        oipfPluginObject.removeStreamEventListener = function (url, eventName, listener) {
            var idx = window.HBBTV_POLYFILL_NS.streamEventListeners.findIndex((e) => {
                return e.listener === listener && e.eventName === eventName && e.url === url;
            });
            window.HBBTV_POLYFILL_NS.streamEventListeners.splice(idx, 1);
        };

    }

}

/***/ }),

/***/ "./src/js/shared/pc-keycodes.js":
/*!**************************************!*\
  !*** ./src/js/shared/pc-keycodes.js ***!
  \**************************************/
/*! exports provided: PC_KEYCODES */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "PC_KEYCODES", function() { return PC_KEYCODES; });
const PC_KEYCODES = {
    r: 82,
    y: 89,
    b: 66,
    g: 71,
    h: 72,
    j: 74,
    k: 75,
    l: 76,
    oe: 192,
    left: 37,
    up: 38,
    right: 39,
    down: 40,
    enter: 13,
    back: 8,
    "0": 96,
    "1": 97,
    "2": 98,
    "3": 99,
    "4": 100,
    "5": 101,
    "6": 102,
    "7": 103,
    "8": 104,
    "9": 105,
    "f5": 116,
    ".": 190,
    ",": 188
};


/***/ })

/******/ });
//# sourceMappingURL=hbbtv_polyfill.js.map