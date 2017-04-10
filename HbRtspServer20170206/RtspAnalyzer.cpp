#include "stdafx.h"
#include "RtspAnalyzer.h"


CRtspAnalyzer::CRtspAnalyzer()
{
	m_cseq = 0;
	m_logicid = -1;
	memset(m_sdp, 0, max_sdp_len);
}


CRtspAnalyzer::~CRtspAnalyzer()
{
}

bool CRtspAnalyzer::getCseq(char*& buffer)
{
	string  msg = buffer;
	int   len = msg.length();
	int pos = msg.find("CSeq");
	if (pos<0 || pos>len)
	{
		return false;
	}
	for (int i = pos + 5; i < len;i++)
	{
		if (buffer[i]>='0' && buffer[i]<='9')
		{
			pos = i;
			break;
		}else if (i==len-1)
		{
			return false;
		}
	}
	int i;
	for (i = pos; i < len;i++)
	{
		if (buffer[i] < '0' || buffer[i] > '9')
		{
			break;
		}else if (i == len - 1)
		{
			return false;
		}
	}
	string cseq = msg.substr(pos, i - pos);
	m_cseq = atoi(cseq.c_str());
	return true;
}

bool CRtspAnalyzer::getLogicid(char*& buffer)
{
	int len = strlen(buffer);
	int pos;
	for (int i = 8; i < len;i++)
	{
		if (buffer[i]!=' ')
		{
			pos = i;
			break;
		}else if (i==len-2)
		{
			return false;
		}
	}
	int k;
	for ( k = pos; k < len;k++)
	{
		if (buffer[k]==' ')
		{
			break;
		}else if (k==len-1)
		{
			return false;
		}
	}
	//url
	m_url = buffer;
	//logicid
	int j;
	for (j = k - 1; j >=pos;j--)
	{
		if (buffer[j] == '/')
		{
			break;
		}
		else if (j == pos)
		{
			return false;
		}
	}
	m_logicid = atoi(m_url.substr(j + 1, k - j - 1).c_str());
	m_url = m_url.substr(pos, k - pos);
	return true;
}

void CRtspAnalyzer::setip(const char* ip, int rtpPort, int rtcpPort, const char* cli_ip)
{
	strcpy(m_ip, ip);
	strcpy(m_client_ip, cli_ip);
	m_rtpPort = rtpPort;
	m_rtcpPort = rtcpPort;
}

void CRtspAnalyzer::getCliContext( char* cli_ip, int& cli_rtp_port, int& cli_rtcp_port)
{
	strcpy(cli_ip, m_client_ip);
	cli_rtp_port = m_client_rtpPort;
	cli_rtcp_port = m_client_rtcpPort;
}

int CRtspAnalyzer::getRtspChannelNum()
{
	return m_logicid;
}

int CRtspAnalyzer::getssrc()
{
	return m_ssrc;
}

void CRtspAnalyzer::setTimeout(int ti)
{
	m_timeout = ti;
}

void CRtspAnalyzer::setStateNode(int node, char*& response,int maxlen)
{
	char status[64] = { 0 };
	sprintf_s(status, 64, "RTSP/1.0 404 Not Found\r\n");
	string temp = response;
	temp = temp.substr(17);
	sprintf_s(response, maxlen,"%s%s", status, temp.c_str());
}


//【modify liyang 20170324】
bool CRtspAnalyzer::getSdp()
{
	sprintf_s(m_sdp, max_sdp_len,
		"v=0\r\n"
		"o=- 1486398727464125 1486398727464125 IN IP4 %s\r\n"
		"s=Media Presentation\r\n"
		"e=NONE\r\n"
		"b=AS:5100\r\n"
		"t=0 0\r\n"
		"m=video 0 RTP/AVP 96\r\n"
		"c=IN IP4 0.0.0.0\r\n"
		"b=AS:5000\r\n"
		"a=recvonly\r\n"
		"a=control:%s\r\n"
		"a=rtpmap:96 H264/90000\r\n\r\n",	   //[annotate liyang 20170324]
		//"a=rtpmap:96 PS/90000\r\n\r\n",      //[add liyang 20170324]--用于播放地铁ps流视频   MP2P
		//"a=rtpmap:96 MP2P/90000\r\n\r\n",
		m_ip,
		m_url.c_str());
	return true;
}

