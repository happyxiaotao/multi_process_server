//
// Created by hc on 2020/5/14.
//

#include "CCodec.h"
#include "./G711.h"
#include "./g726.h"
#include "./Adpcm.h"

#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>
#include <string.h>

CCodec::CCodec()
{
    m_g726_decoder_ctx = nullptr;
    m_iResult.m_nOutBufLen = 2048;
    m_iResult.m_pOutBuf = (char *)new char[2048];
    m_iResult.m_eType = AUDIO_CODING_TYPE::eUnSupport;

    m_resultEncode.m_nOutBufLen = 2048;
    m_resultEncode.m_pOutBuf = (char *)new char[2048];
    m_resultEncode.m_eType = AUDIO_CODING_TYPE::eUnSupport;

    m_adpcm_state_encode = new adpcm_state();
    memset(m_adpcm_state_encode, 0, sizeof(adpcm_state));

    m_uPT = 0; // PT表示负载类型，比如26是adpcm编码
    m_bDecodeFirstFrame = true;
    m_bHaveHaiSiHeader = false;

    m_g726_bit_rate = 16000;
}

CCodec::~CCodec()
{
    if (m_iResult.m_pOutBuf != nullptr && m_iResult.m_nOutBufLen != 0)
    {
        delete[] m_iResult.m_pOutBuf;
        m_iResult.m_nOutBufLen = 0;
    }
    if (m_resultEncode.m_pOutBuf != nullptr && m_resultEncode.m_nOutBufLen != 0)
    {
        delete[] m_resultEncode.m_pOutBuf;
        m_resultEncode.m_nOutBufLen = 0;
    }
    if (m_g726_decoder_ctx != nullptr)
    {
        decodec_destroy(&m_g726_decoder_ctx);
        m_g726_decoder_ctx = nullptr;
    } //............................g726part..release

    if (m_adpcm_state_encode != nullptr)
    {
        delete (adpcm_state *)m_adpcm_state_encode;
        m_adpcm_state_encode = nullptr;
    }
}

bool CCodec::HaveHaiSiHeader(const char *pData, size_t nDataLen)
{
    if (pData == nullptr || nDataLen < 4)
    {
        return false;
    }
    if (pData[0] != 0x00 ||
        pData[1] != 0x01 ||
        pData[3] != 0x00)
    {
        return false;
    }
    uint16_t nBodyLen = pData[2];
    if ((nBodyLen * 2) != (nDataLen - 4))
    {
        return false;
    }
    return true;
}
// 添加海思头
void CCodec::AddHaiSiHeader(char *pData, size_t uBodyLen)
{
    pData[0] = 0x00;
    pData[1] = 0x01;
    pData[2] = uBodyLen / 2;
    pData[3] = 0x00;
}

void CCodec::__DecodeG7262Pcm(const char *cInBuf, int nInBufLen)
{ //.............................g726part
    if (m_bDecodeFirstFrame)
    {
        m_bHaveHaiSiHeader = HaveHaiSiHeader(cInBuf, nInBufLen);
        m_bDecodeFirstFrame = false;
    }
    if (m_bHaveHaiSiHeader)
    {
        cInBuf += 4;
        nInBufLen -= 4;
    }
    if (m_g726_decoder_ctx == nullptr)
    {
        switch (nInBufLen) // genju Length panduanweishu chushihua
        {
        case 80: // 2bit
            decodec_init(&m_g726_decoder_ctx, 16000);
            m_g726_bit_rate = 16000;
            break;
        case 120: // 3bit
            decodec_init(&m_g726_decoder_ctx, 24000);
            m_g726_bit_rate = 24000;
            break;
        case 160: // 4bit
            decodec_init(&m_g726_decoder_ctx, 32000);
            m_g726_bit_rate = 32000;
            break;
        default: // case 240:5bit
            decodec_init(&m_g726_decoder_ctx, 40000);
            m_g726_bit_rate = 40000;
            break;
        }
    }

    int n = decode(m_g726_decoder_ctx, cInBuf, nInBufLen, m_iResult.m_pOutBuf, 2048); // Decode
    m_iResult.m_nOutBufLen = n;

    m_iResult.m_eType = AUDIO_CODING_TYPE::eG726;
}

