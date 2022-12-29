#include <stdio.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
}
#include "g726.h"
typedef struct
{
	AVCodec *codec;
	AVCodecContext *c;
	AVFrame *frame;
	AVPacket avpkt;

} ffmpeg_g726ctx;

int decodec_init(void **in_ctx, int bit_rate)
{
	avcodec_register_all();
	*in_ctx = (ffmpeg_g726ctx *)malloc(sizeof(ffmpeg_g726ctx));
	ffmpeg_g726ctx *ctx = (ffmpeg_g726ctx *)(*in_ctx);
	ctx->codec = avcodec_find_decoder(AV_CODEC_ID_ADPCM_G726LE);
	if (!ctx->codec)
		return 1;

	auto c = avcodec_alloc_context3(ctx->codec);
	c->bits_per_coded_sample = bit_rate / 8000;
	c->channels = 1;
	c->sample_fmt = AV_SAMPLE_FMT_S16;
	c->sample_rate = 8000;
	c->codec_type = AVMEDIA_TYPE_AUDIO;
	c->bit_rate = bit_rate;
	ctx->c = c;

	int iRet = avcodec_open2(c, ctx->codec, NULL);
	if (iRet < 0)
		return 2;

	ctx->frame = av_frame_alloc();
	av_init_packet(&ctx->avpkt);
	return 0;
}

void decodec_destroy(void **ctx)
{
	auto x = (ffmpeg_g726ctx *)(*ctx);
	if (x == nullptr)
	{
		return;
	}

	if (x->frame)
	{
		av_frame_free(&x->frame);
	}

	if (x->c)
	{
		avcodec_close(x->c);
		avcodec_free_context(&x->c);
	}

	free(x);
}

int decode(void *ctx, const char *g726data, int in_len, char *pcm, int out_len)
{

	int data_size = 0;
	int sample_size = 0;
	ffmpeg_g726ctx *ffctx = (ffmpeg_g726ctx *)ctx;
	ffctx->avpkt.data = (uint8_t *)g726data;
	ffctx->avpkt.size = in_len;

	sample_size = av_get_bytes_per_sample(ffctx->c->sample_fmt);
	int len = avcodec_send_packet(ffctx->c, &ffctx->avpkt);
	if (len < 0)
	{
		printf("avcodec_send_packet faild, return: %d\n", len);
		return len;
	}

	if (len >= 0)
	{
		if (!ffctx->frame && !(ffctx->frame = av_frame_alloc()))
		{
			fprintf(stderr, "Could not allocate audio frame\n");
			return 0;
		}
		int ret = avcodec_receive_frame(ffctx->c, ffctx->frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			printf("avcodec_receive_frame faild: %d\n", ret);
			return 0;
		}

		data_size = sample_size * ffctx->frame->nb_samples;
		memcpy(pcm, ffctx->frame->data[0], FFMIN(out_len, data_size));
	}

	return data_size;
}

int codec_init(void **codec_ctx, int bit_rate)
{
	avcodec_register_all();
	*codec_ctx = (ffmpeg_g726ctx *)malloc(sizeof(ffmpeg_g726ctx));
	ffmpeg_g726ctx *ctx = (ffmpeg_g726ctx *)(*codec_ctx);

	ctx->codec = avcodec_find_encoder(AV_CODEC_ID_ADPCM_G726);
	if (ctx->codec == nullptr)
	{
		return -1;
	}
	auto c = avcodec_alloc_context3(ctx->codec);
	c->bits_per_coded_sample = bit_rate / 8000;
	c->bit_rate = bit_rate;
	c->sample_rate = 8000;
	c->channels = 1;
	c->sample_fmt = AV_SAMPLE_FMT_S16;
	c->codec_type = AVMEDIA_TYPE_AUDIO;

	ctx->c = c;

	int iRet = avcodec_open2(c, ctx->codec, nullptr);
	if (iRet < 0)
	{
		return -2;
	}

	ctx->frame = av_frame_alloc();
	if (ctx->frame == nullptr)
	{
		return -3;
	}

	av_init_packet(&ctx->avpkt);

	return 0;
}
void codec_destroy(void **ctx)
{
	decodec_destroy(ctx); // 相同的释放逻辑
}
int encode(void *_ctx, const char *pcm_data, int pcm_len, char *g726_data, int g726_len)
{
	ffmpeg_g726ctx *ctx = (ffmpeg_g726ctx *)_ctx;

	int sample_size = av_get_bytes_per_sample(ctx->c->sample_fmt);
	ctx->frame->nb_samples = pcm_len /
							 (ctx->c->channels * sample_size);
	int ret = avcodec_fill_audio_frame(ctx->frame,
									   ctx->c->channels,
									   ctx->c->sample_fmt,
									   (unsigned char *)pcm_data,
									   pcm_len,
									   0);
	if (ret < 0)
	{
		return -1;
	}
	int got_pkt = 0;
	if (avcodec_encode_audio2(ctx->c, &ctx->avpkt, ctx->frame, &got_pkt) < 0)
	{
		return -2;
	}
	if (got_pkt)
	{
		memcpy(g726_data, ctx->frame->data, FFMIN(g726_len, ret));
		return ret;
	}
	return 0;
}
