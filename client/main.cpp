// plain-client: PlainTransport MP4 推流客户端
// 用法: ./plain-client ws://SERVER:PORT/ws ROOM PEER file.mp4
//
// 依赖: libwebsockets, ffmpeg (libavformat/libavcodec/libavutil), nlohmann_json
// 编译: cd client && mkdir build && cd build && cmake .. && make

#include <nlohmann/json.hpp>
#include <libwebsockets.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;
static std::atomic<bool> g_running{true};
static void onSignal(int) { g_running = false; }

// ═══════════════════════════════════════════════════════════
// 1. WebSocket signaling client (libwebsockets)
// ═══════════════════════════════════════════════════════════

struct WsClient {
	struct lws_context* ctx = nullptr;
	struct lws* wsi = nullptr;
	std::mutex mu;
	std::condition_variable cv;
	bool connected = false;
	uint32_t nextId = 1;

	// pending responses: id → json
	std::map<uint32_t, json> responses;
	std::queue<std::string> sendQueue;
	std::string recvBuf;

	// notification callback
	std::function<void(const json&)> onNotification;

	json request(const std::string& method, const json& data, int timeoutSec = 5) {
		uint32_t id;
		{
			std::lock_guard<std::mutex> lk(mu);
			id = nextId++;
			json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", data}};
			sendQueue.push(msg.dump());
		}
		lws_callback_on_writable(wsi);

		// Wait for response
		std::unique_lock<std::mutex> lk(mu);
		auto ok = cv.wait_for(lk, std::chrono::seconds(timeoutSec), [&] {
			return responses.count(id) > 0;
		});
		if (!ok) throw std::runtime_error("timeout waiting for " + method);
		json resp = std::move(responses[id]);
		responses.erase(id);
		if (!resp.value("ok", false))
			throw std::runtime_error(method + " failed: " + resp.value("error", "unknown"));
		return resp["data"];
	}

	void handleMessage(const std::string& text) {
		auto msg = json::parse(text, nullptr, false);
		if (msg.is_discarded()) return;

		if (msg.value("response", false)) {
			std::lock_guard<std::mutex> lk(mu);
			responses[msg["id"].get<uint32_t>()] = msg;
			cv.notify_all();
		} else if (msg.value("notification", false) && onNotification) {
			onNotification(msg);
		}
	}
};

static WsClient* g_ws = nullptr;

static int wsCallback(struct lws* wsi, enum lws_callback_reasons reason,
	void* /*user*/, void* in, size_t len)
{
	if (!g_ws) return 0;
	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		g_ws->connected = true;
		g_ws->cv.notify_all();
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		g_ws->recvBuf.append((char*)in, len);
		if (lws_is_final_fragment(wsi)) {
			g_ws->handleMessage(g_ws->recvBuf);
			g_ws->recvBuf.clear();
		}
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE: {
		std::lock_guard<std::mutex> lk(g_ws->mu);
		while (!g_ws->sendQueue.empty()) {
			auto& msg = g_ws->sendQueue.front();
			std::vector<uint8_t> buf(LWS_PRE + msg.size());
			memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
			lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
			g_ws->sendQueue.pop();
		}
		break;
	}
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	case LWS_CALLBACK_CLIENT_CLOSED:
		g_ws->connected = false;
		g_running = false;
		g_ws->cv.notify_all();
		break;
	default:
		break;
	}
	return 0;
}

static const struct lws_protocols protocols[] = {
	{"ws", wsCallback, 0, 65536}, {nullptr, nullptr, 0, 0}
};

// ═══════════════════════════════════════════════════════════
// 2. RTP packetizer
// ═══════════════════════════════════════════════════════════

struct RtpHeader {
	uint8_t buf[12];
	void init(uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc, bool marker = false) {
		buf[0] = 0x80; // V=2
		buf[1] = (marker ? 0x80 : 0) | (pt & 0x7F);
		buf[2] = (seq >> 8) & 0xFF;
		buf[3] = seq & 0xFF;
		buf[4] = (ts >> 24) & 0xFF; buf[5] = (ts >> 16) & 0xFF;
		buf[6] = (ts >> 8) & 0xFF;  buf[7] = ts & 0xFF;
		buf[8] = (ssrc >> 24) & 0xFF; buf[9] = (ssrc >> 16) & 0xFF;
		buf[10] = (ssrc >> 8) & 0xFF; buf[11] = ssrc & 0xFF;
	}
};