void CCodec::__DecodeAdpcm2Pcm(const char *cInBuf, int nInBufLen)
{
    if (m_bDecodeFirstFrame)
    {
        m_bHaveHaiSiHeader = HaveHaiSiHeader(cInBuf, nInBufLen);
        m_bDecodeFirstFrame = false;
    }
    if (m_bHaveHaiSiHeader)
    {
        cInBuf += 4;
        nInBufLen -= 4;
    }
    adpcm_state iAdpcmState{0};
    iAdpcmState.valprev = (cInBuf[1] << 8) | (cInBuf[0] & 0xFF);
    iAdpcmState.index = cInBuf[2];
    nInBufLen -= 4;
    cInBuf += 4;
    adpcm_decoder((char *)cInBuf, (short *)m_iResult.m_pOutBuf, nInBufLen * 2, &iAdpcmState);
    m_iResult.m_nOutBufLen = 4 * nInBufLen;
    m_iResult.m_eType = AUDIO_CODING_TYPE::eAdpcm;
}

void CCodec::__DecodeG711A2Pcm(const char *cInBuf, int nInBufLen)
{
    if (m_bDecodeFirstFrame)
    {
        m_bHaveHaiSiHeader = HaveHaiSiHeader(cInBuf, nInBufLen);
        m_bDecodeFirstFrame = false;
    }
    if (m_bHaveHaiSiHeader)
    {
        cInBuf += 4;
        nInBufLen -= 4;
    }

    int nRet = g711a_decode((short *)m_iResult.m_pOutBuf, (unsigned char *)cInBuf, nInBufLen);
    m_iResult.m_nOutBufLen = nRet;
    m_iResult.m_eType = AUDIO_CODING_TYPE::eG711A;
}

void CCodec::__DecodeG711U2Pcm(const char *cInBuf, int nInBufLen)
{
    if (m_bDecodeFirstFrame)
    {
        m_bHaveHaiSiHeader = HaveHaiSiHeader(cInBuf, nInBufLen);
        m_bDecodeFirstFrame = false;
    }
    if (m_bHaveHaiSiHeader)
    {
        cInBuf += 4;
        nInBufLen -= 4;
    }
    int nRet = g711u_decode((short *)m_iResult.m_pOutBuf, (unsigned char *)cInBuf, nInBufLen);
    m_iResult.m_nOutBufLen = nRet;
    m_iResult.m_eType = AUDIO_CODING_TYPE::eG711U;
}

DECODE_RESULT &CCodec::DecodeAudio(const char *pInBuf, int nInBufLen, AUDIO_CODING_TYPE eType)
{
    switch (eType)
    {
    case AUDIO_CODING_TYPE::eG711A:
        // printf("eG711A\n");
        __DecodeG711A2Pcm(pInBuf, nInBufLen);
        break;
    case AUDIO_CODING_TYPE::eG711U:
        // printf("eG711U\n");
        __DecodeG711U2Pcm(pInBuf, nInBufLen);
        break;
    case AUDIO_CODING_TYPE::eAdpcm:
        // printf("eAdpcm\n");
        __DecodeAdpcm2Pcm(pInBuf, nInBufLen);
        break;
    case AUDIO_CODING_TYPE::eG726:
        // printf("eG726\n");
        __DecodeG7262Pcm(pInBuf, nInBufLen);
        break;
    default:
        m_iResult.m_eType = AUDIO_CODING_TYPE::eUnSupport;
        break;
    }
    return m_iResult;
}

