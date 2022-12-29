//
// Created by hc on 2020/5/14.
//

#ifndef JT1078SERVER_CCODEC_H
#define JT1078SERVER_CCODEC_H
#include <iostream>
#include <memory>

class CCodec
{
public:
    CCodec();

    ~CCodec();

public:
    enum AUDIO_CODING_TYPE
    {
        eG711A,
        eG711U,
        eAdpcm,
        eG726,
        eUnSupport,
    };
    typedef struct
    {
        char *m_pOutBuf;
        int m_nOutBufLen;
        AUDIO_CODING_TYPE m_eType;
    } DECODE_RESULT;

    typedef DECODE_RESULT ENCODE_RESULT;

    DECODE_RESULT &DecodeAudio(const char *pInBuf, int nInBufLen, AUDIO_CODING_TYPE eType);
    const ENCODE_RESULT &EncodeAudio(const char *pInBuf, int nInBufLen, AUDIO_CODING_TYPE eType);

    // 判断是否有海思头
    bool HaveHaiSiHeader(const char *pData, size_t nDataLen);
    // 添加海思头
    inline void AddHaiSiHeader(char *pData, size_t uBodyLen);

public:
    DECODE_RESULT m_iResult;
    ENCODE_RESULT m_resultEncode;
    void *m_g726_decoder_ctx; //.............................g726part
    void *m_g726_coder_ctx;

    // adpcm编解码器保存的数据
    void *m_adpcm_state_encode;
    // g726保存的码率
    int m_g726_bit_rate;

    // 当前编码的类型
    uint8_t m_uPT;

    // 是否是解码的第一帧
    bool m_bDecodeFirstFrame;
    // 解码时是否有海思头
    bool m_bHaveHaiSiHeader;

private:
    // 下面是解码函数
    void __DecodeG711A2Pcm(const char *pInBuf, int nInBufLen);

    void __DecodeG711U2Pcm(const char *pInBuf, int nInBufLen);

    void __DecodeAdpcm2Pcm(const char *pInBuf, int nInBufLen);

    void __DecodeG7262Pcm(const char *pInBuf, int nInBufLen);

    // 下面是编码
    void __EncodeG711A2Pcm(const char *pInBuf, int nInBufLen);

    void __EncodeG711U2Pcm(const char *pInBuf, int nInBufLen);

    void __EncodeAdpcm2Pcm(const char *pInBuf, int nInBufLen);

    void __EncodeG7262Pcm(const char *pInBuf, int nInBufLen);
};

typedef CCodec::DECODE_RESULT DECODE_RESULT;
typedef CCodec::ENCODE_RESULT ENCODE_RESULT;
typedef CCodec::AUDIO_CODING_TYPE AUDIO_CODING_TYPE;
#endif // JT1078SERVER_CCODEC_H
