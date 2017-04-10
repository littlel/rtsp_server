#include "stdafx.h"
#include "HbMpeg2psDemux.h"
#include <stdio.h>

CHbMpeg2psDemux::CHbMpeg2psDemux()
{
}


CHbMpeg2psDemux::~CHbMpeg2psDemux()
{
}
/*
PES stream id table:
stream_id	stream coding
1011 1100	program_stream_map
1011 1101	private_stream_1
1011 1110	padding_stream
1011 1111	private_stream_2
110x xxxx	ISO/IEC 13818-3 or ISO/IEC 11172-3 or ISO/IEC 13818-7 or ISO/IEC14496-3 audio stream number x xxxx
1110 xxxx	ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 or ISO/IEC14496-2 video stream number xxxx
1111 0000	ECM_stream
1111 0001	EMM_stream
1111 0010	ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A or ISO/IEC 13818-6_DSMCC_stream
1111 0011	ISO/IEC_13522_stream
1111 0100	ITU-T Rec. H.222.1 type A
1111 0101	ITU-T Rec. H.222.1 type B
1111 0110	ITU-T Rec. H.222.1 type C
1111 0111	ITU-T Rec. H.222.1 type D
1111 1000	ITU-T Rec. H.222.1 type E
1111 1001	ancillary_stream
1111 1010	ISO/IEC14496-1_SL-packetized_stream
1111 1011	ISO/IEC14496-1_FlexMux_stream
1111 11xx	reserved data stream
1111 1111	program_stream_directory
*/
int CHbMpeg2psDemux::Mpeg2psFrame_Demux(uint8_t* psbuffer, int pslen, uint8_t* esbuffer, int maxlen,int& type)
{
	m_esbuffer = esbuffer;
	m_esbufferLen = 0;

	int ret_len = 0;
	uint64_t pts;
	for (int i = 0; i < pslen-4;i++)
	{
		if (psbuffer[i]==0x00 &&psbuffer[i+1]==0x00 && psbuffer[i+2]==0x01)
		{
			ret_len = 0;
			int type = psbuffer[i + 3];
			if (type < 0xB9)
			{
				continue;
			}
			switch (type)
			{
			case 0xBA:
				// PS header
				ret_len = handle_header(psbuffer + i, pslen - i);
				break;

			case 0xBB:
				// PS system header
				ret_len = handle_system_header(psbuffer+ i, pslen - i);
				break;

			case 0xBC:
				// PS map
				ret_len = handle_map(psbuffer + i, pslen - i);
				break;

			case 0xB9:
				// PS finish
				ret_len = handle_finish(psbuffer + i, pslen - i);
				break;
			case 0xF1:
				//EMM_stream
				ret_len = handle_common_pack(psbuffer + i, pslen - i);
				break;
			default:
				// PES
				if (type>=0xe0 && type<=0xef)
				{
					type = 0;//video
					ret_len = handle_pes(psbuffer + i, pslen - i,&pts,type);
					//type = 0;//video
				}
				else if (type >= 0xc0 && type <= 0xdf)
				{
					type = 1;//audio
					ret_len = handle_pes(psbuffer + i, pslen - i,&pts,type);

					
				}
				break;
			}
			if (ret_len>0)
			{
				i += ret_len;
				i--;
			}
		}
	}

	char log[32] = { 0 };
	sprintf_s(log, 32, "%I64d\r\n", pts);

	//printf("pts: %s",log);

//	OutputDebugStringA(log);

	return m_esbufferLen;
}

int CHbMpeg2psDemux::handle_header(uint8_t *buf, int len)
{
	int psize = 0;
	int staffing_len = 0;
	ps_pack_header *head = (ps_pack_header *)buf;

	if (len < sizeof(ps_pack_header))
	{
		return 0;
	}

	staffing_len = head->pack_stuffing_length & 0x07;
	psize = sizeof(ps_pack_header)+staffing_len;
	if (len < psize)
	{
		return 0;
	}
	return psize;
}

