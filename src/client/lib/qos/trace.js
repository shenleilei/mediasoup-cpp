"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.QosTraceBuffer = void 0;
exports.createQosTraceBuffer = createQosTraceBuffer;
const constants_1 = require("./constants");
/**
 * Fixed-capacity trace buffer for QoS diagnostics.
 */
class QosTraceBuffer {
    constructor(capacity = constants_1.DEFAULT_TRACE_BUFFER_SIZE) {
        this._entries = [];
        this._capacity = Math.max(1, Math.floor(capacity));
    }
    get capacity() {
        return this._capacity;
    }
    get size() {
        return this._entries.length;
    }
    append(entry) {
        this._entries.push(entry);
        if (this._entries.length > this._capacity) {
            this._entries.splice(0, this._entries.length - this._capacity);
        }
    }
    getEntries() {
        return this._entries.slice();
    }
    clear() {
        this._entries.length = 0;
    }
    appendTraceEntry(entry) {
        this.append(entry);
    }
    getTraceEntries() {
        return this.getEntries();
    }
    clearTrace() {
        this.clear();
    }
}
exports.QosTraceBuffer = QosTraceBuffer;
function createQosTraceBuffer(capacity) {
    return new QosTraceBuffer(capacity);
}
