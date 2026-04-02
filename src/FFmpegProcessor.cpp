#include "FFmpegProcessor.h"

#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QBuffer>
#include <QDebug>
#include <algorithm>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

// ================================================================
// ヘルパー
// ================================================================
static QString avErr(int e) {
    char buf[256]; av_strerror(e, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

// 拡張子 → Qt フォーマット文字列
static QByteArray qtFmt(const QString& ext)
{
    static const struct { const char* e; const char* f; } T[] = {
        {"jpg","JPEG"},{"jpeg","JPEG"},{"png","PNG"},{"bmp","BMP"},
        {"gif","GIF"}, {"webp","WEBP"},{"tif","TIFF"},{"tiff","TIFF"},
        {"ico","ICO"}, {nullptr,nullptr}
    };
    for (int i = 0; T[i].e; i++)
        if (ext == T[i].e) return QByteArray(T[i].f);
    return ext.toUpper().toLatin1();
}

// 拡張子 → 音声エンコーダ
static const AVCodec* pickAudioEncoder(const QString& ext)
{
    static const struct { const char* e; const char* n; } T[] = {
        {"mp3","libmp3lame"},{"ogg","libvorbis"},{"opus","libopus"},
        {"flac","flac"},    {"wav","pcm_s16le"},{"aiff","pcm_s16be"},
        {"aif","pcm_s16be"},{"m4a","aac"},      {"aac","aac"},
        {"wma","wmav2"},    {"alac","alac"},     {"webm","libopus"},
        {nullptr,nullptr}
    };
    for (int i = 0; T[i].e; i++) {
        if (ext == T[i].e) {
            auto* c = avcodec_find_encoder_by_name(T[i].n);
            if (c) return c;
        }
    }
    return avcodec_find_encoder(AV_CODEC_ID_AAC);
}

// ================================================================
// GPU ハードウェア情報
// ================================================================
struct GpuInfo {
    AVHWDeviceType hwType    = AV_HWDEVICE_TYPE_NONE;
    AVPixelFormat  swFmt     = AV_PIX_FMT_NV12;   // GPU へ渡す前の SW フォーマット
};

static GpuInfo getGpuInfo(const QString& gpuType)
{
    GpuInfo info;
    if      (gpuType == "nvenc") { info.hwType = AV_HWDEVICE_TYPE_CUDA;  info.swFmt = AV_PIX_FMT_NV12; }
    else if (gpuType == "qsv")   { info.hwType = AV_HWDEVICE_TYPE_QSV;   info.swFmt = AV_PIX_FMT_NV12; }
    else if (gpuType == "amf")   { info.hwType = AV_HWDEVICE_TYPE_D3D11VA; info.swFmt = AV_PIX_FMT_NV12; }
    return info;
}

// GPU エンコーダ名
static QString gpuEncName(VideoCodec codec, const QString& gpuType)
{
    if (gpuType == "nvenc") return codec==VideoCodec::H265 ? "hevc_nvenc" : "h264_nvenc";
    if (gpuType == "qsv")   return codec==VideoCodec::H265 ? "hevc_qsv"   : "h264_qsv";
    if (gpuType == "amf")   return codec==VideoCodec::H265 ? "hevc_amf"   : "h264_amf";
    return {};
}

// ================================================================
// コンストラクタ / デストラクタ
// ================================================================
FFmpegProcessor::FFmpegProcessor()  {}
FFmpegProcessor::~FFmpegProcessor() {}

// ================================================================
// GPU エンコーダ確認 (実際にデバイスを開けるかまで確認)
// ================================================================
bool FFmpegProcessor::isGpuEncoderAvailable(const QString& gpuType)
{
    QString name = gpuEncName(VideoCodec::H264, gpuType);
    if (name.isEmpty()) return false;
    if (!avcodec_find_encoder_by_name(name.toUtf8())) return false;

    // デバイスが実際に開けるか確認
    GpuInfo info = getGpuInfo(gpuType);
    if (info.hwType == AV_HWDEVICE_TYPE_NONE) return false;
    AVBufferRef* hwCtx = nullptr;
    int ret = av_hwdevice_ctx_create(&hwCtx, info.hwType, nullptr, nullptr, 0);
    if (hwCtx) av_buffer_unref(&hwCtx);
    return ret >= 0;
}

// ================================================================
// エンコーダ名 (CPU)
// ================================================================
QString FFmpegProcessor::buildVideoEncoderName(VideoCodec codec, bool /*useGPU*/, const QString& /*gpuType*/)
{
    switch (codec) {
        case VideoCodec::H264:  return "libx264";
        case VideoCodec::H265:  return "libx265";
        case VideoCodec::MPEG4: return "mpeg4";
    }
    return "libx264";
}

// ================================================================
// 出力パス生成
// ================================================================
QString FFmpegProcessor::buildOutputPath(const QString& in, const QString& dir,
                                          const QString& suffix, const QString& newExt)
{
    QFileInfo fi(in);
    QString d = dir.isEmpty() ? fi.absolutePath() : dir;
    QString e = newExt.isEmpty() ? fi.suffix() : newExt;
    return QDir(d).filePath(fi.completeBaseName() + suffix + "." + e);
}

// ================================================================
// メディア情報取得
// ================================================================
MediaType FFmpegProcessor::detectMediaType(const QString& path)
{
    return getMediaType(QFileInfo(path).suffix());
}

QSize FFmpegProcessor::getVideoDimensions(const QString& path)
{
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.toUtf8(), nullptr, nullptr) < 0) return {};
    avformat_find_stream_info(ctx, nullptr);
    QSize sz;
    for (unsigned i = 0; i < ctx->nb_streams; i++)
        if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            { sz = {ctx->streams[i]->codecpar->width, ctx->streams[i]->codecpar->height}; break; }
    avformat_close_input(&ctx);
    return sz;
}