void CRtspAnalyzer::getDate(SYSTEMTIME& st,  char* week,  char* month)
{
	switch (st.wDayOfWeek)
	{
	case 1:
		strcpy(week, "Mon");
		break;
	case 2:
		strcpy(week, "Tues");
		break;
	case 3:
		strcpy(week, "Wed");
		break;
	case 4:
		strcpy(week, "Thur");
		break;
	case 5:
		strcpy(week, "Fri");
		break;
	case 6:
		strcpy(week, "Sat");
		break;
	case 0:
		strcpy(week, "Sun");
		break;
	default:
		break;
	}
	switch (st.wMonth)
	{
	case 1:
		strcpy(month, "Jan");
		break;
	case 2:
		strcpy(month, "Feb");
		break;
	case 3:
		strcpy(month, "Mar");
		break;
	case 4:
		strcpy(month, "Apr");
		break;
	case 5:
		strcpy(month, "May");
		break;
	case 6:
		strcpy(month, "Jun");
		break;
	case 7:
		strcpy(month, "Jul");
		break;
	case 8:
		strcpy(month, "Aug");
		break;
	case 9:
		strcpy(month, "Sep");
		break;
	case 10:
		strcpy(month, "Oct");
		break;
	case 11:
		strcpy(month, "Nov");
		break;
	case 12:
		strcpy(month, "Dec");
		break;
	default:
		break;
	}
}

bool CRtspAnalyzer::getTransContext(char*& buffer)
{
	int len = strlen(buffer);
	string xml = buffer;
	int pos = xml.find("Transport");
	if (pos<0 || pos>len-1)
	{
		return false;
	}
	string xml2 = xml.substr(pos);
	int pos2 = xml2.find("unicast");
	if (pos2<0 || pos2>xml2.length() - 1)
	{
		return false;
	}
	int pos3 = xml2.find("client_port=");
	if (pos3 < 0 || pos3 > xml2.length() - 1)
	{
		return false;
	}
	string xml3 = xml2.substr(pos3 + 12);
	int    len3 = xml3.length();
	if (len3>max_sdp_len)
	{
		return false;
	}
	char c_xml3[max_sdp_len] = { 0 };
	strcpy(c_xml3, xml3.c_str());
	int i;
	for ( i = 0; i < len3;i++)
	{
		if (c_xml3[i]==' '|| c_xml3[i]=='\r')
		{
			break;
		}else if (i==len3-1)
		{
			free(c_xml3);
			return false;
		}
	}
	int j;
	for (j = 0; j < len - 3;j++)
	{
		if (c_xml3[j] == '-')
		{
			break;
		}else if (j == len3 - 1)
		{
			free(c_xml3);
			return false;
		}
	};
	m_client_rtpPort = atoi(xml3.substr(0, j).c_str());
	m_client_rtcpPort = atoi(xml3.substr(j+1,i-j-1).c_str());
	return true;
}

void  CRtspAnalyzer::options_response(char*& response, int maxlen)
{
	sprintf_s(response, maxlen,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
	    "Public: OPTIONS, DESCRIBE, PLAY, PAUSE, SETUP, TEARDOWN, SET_PARAMETER,GET_PARAMETER\r\n"
		"Date:  Mon, Feb 06 2017 16:32:07 GMT\r\n\r\n",
		m_cseq);

}

void  CRtspAnalyzer::options_proc(char*& buffer, char*& response, int maxlen)
{
	if (!getCseq(buffer))
	{
		return;
	}
	options_response(response, maxlen);

}

void  CRtspAnalyzer::describe_proc(char*& buffer, char*& response, int maxlen)
{
	if (!getCseq(buffer))
	{
		return;
	}
	if (!getLogicid(buffer))
	{
		return;
	}
	if (!getSdp())
	{
		return;
	}
	describe_response(response, maxlen);
}

void  CRtspAnalyzer::describe_response(char*& response, int maxlen)
{
	sprintf_s(response, maxlen,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Content-Type: application/sdp\r\n"
		"Content-Base: %s\r\n"
		"Content-Length: %d\r\n\r\n"
		"%s", 
		m_cseq,
		m_url.c_str(),
		strlen(m_sdp),
		m_sdp
		);
}

void CRtspAnalyzer::setup_proc(char*& buffer, char*& response, int maxlen)
{
	if (!getCseq(buffer))
	{
		return;
	}
	if (!getTransContext(buffer))
	{
		return;
	}
	setup_response(response, maxlen);
}

void CRtspAnalyzer::setup_response(char*& response, int maxlen)
{
	m_session = GetTickCount();
	m_ssrc = GetTickCount();
	char   week[16] = { 0 };
	char   month[16] = { 0 };
	SYSTEMTIME st;
	GetLocalTime(&st);
	getDate(st, week, month);
	sprintf_s(response, maxlen, 
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Session:       %d;timeout=%d\r\n"
		"Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d;ssrc=%d;mode=\"play\"\r\n"
		"Date:  %s, %s %02d %04d %02d:%02d:%02d GMT\r\n\r\n",
		m_cseq,
		m_session,m_timeout,
		m_client_rtpPort,m_client_rtcpPort,m_rtpPort,m_rtcpPort,m_ssrc,
		week,month,st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);
}

