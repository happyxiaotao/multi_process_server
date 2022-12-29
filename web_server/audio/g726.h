#ifndef __AUDIO_G726_H_
#define __AUDIO_G726_H_

int decodec_init(void **in_ctx, int bit_rate); //返回0成功， 其他值表示相应错误码。
void decodec_destroy(void **ctx);
int decode(void *ctx, const char *g726data, int in_len, char *pcm, int out_len);

int codec_init(void **ctx, int bit_rate);
void codec_destroy(void **ctx);
int encode(void *ctx, const char *pcm_data, int pcm_len, char *g726_data, int g726_len);

#endif //__AUDIO_G726_H_
