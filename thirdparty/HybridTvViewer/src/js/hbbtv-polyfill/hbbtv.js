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

export const hbbtvFn = function () {
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
        '<video_profile name=\"MP4_AVC_SD_25_HEAAC\" type=\"video/mp4\" transport=\"dash\" DRMSystemID=\"urn:dvb:casystemid:19219\"/>' +
        '<video_profile name=\"MP4_AVC_HD_25_HEAAC\" type=\"video/mp4\" transport=\"dash\" DRMSystemID=\"urn:dvb:casystemid:19219\"/>' +
        '</profilelist>';
    var videoProfiles = currentCapabilities.split('video_profile');
    window.oipfCapabilities.xmlCapabilities = (new window.DOMParser()).parseFromString(currentCapabilities, 'text/xml');
    window.oipfCapabilities.extraSDVideoDecodes = videoProfiles.length > 1 ? videoProfiles.slice(1).join().split('_SD_').slice(1).length : 0;
    window.oipfCapabilities.extraHDVideoDecodes = videoProfiles.length > 1 ? videoProfiles.slice(1).join().split('_HD_').slice(1).length : 0;
    window.oipfCapabilities.hasCapability = function (capability) {
        return !!~new window.XMLSerializer().serializeToString(window.oipfCapabilities.xmlCapabilities).indexOf(capability.toString() || '??');
    };
    if (document.querySelector('[type="application/oipfCapabilities"]')) {
        document.querySelector('[type="application/oipfCapabilities"]').xmlCapabilities = window.oipfCapabilities.xmlCapabilities;
    }

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
