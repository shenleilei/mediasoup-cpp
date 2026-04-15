// plain-client: PlainTransport MP4 推流客户端 (zero external WS dependency)
// Usage: ./plain-client SERVER_IP SERVER_PORT ROOM PEER file.mp4 [--copy]
// --copy: skip re-encoding, use original H264 (no QoS bitrate control)

#include <nlohmann/json.hpp>
#include "RtcpHandler.h"
#include "qos/QosController.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <random>

using json = nlohmann::json;
static std::atomic<bool> g_running{true};
static void onSignal(int) { g_running = false; }

// ═══════════════════════════════════════════════════════════
// Minimal WebSocket client (RFC 6455, text frames only)
// ═══════════════════════════════════════════════════════════

static std::string base64Encode(const unsigned char* data, int len) {
	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* mem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, mem);
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(b64, data, len);
	BIO_flush(b64);
	BUF_MEM* bptr;
	BIO_get_mem_ptr(b64, &bptr);
	std::string result(bptr->data, bptr->length);
	BIO_free_all(b64);
	return result;
}

struct WsClient {
	int fd = -1;

	bool connect(const std::string& host, int port, const std::string& path) {
		struct addrinfo hints{}, *res;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) return false;
		fd = socket(res->ai_family, SOCK_STREAM, 0);
		if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) { freeaddrinfo(res); return false; }
		freeaddrinfo(res);
		int flag = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

		// Generate random key
		unsigned char keyBytes[16];
		std::random_device rd;
		for (auto& b : keyBytes) b = rd() & 0xFF;
		std::string key = base64Encode(keyBytes, 16);

		// HTTP upgrade
		std::string req = "GET " + path + " HTTP/1.1\r\n"
			"Host: " + host + ":" + std::to_string(port) + "\r\n"
			"Upgrade: websocket\r\nConnection: Upgrade\r\n"
			"Sec-WebSocket-Key: " + key + "\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n";
		::send(fd, req.data(), req.size(), 0);

		// Read response
		char buf[4096];
		int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
		if (n <= 0) return false;
		buf[n] = 0;
		return strstr(buf, "101") != nullptr;
	}

	void sendText(const std::string& msg) {
		std::vector<uint8_t> frame;
		frame.push_back(0x81); // FIN + text
		// Mask bit set (client must mask)
		uint8_t maskKey[4];
		std::random_device rd;
		for (auto& b : maskKey) b = rd() & 0xFF;

		if (msg.size() < 126) {
			frame.push_back(0x80 | (uint8_t)msg.size());
		} else if (msg.size() < 65536) {
			frame.push_back(0x80 | 126);
			frame.push_back((msg.size() >> 8) & 0xFF);
			frame.push_back(msg.size() & 0xFF);
		} else {
			frame.push_back(0x80 | 127);
			for (int i = 7; i >= 0; i--)
				frame.push_back((msg.size() >> (i * 8)) & 0xFF);
		}
		frame.insert(frame.end(), maskKey, maskKey + 4);
		for (size_t i = 0; i < msg.size(); i++)
			frame.push_back(msg[i] ^ maskKey[i % 4]);
		::send(fd, frame.data(), frame.size(), 0);
	}

	std::string recvText(int timeoutMs = 10000) {
		struct pollfd pfd{fd, POLLIN, 0};
		if (poll(&pfd, 1, timeoutMs) <= 0) return "";

		uint8_t hdr[2];
		if (::recv(fd, hdr, 2, MSG_WAITALL) != 2) return "";
		bool masked = hdr[1] & 0x80;
		uint64_t len = hdr[1] & 0x7F;
		if (len == 126) {
			uint8_t ext[2];
			::recv(fd, ext, 2, MSG_WAITALL);
			len = (ext[0] << 8) | ext[1];
		} else if (len == 127) {
			uint8_t ext[8];
			::recv(fd, ext, 8, MSG_WAITALL);
			len = 0;
			for (int i = 0; i < 8; i++) len = (len << 8) | ext[i];
		}
		uint8_t mask[4] = {};
		if (masked) ::recv(fd, mask, 4, MSG_WAITALL);

		std::string data(len, 0);
		size_t got = 0;
		while (got < len) {
			int n = ::recv(fd, &data[got], len - got, 0);
			if (n <= 0) return "";
			got += n;
		}
		if (masked) for (size_t i = 0; i < len; i++) data[i] ^= mask[i % 4];
		return data;
	}

	json request(const std::string& method, const json& reqData, int timeoutMs = 5000) {
		static uint32_t nextId = 1;
		uint32_t id = nextId++;
		json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", reqData}};
		sendText(msg.dump());

		// Read until we get our response
		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		while (std::chrono::steady_clock::now() < deadline) {
			int remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
				deadline - std::chrono::steady_clock::now()).count();
			auto text = recvText(std::max(remaining, 1));
			if (text.empty()) continue;
			auto resp = json::parse(text, nullptr, false);
			if (resp.is_discarded()) continue;
			if (resp.value("response", false) && resp.value("id", 0u) == id) {
				if (!resp.value("ok", false))
					throw std::runtime_error(method + ": " + resp.value("error", "unknown"));
				return resp["data"];
			}
			// else notification, ignore for now
		}
		throw std::runtime_error("timeout: " + method);
	}

	void close() { if (fd >= 0) { ::close(fd); fd = -1; } }
};

