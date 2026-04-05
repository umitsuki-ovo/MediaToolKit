#pragma once
#include <QString>
#include <QStringList>

// -------------------------------------------------------
// メディア種別
// -------------------------------------------------------
enum class MediaType { Unknown, Image, Video, Audio };

enum class ProcessMode { Compress, Convert };

// -------------------------------------------------------
// 動画設定
// -------------------------------------------------------
enum class VideoCodec { H264, H265, MPEG4 };

struct ResolutionEntry {
    int width;
    int height;
    QString label;
};

static constexpr int VIDEO_RES_COUNT = 9;
static const ResolutionEntry VIDEO_RESOLUTIONS[VIDEO_RES_COUNT] = {
    {256,  144,  "144p"},
    {426,  240,  "240p"},
    {640,  360,  "360p"},
    {854,  480,  "SD (480p)"},
    {1280, 720,  "HD (720p)"},
    {1920, 1080, "FHD (1080p)"},
    {2560, 1440, "1440p"},
    {3840, 2160, "4K (2160p)"},
    {7680, 4320, "8K (4320p)"},
};

static constexpr int FPS_COUNT = 10;
static const double FPS_VALUES[FPS_COUNT] = {
    23.98, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0, 120.0, 240.0
};
static const QString FPS_LABELS[FPS_COUNT] = {
    "23.98", "24", "25", "29.97", "30", "50", "59.94", "60", "120", "240"
};

// -------------------------------------------------------
// 音声設定
// -------------------------------------------------------
static constexpr int AUDIO_BITRATE_COUNT = 4;
static const int AUDIO_BITRATES[AUDIO_BITRATE_COUNT] = {96, 128, 192, 320};
static const QString AUDIO_BITRATE_LABELS[AUDIO_BITRATE_COUNT] = {
    "96 kbps", "128 kbps", "192 kbps", "320 kbps"
};

static constexpr int AUDIO_SAMPLERATE_COUNT = 6;
static const int AUDIO_SAMPLE_RATES[AUDIO_SAMPLERATE_COUNT] = {
    44100, 48000, 88200, 96000, 176400, 192000
};
static const QString AUDIO_SAMPLERATE_LABELS[AUDIO_SAMPLERATE_COUNT] = {
    "44.1 kHz", "48 kHz", "88.2 kHz", "96 kHz", "176.4 kHz", "192 kHz"
};

static constexpr int AUDIO_BITDEPTH_COUNT = 4;
static const int AUDIO_BIT_DEPTHS[AUDIO_BITDEPTH_COUNT] = {8, 16, 24, 32};
static const QString AUDIO_BITDEPTH_LABELS[AUDIO_BITDEPTH_COUNT] = {
    "8 bit", "16 bit", "24 bit", "32 bit float"
};

// -------------------------------------------------------
// 対応フォーマット
// -------------------------------------------------------
static const QStringList IMAGE_EXTENSIONS = {
    "jpg","jpeg","png","gif","webp",
    "heic","heif",   // Apple HEIF
    "svg",           // SVG ベクター
    "tif","tiff","bmp","ico"
};
static const QStringList VIDEO_EXTENSIONS = {
    "mp4","mov","avi","webm",
    "mpg","mpeg",    // MPEG-1/2
    "mkv",
    "3gp","3g2",     // モバイル動画
    "m4v","flv","ts","mts","m2ts"
};
static const QStringList AUDIO_EXTENSIONS = {
    "mp3","wav","m4a","aac","flac",
    "wma",           // Windows Media Audio
    "ogg",
    "aif","aiff",    // Apple AIFF
    "alac",          // Apple Lossless
    "opus","ape","tak","tta"
};

// 変換先フォーマットリスト
static const QStringList IMAGE_OUTPUT_FORMATS = {
    "jpg","png","webp","gif","bmp","tiff","ico",
    "svg",       // Potrace によるベクタ変換
    // heic/heif は出力エンコーダが通常の FFmpeg ビルドに含まれないため非対応
};
static const QStringList VIDEO_OUTPUT_FORMATS = {
    "mp4","mov","avi","webm","mkv","m4v","flv",
    "mpg",       // MPEG-1/2 出力
    "3gp",       // モバイル動画
};
static const QStringList AUDIO_OUTPUT_FORMATS = {
    // m4a は UI 上で音声コーデック (AAC/ALAC) を選べるため1エントリのみ
    "mp3","wav","aac","flac","ogg","m4a","opus",
    "wma",       // Windows Media Audio
    "aif",       // Apple AIFF
    // alac は m4a コンテナ経由なので独立エントリとしては不要
};
// 動画→音声
static const QStringList VIDEO_TO_AUDIO_FORMATS = {
    "mp3","wav","aac","flac","ogg","m4a",  // m4a を追加 (AAC/ALAC 選択可)
    "wma","aif",
    // alac は m4a + 音声コーデック選択で対応するため除外
};
// 動画→画像（フレーム抽出）
static const QStringList VIDEO_TO_IMAGE_FORMATS = {
    "jpg","png"
};

inline MediaType getMediaType(const QString& ext)
{
    QString e = ext.toLower().trimmed();
    if (IMAGE_EXTENSIONS.contains(e)) return MediaType::Image;
    if (VIDEO_EXTENSIONS.contains(e)) return MediaType::Video;
    if (AUDIO_EXTENSIONS.contains(e)) return MediaType::Audio;
    return MediaType::Unknown;
}

inline QString mediaTypeLabel(MediaType t)
{
    switch (t) {
        case MediaType::Image: return "画像";
        case MediaType::Video: return "動画";
        case MediaType::Audio: return "音声";
        default:               return "不明";
    }
}