int CHbMpeg2psDemux::handle_system_header(uint8_t *buf, int len)
{
	int psize = handle_common_pack(buf, len);
	ps_system_header *head;
	if (psize > 0)
	{
		head = (ps_system_header *)buf;
	}
	return psize;
}

int CHbMpeg2psDemux::handle_common_pack(uint8_t *buf, int len)
{
	int psize = 0;

	if (len < 6)
	{
		return 0;
	}

	psize = 6 + BUF2U16(&buf[4]);
	if (len < psize)
	{
		return 0;
	}

	return psize;
}

int CHbMpeg2psDemux::handle_map(uint8_t *buf, int len)
{
	int psize = handle_common_pack(buf, len);

	if (psize > 0)
	{
		ps_map *head = (ps_map *)buf;
		int info_len = BUF2U16(head->ps_info_length);
		int map_len = BUF2U16(head->es_map_length + info_len);
		uint8_t *map_buf = buf + sizeof(ps_map)+info_len;
		int pos = 0;
		int i = 0;

		while (pos < map_len)
		{
			ps_map_es *mes = (ps_map_es *)(map_buf + pos);
			int es_info_len = BUF2U16(mes->es_info_length);
			if (es_info_len < 0)
				break;

			if (i >= MAX_STREAM_NUM)
			{
				break;
			}

			pos += (sizeof(ps_map_es)+es_info_len);
		}
	}

	return psize;
}

int CHbMpeg2psDemux::handle_finish(uint8_t *buf, int len)
{
	int psize = 4;
	return psize;
}

int CHbMpeg2psDemux::handle_pes(uint8_t *buf, int len,uint64_t* pts,int type)
{
	int psize = handle_common_pack(buf, len);
	if (psize > 0)
	{
		uint8_t stream_id;
		int es_len;
		int pes_head_len = lts_pes_parse_header(buf, len, &stream_id, pts, &es_len);
		if (pes_head_len > 0)
		{
			if(type==0)
			{
				memcpy(m_esbuffer + m_esbufferLen, buf + pes_head_len, es_len);
			    m_esbufferLen += es_len;
			}
			
		}
	}
	return psize;
}

int CHbMpeg2psDemux::lts_pes_parse_header(uint8_t *pes, int len, uint8_t *stream_id, uint64_t *pts, int *es_len)
{
	int pes_head_len = get_pes_head_len(pes, len);
	if (pes_head_len <= 0)
	{
		return 0;
	}

	if (stream_id)
	{
		*stream_id = pes[3];
	}

	// 解析PTS
	if (pts)
	{
		uint8_t flags_2 = pes[7];
		if (flags_2 & 0x80)
		{
			uint8_t *pts_buf = &pes[9];
			*pts = ((uint64_t)pts_buf[0] & 0x0E) << 29;
			*pts |= ((uint64_t)pts_buf[1]) << 22;
			*pts |= ((uint64_t)pts_buf[2] & 0xFE) << 14;
			*pts |= ((uint64_t)pts_buf[3]) << 7;
			*pts |= ((uint64_t)pts_buf[4] & 0xFE) >> 1;
			*pts /= 90;
		}
	}

	// 解析ES长度
	if (es_len)
	{
		int pes_len = BUF2U16(&pes[4]);
		int pes_head_len = pes[8];
		*es_len = pes_len - 3 - pes_head_len;
	}

	return pes_head_len;
}

int CHbMpeg2psDemux::get_pes_head_len(uint8_t *pes, int len)
{
	int pes_head_len = 0;

	if (len < 9)
	{
		return 0;
	}

	if (pes[0] == 0 && pes[1] == 0 && pes[2] == 1)
	{
		if ((pes[3] & 0xC0) || (pes[3] & 0xE0))
		{
			pes_head_len = 9 + pes[8];
		}
	}

	return pes_head_len;
}
