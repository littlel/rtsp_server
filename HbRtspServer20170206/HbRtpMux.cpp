#include "stdafx.h"
#include "HbRtpMux.h"
#include <stdlib.h>
#include <windows.h>




#define FIXED_MTU_SZIE		1500
#define FIXED_SIZE_PER_RTP  1400


CHbRtpMux::CHbRtpMux()
{
	//m_rtpnum_video = 0;
	m_rtpTimeStamp_video = 0;
	m_rtplist_video = nullptr;
	m_rtpnum_audio = 0;
	m_rtpTimeStamp_audio=0;
	m_rtplist_audio=nullptr;
	m_video_ssrc = htonl2(GetTickCount());
	m_audio_ssrc = htonl2(GetTickCount());
}


CHbRtpMux::~CHbRtpMux()
{
	if (m_rtplist_video != nullptr)
	{
		for (int i = 0; i < max_rtp_count; i++)
		{
			if (m_rtplist_video[i].buffer!=nullptr)
			{
				free(m_rtplist_video[i].buffer);
			}
		}
		free(m_rtplist_video);
	}
}
uint16_t CHbRtpMux::htons2(uint16_t val)
{
	uint16_t temp1 = val << 8;
	uint16_t temp2 = val >> 8;
	return temp1 | temp2;
}
uint32_t CHbRtpMux::htonl2(uint32_t val)
{
	uint32_t temp1 = val << 24;
	uint32_t temp4 = val >> 24;
	uint32_t temp2 = (val & 0x0000ff00) << 8;
	uint32_t temp3 = (val & 0x00ff0000) >> 8;
	return temp1 | temp2 | temp3 | temp4;
}