static constexpr int kMaxRtpPayload = 1200;

// Send H264 NALUs over RTP with FU-A fragmentation
static void sendH264Frame(int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	// Skip Annex-B start codes, split into NALUs
	std::vector<std::pair<const uint8_t*, int>> nalus;
	int i = 0;
	while (i < size) {
		// Find start code
		int scLen = 0;
		if (i + 3 < size && data[i] == 0 && data[i+1] == 0 && data[i+2] == 1) scLen = 3;
		else if (i + 4 < size && data[i] == 0 && data[i+1] == 0 && data[i+2] == 0 && data[i+3] == 1) scLen = 4;
		if (scLen > 0) { i += scLen; continue; }
		// Find end of NALU
		int start = i;
		while (i < size) {
			if (i + 3 < size && data[i] == 0 && data[i+1] == 0 && (data[i+2] == 1 || (data[i+2] == 0 && i+3 < size && data[i+3] == 1)))
				break;
			i++;
		}
		if (i > start) nalus.push_back({data + start, i - start});
	}

	for (size_t n = 0; n < nalus.size(); n++) {
		auto [nalu, naluLen] = nalus[n];
		bool lastNalu = (n == nalus.size() - 1);

		if (naluLen <= kMaxRtpPayload) {
			// Single NALU packet
			uint8_t pkt[12 + kMaxRtpPayload];
			RtpHeader hdr;
			hdr.init(pt, seq++, ts, ssrc, lastNalu);
			memcpy(pkt, hdr.buf, 12);
			memcpy(pkt + 12, nalu, naluLen);
			send(fd, pkt, 12 + naluLen, 0);
		} else {
			// FU-A fragmentation
			uint8_t naluType = nalu[0] & 0x1F;
			uint8_t nri = nalu[0] & 0x60;
			int offset = 1; // skip NALU header byte
			bool first = true;
			while (offset < naluLen) {
				int chunkLen = std::min(kMaxRtpPayload - 2, naluLen - offset);
				bool last = (offset + chunkLen >= naluLen);

				uint8_t pkt[12 + 2 + kMaxRtpPayload];
				RtpHeader hdr;
				hdr.init(pt, seq++, ts, ssrc, last && lastNalu);
				memcpy(pkt, hdr.buf, 12);
				pkt[12] = nri | 28; // FU indicator: type=28 (FU-A)
				pkt[13] = naluType | (first ? 0x80 : 0) | (last ? 0x40 : 0); // FU header
				memcpy(pkt + 14, nalu + offset, chunkLen);
				send(fd, pkt, 14 + chunkLen, 0);

				offset += chunkLen;
				first = false;
			}
		}
	}
}

// Send Opus frame over RTP (single packet, no fragmentation needed)
static void sendOpusFrame(int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	uint8_t pkt[12 + 4096];
	RtpHeader hdr;
	hdr.init(pt, seq++, ts, ssrc, true);
	memcpy(pkt, hdr.buf, 12);
	memcpy(pkt + 12, data, size);
	send(fd, pkt, 12 + size, 0);
}

// ═══════════════════════════════════════════════════════════
// 3. MP4 reader + AAC→Opus transcoder
// ═══════════════════════════════════════════════════════════

struct Mp4Reader {
	AVFormatContext* fmtCtx = nullptr;
	int videoIdx = -1, audioIdx = -1;
	// Audio transcode: AAC decoder + Opus encoder
	AVCodecContext* audioDecCtx = nullptr;
	AVCodecContext* audioEncCtx = nullptr;
	AVFrame* audioFrame = nullptr;

