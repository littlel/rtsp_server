#pragma once
#include <vector>
using namespace std;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#define  max_rtp_len     1400
#define  nal_type_slice  1
#define  nal_type_idr    5
#define  nal_type_sei    6
#define  nal_type_sps    7
#define  nal_type_pps    8
#define  nal_type_au     9
#define  nal_type_fua    (28)
struct nal3_t
{
	uint8_t type;
	int     s_posi;
	int     size;
	int     startcodeLen;
};


struct rtp_Over_tcp_t
{
//rtp
	uint8_t   csic : 4,
	          extension : 1,
			  padding : 1,
		      version : 2;
	uint8_t   payloadType : 7,
	          marker : 1;
	uint16_t  cseq;
	uint32_t  timestamp;
	uint32_t  ssrc;
};

struct rtp_Over_tcp_t2
{
	//rtp
	uint8_t   csic : 4,
	extension : 1,
			padding : 1,
				  version : 2;
	uint8_t   payloadType : 7,
	marker : 1;
	uint16_t  cseq;
	uint32_t  timestamp;
	uint32_t  ssrc;
	//fua
	uint8_t   fu_type : 5,
	          fu_nri : 2,
		      fu_f : 1;
	uint8_t   fu_h_nalType : 5,
	          fu_h_f : 1,
		      fu_h_e : 1,
			  fu_h_s : 1;
};

struct rtp3_t
{
	char* buffer;
	int   len;
};
#define  min_rtp_count   (3124)
#define  max_rtp_cseq    (2<<16-1)
#define  max_time_stamp  (2<<31-3600)

class rtpMuxer_tcp
{
public:
	rtpMuxer_tcp();
	~rtpMuxer_tcp();
private:
	int             m_rtpCount;
	rtp3_t*          m_rtplist;
	uint16_t        m_cseq;
	uint32_t        m_timestamp;
	rtp_Over_tcp_t  m_rtpHeader;
private:
	uint16_t htons2(uint16_t val);
	uint32_t htonl2(uint32_t val);
public:
	bool open();
	int  pack_video(rtp3_t*& rtplist/*IN*/,char* buffer,int len,bool bkey,uint32_t timestamp,uint32_t ssrc);
	void close();
};

