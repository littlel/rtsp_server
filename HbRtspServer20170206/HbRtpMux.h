#pragma once
#include <math.h>
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned char  uint8_t;
#include <vector>
using namespace std;

#define  max_rtp_num          ((2<<16)-1)
#define  max_rtp_TimeStamp    ((2<<32)-1)
#define  max_rtp_count        3072
#define  fix_rtp_TimeStampInc_video 3600
#define  fix_rtp_TimeStampInc_audio 8000
#define  max_rtp_len          1400
enum streamType
{
	st_h264=0,
	st_h265=1,
	st_mp2p=2,
	st_mp2t=3,
	st_aac=4,
	st_pcma=5,
	st_pcmu=6
};
struct rtp2_t
{
	uint8_t* buffer;
	int      len;
};
typedef struct
{      
	uint8_t  Csrc : 4,
Extension : 1,
Padding : 1,
VersionNum : 2;
	uint8_t  Ptype : 7,
Marker : 1;
	uint16_t seqNum;
	uint32_t timeStamp;
	uint32_t ssrc;
}rtp_header;

typedef struct  
{
	uint8_t fu_indicate_type : 5,
fu_indicate_ref_idc : 2,
fu_indicate_f : 1;

	uint8_t fu_header_type : 5,
fu_header_forbidden : 1,
fu_header_e_bit : 1,
fu_header_s_bit : 1;
} fu_h;

struct nal5_t 
{
	uint8_t type;
	int     s_posi;
	int     size;
	int     startcodelen;
};

#define  nal_type_slice 1
#define  nal_type_idr   5
#define  nal_type_sei   6
#define  nal_type_sps   7
#define  nal_type_pps   8
#define  nal_type_au    9


class CHbRtpMux
{
public:
	CHbRtpMux();
	~CHbRtpMux();
private://video
	uint16_t      m_rtpnum_video;
	uint32_t      m_rtpTimeStamp_video;
	rtp2_t*       m_rtplist_video;
	uint32_t      m_video_ssrc;
private://audio
	uint16_t      m_rtpnum_audio;
	uint32_t      m_rtpTimeStamp_audio;
	rtp2_t*        m_rtplist_audio;
	uint32_t      m_audio_ssrc;
	uint16_t      htons2(uint16_t val);
	uint32_t      htonl2(uint32_t val);
	int           packRtp(uint8_t* esbuffer, int eslen, int timestampInc, int isvideo, streamType st,bool bkey,uint8_t nalType,int ssrc);
	int           packRtp2(uint8_t* esbuffer, int eslen, int timestampInc, int isvideo, streamType st, bool bkey, uint8_t nalType,int&ssrc);
public:
	int PackVideoFrame(uint8_t* esbuffer, int eslen, rtp2_t*& rtplist,int timestampInc,streamType st,bool bkey,int& ssrc);

};