int CHbRtpMux::packRtp(uint8_t* esbuffer, int eslen, int timestampInc, int isvideo, streamType st,bool bkey,uint8_t nalType,int ssrc)
{
	rtp_header header;
	int rtp_cnt = 0;//rtp_number

/*rtp_head*/
		/*byte0    V、P、X、CC*/
			header.VersionNum = 2;
			header.Padding = 0;
			header.Extension = 0;
			header.Csrc = 0;

		/*byte1    M、PT*/
			header.Ptype = 96;

		/*byte8.9.10.11     ssrc*/ //??
			header.ssrc = m_video_ssrc;
/*rtp_head*/

		
//RTP_Package   

	char  dst_rtp_buffer[FIXED_MTU_SZIE];
	int   dst_rtp_buffer_size;
	fu_h  fu_a;

/*单帧包*/
	if (eslen <= FIXED_SIZE_PER_RTP)
	{
		rtp_cnt = 1;
		memset(dst_rtp_buffer, 0, FIXED_MTU_SZIE);
	//rtp_header
		header.Marker = 0;
		/*byte2、3  seq_num*/
		header.seqNum = htons2(ssrc);
		if (m_rtpnum_video > max_rtp_num)
		{
			m_rtpnum_video = 0;
		}
		/*byte4.5.6.7    timestamp*/
		header.timeStamp = htonl2(m_rtpTimeStamp_video);

	//fu_indicate
		fu_a.fu_indicate_f = 0;
		fu_a.fu_indicate_type = nalType; 
		if (bkey)
			fu_a.fu_indicate_ref_idc = 3;
		else
			fu_a.fu_indicate_ref_idc = 1;
	//fu_header
		fu_a.fu_header_s_bit = 1;
		fu_a.fu_header_e_bit = 1;
		fu_a.fu_header_forbidden = 0;
		fu_a.fu_header_type = 28;

	//copy data 
		int len = 0;
		memcpy(m_rtplist_video[0].buffer + len, &header, sizeof(rtp_header));
		len += sizeof(rtp_header);
		memcpy(m_rtplist_video[0].buffer + len, &fu_a, sizeof(fu_h));
		len += sizeof(fu_h);
		memcpy(m_rtplist_video[0].buffer + len, esbuffer, max_rtp_len - len);
		m_rtplist_video[0].len = max_rtp_len;
	}

/*帧分包*/
	else if (eslen > FIXED_SIZE_PER_RTP)
	{
		memset(dst_rtp_buffer, 0, FIXED_MTU_SZIE);

	//rtp  timestamp 
		int key = esbuffer[4] & 0x1f;
		if (key == 1) //slice
		{
			header.timeStamp = htonl2(m_rtpTimeStamp_video);
		}	
		else if (key == 5)//idr
		{
			header.timeStamp = htonl2(m_rtpTimeStamp_video);
		}

		int k = 0,l=0;


		k = eslen / FIXED_SIZE_PER_RTP;
		l = eslen % FIXED_SIZE_PER_RTP;

		int t = 0;//当前第几个分片

		//分包数
		rtp_cnt = k + 1;

		if (t == 0)//第一包
		{
	//rtp_header
			header.Marker = 0;

			/*byte2、3  seq_num*/
			header.seqNum = htons2(ssrc);
			if (m_rtpnum_video > max_rtp_num)
			{
				m_rtpnum_video = 0;
			}
	//fu_indicate
			fu_a.fu_indicate_f = 0;
			fu_a.fu_indicate_type = 28; //FU-A type
			if (bkey)
				fu_a.fu_indicate_ref_idc = 3;
			else
				fu_a.fu_indicate_ref_idc = 1;
	//fu_header
			fu_a.fu_header_s_bit = 1;
			fu_a.fu_header_e_bit = 0;
			fu_a.fu_header_forbidden = 0;
			fu_a.fu_header_type = 28;

		}
		else if (k == t)//最后一包
		{
	//rtp_header
			header.Marker = 1;

			/*byte2、3  seq_num*/
			header.seqNum = htons2(ssrc);

			/*if (m_rtpnum_video > max_rtp_num)
			{
				m_rtpnum_video = 0;
			}*/

	//fu_indicate
			fu_a.fu_indicate_f = 0;
			fu_a.fu_indicate_type = 28; //FU-A type
			if (bkey)
				fu_a.fu_indicate_ref_idc = 3;
			else
				fu_a.fu_indicate_ref_idc = 1;
   //fu_header
			fu_a.fu_header_s_bit = 0;
			fu_a.fu_header_e_bit = 1;
			fu_a.fu_header_forbidden = 0;
			fu_a.fu_header_type = 28;

		}
		else if (t<k && 0 != t)//中间包
		{
	//rtp_header
			header.Marker = 0;

			/*byte2、3  seq_num*/
			header.seqNum = htons2(m_rtpnum_video++);
			if (m_rtpnum_video > max_rtp_num)
			{
				m_rtpnum_video = 0;
			}
	//fu_indicate
			fu_a.fu_indicate_f = 0;
			fu_a.fu_indicate_type = 28; //FU-A type
			if (bkey)
				fu_a.fu_indicate_ref_idc = 3;
			else
				fu_a.fu_indicate_ref_idc = 1;
	//fu_header
			fu_a.fu_header_s_bit = 0;
			fu_a.fu_header_e_bit = 0;
			fu_a.fu_header_forbidden = 0;
			fu_a.fu_header_type = 28;
		}

	//copy data 
		int len = 0;
		memcpy(m_rtplist_video[t].buffer + len, &header, sizeof(rtp_header));
		len += sizeof(rtp_header);
		memcpy(m_rtplist_video[t].buffer + len, &fu_a, sizeof(fu_h));
		len += sizeof(fu_h);
		memcpy(m_rtplist_video[t].buffer + len, esbuffer, max_rtp_len - len);
		m_rtplist_video[t].len = max_rtp_len;

		t++;
	}

	//timestamp
	if (timestampInc <= 0)
	{
		m_rtpTimeStamp_video += fix_rtp_TimeStampInc_video;
	}
	else
	{
		m_rtpTimeStamp_video += timestampInc;
	}
	if (m_rtpTimeStamp_video > max_rtp_TimeStamp)
	{
		m_rtpTimeStamp_video = 0;
	}

	//fu_h  fu_a;
	//fu_a.fu_indicate_f = 0;
	//fu_a.fu_header_forbidden = 0;
	//fu_a.fu_indicate_type = 28;//fu_a
	//fu_a.fu_header_type = nalType;
	//if (bkey)
	//{
	//	fu_a.fu_indicate_ref_idc = 3;
	//}
	//else
	//	fu_a.fu_indicate_ref_idc = 1;

	////第一包
	//header.Marker = 0;
	//header.seqNum = htons2(m_rtpnum_video++);
	//if (m_rtpnum_video > max_rtp_num)
	//{
	//	m_rtpnum_video = 0;
	//}
	//fu_a.fu_header_s_bit = 1;
	//fu_a.fu_header_e_bit = 0;
	//int len = 0;
	//memcpy(m_rtplist_video[0].buffer + len, &header, sizeof(rtp_header));
	//len += sizeof(rtp_header);
	//memcpy(m_rtplist_video[0].buffer + len, &fu_a, sizeof(fu_h));
	//len += sizeof(fu_h);
	//memcpy(m_rtplist_video[0].buffer + len,esbuffer,max_rtp_len-len);
	//m_rtplist_video[0].len = max_rtp_len;
	
	////其余
	//int es_restLen = eslen - max_rtp_len + len;
	//int retlen = 0;
	//int rtpeslen = max_rtp_len - sizeof(rtp_header)-sizeof(fu_h)+1;
	//int rtpcnt = es_restLen / rtpeslen;
	//int modcnt = es_restLen%rtpeslen;
	//for (int i = 0; i < rtpcnt; i++)
	//{
	//	if (i==rtpcnt-1 && modcnt==0)
	//	{
	//		header.Marker = 1;
	//	}else
	//	    header.Marker = 0;
	//	header.seqNum = htons2(m_rtpnum_video++);
	//	if (m_rtpnum_video>max_rtp_num)
	//	{
	//		m_rtpnum_video = 0;
	//	}
	//	//copy 1
	//	int len = 0;
	//	memcpy(m_rtplist_video[i].buffer + len, &header, sizeof(rtp_header));
	//	len += sizeof(rtp_header);
	//	//copy 2
	//	if (i==rtpcnt-1 && modcnt==0)
	//	{
	//		fu_a.fu_header_e_bit = 1;
	//	}else
	//	    fu_a.fu_header_e_bit = 0;

	//	memcpy(m_rtplist_video[i].buffer + len, &fu_a, sizeof(fu_h));
	//	len += sizeof(fu_h);
	//	memcpy(m_rtplist_video[i].buffer + len, esbuffer + retlen, rtpeslen);
	//	len += rtpeslen;
	//	m_rtplist_video[i].len = len;
	//	retlen += rtpeslen;
	//}

	//if (modcnt!=0)
	//{
	//	header.Marker = 1;
	//	header.seqNum = htons2(m_rtpnum_video++);
	//	if (m_rtpnum_video > max_rtp_num)
	//	{
	//		m_rtpnum_video = 0;
	//	}
	//	fu_a.fu_header_e_bit = 1;
	//	int len = 0;
	//	memcpy(m_rtplist_video[rtpcnt].buffer + len, &header, sizeof(rtp_header));
	//	len += sizeof(rtp_header);
	//	memcpy(m_rtplist_video[rtpcnt].buffer + len, &fu_a, sizeof(fu_h));
	//	len += sizeof(fu_h);
	//	memcpy(m_rtplist_video[rtpcnt].buffer + len, esbuffer + retlen, modcnt);
	//	len += modcnt;
	//	m_rtplist_video[rtpcnt].len = len;
	//}  

	return rtp_cnt;
}

