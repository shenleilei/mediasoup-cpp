import dgram from 'node:dgram';
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';
import { WsJsonClient } from './ws_json_client.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const host = process.argv[2] || '127.0.0.1';
const port = Number(process.argv[3] || '1770');
const roomId = process.argv[4] || `remote_plain_abs_${Date.now()}`;
const peerId = process.argv[5] || 'plain-publisher';
const holdMs = Number(process.argv[6] || '20000');
const startupDelayMs = Number(process.argv[7] || '5000');
const warmupKeyframeIterations = Number(process.argv[8] || '3');

const H264_FILE = path.join(__dirname, 'assets', 'plain_abs_capture_tiny.h264');
const MAX_PAYLOAD_BYTES = 1200;
const FRAME_INTERVAL_MS = 200;
const RTP_CLOCK_RATE = 90000;
const RTP_TIMESTAMP_STEP = RTP_CLOCK_RATE / (1000 / FRAME_INTERVAL_MS);

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function unixMsToNtp64(unixMs) {
  const unixNtpOffset = 0x83aa7e80n;
  const ntpFractionalUnit = 1n << 32n;
  const ms = BigInt(unixMs);
  const seconds = ms / 1000n + unixNtpOffset;
  const remainderMs = ms % 1000n;
  const fractions = (remainderMs * ntpFractionalUnit + 500n) / 1000n;
  return (seconds << 32n) | (fractions & 0xffffffffn);
}

function findStartCode(data, from) {
  for (let i = from; i + 3 < data.length; ++i) {
    if (data[i] === 0x00 && data[i + 1] === 0x00) {
      if (data[i + 2] === 0x01) {
        return { index: i, size: 3 };
      }
      if (i + 3 < data.length && data[i + 2] === 0x00 && data[i + 3] === 0x01) {
        return { index: i, size: 4 };
      }
    }
  }
  return null;
}

function parseAnnexBNalus(buffer) {
  const nalus = [];
  let cursor = 0;

  while (cursor < buffer.length) {
    const start = findStartCode(buffer, cursor);
    if (!start) {
      break;
    }
    const payloadStart = start.index + start.size;
    const next = findStartCode(buffer, payloadStart);
    const payloadEnd = next ? next.index : buffer.length;
    if (payloadEnd > payloadStart) {
      nalus.push(buffer.slice(payloadStart, payloadEnd));
    }
    cursor = payloadEnd;
  }

  return nalus;
}

function groupNalusIntoFrames(nalus) {
  const frames = [];
  let current = [];

  const flush = () => {
    if (current.length > 0) {
      frames.push(current);
      current = [];
    }
  };

  let sawAud = false;
  for (const nalu of nalus) {
    const naluType = nalu[0] & 0x1f;
    if (naluType === 9) {
      sawAud = true;
      flush();
      continue;
    }
    current.push(nalu);
  }
  if (sawAud) {
    flush();
    return frames;
  }

  // No AUD present. Group by access-unit boundaries using parameter-set /
  // keyframe starts and non-IDR VCL boundaries.
  current = [];
  for (const nalu of nalus) {
    const naluType = nalu[0] & 0x1f;
    const startsIdrFrame = naluType === 5 && current.length > 0;
    const startsNonIdrFrame = naluType === 1 && current.some(item => (item[0] & 0x1f) === 1);
    const startsParameterSetGroup =
      (naluType === 7 || naluType === 8) &&
      current.some(item => {
        const type = item[0] & 0x1f;
        return type === 1 || type === 5;
      });

    if (startsIdrFrame || startsNonIdrFrame || startsParameterSetGroup) {
      flush();
    }

    current.push(nalu);
  }
  flush();
  return frames;
}

function buildRtpHeader(payloadType, seq, timestamp, ssrc, marker, hasExtension) {
  const header = Buffer.alloc(12);
  header[0] = hasExtension ? 0x90 : 0x80;
  header[1] = (payloadType & 0x7f) | (marker ? 0x80 : 0x00);
  header.writeUInt16BE(seq & 0xffff, 2);
  header.writeUInt32BE(timestamp >>> 0, 4);
  header.writeUInt32BE(ssrc >>> 0, 8);
  return header;
}