	bool open(const char* path) {
		if (avformat_open_input(&fmtCtx, path, nullptr, nullptr) < 0) return false;
		if (avformat_find_stream_info(fmtCtx, nullptr) < 0) return false;

		for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
			auto* par = fmtCtx->streams[i]->codecpar;
			if (par->codec_type == AVMEDIA_TYPE_VIDEO && videoIdx < 0) videoIdx = i;
			if (par->codec_type == AVMEDIA_TYPE_AUDIO && audioIdx < 0) audioIdx = i;
		}

		// Setup audio transcoder if audio track exists
		if (audioIdx >= 0) {
			auto* par = fmtCtx->streams[audioIdx]->codecpar;
			auto* dec = avcodec_find_decoder(par->codec_id);
			if (dec) {
				audioDecCtx = avcodec_alloc_context3(dec);
				avcodec_parameters_to_context(audioDecCtx, par);
				avcodec_open2(audioDecCtx, dec, nullptr);

				auto* enc = avcodec_find_encoder(AV_CODEC_ID_OPUS);
				if (enc) {
					audioEncCtx = avcodec_alloc_context3(enc);
					audioEncCtx->sample_rate = 48000;
					audioEncCtx->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
					audioEncCtx->bit_rate = 64000;
					audioEncCtx->sample_fmt = AV_SAMPLE_FMT_FLT;
					avcodec_open2(audioEncCtx, enc, nullptr);
				}
				audioFrame = av_frame_alloc();
			}
		}
		return videoIdx >= 0;
	}

	void close() {
		if (audioFrame) av_frame_free(&audioFrame);
		if (audioEncCtx) avcodec_free_context(&audioEncCtx);
		if (audioDecCtx) avcodec_free_context(&audioDecCtx);
		if (fmtCtx) avformat_close_input(&fmtCtx);
	}
};

