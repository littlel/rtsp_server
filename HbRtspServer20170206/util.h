#pragma once
#include <WinSock2.h>
#include <vector>
#include <process.h>
#include <string>
#include <vector>
#include <Shlwapi.h>
using namespace std;
#pragma comment(lib,"Shlwapi")
#pragma comment(lib,"ws2_32")

//错误码定义
#define  ERRCODE_SUCESS   0
//长度定义
#define MAX_ERR_MSG_LEN      1024
#define MAX_H264_FRAME_LEN   3*1024*1024
#define MAX_FRAME_RATE       50
#define MIN_FRAME_LEN        4*1024
#define MAX_NALS_PER_FRAME   10
#define MAX_NAL_COUNT        64*1024
#define MAX_GIVEUP_LEN       1024*1024
#define MIN_SKIP_FRAME_COUNT 75
//符号定义
#define HB_PLAYER_EXPORT_API   _declspec(dllexport)
//自定义
#define  BadPtr(ptr)     (0xcdcdcdcd==(unsigned int)ptr || NULL==(unsigned int)ptr)

enum AudioEncodeType{T_PCMU=0,T_PCMA=8,T_AAC=104,T_AUDIOUNKNOWN=128};
enum VideoEncodeType{T_H264=96,T_VIDEOUNKNOWN=128};




#define  MAX_H264_FRAME_SIZE    (8*1024*1024)
#define  MAX_AUDIO_FRAME_SIZE   512*1024
#define  MAX_MTU_SIZE           1500
#define  MAX_RTP_PACKET_COUNT   MAX_H264_FRAME_SIZE/MAX_MTU_SIZE


struct rtp_t
{
	char rtpdata[MAX_MTU_SIZE];
	int  rtpdata_size;
	int  h264_frame_offset;
	int  h264_frame_len;
	unsigned short seq_num;  //[add liyang]
};




#define NAL_TYPE_SLICE  1
#define NAL_TYPE_DPA    2
#define NAL_TYPE_DPB    3
#define NAL_TYPE_DPC    4
#define NAL_TYPE_IDR    5
#define NAL_TYPE_SEI    6
#define NAL_TYPE_SPS    7
#define NAL_TYPE_PPS    8
#define NAL_TYPE_AUD    9


#define PACKET_TYPE_FU_A  28//目前仅支持fu_a分包方式



#define  MAX_SDP_LEN   1024
#define  MAX_ATTIBUTE_LEN  128
//a=rtpmap:<payload type> <encoding name>/<clock rate> [/<encodingparameters>]
enum MediaType{video=1,audio=2,text=3,application=4,message=5,m_unknown=6};
enum Proto{udp=1,RTP_AVP=2,RTP_SAVP=3};
enum PayloadType{PCMU=0,PCMA=8,H264=96,AAC=104};
enum AACMode{CELP_cbr=1,CELP_vbr=2,AAC_lbr=3,AAC_hbr=4};
static const char* EncodingNames[]={"PCMU","PCMA","mpeg4-generic"};

