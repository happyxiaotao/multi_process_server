
#ifndef _RTMP_STREAM_FFMPEG_H_
#define _RTMP_STREAM_FFMPEG_H_

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/common.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#endif //_RTMP_STREAM_FFMPEG_H_