int CHbRtpMux::packRtp2(uint8_t* esbuffer, int eslen, int timestampInc, int isvideo, streamType st, bool bkey, uint8_t nalType,int& ssrc)
{
	int rtp_cnt;
	vector<nal5_t> m_nals;
	for (int i = 0; i < eslen-4;i++)
	{
		if (esbuffer[i]==0x00 && esbuffer[i+1]==0x00)
		{
			if (esbuffer[i+2]==0x00 && esbuffer[i+3]==0x01)
			{
				nal5_t nal;
				nal.type = esbuffer[i + 4] & 0x1f;
				nal.s_posi=i;
				nal.startcodelen = 4;
				m_nals.push_back(nal);
				if (nal.type==nal_type_idr || nal.type==nal_type_slice)
				{
					break;
				}
				i += 4;
				continue;
			}
			else{//暂时不考虑

			}
		}
	}
	//
	for (int i = 0; i < m_nals.size();i++)
	{
		if (i==m_nals.size()-1)
		{
			m_nals[i].size = eslen - m_nals[i].s_posi;
			break;
		}
		m_nals[i].size = m_nals[i + 1].s_posi - m_nals[i].s_posi;
	}

	//
	for (int i = 0; i < m_nals.size();i++)
	{
		rtp_header header;

		/*rtp_head*/
		/*byte0    V、P、X、CC*/
		header.VersionNum = 2;
		header.Padding = 0;
		header.Extension = 0;
		header.Csrc = 0;

		/*byte1    M、PT*/
		header.Ptype = 96;

		/*byte8.9.10.11     ssrc*/ //??
		header.ssrc = m_video_ssrc;
		/*rtp_head*/

		fu_h  fu_a;

/*单帧包*/
		if (m_nals[i].size - m_nals[i].startcodelen < max_rtp_len - sizeof(rtp_header) - sizeof(fu_h) )
		{
			rtp_cnt = 1;
			//rtp_header
			header.Marker = 0;
			/*byte2、3  seq_num*/
			header.seqNum = htons2(m_rtpnum_video++);
			if (m_rtpnum_video > max_rtp_num)
			{
				m_rtpnum_video = 0;
			}
			/*byte4.5.6.7    timestamp*/
			header.timeStamp = htonl2(m_rtpTimeStamp_video);

			//fu_indicate
			fu_a.fu_indicate_f = 0;
			fu_a.fu_indicate_type = nalType;
			if (bkey)
				fu_a.fu_indicate_ref_idc = 3;
			else
				fu_a.fu_indicate_ref_idc = 1;
			//fu_header
			fu_a.fu_header_s_bit = 1;
			fu_a.fu_header_e_bit = 1;
			fu_a.fu_header_forbidden = 0;
			fu_a.fu_header_type = m_nals[i].type;

			//copy data 
			int len = 0;
			memcpy(m_rtplist_video[0].buffer + len, &header, sizeof(rtp_header));
			len += sizeof(rtp_header);
			memcpy(m_rtplist_video[0].buffer + len, &fu_a, sizeof(fu_h));
			len += sizeof(fu_h);
			memcpy(m_rtplist_video[0].buffer + len, &esbuffer[m_nals[i].s_posi + 5],m_nals[i].size);
			m_rtplist_video[0].len = len + m_nals[i].size - m_nals[i].startcodelen;
		}

/*帧分包*/
		else if (m_nals[i].size > max_rtp_len - sizeof(rtp_header) - sizeof(fu_h) )
		{
			//rtp  timestamp 
			int key = esbuffer[4] & 0x1f;
			if (key == 1) //slice
			{
				header.timeStamp = htonl2(m_rtpTimeStamp_video);
			}
			else if (key == 5)//idr
			{
				header.timeStamp = htonl2(m_rtpTimeStamp_video);
			}

			int k = 0, l = 0;

			k = (m_nals[i].size - m_nals[i].startcodelen - 1) / FIXED_SIZE_PER_RTP - sizeof(rtp_header)-sizeof(fu_h);
			l = (m_nals[i].size - m_nals[i].startcodelen - 1) % FIXED_SIZE_PER_RTP - sizeof(rtp_header)-sizeof(fu_h);

			int t = 0;//当前第几个分片
		
			//分包数
			rtp_cnt = k + 1;

			for (t=0; t < k;t++)
			{
				//rtp_header
				header.Marker = 0;

				/*byte2、3  seq_num*/
				header.seqNum = htons2(ssrc);
				if (m_rtpnum_video > max_rtp_num)
				{
					m_rtpnum_video = 0;
				}

				if (l == 0 && t == l - 1)
				{
					header.Marker = 1;
				}
				else{
					header.Marker = 0;
				}

				//fu_indicate
				fu_a.fu_indicate_f = 0;
				fu_a.fu_indicate_type = 28; //FU-A type
				if (bkey)
					fu_a.fu_indicate_ref_idc = 3;
				else
					fu_a.fu_indicate_ref_idc = 1;
				//fu_header
				fu_a.fu_header_s_bit = 1;
				fu_a.fu_header_e_bit = 0;
				fu_a.fu_header_forbidden = 0;
				fu_a.fu_header_type = m_nals[i].type;

				//copy data 
				int len = 0;
				memcpy(m_rtplist_video[t].buffer + len, &header, sizeof(rtp_header));
				len += sizeof(rtp_header);
				memcpy(m_rtplist_video[t].buffer + len, &fu_a, sizeof(fu_h));
				len += sizeof(fu_h);
				//memcpy(m_rtplist_video[t].buffer + len, esbuffer, max_rtp_len - len);
				memcpy(m_rtplist_video[t].buffer + len, &esbuffer[m_nals[i].s_posi + 5], m_nals[i].size);
				m_rtplist_video[t].len = max_rtp_len;

			}
		
			if (l > 0)//最后一包
			{
				//rtp_header
				header.Marker = 1;

				/*byte2、3  seq_num*/
				header.seqNum = htons2(ssrc);
				if (m_rtpnum_video > max_rtp_num)
				{
					m_rtpnum_video = 0;
				}

				//fu_indicate
				fu_a.fu_indicate_f = 0;
				fu_a.fu_indicate_type = 28; //FU-A type
				if (bkey)
					fu_a.fu_indicate_ref_idc = 3;
				else
					fu_a.fu_indicate_ref_idc = 1;
				//fu_header
				fu_a.fu_header_s_bit = 0;
				fu_a.fu_header_e_bit = 1;
				fu_a.fu_header_forbidden = 0;
				fu_a.fu_header_type = m_nals[i].type;


				//copy data 
				int len = 0;
				memcpy(m_rtplist_video[t].buffer + len, &header, sizeof(rtp_header));
				len += sizeof(rtp_header);
				memcpy(m_rtplist_video[t].buffer + len, &fu_a, sizeof(fu_h));
				len += sizeof(fu_h);
				//memcpy(m_rtplist_video[t].buffer + len, esbuffer, max_rtp_len - len);
				memcpy(m_rtplist_video[t].buffer + len, &esbuffer[m_nals[i].s_posi + 5], m_nals[i].size);
				m_rtplist_video[t].len = len + m_nals[i].size - 1;
			}
		
		
		}


		//timestamp
		if (timestampInc <= 0)
		{
			m_rtpTimeStamp_video += fix_rtp_TimeStampInc_video;
		}
		else
		{
			m_rtpTimeStamp_video += timestampInc;
		}
		if (m_rtpTimeStamp_video > max_rtp_TimeStamp)
		{
			m_rtpTimeStamp_video = 0;
		}
		
		
	}
	return rtp_cnt;
}

int CHbRtpMux::PackVideoFrame(uint8_t* esbuffer, int eslen, rtp2_t*& rtplist, int timestampInc, streamType st,bool bkey,int& ssrc)
{
	if (m_rtplist_video==nullptr)
	{
		m_rtplist_video = (rtp2_t*)calloc(max_rtp_count, sizeof(rtp2_t));
		if (m_rtplist_video==nullptr)
		{
			return 0; 
		}
		for (int i = 0; i < max_rtp_count;i++)
		{
			m_rtplist_video[i].buffer = (uint8_t*)calloc(max_rtp_len, 1);
			if (m_rtplist_video[i].buffer==nullptr)
			{
				goto fail;
			}
			m_rtplist_video[i].len = 0;
		}
	}
	rtplist = m_rtplist_video;

	return packRtp2(esbuffer, eslen, timestampInc, 1, st, bkey, esbuffer[4]&0x1f,ssrc);

fail:
	for (int i = 0; i < max_rtp_count; i++)
	{
		if (m_rtplist_video[i].buffer != nullptr)
		{
			free(m_rtplist_video[i].buffer);
		}
	}
	return 0;

}
