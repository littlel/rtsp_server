#pragma once
#include "util.h"

struct SessionLevel_t//此版本主要应用于接收组播流，与rtsp版本不是完全兼容
{
	int   version;//版本号
	char  connip;//会话级别ip地址
};


struct MediaLevel_Video_t
{
	MediaType  mt;//媒体类型
	char       connip;//接收ip
	short      recvport;//接收端口
	Proto      tranpro;//传输协议
	char       encodename[MAX_ATTIBUTE_LEN];//编码名称
	int        timescale;//采样基准
	int        packetization_mode;//打包模式
};

struct MediaLevel_Audio_t
{
	MediaType  mt;//媒体类型
	char       connip;//接收ip
	short      recvport;//接收端口
	Proto      tranpro;//传输协议
	char       encodename[MAX_ATTIBUTE_LEN];//编码名称
	int        samplerate;//采样频率
	int        channels;//音频通道数
	int        mode;//传输模式(aac传输模式比较多)
};


static const char h264_start_code[4]={0x00,0x00,0x00,0x01};

#include<map>

class CHbRtpDemux
{
public:
	CHbRtpDemux(void);
	~CHbRtpDemux(void);

private:
	char*     m_h264ptr;//h264帧指针
	char*     m_audioptr;//音频指针
	bool      firstframe;//是否第一帧h264数据
private:
	unsigned        timestamp;
	unsigned        currnumsamples;
	vector<rtp_t>   v_rtp_list;
private://test
	map<int,int>    m_newList;
public:
	bool  open();
	void  close();
private:
	unsigned short  NetHostBytes(unsigned short val);
public:
	bool  parse_video_rtp_packet(char* rtp_buffer_ptr,int rtp_buffer_size,char*& h264_frame_ptr,int& h264_frame_size,bool& b_key,int& mstimestamp,int payloadtype,int timescale);
	bool  parse_audio_rtp_packet(char* rtp_buffer_ptr,int rtp_buffer_size,char*& audio_frame_ptr,int& audio_frame_size,int& numsamples,int payloadtype);
	bool  parse_video_rtp_packet2(char* rtp_buffer_ptr,int rtp_buffer_size,char*& h264_frame_ptr,int& h264_frame_size,bool& b_key,int& mstimestamp,int payloadtype,int timescale);
	bool  parse_video_rtp_packet2_mpg2ps(char* rtp_buffer_ptr, int rtp_buffer_size, char*& h264_frame_ptr, int& h264_frame_size, bool& b_key, int& mstimestamp, int payloadtype, int timescale);

};