// ═══════════════════════════════════════════════════════════
// RTP packetizer
// ═══════════════════════════════════════════════════════════

static constexpr int kMaxRtpPayload = 1200;

static void rtpHeader(uint8_t* buf, uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc, bool marker) {
	buf[0] = 0x80;
	buf[1] = (marker ? 0x80 : 0) | (pt & 0x7F);
	buf[2] = seq >> 8; buf[3] = seq & 0xFF;
	buf[4] = ts >> 24; buf[5] = ts >> 16; buf[6] = ts >> 8; buf[7] = ts;
	buf[8] = ssrc >> 24; buf[9] = ssrc >> 16; buf[10] = ssrc >> 8; buf[11] = ssrc;
}

// Global RTCP context for RTP packet store (NACK retransmission)
static RtcpContext* g_rtcp = nullptr;

static inline void rtpSendAndStore(int fd, const uint8_t* pkt, size_t len) {
	send(fd, pkt, len, 0);
	if (g_rtcp && len >= 12) {
		uint16_t seq = (pkt[2] << 8) | pkt[3];
		g_rtcp->videoStore.store(seq, pkt, len);
		g_rtcp->videoPacketCount++;
		g_rtcp->videoOctetCount += (len > 12) ? (len - 12) : 0;
	}
}

static void sendH264(int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	std::vector<std::pair<const uint8_t*, int>> nalus;
	int i = 0;
	while (i < size) {
		int scLen = 0;
		if (i + 3 <= size && data[i]==0 && data[i+1]==0 && data[i+2]==1) scLen = 3;
		else if (i + 4 <= size && data[i]==0 && data[i+1]==0 && data[i+2]==0 && data[i+3]==1) scLen = 4;
		if (scLen) { i += scLen; continue; }
		int start = i;
		i++;
		while (i < size) {
			if (i + 3 <= size && data[i]==0 && data[i+1]==0 && (data[i+2]==1 || (data[i+2]==0 && i+3<size && data[i+3]==1)))
				break;
			i++;
		}
		nalus.push_back({data + start, i - start});
	}

	for (size_t n = 0; n < nalus.size(); n++) {
		auto [nalu, nLen] = nalus[n];
		bool last = (n == nalus.size() - 1);
		if (nLen <= kMaxRtpPayload) {
			uint8_t pkt[12 + 1400];
			rtpHeader(pkt, pt, seq++, ts, ssrc, last);
			memcpy(pkt + 12, nalu, nLen);
			rtpSendAndStore(fd, pkt, 12 + nLen);
		} else {
			uint8_t naluHdr = nalu[0];
			int off = 1;
			bool first = true;
			while (off < nLen) {
				int chunk = std::min(kMaxRtpPayload - 2, nLen - off);
				bool end = (off + chunk >= nLen);
				uint8_t pkt[12 + 2 + 1400];
				rtpHeader(pkt, pt, seq++, ts, ssrc, end && last);
				pkt[12] = (naluHdr & 0x60) | 28;
				pkt[13] = (naluHdr & 0x1F) | (first ? 0x80 : 0) | (end ? 0x40 : 0);
				memcpy(pkt + 14, nalu + off, chunk);
				rtpSendAndStore(fd, pkt, 14 + chunk);
				off += chunk;
				first = false;
			}
		}
	}
}