function buildAbsCaptureExtension(absCaptureTimeExtId, captureUnixMs) {
  // One-byte RTP header extension:
  // - 4-byte BEDE header
  // - 1-byte extension header
  // - 16-byte abs-capture-time payload
  // - 3 bytes padding to 32-bit alignment
  const ext = Buffer.alloc(24, 0);
  ext.writeUInt16BE(0xbede, 0);
  ext.writeUInt16BE(5, 2);
  ext[4] = ((absCaptureTimeExtId & 0x0f) << 4) | 0x0f; // 16-byte extension
  const ntp64 = unixMsToNtp64(captureUnixMs);
  for (let i = 0; i < 8; ++i) {
    ext[5 + i] = Number((ntp64 >> BigInt((7 - i) * 8)) & 0xffn);
  }
  // Encode a zero estimated capture clock offset so page side gets an explicit 0 ms.
  for (let i = 0; i < 8; ++i) {
    ext[13 + i] = 0;
  }
  return ext;
}

function buildStapAPacket({
  payloadType,
  seq,
  timestamp,
  ssrc,
  absCaptureTimeExtId,
  captureUnixMs,
  nalus,
  marker = false,
}) {
  let maxNri = 0;
  let total = 1;
  for (const nalu of nalus) {
    maxNri = Math.max(maxNri, nalu[0] & 0x60);
    total += 2 + nalu.length;
  }

  const stapPayload = Buffer.alloc(total);
  stapPayload[0] = maxNri | 24;
  let offset = 1;
  for (const nalu of nalus) {
    stapPayload.writeUInt16BE(nalu.length, offset);
    offset += 2;
    nalu.copy(stapPayload, offset);
    offset += nalu.length;
  }

  const header = buildRtpHeader(payloadType, seq, timestamp, ssrc, marker, true);
  const extension = buildAbsCaptureExtension(absCaptureTimeExtId, captureUnixMs);
  return Buffer.concat([header, extension, stapPayload]);
}

function isVclNaluType(naluType) {
  return naluType >= 1 && naluType <= 5;
}

function packetizeNalu({
  nalu,
  payloadType,
  seqRef,
  timestamp,
  ssrc,
  absCaptureTimeExtId,
  captureUnixMs,
  isLastNaluOfFrame,
}) {
  const packets = [];
  const extension = buildAbsCaptureExtension(absCaptureTimeExtId, captureUnixMs);

  if (nalu.length <= MAX_PAYLOAD_BYTES) {
    const header = buildRtpHeader(payloadType, seqRef.value++, timestamp, ssrc, isLastNaluOfFrame, true);
    packets.push(Buffer.concat([header, extension, nalu]));
    return packets;
  }

  const naluHeader = nalu[0];
  const fuIndicator = (naluHeader & 0xe0) | 28;
  let offset = 1;
  let first = true;

  while (offset < nalu.length) {
    const chunkSize = Math.min(MAX_PAYLOAD_BYTES - 2, nalu.length - offset);
    const end = (offset + chunkSize) >= nalu.length;
    const header = buildRtpHeader(payloadType, seqRef.value++, timestamp, ssrc, end && isLastNaluOfFrame, true);
    const fuHeader = Buffer.from([
      fuIndicator,
      (naluHeader & 0x1f) | (first ? 0x80 : 0x00) | (end ? 0x40 : 0x00),
    ]);
    const chunk = nalu.subarray(offset, offset + chunkSize);
    packets.push(Buffer.concat([header, extension, fuHeader, chunk]));
    offset += chunkSize;
    first = false;
  }

  return packets;
}