bool   CRtspAnalyzer::getSession(char* buffer,  char* session)
{
	string xml = buffer;
	int len = xml.length();
	int pos = xml.find("Session");
	if (pos<0 || pos>len)
	{
		return false;
	}
	int i;
	for (i = pos + 8; i < len; i++)
	{
		if (buffer[i] != ' ')
		{
			break;
		}
		else if (i == len - 1)
		{
			return false;
		}
	}
	int j;
	for (j = i; j < len; j++)
	{
		if (buffer[j] == '\r')
		{
			break;
		}
		else if (j == len - 1)
		{
			return false;
		}
	}
	string session2 = xml.substr(i, j - i);
	strcpy(session,session2.c_str());
	return true;
}

void CRtspAnalyzer::play_proc(char*& buffer, char*& response, int maxlen)
{
	if (!getCseq(buffer))
	{
		return;
	}
	play_response(buffer,response, maxlen);
}

void CRtspAnalyzer::play_response(char*& buffer,char*& response, int maxlen)
{
	char session[max_session_len] = { 0 };
	if (!getSession(buffer, session))
	{
		return;
	}
	char   week[16] = { 0 };
	char   month[16] = { 0 };
	SYSTEMTIME st;
	GetLocalTime(&st);
	getDate(st, week, month);
	sprintf_s(response, maxlen, 
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Session:       %s\r\n"
		"Date:  %s, %s %02d %04d %02d:%02d:%02d GMT\r\n\r\n", 
		m_cseq, 
		session,
		week, month, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
}

void   CRtspAnalyzer::teardown_proc(char*& buffer, char*& response, int maxlen)
{
	if (!getCseq(buffer))
	{
		return;
	}
	teardown_response(buffer,response, maxlen);
}

void   CRtspAnalyzer::teardown_response(char*& buffer, char*& response, int maxlen)
{
	char session[max_session_len] = { 0 };
	if (!getSession(buffer, session))
	{
		return;
	}
	char   week[16] = { 0 };
	char   month[16] = { 0 };
	SYSTEMTIME st;
	GetLocalTime(&st);
	getDate(st, week, month);
	sprintf_s(response, maxlen,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Session:       %s\r\n"
		"Date:  %s, %s %02d %04d %02d:%02d:%02d GMT\r\n\r\n", 
		m_cseq, 
		session, 
		week, month, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
}

void   CRtspAnalyzer::get_parameter_proc(char*& buffer, char*& response, int maxlen)
{
	if (!getCseq(buffer))
	{
		return;
	}
	get_parameter_response(buffer,response, maxlen);
}

void   CRtspAnalyzer::get_parameter_response(char*& buffer, char*& response, int maxlen)
{
	char session[max_session_len] = { 0 };
	if (!getSession(buffer, session))
	{
		return;
	}
	char   week[16] = { 0 };
	char   month[16] = { 0 };
	SYSTEMTIME st;
	GetLocalTime(&st);
	getDate(st, week, month);
	sprintf_s(response, maxlen,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Session:       %s\r\n"
		"Date:  %s, %s %02d %04d %02d:%02d:%02d GMT\r\n\r\n",
		m_cseq, 
		session, 
		week, month, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
}

e_rtsp_method  CRtspAnalyzer::analyzeMessage(char*& buffer, char*& response, int maxlen)
{
	memset(response, 0, maxlen);
	if (strncmp(buffer,"OPTIONS",7)==0)
	{
		options_proc(buffer, response, maxlen); 
	}else if (strncmp(buffer, "DESCRIBE", 8) == 0)
	{
		describe_proc(buffer, response, maxlen);
		return rtsp_describe;
	}else if (strncmp(buffer, "SETUP", 5) == 0)
	{
		setup_proc(buffer, response, maxlen);
	}else if (strncmp(buffer, "PLAY", 4) == 0)
	{
		play_proc(buffer, response, maxlen);
		return rtsp_play;
	}else if (strncmp(buffer, "TEARDOWN", 8) == 0)
	{
		teardown_proc(buffer, response, maxlen);
		return rtsp_teardown;
	}else if (strncmp(buffer, "GET_PARAMETER", 13) == 0)
	{
		get_parameter_proc(buffer, response, maxlen);
		return rtsp_get_parameter;
	}
}