double FFmpegProcessor::getVideoDuration(const QString& path)
{
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.toUtf8(), nullptr, nullptr) < 0) return 0;
    avformat_find_stream_info(ctx, nullptr);
    double d = ctx->duration / (double)AV_TIME_BASE;
    avformat_close_input(&ctx);
    return d;
}

double FFmpegProcessor::getVideoFps(const QString& path)
{
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.toUtf8(), nullptr, nullptr) < 0) return 0;
    avformat_find_stream_info(ctx, nullptr);
    double fps = 0;
    for (unsigned i = 0; i < ctx->nb_streams; i++)
        if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational r = ctx->streams[i]->avg_frame_rate;
            if (r.den > 0) fps = (double)r.num / r.den;
            break;
        }
    avformat_close_input(&ctx);
    return fps;
}

int FFmpegProcessor::getAudioBitrate(const QString& path)
{
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.toUtf8(), nullptr, nullptr) < 0) return 0;
    avformat_find_stream_info(ctx, nullptr);
    int br = 0;
    for (unsigned i = 0; i < ctx->nb_streams; i++)
        if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            { br = (int)ctx->streams[i]->codecpar->bit_rate; break; }
    if (!br) br = (int)ctx->bit_rate;
    avformat_close_input(&ctx);
    return br;
}

int FFmpegProcessor::getAudioSampleRate(const QString& path)
{
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.toUtf8(), nullptr, nullptr) < 0) return 0;
    avformat_find_stream_info(ctx, nullptr);
    int sr = 0;
    for (unsigned i = 0; i < ctx->nb_streams; i++)
        if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            { sr = ctx->streams[i]->codecpar->sample_rate; break; }
    avformat_close_input(&ctx);
    return sr;
}

// ================================================================
// 圧縮エントリ
// ================================================================
QString FFmpegProcessor::compress(const QString& in, const CompressOptions& opts, ProgressCb cb)
{
    if (getMediaType(QFileInfo(in).suffix()) == MediaType::Image)
        return compressImage(in, opts, cb);
    return compressVideoAudio(in, opts, cb);
}

// ================================================================
// 変換エントリ
// ================================================================
QString FFmpegProcessor::convert(const QString& in, const ConvertOptions& opts, ProgressCb cb)
{
    if (getMediaType(QFileInfo(in).suffix()) == MediaType::Image)
        return convertImage(in, opts, cb);
    if (opts.target == ConvertTarget::VideoToImage)
        return extractFrameAsImage(in, opts, cb);
    return convertVideoAudio(in, opts, cb);
}

// ================================================================
// 画像読み込み
// ================================================================
static QImage loadImage(const QString& path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);
    if (reader.canRead()) {
        QImage img = reader.read();
        if (!img.isNull()) return img;
    }
    return QImage(path);
}