function packetizeFrame({
  frame,
  payloadType,
  seqRef,
  timestamp,
  ssrc,
  absCaptureTimeExtId,
  captureUnixMs,
}) {
  const packets = [];
  const nonVcl = [];
  const vcl = [];

  for (const nalu of frame) {
    const naluType = nalu[0] & 0x1f;
    if (isVclNaluType(naluType)) {
      vcl.push(nalu);
    } else {
      nonVcl.push(nalu);
    }
  }

  if (nonVcl.length > 0) {
    packets.push(buildStapAPacket({
      payloadType,
      seq: seqRef.value++,
      timestamp,
      ssrc,
      absCaptureTimeExtId,
      captureUnixMs,
      nalus: nonVcl,
      marker: false,
    }));
  }

  for (let i = 0; i < vcl.length; ++i) {
    const naluPackets = packetizeNalu({
      nalu: vcl[i],
      payloadType,
      seqRef,
      timestamp,
      ssrc,
      absCaptureTimeExtId,
      captureUnixMs,
      isLastNaluOfFrame: i === vcl.length - 1,
    });
    packets.push(...naluPackets);
  }

  return packets;
}

async function main() {
  const annexB = fs.readFileSync(H264_FILE);
  const nalus = parseAnnexBNalus(annexB);
  if (nalus.length === 0) {
    throw new Error(`no NAL units parsed from ${H264_FILE}`);
  }
  const frames = groupNalusIntoFrames(nalus);
  if (frames.length === 0) {
    throw new Error(`no access units grouped from ${H264_FILE}`);
  }
  console.log(JSON.stringify({
    h264File: H264_FILE,
    frameSummary: frames.map((frame, index) => ({
      frameIndex: index,
      nalTypes: frame.map(nalu => nalu[0] & 0x1f),
      sizes: frame.map(nalu => nalu.length),
    })),
  }));

  const ws = new WsJsonClient(host, port, '/ws');
  await ws.connect();
  const joinResp = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: { codecs: [], headerExtensions: [] },
  });
  if (!joinResp.ok) {
    throw new Error(`join failed: ${JSON.stringify(joinResp)}`);
  }

  const socket = dgram.createSocket('udp4');
  await new Promise((resolve, reject) => {
    socket.once('error', reject);
    socket.bind(0, '0.0.0.0', resolve);
  });

  const senderPort = socket.address().port;
  const publishResp = await ws.request('plainPublish', {
    videoSsrc: 11111111,
    enableAudio: false,
    senderIp: '127.0.0.1',
    senderPort,
  });
  if (!publishResp.ok) {
    throw new Error(`plainPublish failed: ${JSON.stringify(publishResp)}`);
  }

  const data = publishResp.data;
  const payloadType = data.videoPt;
  const absCaptureTimeExtId = data.videoAbsCaptureTimeExtId;
  if (!absCaptureTimeExtId) {
    throw new Error(`plainPublish did not return videoAbsCaptureTimeExtId: ${JSON.stringify(data)}`);
  }

  await sleep(startupDelayMs);

  let seq = 1;
  let timestamp = 3000;
  let frameIndex = 0;
  let iteration = 0;
  const startAt = Date.now();
  while (Date.now() - startAt < holdMs) {
    const captureMs = Date.now() - 120;
    const frame = frames[frameIndex];

    const packets = packetizeFrame({
      frame,
      payloadType,
      seqRef: { value: seq },
      timestamp,
      ssrc: 11111111,
      absCaptureTimeExtId,
      captureUnixMs: captureMs,
    });
    seq += packets.length;
    for (const packet of packets) {
      await new Promise((resolve, reject) => {
        socket.send(packet, data.port, '127.0.0.1', error => error ? reject(error) : resolve());
      });
    }

    // Start with a few repeated keyframes so late subscribers can lock quickly,
    // then loop the whole access-unit sequence so browser stats keep advancing.
    if (iteration < warmupKeyframeIterations - 1) {
      frameIndex = 0;
    } else {
      frameIndex = (frameIndex + 1) % frames.length;
    }
    iteration += 1;
    timestamp += RTP_TIMESTAMP_STEP;
    await sleep(FRAME_INTERVAL_MS);
  }

  console.log(JSON.stringify({ ok: true, roomId, holdMs, startupDelayMs, plainPublish: data }, null, 2));
  socket.close();
  ws.close();
}

main().catch(error => {
  console.error(error);
  process.exit(1);
});
