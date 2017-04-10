#include "stdafx.h"
#include "rtpMuxer_tcp.h"
#include <stdlib.h>

#include <iostream>


rtpMuxer_tcp::rtpMuxer_tcp()
{
}

rtpMuxer_tcp::~rtpMuxer_tcp()
{
}

uint16_t rtpMuxer_tcp::htons2(uint16_t val)
{
	uint16_t temp1 = val << 8;
	uint16_t temp2 = val >> 8;
	return temp1 | temp2;
}

uint32_t rtpMuxer_tcp::htonl2(uint32_t val)
{
	uint32_t temp1 = val << 24;
	uint32_t temp4 = val >> 24;
	uint32_t temp2 = (val & 0x0000ff00) << 8;
	uint32_t temp3 = (val & 0x00ff0000) >> 8;
	return temp1 | temp2 | temp3 | temp4;
}

bool rtpMuxer_tcp::open()
{
	m_rtpCount = min_rtp_count;
	m_rtplist = (rtp3_t*)calloc(m_rtpCount,sizeof(rtp3_t*));
	int i;
	for (i = 0; i < m_rtpCount;i++)
	{
		m_rtplist[i].buffer = (char*)calloc(1, max_rtp_len);
		if (m_rtplist[i].buffer==NULL)
		{
			goto fail;
		}
		m_rtplist[i].len = 0;
	}
	m_cseq = 0;
	return true;
fail:
	for (int k = 0; k < i;k++)
	{
		free(m_rtplist[i].buffer);
	}
	return false;
}

int  rtpMuxer_tcp::pack_video(rtp3_t*& rtplist/*IN*/,char* buffer, int len, bool bkey, uint32_t timestamp, uint32_t ssrc)
{
	vector<nal3_t> m_nals;
	//
	for (int i = 0; i < len-4; i++)
	{
		if (buffer[i]==0x00 && buffer[i+1]==0x00)
		{
			if (buffer[i+2]==0x00 && buffer[i+3]==0x01)//1
			{
				nal3_t nal;
				nal.type = buffer[i + 4] & 0x1f;
				nal.s_posi = i;
				nal.startcodeLen = 4;
				m_nals.push_back(nal);
				if (nal.type==nal_type_idr || nal.type==nal_type_slice)
				{
					break;
				}
				i += 4;
				continue;
			}else if (buffer[i+2]==0x01)//2
			{

			}
		}
	}




	//
	for (int i = 0; i < m_nals.size();i++)
	{
		if (i==m_nals.size()-1)
		{
			m_nals[i].size = m_nals[i].s_posi;
			break;
		}
		m_nals[i].size = m_nals[i + 1].s_posi - m_nals[i].s_posi;
	}

	//LOG4CPLUS_DEBUG(gvlog,"nal——num："<<m_nals.size());

	std::cout << "nal——num：" << m_nals.size()<<std::endl;
	//
	int   rtp_index = 0;
	bool  bFirst = true;
	for (int i = 0; i < m_nals.size();i++)
	{

		if (m_nals[i].size - m_nals[i].startcodeLen <= max_rtp_len - sizeof(rtp_Over_tcp_t))//1
		{

			cout << "小于RTP——len" << endl;
			
			//header
			rtp_Over_tcp_t header;
			header.version = 0x02;
			header.padding = 0;
			header.extension = 0;
			header.csic = 0;
			header.marker = 0;
			header.payloadType = 96;
			header.cseq = htons2(m_cseq++);
			if (m_cseq >= max_rtp_cseq)
			{
				m_cseq = 0;
			}
			header.timestamp = htonl2(m_timestamp);
			header.ssrc = htonl2(ssrc);
			memcpy(m_rtplist[rtp_index].buffer, &header, sizeof(rtp_Over_tcp_t));
			memcpy(m_rtplist[rtp_index++].buffer + sizeof(rtp_Over_tcp_t), buffer + m_nals[i].s_posi + m_nals[i].startcodeLen, m_nals[i].size - m_nals[i].startcodeLen);
			m_rtplist[i].len = sizeof(rtp_Over_tcp_t)+m_nals[i].size-m_nals[i].startcodeLen;
		}
		else {//2

			cout << "大于RTP——len" << endl;
			
			int rtpCount = (m_nals[i].size - m_nals[i].startcodeLen-1) / (max_rtp_len-sizeof(rtp_Over_tcp_t2));
			int rtpResLen = (m_nals[i].size - m_nals[i].startcodeLen - 1) % (max_rtp_len - sizeof(rtp_Over_tcp_t2));

			cout << "一nal拆为：" << rtpCount + 1 << "包"<<endl;

			for (int r = 0; r < rtpCount;r++)
			{
				//header
				rtp_Over_tcp_t2 header2;
				header2.version = 0x02;
				header2.padding = 0;
				header2.extension = 0;
				header2.csic = 0;
				if (rtpResLen==0 && r==rtpCount-1)
				{
					header2.marker = 1;
				}
				else{
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
				header2.fu_type = nal_type_fua;
				if (bkey)
				{
					header2.fu_nri = 3;
				}else
				{
					header2.fu_nri = 1;
				}
				header2.fu_f = 0;
				header2.fu_h_nalType = m_nals[i].type;
				header2.fu_h_f = 0;
				if (r==rtpCount-1)
				{
					header2.fu_h_e = 1;
				}
				else{
					header2.fu_h_e = 0;
				}
				if (0==r)
				{
					header2.fu_h_s = 1;
				}else {
					header2.fu_h_s = 0;
				}
				memcpy(m_rtplist[rtp_index].buffer, &header2, sizeof(rtp_Over_tcp_t2));
				memcpy(m_rtplist[rtp_index++].buffer + sizeof(rtp_Over_tcp_t2), buffer + m_nals[i].s_posi + m_nals[i].startcodeLen +1+ r*(max_rtp_len - sizeof(rtp_Over_tcp_t2)), max_rtp_len - sizeof(rtp_Over_tcp_t2));
				m_rtplist[i].len = max_rtp_len;
			}
			//res
			if (rtpResLen>0)
			{
				rtp_Over_tcp_t2 header3;
				header3.version = 0x02;
				header3.padding = 0;
				header3.extension = 0;
				header3.csic = 0;
				header3.marker = 1;
				header3.payloadType = 96;
				header3.cseq = htons2(m_cseq++);
				if (m_cseq >= max_rtp_cseq)
				{
					m_cseq = 0;
				}
				header3.timestamp = htonl2(m_timestamp);
				header3.ssrc = htonl2(ssrc);
				header3.fu_type = nal_type_fua;
				if (bkey)
				{
					header3.fu_nri = 3;
				}else
				{
					header3.fu_nri = 1;
				}
				header3.fu_f = 0;
				header3.fu_h_nalType = m_nals[i].type;
				header3.fu_h_f = 0;
				header3.fu_h_e = 1;
				header3.fu_h_s = 0;
				
				memcpy(m_rtplist[rtp_index].buffer, &header3, sizeof(rtp_Over_tcp_t2));
				memcpy(m_rtplist[rtp_index++].buffer + sizeof(rtp_Over_tcp_t2), buffer + m_nals[i].s_posi + m_nals[i].startcodeLen + 1+rtpCount*(max_rtp_len - sizeof(rtp_Over_tcp_t2)),rtpResLen);
				m_rtplist[i].len = sizeof(rtpMuxer_tcp)+rtpResLen;
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

void rtpMuxer_tcp::close()
{
	for (int k = 0; k < m_rtpCount; k++)
	{
		free(m_rtplist[k].buffer);
	}
	free(m_rtplist);
}