static void sendOpus(int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	uint8_t pkt[12 + 4096];
	rtpHeader(pkt, pt, seq++, ts, ssrc, true);
	memcpy(pkt + 12, data, size);
	send(fd, pkt, 12 + size, 0);
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
	if (argc < 6) {
		fprintf(stderr, "Usage: %s SERVER_IP SERVER_PORT ROOM PEER file.mp4\n", argv[0]);
		return 1;
	}
	const char* serverIp = argv[1];
	int serverPort = atoi(argv[2]);
	const char* roomId = argv[3];
	const char* peerId = argv[4];
	const char* mp4Path = argv[5];
	bool copyMode = (argc > 6 && std::string(argv[6]) == "--copy");

	signal(SIGINT, onSignal);
	signal(SIGPIPE, SIG_IGN);

	// Open MP4
	AVFormatContext* fmtCtx = nullptr;
	if (avformat_open_input(&fmtCtx, mp4Path, nullptr, nullptr) < 0) {
		fprintf(stderr, "Cannot open %s\n", mp4Path); return 1;
	}
	avformat_find_stream_info(fmtCtx, nullptr);
	int vidIdx = -1, audIdx = -1;
	for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && vidIdx < 0) vidIdx = i;
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audIdx < 0) audIdx = i;
	}
	printf("MP4: video=%d audio=%d\n", vidIdx, audIdx);

	// Setup H264 bitstream filter (MP4 AVCC → Annex-B for RTP)
	const AVBitStreamFilter* bsf = av_bsf_get_by_name("h264_mp4toannexb");
	AVBSFContext* bsfCtx = nullptr;
	if (vidIdx >= 0 && bsf) {
		av_bsf_alloc(bsf, &bsfCtx);
		avcodec_parameters_copy(bsfCtx->par_in, fmtCtx->streams[vidIdx]->codecpar);
		bsfCtx->time_base_in = fmtCtx->streams[vidIdx]->time_base;
		av_bsf_init(bsfCtx);
	}

	// Audio transcode setup (AAC→Opus)
	AVCodecContext* adec = nullptr, *aenc = nullptr;
	AVFrame* aframe = nullptr;
	if (audIdx >= 0) {
		auto* par = fmtCtx->streams[audIdx]->codecpar;
		auto* dec = avcodec_find_decoder(par->codec_id);
		if (dec) {
			adec = avcodec_alloc_context3(dec);
			avcodec_parameters_to_context(adec, par);
			avcodec_open2(adec, dec, nullptr);
			auto* enc = avcodec_find_encoder(AV_CODEC_ID_OPUS);
			if (enc) {
				aenc = avcodec_alloc_context3(enc);
				aenc->sample_rate = 48000;
				aenc->channels = 2;
				aenc->channel_layout = AV_CH_LAYOUT_STEREO;
				aenc->bit_rate = 64000;
				aenc->sample_fmt = AV_SAMPLE_FMT_FLT;
				if (avcodec_open2(aenc, enc, nullptr) < 0) {
					avcodec_free_context(&aenc); aenc = nullptr;
					printf("WARN: Opus encoder init failed, audio disabled\n");
				}
			}
			aframe = av_frame_alloc();
		}
	}

	// Video decode → re-encode setup (for QoS bitrate control)
	AVCodecContext* vdec = nullptr, *venc = nullptr;
	AVFrame* vframe = nullptr;
	int encBitrate = 900000; // initial bitrate, QoS will adjust
	if (!copyMode && vidIdx >= 0) {
		auto* par = fmtCtx->streams[vidIdx]->codecpar;
		auto* dec = avcodec_find_decoder(par->codec_id);
		if (dec) {
			vdec = avcodec_alloc_context3(dec);
			avcodec_parameters_to_context(vdec, par);
			avcodec_open2(vdec, dec, nullptr);

			auto* enc = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (enc) {
				venc = avcodec_alloc_context3(enc);
				venc->width = par->width;
				venc->height = par->height;
				venc->pix_fmt = AV_PIX_FMT_YUV420P;
				venc->time_base = {1, 25};
				venc->framerate = {25, 1};
				venc->bit_rate = encBitrate;
				venc->rc_max_rate = encBitrate;
				venc->rc_buffer_size = encBitrate;
				venc->gop_size = 25;
				venc->max_b_frames = 0;
				av_opt_set(venc->priv_data, "preset", "ultrafast", 0);
				av_opt_set(venc->priv_data, "tune", "zerolatency", 0);
				av_opt_set(venc->priv_data, "profile", "baseline", 0);
				if (avcodec_open2(venc, enc, nullptr) < 0) {
					avcodec_free_context(&venc); venc = nullptr;
					printf("WARN: x264 encoder init failed, falling back to copy mode\n");
					copyMode = true;
				}
			}
			vframe = av_frame_alloc();
		}
	}
	printf("Mode: %s\n", copyMode ? "copy (no QoS bitrate control)" : "re-encode (QoS enabled)");

	// WebSocket connect
	WsClient ws;
	if (!ws.connect(serverIp, serverPort, "/ws")) {
		fprintf(stderr, "WS connect failed\n"); return 1;
	}
	printf("WS connected to %s:%d\n", serverIp, serverPort);

	try {
		// Join
		ws.request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", json::object()}
		});
		printf("Joined room=%s peer=%s\n", roomId, peerId);

		// PlainPublish
		uint32_t vSsrc = 11111111, aSsrc = 22222222;
		auto pub = ws.request("plainPublish", {{"videoSsrc", vSsrc}, {"audioSsrc", aSsrc}});
		std::string srvIp = pub["ip"];
		uint16_t srvPort = pub["port"];
		uint8_t vPt = pub["videoPt"], aPt = pub["audioPt"];
		// Use the signaling server IP for UDP too (announced IP may be public,
		// but we're connecting from the same network as the signaling endpoint)
		std::string udpIp = serverIp;
		printf("Publish → %s:%d (announced=%s) vPt=%d aPt=%d\n",
			udpIp.c_str(), srvPort, srvIp.c_str(), vPt, aPt);

		// UDP socket
		int udp = socket(AF_INET, SOCK_DGRAM, 0);
		sockaddr_in dst{};
		dst.sin_family = AF_INET;
		dst.sin_port = htons(srvPort);
		inet_pton(AF_INET, udpIp.c_str(), &dst.sin_addr);
		::connect(udp, (sockaddr*)&dst, sizeof(dst));

		// Set non-blocking for RTCP recv
		{
			int flags = fcntl(udp, F_GETFL, 0);
			fcntl(udp, F_SETFL, flags | O_NONBLOCK);
		}

		uint16_t vSeq = 0, aSeq = 0;

		// Setup RTCP context (SR + NACK + PLI)
		RtcpContext rtcp;
		rtcp.videoSsrc = vSsrc;
		rtcp.audioSsrc = aSsrc;
		rtcp.videoPt = vPt;
		rtcp.videoSeqPtr = &vSeq;
		rtcp.sendH264Fn = sendH264;
		g_rtcp = &rtcp;

		// Send probe packets for comedia
		for (int i = 0; i < 5; i++) {
			uint8_t dummy[12 + 1];
			rtpHeader(dummy, vPt, vSeq++, 0, vSsrc, false);
			dummy[12] = 0;
			send(udp, dummy, 13, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
		printf("Sent probe packets for comedia\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Setup QoS controller (only in re-encode mode)
		std::unique_ptr<qos::PublisherQosController> qosCtrl;
		if (!copyMode && venc) {
			qosCtrl = std::make_unique<qos::PublisherQosController>(qos::PublisherQosController::Options{
				qos::Source::Camera, "video", pub.value("videoProdId", ""), 0,
				[&](const qos::PlannedAction& action) -> bool {
					if (action.type == qos::ActionType::SetEncodingParameters && action.encodingParameters.has_value()) {
						auto& ep = *action.encodingParameters;
						if (ep.maxBitrateBps.has_value() && venc) {
							encBitrate = *ep.maxBitrateBps;
							venc->bit_rate = encBitrate;
							venc->rc_max_rate = encBitrate;
							printf("[QoS] encoder bitrate → %d bps\n", encBitrate);
						}
					} else if (action.type == qos::ActionType::EnterAudioOnly) {
						printf("[QoS] → audio only mode\n");
					} else if (action.type == qos::ActionType::ExitAudioOnly) {
						printf("[QoS] → exit audio only\n");
					}
					return true;
				},
				[&](const nlohmann::json& snap) {
					try { ws.request("clientStats", snap, 2); } catch (...) {}
				}
			});
		}

		int64_t lastQosSampleMs = 0;

		// Stream
		AVPacket* pkt = av_packet_alloc();
		auto t0 = std::chrono::steady_clock::now();
		double firstPts = -1;
		int frameCnt = 0;

		while (g_running && av_read_frame(fmtCtx, pkt) >= 0) {
			auto* st = fmtCtx->streams[pkt->stream_index];
			double pts = pkt->pts * av_q2d(st->time_base);
			if (firstPts < 0) firstPts = pts;
			double elapsed = pts - firstPts;

			// Pace
			auto target = t0 + std::chrono::microseconds((int64_t)(elapsed * 1e6));
			std::this_thread::sleep_until(target);

			if (pkt->stream_index == vidIdx) {
				uint32_t rtpTs = (uint32_t)(pts * 90000);
				rtcp.lastVideoRtpTs = rtpTs;

				if (!copyMode && vdec && venc) {
					// Re-encode path: decode → encode with QoS-controlled bitrate
					if (avcodec_send_packet(vdec, pkt) == 0) {
						while (avcodec_receive_frame(vdec, vframe) == 0) {
							vframe->pict_type = AV_PICTURE_TYPE_NONE;
							if (avcodec_send_frame(venc, vframe) == 0) {
								AVPacket* encPkt = av_packet_alloc();
								while (avcodec_receive_packet(venc, encPkt) == 0) {
									// encPkt is already Annex-B from x264
									if (encPkt->flags & AV_PKT_FLAG_KEY)
										rtcp.cacheKeyframe(encPkt->data, encPkt->size, rtpTs);
									sendH264(udp, encPkt->data, encPkt->size, vPt, rtpTs, vSsrc, vSeq);
									av_packet_unref(encPkt);
								}
								av_packet_free(&encPkt);
							}
						}
					}
				} else {
					// Copy path: BSF convert AVCC→Annex-B
					if (bsfCtx) {
						AVPacket* bsfPkt = av_packet_alloc();
						av_packet_ref(bsfPkt, pkt);
						av_bsf_send_packet(bsfCtx, bsfPkt);
						while (av_bsf_receive_packet(bsfCtx, bsfPkt) == 0) {
							if (bsfPkt->flags & AV_PKT_FLAG_KEY)
								rtcp.cacheKeyframe(bsfPkt->data, bsfPkt->size, rtpTs);
							sendH264(udp, bsfPkt->data, bsfPkt->size, vPt, rtpTs, vSsrc, vSeq);
							av_packet_unref(bsfPkt);
						}
						av_packet_free(&bsfPkt);
					} else {
						if (pkt->flags & AV_PKT_FLAG_KEY)
							rtcp.cacheKeyframe(pkt->data, pkt->size, rtpTs);
						sendH264(udp, pkt->data, pkt->size, vPt, rtpTs, vSsrc, vSeq);
					}
				}

				// Process incoming RTCP (PLI + NACK) and send periodic SR
				rtcp.processIncomingRtcp(udp);
				rtcp.maybeSendSR(udp);

				// QoS sampling (every 1s)
				if (qosCtrl) {
					auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::steady_clock::now().time_since_epoch()).count();
					if (now - lastQosSampleMs >= 1000) {
						lastQosSampleMs = now;
						qos::RawSenderSnapshot snap;
						snap.timestampMs = now;
						snap.bytesSent = rtcp.videoOctetCount;
						snap.packetsSent = rtcp.videoPacketCount;
						snap.packetsLost = 0; // TODO: extract from RTCP RR
						snap.targetBitrateBps = encBitrate;
						snap.configuredBitrateBps = encBitrate;
						snap.roundTripTimeMs = -1; // TODO: extract from RTCP RR
						qosCtrl->onSample(snap);
					}
				}

				if (++frameCnt % 100 == 0)
					printf("sent %d frames [nack=%d pli=%d br=%dk level=%d]\n",
						frameCnt, rtcp.nackRetransmitted, rtcp.pliResponded,
						encBitrate / 1000,
						qosCtrl ? qosCtrl->currentLevel() : -1);
			} else if (pkt->stream_index == audIdx && adec && aenc) {
				if (avcodec_send_packet(adec, pkt) == 0) {
					while (avcodec_receive_frame(adec, aframe) == 0) {
						if (avcodec_send_frame(aenc, aframe) == 0) {
							AVPacket* ep = av_packet_alloc();
							while (avcodec_receive_packet(aenc, ep) == 0) {
								sendOpus(udp, ep->data, ep->size, aPt, (uint32_t)(pts * 48000), aSsrc, aSeq);
								rtcp.onAudioRtpSent(ep->size, (uint32_t)(pts * 48000));
								av_packet_unref(ep);
							}
							av_packet_free(&ep);
						}
					}
				}
			}
			av_packet_unref(pkt);
		}

		g_rtcp = nullptr;
		av_packet_free(&pkt);
		::close(udp);
		printf("Done: %d video frames sent\n", frameCnt);

	} catch (const std::exception& e) {
		fprintf(stderr, "Error: %s\n", e.what());
	}

	ws.close();
	if (bsfCtx) av_bsf_free(&bsfCtx);
	if (vframe) av_frame_free(&vframe);
	if (venc) avcodec_free_context(&venc);
	if (vdec) avcodec_free_context(&vdec);
	if (aframe) av_frame_free(&aframe);
	if (aenc) avcodec_free_context(&aenc);
	if (adec) avcodec_free_context(&adec);
	avformat_close_input(&fmtCtx);
	return 0;
}
