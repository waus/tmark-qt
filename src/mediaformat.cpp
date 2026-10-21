#include "mediaformat.hpp"

#include <QFile>
#include <QImageReader>
#ifndef __EMSCRIPTEN__
#include <QImage>
#include <QScopeGuard>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#endif

#ifndef __EMSCRIPTEN__
namespace {

bool decodeAvif(const QString& path) {
    AVFormatContext* format = nullptr;
    const QByteArray name = QFile::encodeName(path);
    if (avformat_open_input(&format, name.constData(), nullptr, nullptr) < 0) return false;
    const auto closeFormat = qScopeGuard([&] { avformat_close_input(&format); });
    if (avformat_find_stream_info(format, nullptr) < 0) return false;
    const int index = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0 || format->streams[index]->codecpar->codec_id != AV_CODEC_ID_AV1) return false;
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_AV1);
    if (!decoder) return false;
    AVCodecContext* codec = avcodec_alloc_context3(decoder);
    if (!codec) return false;
    const auto closeCodec = qScopeGuard([&] { avcodec_free_context(&codec); });
    codec->max_pixels = 32'000'000;
    if (avcodec_parameters_to_context(codec, format->streams[index]->codecpar) < 0 || avcodec_open2(codec, decoder, nullptr) < 0) return false;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (!packet || !frame) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        return false;
    }
    const auto freePacket = qScopeGuard([&] { av_packet_free(&packet); av_frame_free(&frame); });
    bool sent = false;
    while (av_read_frame(format, packet) >= 0) {
        if (packet->stream_index == index && avcodec_send_packet(codec, packet) >= 0) sent = true;
        av_packet_unref(packet);
        if (sent && avcodec_receive_frame(codec, frame) == 0) break;
    }
    if (frame->width <= 0 || frame->height <= 0 || qint64(frame->width) * frame->height > 32'000'000) return false;
    QImage image(frame->width, frame->height, QImage::Format_RGBA8888);
    if (image.isNull()) return false;
    SwsContext* scale = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                                       frame->width, frame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!scale) return false;
    const auto freeScale = qScopeGuard([&] { sws_freeContext(scale); });
    uint8_t* planes[] = {image.bits()};
    int strides[] = {static_cast<int>(image.bytesPerLine())};
    if (sws_scale(scale, frame->data, frame->linesize, 0, frame->height, planes, strides) <= 0) return false;
    return image.save(path, "PNG");
}

} // namespace
#endif

QString validateMediaFile(const QString& path, TmarkMediaKind kind, QSize* lottieSize) {
    if (lottieSize) *lottieSize = {};
    if (kind == TmarkMediaKind::Video) return {};
    if (kind == TmarkMediaKind::Image) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return QStringLiteral("Could not read image");
        const QByteArray header = file.read(64);
        const bool jpeg = header.startsWith(QByteArray::fromHex("ffd8ff"));
        const bool png = header.startsWith(QByteArray::fromHex("89504e470d0a1a0a"));
        const bool webp = header.startsWith("RIFF") && header.mid(8, 4) == "WEBP";
        bool avif = false;
        if (header.mid(4, 4) == "ftyp") {
            for (qsizetype offset = 8; offset + 4 <= header.size(); offset += 4) {
                if (offset == 12) continue; // Minor version, not a brand.
                if (header.mid(offset, 4) == "avif" || header.mid(offset, 4) == "avis") avif = true;
            }
        }
        if (!jpeg && !png && !webp && !avif) {
            const QSize size = selfContainedLottieSize(file.readAll().prepend(header));
            if (size.isEmpty()) return QStringLiteral("Unsupported image format");
            if (lottieSize) *lottieSize = size;
            return {};
        }
        if (avif) {
#ifdef __EMSCRIPTEN__
            return QStringLiteral("AVIF decoding is unavailable in this WebAssembly build");
#else
            if (!decodeAvif(path)) return QStringLiteral("Could not decode AVIF image");
#endif
        }
        QImageReader reader(path);
        reader.setAutoDetectImageFormat(true);
        if (reader.read().isNull()) return QStringLiteral("Could not decode image");
        return {};
    }
#ifdef __EMSCRIPTEN__
    return QStringLiteral("Audio playback is unavailable in this WebAssembly build");
#else
    AVFormatContext* context = nullptr;
    const QByteArray name = QFile::encodeName(path);
    if (avformat_open_input(&context, name.constData(), nullptr, nullptr) < 0)
        return QStringLiteral("Could not decode audio");
    const auto close = qScopeGuard([&] { avformat_close_input(&context); });
    if (avformat_find_stream_info(context, nullptr) < 0) return QStringLiteral("Could not decode audio");
    const auto* input = context->iformat;
    const QString container = QString::fromLatin1(input->name).section(',', 0, 0);
    for (unsigned i = 0; i < context->nb_streams; ++i) {
        const auto* codec = context->streams[i]->codecpar;
        if (codec->codec_type != AVMEDIA_TYPE_AUDIO) continue;
        if (container == "mp3" && codec->codec_id == AV_CODEC_ID_MP3) return {};
        if (container == "flac" && codec->codec_id == AV_CODEC_ID_FLAC) return {};
        if (container == "ogg" && codec->codec_id == AV_CODEC_ID_OPUS) return {};
        if (container == "mov" && codec->codec_id == AV_CODEC_ID_AAC && codec->profile == AV_PROFILE_AAC_LOW) return {};
    }
    return QStringLiteral("Unsupported audio container, codec, or profile");
#endif
}
