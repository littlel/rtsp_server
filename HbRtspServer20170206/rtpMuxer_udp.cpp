#include "stdafx.h"
#include "rtpMuxer_udp.h"


rtpMuxer_udp::rtpMuxer_udp()
{
}

rtpMuxer_udp::~rtpMuxer_udp()
{
}

uint16_t rtpMuxer_udp::htons2(uint16_t val)
{
	uint16_t temp1 = val << 8;
	uint16_t temp2 = val >> 8;
	return temp1 | temp2;
}

uint32_t rtpMuxer_udp::htonl2(uint32_t val)
{
	uint32_t temp1 = val << 24;
	uint32_t temp4 = val >> 24;
	uint32_t temp2 = (val & 0x0000ff00) << 8;
	uint32_t temp3 = (val & 0x00ff0000) >> 8;
	return temp1 | temp2 | temp3 | temp4;
}

bool rtpMuxer_udp::open()
{
	/*m_rtpCount = min_rtp_count;
	m_rtplist = (rtp6_t*)calloc(m_rtpCount, sizeof(rtp6_t*));*/
	int i;
	for (i = 0; i < min_rtp_count; i++)
	{
		m_rtplist[i].buffer = (char*)calloc(1, max_rtp_len);
		if (m_rtplist[i].buffer == NULL)
		{
			goto fail;
		}
		m_rtplist[i].len = 0;
	}
	m_cseq = 0;
	m_timestamp = 0;
	return true;
fail:
	for (int k = 0; k < i; k++)
	{
		free(m_rtplist[i].buffer);
	}
	return false;
}

