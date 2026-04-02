#pragma once
#include <QString>
#include <QSize>
#include <functional>
#include "MediaTypes.h"

// 進捗コールバック: 0.0〜1.0 を受け取り、false を返すと処理を中断する
using ProgressCb = std::function<bool(float)>;

// -------------------------------------------------------
// 圧縮オプション
// -------------------------------------------------------
struct CompressOptions {
    // --- 動画 ---
    int    videoWidth    = -1;   // -1: 変更しない
    int    videoHeight   = -1;
    double videoFps      = -1.0; // -1: 変更しない
    VideoCodec videoCodec = VideoCodec::H264;
    int    videoBitrate  = -1;   // bps, -1: auto
    bool   autoQuality   = true; // true=最適化, false=手動設定

    // --- 音声 ---
    int    audioBitrate     = -1;  // bps (-1: 変更しない)
    int    audioSampleRate  = -1;  // Hz (-1: 変更しない)
    int    audioBitDepth    = -1;  // bits (-1: 変更しない)
    bool   audioQualityFirst = true;

    // --- 画像 ---
    int    imageQuality  = 85;   // 0-100 (JPEG等)
    bool   imageLossless = false;
    int    imageWidth    = -1;
    int    imageHeight   = -1;
    int    targetSizeKB  = -1;   // -1: 使用しない

    // --- 共通 ---
    bool   useGPU        = false;
    QString gpuType;             // "nvenc" / "qsv" / "amf"
    QString outputDir;
};

// -------------------------------------------------------
// 変換オプション
// -------------------------------------------------------
enum class ConvertTarget { VideoToVideo, VideoToAudio, VideoToImage, AudioToAudio, ImageToImage };

struct ConvertOptions {
    ConvertTarget target  = ConvertTarget::VideoToVideo;
    QString       format;        // 出力フォーマット拡張子 (例: "mp4", "mp3")
    VideoCodec    videoCodec = VideoCodec::H264;

    // フレーム抽出
    double frameTimeSec  = -1.0; // 時間指定 (秒), -1: 先頭
    int    frameNumber   = -1;   // フレーム番号指定, -1: 未使用

    bool   useGPU        = false;
    QString gpuType;
    QString outputDir;
};

// -------------------------------------------------------
// FFmpegProcessor
// -------------------------------------------------------
class FFmpegProcessor
{
public:
    FFmpegProcessor();
    ~FFmpegProcessor();

    // 圧縮処理 (拡張子は維持)
    QString compress(const QString& inputPath,
                     const CompressOptions& opts,
                     ProgressCb progress = nullptr);

    // 変換処理 (拡張子変更)
    QString convert(const QString& inputPath,
                    const ConvertOptions& opts,
                    ProgressCb progress = nullptr);
    static MediaType detectMediaType(const QString& path);
    static QSize     getVideoDimensions(const QString& path);
    static double    getVideoDuration(const QString& path);   // 秒
    static double    getVideoFps(const QString& path);
    static int       getAudioBitrate(const QString& path);    // bps
    static int       getAudioSampleRate(const QString& path); // Hz

    // GPU エンコーダが利用可能か確認
    static bool isGpuEncoderAvailable(const QString& gpuType); // "nvenc"/"qsv"/"amf"

private:
    // 画像処理 (Qt QImage ベース)
    QString compressImage(const QString& input, const CompressOptions& opts, ProgressCb cb);
    QString convertImage(const QString& input, const ConvertOptions& opts, ProgressCb cb);

    // 動画→画像フレーム抽出 (FFmpeg decode → QImage)
    QString extractFrameAsImage(const QString& input, const ConvertOptions& opts, ProgressCb cb);

    // 動画/音声処理 (FFmpeg ベース)
    QString compressVideoAudio(const QString& input, const CompressOptions& opts, ProgressCb cb);
    QString convertVideoAudio(const QString& input, const ConvertOptions& opts, ProgressCb cb);

    // FFmpeg トランスコード共通処理
    struct TranscodeParams {
        QString     outputPath;
        int         outWidth      = -1;
        int         outHeight     = -1;
        double      outFps        = -1;
        int         videoBitrateB = -1;   // bps
        int         audioBitrateB = -1;
        int         outSampleRate = -1;
        VideoCodec  codec         = VideoCodec::H264;
        bool        copyVideo     = false; // コーデックコピー
        bool        copyAudio     = false;
        bool        extractAudio  = false; // 音声のみ出力
        bool        extractFrame  = false; // フレーム1枚のみ
        double      frameTimeSec  = 0.0;
        bool        useGPU        = false;
        QString     gpuType;
    };

    QString runTranscode(const QString& inputPath,
                         const TranscodeParams& p,
                         ProgressCb cb);

    QString buildVideoEncoderName(VideoCodec codec, bool useGPU, const QString& gpuType);
    QString buildOutputPath(const QString& inputPath,
                            const QString& outputDir,
                            const QString& suffix,
                            const QString& newExt = {});
};
