#pragma once
typedef unsigned char uint8_t;
typedef unsigned long long uint64_t;
#define BUF2U16(buf) (((buf)[0] << 8) | (buf)[1])
#define MAX_STREAM_NUM (4)
typedef struct
{
	char start_code[4];				// == 0x000001BA
	char scr[6];
	char program_mux_rate[3];		// == 0x000003
	char pack_stuffing_length;		// == 0xF8
} ps_pack_header;
typedef struct
{
	char start_code[4];				// == 0x000001BB
	char header_length[2];			// == 6 + 3 * stream_count(2)
	char rate_bound[3];				// == 0x800001
	char CSPS_flag : 1,				// == 0
	fixed_flag : 1,				// == 0
			 audio_bound : 6;			// audio stream number
	char video_bound : 5,			// video stream number
	marker_bit : 1,				// == 1
			 system_video_lock_flag : 1,	// == 1
								  system_audio_lock_flag : 1;	// == 1
	char reserved_byte;				// == 0xFF
}ps_system_header;
typedef struct
{
	char start_code[4];				// == 0x000001BC
	char header_length[2];			// == 6 + es_map_length
	char ps_map_version : 5,			// == 0
	reserved1 : 2,				// == 3
			current_next_indicator : 1;	// == 1
	char marker_bit : 1,				// == 1
	reserved2 : 7;				// == 127
	char ps_info_length[2];			// == 0
	char es_map_length[2];			// == 4 * es_num
}ps_map;
typedef struct
{
	char stream_type;
	char es_id;
	char es_info_length[2];			// == 0
}ps_map_es;
class CHbMpeg2psDemux
{
public:
	CHbMpeg2psDemux();
	~CHbMpeg2psDemux();
private:
	uint8_t*   m_esbuffer;
	int        m_esbufferLen;
private:
	int handle_header(uint8_t *buf, int len);
	int handle_system_header(uint8_t *buf, int len);
	int handle_common_pack(uint8_t *buf, int len);
	int handle_map(uint8_t *buf, int len);
	int handle_finish(uint8_t *buf, int len);
	int handle_pes(uint8_t *buf, int len,uint64_t* pts,int type);
	int lts_pes_parse_header(uint8_t *pes, int len, uint8_t *stream_id, uint64_t *pts, int *es_len);
	int get_pes_head_len(uint8_t *pes, int len);
public:
	int Mpeg2psFrame_Demux(uint8_t* psbuffer,int pslen,uint8_t* esbuffer,int maxlen,int& type);
};

