/**
 * OIPF 
 * Release 1 Specification
 * Volume 5 - Declarative Application Environment 
 * 7.13.1 The video/broadcast embedded object
 */

export class OipfVideoBroadcastMapper {
    constructor(node) { // the vide/broadcast object tag
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: Create video to oipf object mapper.');

        this.oipfPluginObject = node;
        this.videoTag = undefined;
        this.injectBroadcastVideoMethods(this.oipfPluginObject);
    }
    injectBroadcastVideoMethods(oipfPluginObject) {
        window._HBBTV_DEBUG_ && console.log('hbbtv-polyfill: injectBroadcastVideoMethods, length ' + oipfPluginObject.children.length);

        // We don't need a video player for broadcast.
        // We only need the position and size of the object

        var isVideoPlayerAlreadyAdded = oipfPluginObject.children.length > 0;
        if (!isVideoPlayerAlreadyAdded) {
            this.videoTag = document.createElement('video');
            this.videoTag.setAttribute('id', 'hbbtv-polyfill-video-player');
            this.videoTag.setAttribute('autoplay', ''); // note: call to bindToCurrentChannel() or play() is doing it
            this.videoTag.setAttribute('loop', '');
            this.videoTag.setAttribute('style', 'top:0px; left:0px; width:100%; height:100%;');
            // this.videoTag.src = "client://movie/transparent-full.webm";

            // this does not work as desired: <object...><video.../></object>
            // it has to be <object/></video>
            // but in this case it doesn't matter, because we only want to show Live TV
            oipfPluginObject.appendChild(this.videoTag);

            // oipfPluginObject.parentNode.insertBefore(this.videoTag, oipfPluginObject.nextSibling);

            oipfPluginObject.playState = 2;
            window._HBBTV_DEBUG_ &&  console.info('hbbtv-polyfill: BROADCAST VIDEO PLAYER ... ADDED');
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