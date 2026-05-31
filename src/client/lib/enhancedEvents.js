"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.EnhancedEventEmitter = void 0;
const Logger_1 = require("./Logger");
const enhancedEventEmitterLogger = new Logger_1.Logger('EnhancedEventEmitter');
class BaseEventEmitter {
    constructor() {
        this._events = new Map();
        this._maxListeners = Infinity;
    }
    setMaxListeners(value) {
        this._maxListeners = value;
        return this;
    }
    emit(eventName, ...args) {
        const listeners = this._events.get(eventName);
        if (!listeners || listeners.length === 0) {
            return false;
        }
        for (const listener of [...listeners]) {
            listener.apply(this, args);
        }
        return true;
    }
    on(eventName, listener) {
        const listeners = this._events.get(eventName) ?? [];
        listeners.push(listener);
        this._events.set(eventName, listeners);
        return this;
    }
    off(eventName, listener) {
        const listeners = this._events.get(eventName);
        if (!listeners) {
            return this;
        }
        this._events.set(eventName, listeners.filter(item => item !== listener));
        return this;
    }
    prependListener(eventName, listener) {
        const listeners = this._events.get(eventName) ?? [];
        listeners.unshift(listener);
        this._events.set(eventName, listeners);
        return this;
    }
    once(eventName, listener) {
        const wrapped = (...args) => {
            this.off(eventName, wrapped);
            listener(...args);
        };
        return this.on(eventName, wrapped);
    }
    prependOnceListener(eventName, listener) {
        const wrapped = (...args) => {
            this.off(eventName, wrapped);
            listener(...args);
        };
        return this.prependListener(eventName, wrapped);
    }
    removeListener(eventName, listener) {
        return this.off(eventName, listener);
    }
    removeAllListeners(eventName) {
        if (typeof eventName === 'undefined') {
            this._events.clear();
        }
        else {
            this._events.delete(eventName);
        }
        return this;
    }
    listenerCount(eventName) {
        return (this._events.get(eventName) ?? []).length;
    }
    listeners(eventName) {
        return [...(this._events.get(eventName) ?? [])];
    }
    rawListeners(eventName) {
        return this.listeners(eventName);
    }
}
class EnhancedEventEmitter extends BaseEventEmitter {
    constructor() {
        super();
        this.setMaxListeners(Infinity);
    }
    emit(eventName, ...args) {
        return super.emit(eventName, ...args);
    }
    /**
     * Special addition to the EventEmitter API.
     */
    safeEmit(eventName, ...args) {
        try {
            return super.emit(eventName, ...args);
        }
        catch (error) {
            enhancedEventEmitterLogger.error('safeEmit() | event listener threw an error [eventName:%s]:%o', eventName, error);
            try {
                super.emit('listenererror', eventName, error);
            }
            catch (error2) {
                // Ignore it.
            }
            return Boolean(super.listenerCount(eventName));
        }
    }
    on(eventName, listener) {
        super.on(eventName, listener);
        return this;
    }
    off(eventName, listener) {
        super.off(eventName, listener);
        return this;
    }
    addListener(eventName, listener) {
        super.on(eventName, listener);
        return this;
    }
    prependListener(eventName, listener) {
        super.prependListener(eventName, listener);
        return this;
    }
    once(eventName, listener) {
        super.once(eventName, listener);
        return this;
    }
    prependOnceListener(eventName, listener) {
        super.prependOnceListener(eventName, listener);
        return this;
    }
    removeListener(eventName, listener) {
        super.off(eventName, listener);
        return this;
    }
    removeAllListeners(eventName) {
        super.removeAllListeners(eventName);
        return this;
    }
    listenerCount(eventName) {
        return super.listenerCount(eventName);
    }
    listeners(eventName) {
        return super.listeners(eventName);
    }
    rawListeners(eventName) {
        return super.rawListeners(eventName);
    }
}
exports.EnhancedEventEmitter = EnhancedEventEmitter;