// ================================================================
// 画像書き込み (WebP/GIF/TIFF プラグイン対応 + FFmpeg フォールバック)
// ================================================================
static QString writeImage(const QImage& imgIn, const QString& outPath,
                           const QByteArray& fmtHint, int quality = -1)
{
    QImage img = imgIn;
    QByteArray fmt = fmtHint;

    // フォーマット固有の前処理
    if (fmt == "JPEG" && img.hasAlphaChannel())
        img = img.convertToFormat(QImage::Format_RGB32);
    if (fmt == "GIF")
        img = img.convertToFormat(QImage::Format_Indexed8);

    // QImageWriter で試みる
    {
        QImageWriter writer(outPath, fmt);
        if (quality >= 0) writer.setQuality(quality);
        if (writer.write(img)) return {};  // 成功
    }

    // QImageWriter が失敗した場合: FFmpeg で書き出す
    // PNG に変換してから再試行 (WebP/TIFF は FFmpegなしで対応困難なため)
    {
        // まず PNG 一時バッファ経由で WebP を試みる
        QByteArray pngData;
        {
            QBuffer buf(&pngData);
            buf.open(QIODevice::WriteOnly);
            img.save(&buf, "PNG");
        }
        if (pngData.isEmpty()) return "画像の PNG 変換に失敗しました";

        // 一時 PNG ファイルを書き出して FFmpeg で変換
        QString tmpPath = outPath + ".tmp.png";
        QFile f(tmpPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(pngData);
            f.close();
        } else {
            return QString("画像の保存に失敗 [%1] : %2")
                   .arg("プラグイン未対応のフォーマット", outPath);
        }

        // FFmpeg で PNG → 目的フォーマット
        AVFormatContext* ifmt = nullptr;
        if (avformat_open_input(&ifmt, tmpPath.toUtf8(), nullptr, nullptr) < 0) {
            QFile::remove(tmpPath);
            return "一時ファイルの読み込みに失敗";
        }
        avformat_find_stream_info(ifmt, nullptr);

        int vidIdx = -1;
        for (unsigned i = 0; i < ifmt->nb_streams; i++)
            if (ifmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                { vidIdx = (int)i; break; }

        bool ok = false;
        if (vidIdx >= 0) {
            const AVCodec* dec = avcodec_find_decoder(ifmt->streams[vidIdx]->codecpar->codec_id);
            AVCodecContext* decCtx = avcodec_alloc_context3(dec);
            avcodec_parameters_to_context(decCtx, ifmt->streams[vidIdx]->codecpar);
            avcodec_open2(decCtx, dec, nullptr);

            // 出力コンテキスト
            AVFormatContext* ofmt = nullptr;
            avformat_alloc_output_context2(&ofmt, nullptr, nullptr, outPath.toUtf8());
            if (ofmt) {
                // 出力エンコーダ (PNG/WebP等)
                const AVCodec* enc = avcodec_find_encoder(ofmt->oformat->video_codec);
                if (enc) {
                    AVCodecContext* encCtx = avcodec_alloc_context3(enc);
                    encCtx->width     = decCtx->width;
                    encCtx->height    = decCtx->height;
                    encCtx->pix_fmt   = enc->pix_fmts ? enc->pix_fmts[0] : AV_PIX_FMT_RGB24;
                    encCtx->time_base = {1, 25};
                    if (ofmt->oformat->flags & AVFMT_GLOBALHEADER)
                        encCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                    if (avcodec_open2(encCtx, enc, nullptr) >= 0) {
                        AVStream* outS = avformat_new_stream(ofmt, nullptr);
                        outS->time_base = encCtx->time_base;
                        avcodec_parameters_from_context(outS->codecpar, encCtx);

                        if (avio_open(&ofmt->pb, outPath.toUtf8(), AVIO_FLAG_WRITE) >= 0) {
                            avformat_write_header(ofmt, nullptr);

                            AVPacket* pkt = av_packet_alloc();
                            AVFrame*  frame = av_frame_alloc();
                            AVFrame*  dstFrm = av_frame_alloc();

                            SwsContext* sws = sws_getContext(
                                decCtx->width, decCtx->height, decCtx->pix_fmt,
                                encCtx->width, encCtx->height, encCtx->pix_fmt,
                                SWS_LANCZOS, nullptr, nullptr, nullptr);

                            while (av_read_frame(ifmt, pkt) >= 0) {
                                if (pkt->stream_index == vidIdx) {
                                    avcodec_send_packet(decCtx, pkt);
                                    if (avcodec_receive_frame(decCtx, frame) == 0) {
                                        dstFrm->width  = encCtx->width;
                                        dstFrm->height = encCtx->height;
                                        dstFrm->format = encCtx->pix_fmt;
                                        av_frame_get_buffer(dstFrm, 0);
                                        if (sws) sws_scale(sws, frame->data, frame->linesize, 0,
                                                           frame->height, dstFrm->data, dstFrm->linesize);
                                        dstFrm->pts = 0;
                                        avcodec_send_frame(encCtx, dstFrm);
                                        AVPacket* ep = av_packet_alloc();
                                        if (avcodec_receive_packet(encCtx, ep) == 0) {
                                            ep->stream_index = outS->index;
                                            av_interleaved_write_frame(ofmt, ep);
                                            ok = true;
                                        }
                                        av_packet_free(&ep);
                                        av_frame_unref(dstFrm);
                                        av_frame_unref(frame);
                                        break;
                                    }
                                }
                                av_packet_unref(pkt);
                            }

                            av_write_trailer(ofmt);
                            if (sws) sws_freeContext(sws);
                            av_packet_free(&pkt);
                            av_frame_free(&frame);
                            av_frame_free(&dstFrm);
                            avio_closep(&ofmt->pb);
                        }
                        avcodec_free_context(&encCtx);
                    }
                }
                avformat_free_context(ofmt);
            }
            avcodec_free_context(&decCtx);
        }
        avformat_close_input(&ifmt);
        QFile::remove(tmpPath);

        if (!ok) return QString("画像の保存に失敗 (プラグイン・FFmpegとも失敗): ") + outPath;
    }
    return {};
}

// ================================================================
// 画像圧縮
// ================================================================
QString FFmpegProcessor::compressImage(const QString& in, const CompressOptions& opts, ProgressCb cb)
{
    if (cb && !cb(0.1f)) return "キャンセルされました";

    QImage img = loadImage(in);
    if (img.isNull()) return "ファイルの読み込みに失敗: " + in;

    if (opts.imageWidth > 0 && opts.imageHeight > 0)
        img = img.scaled(opts.imageWidth, opts.imageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else if (opts.imageWidth > 0)
        img = img.scaledToWidth(opts.imageWidth, Qt::SmoothTransformation);
    else if (opts.imageHeight > 0)
        img = img.scaledToHeight(opts.imageHeight, Qt::SmoothTransformation);

    if (cb && !cb(0.4f)) return "キャンセルされました";

    QString    out = buildOutputPath(in, opts.outputDir, "_compressed");
    QString    ext = QFileInfo(in).suffix().toLower();
    QByteArray fmt = qtFmt(ext);
    int        q   = opts.imageLossless ? 0 : opts.imageQuality;

    QString err;
    if (opts.targetSizeKB > 0 && fmt == "JPEG") {
        // サイズ指定: 二分探索
        if (img.hasAlphaChannel()) img = img.convertToFormat(QImage::Format_RGB32);
        int lo = 1, hi = 100, bestQ = 85;
        for (int i = 0; i < 12; i++) {
            int mid = (lo + hi) / 2;
            QByteArray ba; QBuffer buf(&ba); buf.open(QIODevice::WriteOnly);
            img.save(&buf, "JPEG", mid);
            if (ba.size() <= opts.targetSizeKB * 1024) { bestQ = mid; lo = mid+1; }
            else hi = mid-1;
        }
        if (!img.save(out, "JPEG", bestQ))
            err = "画像の保存に失敗: " + out;
    } else {
        err = writeImage(img, out, fmt, q);
    }

    if (!err.isEmpty()) return err;
    if (cb) (void)cb(1.0f);
    return {};
}

// ================================================================
// 画像変換
// ================================================================
QString FFmpegProcessor::convertImage(const QString& in, const ConvertOptions& opts, ProgressCb cb)
{
    if (cb && !cb(0.1f)) return "キャンセルされました";

    QImage img = loadImage(in);
    if (img.isNull()) return "ファイルの読み込みに失敗: " + in;

    if (cb && !cb(0.5f)) return "キャンセルされました";

    QString    out = buildOutputPath(in, opts.outputDir, "", opts.format);
    QByteArray fmt = qtFmt(opts.format);

    QString err = writeImage(img, out, fmt, 85);
    if (!err.isEmpty()) return err;

    if (cb) (void)cb(1.0f);
    return {};
}

// ================================================================
// 動画→画像 フレーム抽出
// ================================================================
QString FFmpegProcessor::extractFrameAsImage(const QString& in, const ConvertOptions& opts, ProgressCb cb)
{
    if (cb && !cb(0.05f)) return "キャンセルされました";

    AVFormatContext* ifmt = nullptr;
    if (avformat_open_input(&ifmt, in.toUtf8(), nullptr, nullptr) < 0)
        return "入力を開けません";
    avformat_find_stream_info(ifmt, nullptr);

    int videoIdx = -1;
    for (unsigned i = 0; i < ifmt->nb_streams; i++)
        if (ifmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            { videoIdx = (int)i; break; }
    if (videoIdx < 0) { avformat_close_input(&ifmt); return "動画ストリームが見つかりません"; }

    double seekSec = 0.0;
    if (opts.frameTimeSec >= 0) seekSec = opts.frameTimeSec;
    else if (opts.frameNumber >= 0) {
        double fps = getVideoFps(in);
        seekSec = (fps > 0) ? opts.frameNumber / fps : 0.0;
    }
    if (seekSec > 0) av_seek_frame(ifmt, -1, (int64_t)(seekSec * AV_TIME_BASE), AVSEEK_FLAG_BACKWARD);

    const AVCodec* dec = avcodec_find_decoder(ifmt->streams[videoIdx]->codecpar->codec_id);
    AVCodecContext* decCtx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(decCtx, ifmt->streams[videoIdx]->codecpar);
    decCtx->thread_count = 0;  // マルチスレッドデコード
    avcodec_open2(decCtx, dec, nullptr);

    AVPacket* pkt   = av_packet_alloc();
    AVFrame*  frame = av_frame_alloc();
    QImage result;

    while (av_read_frame(ifmt, pkt) >= 0 && result.isNull()) {
        if (pkt->stream_index == videoIdx) {
            avcodec_send_packet(decCtx, pkt);
            if (avcodec_receive_frame(decCtx, frame) == 0) {
                SwsContext* sws = sws_getContext(
                    frame->width, frame->height, (AVPixelFormat)frame->format,
                    frame->width, frame->height, AV_PIX_FMT_RGB24,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (sws) {
                    result = QImage(frame->width, frame->height, QImage::Format_RGB888);
                    uint8_t* dst[1] = { result.bits() };
                    int linesz[1]   = { (int)result.bytesPerLine() };
                    sws_scale(sws, frame->data, frame->linesize, 0, frame->height, dst, linesz);
                    sws_freeContext(sws);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&decCtx);
    avformat_close_input(&ifmt);

    if (result.isNull()) return "フレームのデコードに失敗しました";
    if (cb && !cb(0.8f)) return "キャンセルされました";

    QString    out = buildOutputPath(in, opts.outputDir, "", opts.format);
    QByteArray fmt = qtFmt(opts.format);
    QString    err = writeImage(result, out, fmt, 90);
    if (!err.isEmpty()) return err;

    if (cb) (void)cb(1.0f);
    return {};
}

// ================================================================
// 動画/音声 圧縮
// ================================================================
QString FFmpegProcessor::compressVideoAudio(const QString& in, const CompressOptions& opts, ProgressCb cb)
{
    TranscodeParams p;
    p.outputPath    = buildOutputPath(in, opts.outputDir, "_compressed");
    p.outWidth      = opts.videoWidth;
    p.outHeight     = opts.videoHeight;
    p.outFps        = opts.videoFps;
    p.videoBitrateB = (opts.videoBitrate > 0) ? opts.videoBitrate * 1000 : -1;
    p.audioBitrateB = (opts.audioBitrate > 0) ? opts.audioBitrate * 1000 : -1;
    p.outSampleRate = opts.audioSampleRate;
    p.codec         = opts.videoCodec;
    p.useGPU        = opts.useGPU;
    p.gpuType       = opts.gpuType;
    return runTranscode(in, p, cb);
}

// ================================================================
// 動画/音声 変換
// ================================================================
QString FFmpegProcessor::convertVideoAudio(const QString& in, const ConvertOptions& opts, ProgressCb cb)
{
    TranscodeParams p;
    p.outputPath  = buildOutputPath(in, opts.outputDir, "", opts.format);
    p.codec       = opts.videoCodec;
    p.useGPU      = opts.useGPU;
    p.gpuType     = opts.gpuType;
    if (opts.target == ConvertTarget::VideoToAudio) p.extractAudio = true;
    return runTranscode(in, p, cb);
}

// ================================================================
// ★ GPU ハードウェアコンテキスト付き エンコーダ構築
// ================================================================
struct VideoEncSetup {
    AVCodecContext* encCtx  = nullptr;
    AVStream*       stream  = nullptr;
    SwsContext*     swsCtx  = nullptr;
    AVBufferRef*    hwDevCtx= nullptr;   // GPU デバイスコンテキスト
    AVBufferRef*    hwFrmCtx= nullptr;   // GPU フレームコンテキスト
    AVPixelFormat   swFmt   = AV_PIX_FMT_NV12;  // CPU→GPU 転送前フォーマット
    bool            useHw   = false;
};

static void freeVideoEncSetup(VideoEncSetup& s)
{
    if (s.encCtx)   avcodec_free_context(&s.encCtx);
    if (s.swsCtx)   sws_freeContext(s.swsCtx);
    if (s.hwFrmCtx) av_buffer_unref(&s.hwFrmCtx);
    if (s.hwDevCtx) av_buffer_unref(&s.hwDevCtx);
}

static bool buildVideoEncoder(VideoEncSetup& setup,
                               AVFormatContext* ofmt,
                               AVCodecContext*  decCtx,
                               int outW, int outH,
                               AVRational outFps,
                               int bitrateB,
                               VideoCodec codec,
                               bool useGpu,
                               const QString& gpuType,
                               const QString& outExt)
{
    // ---- エンコーダ名候補リストを作る ----
    // 優先順: GPU指定 → CPU版コーデック → libx264 最終手段
    QStringList candidates;
    if (useGpu && !gpuType.isEmpty())
        candidates << gpuEncName(codec, gpuType);
    candidates << (outExt=="webm" ? "libvpx-vp9"
                 : outExt=="avi"  ? "libx264"
                 : codec==VideoCodec::H265  ? "libx265"
                 : codec==VideoCodec::MPEG4 ? "mpeg4"
                                            : "libx264");
    if (!candidates.contains("libx264")) candidates << "libx264";

    for (const QString& encName : candidates) {
        const AVCodec* enc = avcodec_find_encoder_by_name(encName.toUtf8());
        if (!enc) continue;

        bool isGpuEnc = encName.contains("nvenc") || encName.contains("qsv") || encName.contains("amf");

        // ---- GPU デバイスコンテキスト生成 ----
        AVBufferRef* hwDev = nullptr;
        AVBufferRef* hwFrm = nullptr;
        AVPixelFormat encPixFmt = AV_PIX_FMT_NV12;
        bool hwOk = false;

        if (isGpuEnc) {
            GpuInfo gi = getGpuInfo(gpuType);
            if (gi.hwType != AV_HWDEVICE_TYPE_NONE &&
                av_hwdevice_ctx_create(&hwDev, gi.hwType, nullptr, nullptr, 0) >= 0)
            {
                // GPU フレームコンテキスト
                hwFrm = av_hwframe_ctx_alloc(hwDev);
                AVHWFramesContext* fc = (AVHWFramesContext*)hwFrm->data;
                fc->format    = AV_PIX_FMT_CUDA;  // NVENC は CUDA, QSV は QSV
                if (gi.hwType == AV_HWDEVICE_TYPE_QSV)   fc->format = AV_PIX_FMT_QSV;
                if (gi.hwType == AV_HWDEVICE_TYPE_D3D11VA) fc->format = AV_PIX_FMT_D3D11;
                fc->sw_format = AV_PIX_FMT_NV12;
                fc->width     = outW;
                fc->height    = outH;
                fc->initial_pool_size = 20;
                if (av_hwframe_ctx_init(hwFrm) >= 0) {
                    encPixFmt = fc->format;
                    hwOk = true;
                } else {
                    av_buffer_unref(&hwFrm); hwFrm = nullptr;
                    av_buffer_unref(&hwDev); hwDev = nullptr;
                }
            }
            // GPU コンテキスト失敗 → この候補はスキップ
            if (!hwOk && isGpuEnc) continue;
        } else {
            // CPU エンコーダのピクセルフォーマット
            if      (enc->pix_fmts)               encPixFmt = enc->pix_fmts[0];
            else if (outExt == "webm")             encPixFmt = AV_PIX_FMT_YUV420P;
            else                                   encPixFmt = AV_PIX_FMT_YUV420P;
        }

        // ---- エンコーダコンテキスト ----
        AVCodecContext* ectx = avcodec_alloc_context3(enc);
        ectx->width       = outW;
        ectx->height      = outH;
        ectx->framerate   = outFps;
        ectx->time_base   = av_inv_q(outFps);
        ectx->pix_fmt     = encPixFmt;
        if (bitrateB > 0) ectx->bit_rate = bitrateB;

        // GPU フレームコンテキストをエンコーダに紐付け
        if (hwFrm) ectx->hw_frames_ctx = av_buffer_ref(hwFrm);

        // プリセット・オプション
        QString nm = QString(enc->name);
        if (nm == "libx264" || nm == "libx265")
            av_opt_set(ectx->priv_data, "preset", "medium", 0);
        if (nm == "h264_nvenc" || nm == "hevc_nvenc") {
            av_opt_set(ectx->priv_data, "preset",  "p4", 0);   // バランス
            av_opt_set(ectx->priv_data, "tune",    "hq", 0);
        }
        if (nm == "h264_qsv" || nm == "hevc_qsv")
            av_opt_set(ectx->priv_data, "preset",  "medium", 0);
        if (outExt == "webm")
            av_opt_set_int(ectx->priv_data, "quality", 10, 0);
        if (ofmt->oformat->flags & AVFMT_GLOBALHEADER)
            ectx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // スレッド数 (CPU エンコーダ)
        if (!isGpuEnc) ectx->thread_count = 0;

        if (avcodec_open2(ectx, enc, nullptr) < 0) {
            avcodec_free_context(&ectx);
            if (hwFrm) av_buffer_unref(&hwFrm);
            if (hwDev) av_buffer_unref(&hwDev);
            continue;  // 次の候補へ
        }

        // ---- 出力ストリーム ----
        AVStream* outS = avformat_new_stream(ofmt, nullptr);
        outS->time_base = ectx->time_base;
        avcodec_parameters_from_context(outS->codecpar, ectx);

        // ---- スケーラ: CPU側のフォーマット変換用 ----
        // GPU エンコーダ時は NV12 まで変換し、その後 GPU に転送
        AVPixelFormat targetSW = isGpuEnc ? AV_PIX_FMT_NV12 : encPixFmt;
        SwsContext* sws = nullptr;
        if (decCtx->width != outW || decCtx->height != outH || decCtx->pix_fmt != targetSW)
        {
            sws = sws_getContext(
                decCtx->width, decCtx->height, decCtx->pix_fmt,
                outW, outH, targetSW,
                SWS_LANCZOS, nullptr, nullptr, nullptr);
        }

        setup.encCtx   = ectx;
        setup.stream   = outS;
        setup.swsCtx   = sws;
        setup.hwDevCtx = hwDev;
        setup.hwFrmCtx = hwFrm;
        setup.swFmt    = targetSW;
        setup.useHw    = isGpuEnc && hwOk;
        return true;
    }
    return false;
}

// ================================================================
// FFmpeg トランスコードコア (GPU対応版)
// ================================================================
QString FFmpegProcessor::runTranscode(const QString& inputPath,
                                       const TranscodeParams& p,
                                       ProgressCb cb)
{
    const QString outExt = QFileInfo(p.outputPath).suffix().toLower();

    // ---- 入力 ----
    AVFormatContext* ifmt = nullptr;
    int ret = avformat_open_input(&ifmt, inputPath.toUtf8(), nullptr, nullptr);
    if (ret < 0) return "入力を開けません: " + avErr(ret);
    avformat_find_stream_info(ifmt, nullptr);

    const double totalDur = (ifmt->duration > 0) ? ifmt->duration / (double)AV_TIME_BASE : 0.0;

    int videoIdx = -1, audioIdx = -1;
    for (unsigned i = 0; i < ifmt->nb_streams; i++) {
        auto t = ifmt->streams[i]->codecpar->codec_type;
        if (t == AVMEDIA_TYPE_VIDEO && videoIdx < 0) videoIdx = (int)i;
        if (t == AVMEDIA_TYPE_AUDIO && audioIdx < 0) audioIdx = (int)i;
    }

    const bool needVideo = (videoIdx >= 0) && !p.extractAudio;
    const bool needAudio = (audioIdx >= 0);

    // ---- 出力コンテキスト ----
    AVFormatContext* ofmt = nullptr;
    ret = avformat_alloc_output_context2(&ofmt, nullptr, nullptr, p.outputPath.toUtf8());
    if (ret < 0 || !ofmt) { avformat_close_input(&ifmt); return "出力コンテキスト作成失敗"; }

    // ==== 動画セットアップ ====
    VideoEncSetup vidSetup;
    AVCodecContext* vidDecCtx = nullptr;

    if (needVideo && videoIdx >= 0) {
        do {
            AVStream* inVS = ifmt->streams[videoIdx];
            const AVCodec* dec = avcodec_find_decoder(inVS->codecpar->codec_id);
            if (!dec) break;
            vidDecCtx = avcodec_alloc_context3(dec);
            avcodec_parameters_to_context(vidDecCtx, inVS->codecpar);
            vidDecCtx->thread_count = 0;  // マルチスレッドデコード
            if (avcodec_open2(vidDecCtx, dec, nullptr) < 0) break;

            int outW = (p.outWidth  > 0) ? p.outWidth  : inVS->codecpar->width;
            int outH = (p.outHeight > 0) ? p.outHeight : inVS->codecpar->height;
            if (p.outWidth  > 0 && p.outHeight <= 0)
                outH = (int)((double)inVS->codecpar->height * p.outWidth / inVS->codecpar->width) & ~1;
            if (p.outHeight > 0 && p.outWidth <= 0)
                outW = (int)((double)inVS->codecpar->width * p.outHeight / inVS->codecpar->height) & ~1;
            outW = std::max(outW & ~1, 2);
            outH = std::max(outH & ~1, 2);

            AVRational outFps = (p.outFps > 0) ? av_d2q(p.outFps, 1000) : inVS->avg_frame_rate;
            if (!outFps.num || !outFps.den) outFps = {30,1};

            if (!buildVideoEncoder(vidSetup, ofmt, vidDecCtx,
                                    outW, outH, outFps, p.videoBitrateB,
                                    p.codec, p.useGPU, p.gpuType, outExt))
            {
                // エンコーダ構築失敗
                if (vidDecCtx) avcodec_free_context(&vidDecCtx);
                vidDecCtx = nullptr;
            }
        } while (false);

        if (!vidSetup.encCtx && vidDecCtx) {
            avcodec_free_context(&vidDecCtx);
            vidDecCtx = nullptr;
        }
    }

    // ==== 音声セットアップ ====
    AVCodecContext* audDecCtx = nullptr;
    AVCodecContext* audEncCtx = nullptr;
    SwrContext*     swrCtx    = nullptr;
    AVAudioFifo*    aFifo     = nullptr;
    AVStream*       outAS     = nullptr;

    if (needAudio && audioIdx >= 0) {
        do {
            AVStream* inAS = ifmt->streams[audioIdx];
            const AVCodec* dec = avcodec_find_decoder(inAS->codecpar->codec_id);
            if (!dec) break;
            audDecCtx = avcodec_alloc_context3(dec);
            avcodec_parameters_to_context(audDecCtx, inAS->codecpar);
            if (avcodec_open2(audDecCtx, dec, nullptr) < 0)
                { avcodec_free_context(&audDecCtx); audDecCtx = nullptr; break; }

            if (audDecCtx->ch_layout.nb_channels == 0 ||
                audDecCtx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
            {
                int ch = inAS->codecpar->ch_layout.nb_channels > 0
                         ? inAS->codecpar->ch_layout.nb_channels : 2;
                av_channel_layout_default(&audDecCtx->ch_layout, ch);
            }

            const AVCodec* enc = pickAudioEncoder(outExt);
            AVSampleFormat outFmt = enc->sample_fmts ? enc->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            int outSR = (p.outSampleRate > 0) ? p.outSampleRate : inAS->codecpar->sample_rate;
            if (outExt == "opus" || (!vidSetup.encCtx && outExt == "webm")) outSR = 48000;
            if (enc->supported_samplerates) {
                int best = enc->supported_samplerates[0], diff = abs(best - outSR);
                for (int i = 1; enc->supported_samplerates[i]; i++) {
                    int d = abs(enc->supported_samplerates[i] - outSR);
                    if (d < diff) { best = enc->supported_samplerates[i]; diff = d; }
                }
                outSR = best;
            }
            int nCh = audDecCtx->ch_layout.nb_channels;
            if ((outExt == "opus" || outExt == "mp3") && nCh > 2) nCh = 2;
            AVChannelLayout outLayout{};
            av_channel_layout_default(&outLayout, nCh);

            audEncCtx = avcodec_alloc_context3(enc);
            audEncCtx->sample_fmt  = outFmt;
            audEncCtx->sample_rate = outSR;
            av_channel_layout_copy(&audEncCtx->ch_layout, &outLayout);
            audEncCtx->bit_rate    = (p.audioBitrateB > 0) ? p.audioBitrateB : 192000;
            audEncCtx->time_base   = {1, outSR};
            if (ofmt->oformat->flags & AVFMT_GLOBALHEADER)
                audEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (avcodec_open2(audEncCtx, enc, nullptr) < 0) {
                avcodec_free_context(&audEncCtx); audEncCtx = nullptr;
                avcodec_free_context(&audDecCtx); audDecCtx = nullptr;
                av_channel_layout_uninit(&outLayout); break;
            }

            outAS = avformat_new_stream(ofmt, nullptr);
            outAS->time_base = audEncCtx->time_base;
            avcodec_parameters_from_context(outAS->codecpar, audEncCtx);

            AVChannelLayout decLayout = audDecCtx->ch_layout;
            swr_alloc_set_opts2(&swrCtx, &outLayout, outFmt, outSR,
                                &decLayout, audDecCtx->sample_fmt, audDecCtx->sample_rate, 0, nullptr);
            swr_init(swrCtx);
            aFifo = av_audio_fifo_alloc(outFmt, nCh, std::max(audEncCtx->frame_size * 4, 8192));
            av_channel_layout_uninit(&outLayout);
        } while (false);
    }

    // ---- 出力ファイルを開く ----
    if (!(ofmt->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt->pb, p.outputPath.toUtf8(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            freeVideoEncSetup(vidSetup);
            if (vidDecCtx) avcodec_free_context(&vidDecCtx);
            if (audDecCtx) avcodec_free_context(&audDecCtx);
            if (audEncCtx) avcodec_free_context(&audEncCtx);
            if (swrCtx)    swr_free(&swrCtx);
            if (aFifo)     av_audio_fifo_free(aFifo);
            avformat_free_context(ofmt);
            avformat_close_input(&ifmt);
            return "出力ファイルを開けません: " + avErr(ret);
        }
    }
    avformat_write_header(ofmt, nullptr);

    // ---- encodeFrame ヘルパー ----
    auto encodeFrame = [&](AVCodecContext* ec, AVFrame* frm, AVStream* os) {
        if (!ec || !os) return;
        avcodec_send_frame(ec, frm);
        AVPacket* ep = av_packet_alloc();
        while (avcodec_receive_packet(ec, ep) == 0) {
            av_packet_rescale_ts(ep, ec->time_base, os->time_base);
            ep->stream_index = os->index;
            av_interleaved_write_frame(ofmt, ep);
            av_packet_unref(ep);
        }
        av_packet_free(&ep);
    };

    // ---- FIFO drain ----
    int64_t audioPts = 0;
    auto drainFifo = [&](bool flush) {
        if (!aFifo || !audEncCtx || !outAS) return;
        int frameSize = (audEncCtx->frame_size > 0) ? audEncCtx->frame_size : 1024;
        for (;;) {
            int avail = av_audio_fifo_size(aFifo);
            if (avail <= 0) break;
            if (!flush && avail < frameSize) break;
            int readN = std::min(avail, frameSize);
            AVFrame* ef = av_frame_alloc();
            ef->nb_samples  = readN;
            ef->format      = audEncCtx->sample_fmt;
            ef->sample_rate = audEncCtx->sample_rate;
            av_channel_layout_copy(&ef->ch_layout, &audEncCtx->ch_layout);
            av_frame_get_buffer(ef, 0);
            av_audio_fifo_read(aFifo, (void**)ef->data, readN);
            ef->pts = audioPts; audioPts += readN;
            encodeFrame(audEncCtx, ef, outAS);
            av_frame_free(&ef);
        }
    };

    // ---- メインループ ----
    AVPacket* pkt    = av_packet_alloc();
    AVFrame*  frame  = av_frame_alloc();
    AVFrame*  swFrm  = av_frame_alloc();  // CPU フォーマット変換用
    AVFrame*  hwFrm  = av_frame_alloc();  // GPU 転送用
    int64_t   vidPts = 0;
    bool      cancelled = false;

    while (av_read_frame(ifmt, pkt) >= 0) {
        if (cb && totalDur > 0 && pkt->pts != AV_NOPTS_VALUE) {
            float prog = (float)std::min(
                pkt->pts * av_q2d(ifmt->streams[pkt->stream_index]->time_base) / totalDur, 0.99);
            if (!cb(prog)) { cancelled = true; av_packet_unref(pkt); break; }
        }

        // --- 動画パケット ---
        if (needVideo && vidDecCtx && vidSetup.encCtx && pkt->stream_index == videoIdx) {
            avcodec_send_packet(vidDecCtx, pkt);
            while (avcodec_receive_frame(vidDecCtx, frame) == 0) {
                AVFrame* srcFrm = frame;

                // スケーリング/フォーマット変換 (CPU側)
                if (vidSetup.swsCtx) {
                    av_frame_unref(swFrm);
                    swFrm->width  = vidSetup.encCtx->width;
                    swFrm->height = vidSetup.encCtx->height;
                    swFrm->format = vidSetup.swFmt;
                    av_frame_get_buffer(swFrm, 0);
                    sws_scale(vidSetup.swsCtx,
                              frame->data, frame->linesize, 0, frame->height,
                              swFrm->data, swFrm->linesize);
                    swFrm->pts = vidPts++;
                    srcFrm = swFrm;
                } else {
                    frame->pts = vidPts++;
                }

                // GPU 転送 (ハードウェアエンコード時)
                if (vidSetup.useHw && vidSetup.hwFrmCtx) {
                    av_frame_unref(hwFrm);
                    hwFrm->format = vidSetup.encCtx->pix_fmt;
                    hwFrm->width  = srcFrm->width;
                    hwFrm->height = srcFrm->height;
                    av_hwframe_get_buffer(vidSetup.hwFrmCtx, hwFrm, 0);
                    av_hwframe_transfer_data(hwFrm, srcFrm, 0);
                    hwFrm->pts = srcFrm->pts;
                    srcFrm = hwFrm;
                }

                encodeFrame(vidSetup.encCtx, srcFrm, vidSetup.stream);
                av_frame_unref(frame);
            }
        }
        // --- 音声パケット ---
        else if (needAudio && audDecCtx && audEncCtx && pkt->stream_index == audioIdx) {
            avcodec_send_packet(audDecCtx, pkt);
            while (avcodec_receive_frame(audDecCtx, frame) == 0) {
                int outN = std::max(swr_get_out_samples(swrCtx, frame->nb_samples), frame->nb_samples + 1024);
                AVFrame* conv = av_frame_alloc();
                conv->nb_samples  = outN;
                conv->format      = audEncCtx->sample_fmt;
                conv->sample_rate = audEncCtx->sample_rate;
                av_channel_layout_copy(&conv->ch_layout, &audEncCtx->ch_layout);
                av_frame_get_buffer(conv, 0);
                int n = swr_convert(swrCtx, conv->data, outN, (const uint8_t**)frame->data, frame->nb_samples);
                if (n > 0) { conv->nb_samples = n; av_audio_fifo_write(aFifo, (void**)conv->data, n); }
                av_frame_free(&conv);
                av_frame_unref(frame);
                drainFifo(false);
            }
        }
        av_packet_unref(pkt);
    }

    // ---- フラッシュ ----
    if (!cancelled) {
        if (swrCtx && aFifo && audEncCtx) {
            int delayed = (int)swr_get_delay(swrCtx, audEncCtx->sample_rate) + 1024;
            if (delayed > 0) {
                AVFrame* fl = av_frame_alloc();
                fl->nb_samples = delayed; fl->format = audEncCtx->sample_fmt;
                fl->sample_rate = audEncCtx->sample_rate;
                av_channel_layout_copy(&fl->ch_layout, &audEncCtx->ch_layout);
                av_frame_get_buffer(fl, 0);
                int n = swr_convert(swrCtx, fl->data, delayed, nullptr, 0);
                if (n > 0) { fl->nb_samples = n; av_audio_fifo_write(aFifo, (void**)fl->data, n); }
                av_frame_free(&fl);
            }
        }
        drainFifo(true);
        encodeFrame(audEncCtx, nullptr, outAS);
        encodeFrame(vidSetup.encCtx, nullptr, vidSetup.stream);
    }
    av_write_trailer(ofmt);

    // ---- クリーンアップ ----
    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_frame_free(&swFrm);
    av_frame_free(&hwFrm);
    if (aFifo)     av_audio_fifo_free(aFifo);
    if (swrCtx)    swr_free(&swrCtx);
    if (vidDecCtx) avcodec_free_context(&vidDecCtx);
    if (audDecCtx) avcodec_free_context(&audDecCtx);
    if (audEncCtx) avcodec_free_context(&audEncCtx);
    freeVideoEncSetup(vidSetup);
    if (!(ofmt->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt->pb);
    avformat_free_context(ofmt);
    avformat_close_input(&ifmt);

    if (!cancelled && cb) (void)cb(1.0f);
    return cancelled ? QString("キャンセルされました") : QString{};
}
