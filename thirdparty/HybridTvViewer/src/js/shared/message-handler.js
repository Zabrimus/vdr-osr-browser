
/**
 * Enable bidirectional message passing.
 * 
 */
export class MessageHandler {

    /**
     *      
     * Listen for "message" on source element, postMessages to target element.
     * @param {*} sourceElem 
     * @param {*} targetElem 
     */
    constructor(sourceElem, targetElem) {
        this.sourceElem = sourceElem;
        this.targetElem = targetElem;

        this.listeners = new Map(); // key: type, val: array(callbacks)
        this.sourceElem.addEventListener("message", this.onMessage.bind(this), false);
    }

    onMessage(message) {
        try {
            const topic = message.data.topic;
            const callbacks = this.listeners.get(topic) || [];
            for (let callback of callbacks) {
                callback(message.data.data);
            }
        }
        catch (e) {
            console.error(e);
        }
    };

    sendMessage(message) { // {topic:string, data: any}
        this.targetElem.postMessage(message, "*");
    }

    subscribe(topic, callback) {
        const callbacks = this.listeners.get(topic);
        if (callbacks) {
            callbacks.push(callback);
            this.listeners.set(topic, callbacks);
        } else {
            this.listeners.set(topic, [callback])
        }
    }
}