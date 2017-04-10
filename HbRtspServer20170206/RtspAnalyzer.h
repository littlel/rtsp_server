#pragma once
#include <string>
#include <windows.h>
using namespace std;
#define  max_sdp_len  2048
#define  max_session_len  64

enum e_rtsp_method{
	rtsp_play=0,
	rtsp_teardown=1,
	rtsp_get_parameter=2,
	rtsp_describe=3
};

class CRtspAnalyzer
{
public:
	CRtspAnalyzer();
	~CRtspAnalyzer();
private:
	int     m_cseq;
	int     m_logicid;
	string  m_url;
	char    m_sdp[max_sdp_len];
	int     m_client_rtpPort;
	int     m_client_rtcpPort;
	int     m_rtpPort;
	int     m_rtcpPort;
	int     m_session;
	int     m_ssrc;
	int     m_timeout;
private:
	char    m_ip[32];//±¾»úip
	char    m_client_ip[32];
private:
	bool   getCseq(char*& buffer);
	bool   getLogicid(char*& buffer);
	bool   getSdp();
	bool   getTransContext(char*& buffer);
	bool   getSession(char* buffer, char* session);
	void   getDate(SYSTEMTIME& st,  char* week,  char* month);
private:
	void   options_proc(char*& buffer,char*& response, int maxlen);
	void   options_response(char*& response, int maxlen);

	void   describe_proc(char*& buffer, char*& response, int maxlen);
	void   describe_response(char*& response, int maxlen);

	void   setup_proc(char*& buffer, char*& response, int maxlen);
	void   setup_response(char*& response, int maxlen);

	void   play_proc(char*& buffer, char*& response, int maxlen);
	void   play_response(char*& buffer,char*& response, int maxlen);

	void   teardown_proc(char*& buffer, char*& response, int maxlen);
	void   teardown_response(char*& buffer, char*& response, int maxlen);

	void   get_parameter_proc(char*& buffer, char*& response, int maxlen);
	void   get_parameter_response(char*& buffer, char*& response, int maxlen);
public:
	e_rtsp_method   analyzeMessage(char*& buffer, char*& response, int maxlen);
public:
	void   setip(const char* server_ip,int rtpPort,int rtcpPort,const char* cli_ip);
	void   getCliContext( char* cli_ip, int& cli_rtp_port,int& cli_rtcp_port);
	int    getRtspChannelNum();
	void   setStateNode(int node,char*& response,int maxlen);
	int    getssrc();
	void   setTimeout(int ti);
};