const ENCODE_RESULT &CCodec::EncodeAudio(const char *pInBuf, int nInBufLen, AUDIO_CODING_TYPE eType)
{
    switch (eType)
    {
    case AUDIO_CODING_TYPE::eG711A:
        // printf("eG711A\n");
        __EncodeG711A2Pcm(pInBuf, nInBufLen);
        break;
    case AUDIO_CODING_TYPE::eG711U:
        // printf("eG711U\n");
        __EncodeG711U2Pcm(pInBuf, nInBufLen);
        break;
    case AUDIO_CODING_TYPE::eAdpcm:
        // printf("eAdpcm\n");
        __EncodeAdpcm2Pcm(pInBuf, nInBufLen);
        break;
    case AUDIO_CODING_TYPE::eG726:
        // printf("eG726\n");
        __EncodeG7262Pcm(pInBuf, nInBufLen);
        break;
    default:
        m_resultEncode.m_eType = AUDIO_CODING_TYPE::eUnSupport;
        break;
    }
    return m_resultEncode;
}
// 下面是编码
void CCodec::__EncodeG711A2Pcm(const char *pInBuf, int nInBufLen)
{
    if (m_bHaveHaiSiHeader)
    {
        int nRet = g711a_encode((unsigned char *)m_resultEncode.m_pOutBuf + 4, (short *)pInBuf, nInBufLen);
        AddHaiSiHeader(m_resultEncode.m_pOutBuf, nRet);
        m_resultEncode.m_nOutBufLen = 4 + nRet;
    }
    else
    {
        int nRet = g711a_encode((unsigned char *)m_resultEncode.m_pOutBuf, (short *)pInBuf, nInBufLen);
        m_resultEncode.m_nOutBufLen = nRet;
    }
    m_resultEncode.m_eType = AUDIO_CODING_TYPE::eG711A;
}

void CCodec::__EncodeG711U2Pcm(const char *pInBuf, int nInBufLen)
{
    if (m_bHaveHaiSiHeader)
    {
        int nRet = g711u_encode((unsigned char *)m_resultEncode.m_pOutBuf + 4, (short *)pInBuf, nInBufLen);
        AddHaiSiHeader(m_resultEncode.m_pOutBuf, nRet);
        m_resultEncode.m_nOutBufLen = 4 + nRet;
    }
    else
    {
        int nRet = g711u_encode((unsigned char *)m_resultEncode.m_pOutBuf, (short *)pInBuf, nInBufLen);
        m_resultEncode.m_nOutBufLen = nRet;
    }

    m_resultEncode.m_eType = AUDIO_CODING_TYPE::eG711U;
}

void CCodec::__EncodeAdpcm2Pcm(const char *pInBuf, int nInBufLen)
{
    adpcm_state *state = static_cast<adpcm_state *>(m_adpcm_state_encode);
    char *p = m_resultEncode.m_pOutBuf;
    if (m_bHaveHaiSiHeader)
    {
        AddHaiSiHeader(p, 4 + nInBufLen / 4);
        p += 4;
    }
    adpcm_coder((short *)pInBuf, p + 4, nInBufLen / 2, state);

    // 保存上一针的情况，占4个字节
    short valprev = state->valprev; //目前测试发现使用htons转的话，会有问题
    memcpy(p, &valprev, sizeof(valprev));
    p[2] = state->index;
    p[3] = 0x0;

    m_resultEncode.m_nOutBufLen = 4 + nInBufLen / 4;
    if (m_bHaveHaiSiHeader)
    {
        m_resultEncode.m_nOutBufLen += 4; // 添加海思头长度4个字节
    }

    m_resultEncode.m_eType = AUDIO_CODING_TYPE::eAdpcm;
}

void CCodec::__EncodeG7262Pcm(const char *pInBuf, int nInBufLen)
{
    m_resultEncode.m_eType = AUDIO_CODING_TYPE::eG726;

    char *p = m_resultEncode.m_pOutBuf;
    if (m_bHaveHaiSiHeader)
    {
        p += 4;
    }

    if (m_g726_coder_ctx == nullptr)
    {
        codec_init(&m_g726_coder_ctx, m_g726_bit_rate);
    }
    int n = encode(m_g726_coder_ctx, pInBuf, nInBufLen, p, 2048 - 4);
    // 编码失败时，发送空数据包
    if (n <= 0)
    {
        m_resultEncode.m_nOutBufLen = 0;
    }
    else
    {
        m_resultEncode.m_nOutBufLen = n;
    }
    if (m_bHaveHaiSiHeader)
    {
        AddHaiSiHeader(m_resultEncode.m_pOutBuf, n);
        m_resultEncode.m_nOutBufLen += 4;
    }
}