int  rtpMuxer_udp::pack_video(rtp6_t*& rtplist/*IN*/, char* buffer, int len, bool bkey, uint32_t timestamp, uint32_t ssrc)
{
	vector<nal6_t> m_nals;
//
	for (int i = 0; i < len - 4; i++)
	{
		if (buffer[i] == 0x00 && buffer[i + 1] == 0x00)
		{
			if (buffer[i + 2] == 0x00 && buffer[i + 3] == 0x01)//1
			{
				nal6_t nal;
				nal.type = buffer[i + 4] & 0x1f;
				nal.s_posi = i;
				nal.startcodeLen = 4;
				m_nals.push_back(nal);
				if (nal.type == nal_type_idr || nal.type == nal_type_slice)
				{
					break;
				}
				i += 4;
				continue;
			}
			else if (buffer[i + 2] == 0x01)//2
			{

			}
		}
	}

	//
	for (int i = 0; i < m_nals.size(); i++)
	{
		if (i == m_nals.size() - 1)
		{
			m_nals[i].size = len - m_nals[i].s_posi;
			break;
		}
		m_nals[i].size = m_nals[i + 1].s_posi - m_nals[i].s_posi;
	}

	//FUA
	int   rtp_index = 0;
	bool  bFirst = true;
	//fua
	uint8_t   fu_type;
	uint8_t   fu_nri;
	uint8_t   fu_f;
	uint8_t   fu_h_nalType;
	uint8_t   fu_h_f;
	uint8_t   fu_h_e;
	uint8_t   fu_h_s;
	for (int i = 0; i < m_nals.size(); i++)
	{
		if (m_nals[i].size - m_nals[i].startcodeLen <= max_rtp_len - sizeof(rtp_Over_tcp_t))//1
		{
			uint16_t trueLen = sizeof(rtp_Over_tcp_t) + (m_nals[i].size - m_nals[i].startcodeLen);
			/*int paddingLen = 4 - trueLen % 4;
			if (paddingLen != 4)
			{
				trueLen += paddingLen;
			}*/

			//header
			rtp_Over_tcp_t header;
			header.version = 0x02;

/*liyang*/
			int paddingLen = 4;
			if (paddingLen == 4)
			{
				header.padding = 0;
			}
			else
			{
				header.padding = 1;
			}
/*liyang*/

			header.extension = 0;
			header.csic = 0;
			header.marker = 0;
			header.payloadType = 96;
			header.cseq = htons2(m_cseq++);
			if (m_cseq >= max_rtp_cseq)
			{
				m_cseq = 0;
			}

			//h264载荷头【add liyang 170404】


			//[end 0404]
			header.timestamp = htonl2(m_timestamp);
			header.ssrc = htonl2(ssrc);
			memcpy(m_rtplist[rtp_index].buffer, &header, sizeof(rtp_Over_tcp_t));
			memcpy(m_rtplist[rtp_index].buffer + sizeof(rtp_Over_tcp_t), buffer + m_nals[i].s_posi + m_nals[i].startcodeLen, m_nals[i].size - m_nals[i].startcodeLen);

//padding
			/*if (paddingLen == 1)
			{
				int posi = sizeof(rtp_Over_tcp_t)+m_nals[i].size - m_nals[i].startcodeLen;
				m_rtplist[rtp_index].buffer[posi] = 0x01;
			}
			else if (paddingLen == 2)
			{
				int posi = sizeof(rtp_Over_tcp_t)+m_nals[i].size - m_nals[i].startcodeLen;
				m_rtplist[rtp_index].buffer[posi] = 0x00;
				m_rtplist[rtp_index].buffer[posi + 1] = 0x02;
			}
			else if (paddingLen == 3)
			{
				int posi = sizeof(rtp_Over_tcp_t)+m_nals[i].size - m_nals[i].startcodeLen;
				m_rtplist[rtp_index].buffer[posi] = 0x00;
				m_rtplist[rtp_index].buffer[posi + 1] = 0x00;
				m_rtplist[rtp_index].buffer[posi + 2] = 0x03;
			}*/
//padding
			m_rtplist[rtp_index++].len =trueLen;
		}
		else
		{//2
			uint8_t fu_a_indicate;
			uint8_t fu_a_header;
			int rtpCount  = (m_nals[i].size - m_nals[i].startcodeLen - 1) / (max_rtp_len - sizeof(rtp_Over_tcp_t)-2);
			int rtpResLen = (m_nals[i].size - m_nals[i].startcodeLen - 1) % (max_rtp_len - sizeof(rtp_Over_tcp_t)-2);
		
			for (int r = 0; r < rtpCount; r++)
			{
				//header
				rtp_Over_tcp_t header2;
				header2.version = 0x02;
				header2.padding = 0;
				header2.extension = 0;
				header2.csic = 0;
				if (rtpResLen == 0 && r == rtpCount - 1 && i == m_nals.size() - 1)
				{
					header2.marker = 1;
				}
				else
				{
					header2.marker = 0;
				}

				header2.payloadType = 96;
				header2.cseq = htons2(m_cseq++);
				if (m_cseq >= max_rtp_cseq)
				{
					m_cseq = 0;
				}
				header2.timestamp = htonl2(m_timestamp);
				header2.ssrc = htonl2(ssrc);
				//fua  indicate
//??	
				fu_f = 0x00;
				if (bkey)
				{
					fu_nri = 0x03;
				}
				else
				{
					fu_nri = 0x03;
				}
//??
				fu_type = 28;  //fua

				fu_a_indicate = (fu_f << 7) | (fu_nri << 5) | (fu_type);
			
				//fu_a header
				if (0 == r)
				{
					fu_h_s = 1;
				}
				else 
				{
					fu_h_s = 0;
				}

				if (r == rtpCount - 1 && rtpResLen == 0)
				{
					fu_h_e = 1;
				}
				else
				{
					fu_h_e = 0;
				}

				fu_h_f = 0;
				fu_h_nalType = m_nals[i].type;
				fu_a_header = (fu_h_s << 7) | (fu_h_e << 6) | (fu_h_f << 5) | (fu_h_nalType);
				
				//copy
				memcpy(m_rtplist[rtp_index].buffer, &header2, sizeof(rtp_Over_tcp_t));
				m_rtplist[rtp_index].buffer[sizeof(rtp_Over_tcp_t)] = fu_a_indicate;
				m_rtplist[rtp_index].buffer[sizeof(rtp_Over_tcp_t)+1] = fu_a_header;
				memcpy(m_rtplist[rtp_index].buffer + sizeof(rtp_Over_tcp_t)+1 + 1, buffer + m_nals[i].s_posi + m_nals[i].startcodeLen + 1 + r*(max_rtp_len - sizeof(rtp_Over_tcp_t)-2), max_rtp_len - sizeof(rtp_Over_tcp_t)-2);
				m_rtplist[rtp_index++].len = max_rtp_len;
			}
			//res
			if (rtpResLen > 0)
			{
				uint16_t trueLen = rtpResLen + 2 + sizeof(rtp_Over_tcp_t);
			
			
				
				/*if (paddingLen != 4)
				{
					trueLen += paddingLen;
				}*/
				rtp_Over_tcp_t header3;
				header3.version = 0x02;

//liyang padding 
				int paddingLen  = 4;
				if (paddingLen != 4)
				{
					header3.padding = 1;
				}
				else
				{
					header3.padding = 0;
				}
//padding

				header3.extension = 0;
				header3.csic = 0;
				if (i == m_nals.size() - 1)
				{
					header3.marker = 1;
				}
				else{
					header3.marker = 0;
				}
				header3.payloadType = 96;
				header3.cseq = htons2(m_cseq++);
				if (m_cseq >= max_rtp_cseq)
				{
					m_cseq = 0;
				}
				header3.timestamp = htonl2(m_timestamp);
				header3.ssrc = htonl2(ssrc);
				//fua  indicate
				fu_f = 0x00;
				if (bkey)
				{
					fu_nri = 0x03;
				}
				else
				{
					fu_nri = 0x03;
				}
				fu_type = 28;
				fu_a_indicate = (fu_f << 7) | (fu_nri << 5) | (fu_type);
				//fu_a header
				fu_h_s = 0;
				fu_h_e = 1;
				fu_h_f = 0;
				fu_h_nalType = m_nals[i].type;
				fu_a_header = (fu_h_s << 7) | (fu_h_e << 6) | (fu_h_f << 5) | (fu_h_nalType);
				//copy
				memcpy(m_rtplist[rtp_index].buffer, &header3, sizeof(rtp_Over_tcp_t));
				m_rtplist[rtp_index].buffer[sizeof(rtp_Over_tcp_t)] = fu_a_indicate;
				m_rtplist[rtp_index].buffer[sizeof(rtp_Over_tcp_t)+1] = fu_a_header;
				memcpy(m_rtplist[rtp_index].buffer + sizeof(rtp_Over_tcp_t)+1 + 1, buffer + m_nals[i].s_posi + m_nals[i].startcodeLen + 1 + rtpCount*(max_rtp_len - sizeof(rtp_Over_tcp_t)-2), rtpResLen);
				
				
			/*	if (paddingLen == 1)
				{
					int posi = sizeof(rtp_Over_tcp_t)+2 + rtpResLen;
					m_rtplist[rtp_index].buffer[posi] = 0x01;
				}
				else if (paddingLen == 2)
				{
					int posi = sizeof(rtp_Over_tcp_t)+2 + rtpResLen;
					m_rtplist[rtp_index].buffer[posi] = 0x00;
					m_rtplist[rtp_index].buffer[posi + 1] = 0x02;
				}
				else if (paddingLen == 3)
				{
					int posi = sizeof(rtp_Over_tcp_t)+2 + rtpResLen;
					m_rtplist[rtp_index].buffer[posi] = 0x00;
					m_rtplist[rtp_index].buffer[posi + 1] = 0x00;
					m_rtplist[rtp_index].buffer[posi + 2] = 0x03;
				}*/
				m_rtplist[rtp_index++].len = trueLen;
			}
		}
	}
	//
	m_timestamp += 3600;
	if (m_timestamp >= max_time_stamp)
	{
		m_timestamp = 0;
	}
	rtplist = m_rtplist;


	return rtp_index;
}

void rtpMuxer_udp::close()
{
	for (int k = 0; k < min_rtp_count; k++)
	{
		free(m_rtplist[k].buffer);
	}		
}

//rtp包链表的长度要动态分配，否则