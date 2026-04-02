cmake_minimum_required(VERSION 3.20)
project(MediaToolKit VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -------------------------------------------------------
# Qt6
# -------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Multimedia MultimediaWidgets
)

# -------------------------------------------------------
# FFmpeg
# -------------------------------------------------------
set(FFMPEG_ROOT "C:/ffmpeg" CACHE PATH "FFmpeg root directory")
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_ROOT}/include")
set(FFMPEG_LIB_DIR      "${FFMPEG_ROOT}/lib")

foreach(_lib avcodec avformat avutil swscale swresample avfilter)
    find_library(FFMPEG_${_lib} ${_lib} HINTS "${FFMPEG_LIB_DIR}" REQUIRED)
    list(APPEND FFMPEG_LIBS ${FFMPEG_${_lib}})
endforeach()

# -------------------------------------------------------
# ソースファイル
# -------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/FileListWidget.cpp
    src/CompressWidget.cpp
    src/ConvertWidget.cpp
    src/VideoSettingsWidget.cpp
    src/AudioSettingsWidget.cpp
    src/ImageSettingsWidget.cpp
    src/PreviewWidget.cpp
    src/FFmpegProcessor.cpp
    src/BatchWorker.cpp
    src/SettingsDialog.cpp
)

set(HEADERS
    src/MediaTypes.h
    src/FileItem.h
    src/MainWindow.h
    src/FileListWidget.h
    src/CompressWidget.h
    src/ConvertWidget.h
    src/VideoSettingsWidget.h
    src/AudioSettingsWidget.h
    src/ImageSettingsWidget.h
    src/PreviewWidget.h
    src/FFmpegProcessor.h
    src/BatchWorker.h
    src/SettingsDialog.h
)

# -------------------------------------------------------
# Windows RC ファイル (アイコン設定)
# -------------------------------------------------------
if(WIN32)
    set(RC_FILE resources/app.rc)
endif()

# -------------------------------------------------------
# ターゲット
# -------------------------------------------------------
add_executable(MediaToolKit WIN32
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
    ${RC_FILE}
)

target_include_directories(MediaToolKit PRIVATE
    src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(MediaToolKit PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${FFMPEG_LIBS}
)

# -------------------------------------------------------
# Windows: コンソール抑制・最適化
# -------------------------------------------------------
if(MSVC)
    target_compile_options(MediaToolKit PRIVATE
        /W3         # 警告レベル3
        /MP         # 並列コンパイル
        /O2         # 最適化
        /Oi         # 組み込み関数
        /GL         # リンク時コード生成
    )
    set_target_properties(MediaToolKit PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /LTCG"
    )
endif()

target_compile_definitions(MediaToolKit PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
    $<$<CONFIG:Release>:NDEBUG>
)
