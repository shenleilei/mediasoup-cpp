"use strict";

class EventEmitter {
  constructor() {
    this._events = new Map();
    this._maxListeners = 10;
  }

  setMaxListeners(n) {
    this._maxListeners = Number.isFinite(n) && n >= 0 ? n : this._maxListeners;
    return this;
  }

  _listeners(eventName) {
    if (!this._events.has(eventName)) {
      this._events.set(eventName, []);
    }
    return this._events.get(eventName);
  }

  on(eventName, listener) {
    this._listeners(eventName).push(listener);
    return this;
  }

  addListener(eventName, listener) {
    return this.on(eventName, listener);
  }

  prependListener(eventName, listener) {
    this._listeners(eventName).unshift(listener);
    return this;
  }

  once(eventName, listener) {
    const wrapped = (...args) => {
      this.off(eventName, wrapped);
      return listener(...args);
    };
    wrapped.listener = listener;
    return this.on(eventName, wrapped);
  }

  prependOnceListener(eventName, listener) {
    const wrapped = (...args) => {
      this.off(eventName, wrapped);
      return listener(...args);
    };
    wrapped.listener = listener;
    return this.prependListener(eventName, wrapped);
  }

  off(eventName, listener) {
    const list = this._events.get(eventName);
    if (!list || list.length === 0) {
      return this;
    }
    const index = list.findIndex(item => item === listener || item.listener === listener);
    if (index !== -1) {
      list.splice(index, 1);
    }
    if (list.length === 0) {
      this._events.delete(eventName);
    }
    return this;
  }

  removeListener(eventName, listener) {
    return this.off(eventName, listener);
  }

  removeAllListeners(eventName) {
    if (typeof eventName === 'undefined') {
      this._events.clear();
      return this;
    }
    this._events.delete(eventName);
    return this;
  }

  emit(eventName, ...args) {
    const list = this._events.get(eventName);
    if (!list || list.length === 0) {
      return false;
    }
    for (const listener of [...list]) {
      listener(...args);
    }
    return true;
  }

  listenerCount(eventName) {
    return this._events.get(eventName)?.length || 0;
  }

  listeners(eventName) {
    return [...(this._events.get(eventName) || [])];
  }

  rawListeners(eventName) {
    return this.listeners(eventName);
  }
}

module.exports = { EventEmitter };