// ═══════════════════════════════════════════════════════════
// 4. Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
	if (argc < 5) {
		fprintf(stderr, "Usage: %s ws://host:port/ws ROOM PEER file.mp4\n", argv[0]);
		return 1;
	}
	const char* wsUrl = argv[1];
	const char* roomId = argv[2];
	const char* peerId = argv[3];
	const char* mp4Path = argv[4];

	signal(SIGINT, onSignal);
	signal(SIGPIPE, SIG_IGN);

	// ── Open MP4 ──
	Mp4Reader mp4;
	if (!mp4.open(mp4Path)) {
		fprintf(stderr, "Failed to open %s\n", mp4Path);
		return 1;
	}
	printf("Opened %s (video=%d audio=%d)\n", mp4Path, mp4.videoIdx, mp4.audioIdx);

	// ── Connect WebSocket ──
	WsClient ws;
	g_ws = &ws;

	struct lws_context_creation_info ctxInfo{};
	ctxInfo.port = CONTEXT_PORT_NO_LISTEN;
	ctxInfo.protocols = protocols;
	ctxInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	ws.ctx = lws_create_context(&ctxInfo);

	// Parse URL
	char host[256], path[256];
	int port = 0, ssl = 0;
	lws_parse_uri(const_cast<char*>(wsUrl), (const char**)&(const char*){nullptr},
		(const char**)&host, &port, (const char**)&path);
	// Simple parse
	std::string url(wsUrl);
	ssl = url.find("wss://") == 0 ? 1 : 0;
	auto hostStart = url.find("://") + 3;
	auto portStart = url.find(':', hostStart);
	auto pathStart = url.find('/', hostStart);
	std::string hostStr = url.substr(hostStart, (portStart != std::string::npos ? portStart : pathStart) - hostStart);
	port = portStart != std::string::npos ? std::stoi(url.substr(portStart + 1, pathStart - portStart - 1)) : (ssl ? 443 : 80);
	std::string pathStr = pathStart != std::string::npos ? url.substr(pathStart) : "/ws";

	struct lws_client_connect_info ccInfo{};
	ccInfo.context = ws.ctx;
	ccInfo.address = hostStr.c_str();
	ccInfo.port = port;
	ccInfo.path = pathStr.c_str();
	ccInfo.host = hostStr.c_str();
	ccInfo.origin = hostStr.c_str();
	ccInfo.protocol = "ws";
	ccInfo.ssl_connection = ssl;
	ws.wsi = lws_client_connect_via_info(&ccInfo);

	// Service thread
	std::thread wsThread([&] {
		while (g_running) lws_service(ws.ctx, 50);
	});

	// Wait for connection
	{
		std::unique_lock<std::mutex> lk(ws.mu);
		ws.cv.wait_for(lk, std::chrono::seconds(5), [&] { return ws.connected; });
	}
	if (!ws.connected) {
		fprintf(stderr, "WebSocket connect failed\n");
		g_running = false;
		wsThread.join();
		return 1;
	}
	printf("WebSocket connected\n");

	try {
		// ── Join ──
		auto joinResp = ws.request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", json::object()}
		});
		printf("Joined room %s\n", roomId);

		// ── PlainPublish ──
		uint32_t videoSsrc = 11111111, audioSsrc = 22222222;
		auto pubResp = ws.request("plainPublish", {
			{"videoSsrc", videoSsrc}, {"audioSsrc", audioSsrc}
		});
		std::string srvIp = pubResp["ip"];
		uint16_t srvPort = pubResp["port"];
		uint8_t videoPt = pubResp["videoPt"];
		uint8_t audioPt = pubResp["audioPt"];
		printf("PlainPublish OK → %s:%d (videoPt=%d audioPt=%d)\n",
			srvIp.c_str(), srvPort, videoPt, audioPt);

		// ── Create UDP socket ──
		int udpFd = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(srvPort);
		inet_pton(AF_INET, srvIp.c_str(), &addr.sin_addr);
		connect(udpFd, (struct sockaddr*)&addr, sizeof(addr));
		printf("UDP connected to %s:%d\n", srvIp.c_str(), srvPort);

		// ── Stream MP4 ──
		uint16_t videoSeq = 0, audioSeq = 0;
		AVPacket* pkt = av_packet_alloc();
		auto startTime = std::chrono::steady_clock::now();
		int64_t firstPts = -1;

		while (g_running && av_read_frame(mp4.fmtCtx, pkt) >= 0) {
			auto* st = mp4.fmtCtx->streams[pkt->stream_index];
			double ptsSec = pkt->pts * av_q2d(st->time_base);
			if (firstPts < 0) firstPts = pkt->pts;

			// Pace: sleep until it's time to send this frame
			double elapsed = ptsSec - (firstPts * av_q2d(st->time_base));
			auto target = startTime + std::chrono::microseconds((int64_t)(elapsed * 1e6));
			std::this_thread::sleep_until(target);

			if (pkt->stream_index == mp4.videoIdx) {
				uint32_t rtpTs = (uint32_t)(ptsSec * 90000);
				sendH264Frame(udpFd, pkt->data, pkt->size,
					videoPt, rtpTs, videoSsrc, videoSeq);
			} else if (pkt->stream_index == mp4.audioIdx && mp4.audioDecCtx && mp4.audioEncCtx) {
				// Transcode AAC → Opus
				if (avcodec_send_packet(mp4.audioDecCtx, pkt) == 0) {
					while (avcodec_receive_frame(mp4.audioDecCtx, mp4.audioFrame) == 0) {
						if (avcodec_send_frame(mp4.audioEncCtx, mp4.audioFrame) == 0) {
							AVPacket* encPkt = av_packet_alloc();
							while (avcodec_receive_packet(mp4.audioEncCtx, encPkt) == 0) {
								uint32_t rtpTs = (uint32_t)(ptsSec * 48000);
								sendOpusFrame(udpFd, encPkt->data, encPkt->size,
									audioPt, rtpTs, audioSsrc, audioSeq);
								av_packet_unref(encPkt);
							}
							av_packet_free(&encPkt);
						}
					}
				}
			}
			av_packet_unref(pkt);
		}
		av_packet_free(&pkt);
		::close(udpFd);
		printf("Streaming finished\n");

		// ── Leave ──
		try { ws.request("leave", {}, 2); } catch (...) {}

	} catch (const std::exception& e) {
		fprintf(stderr, "Error: %s\n", e.what());
	}

	g_running = false;
	wsThread.join();
	lws_context_destroy(ws.ctx);
	mp4.close();
	return 0;
}
