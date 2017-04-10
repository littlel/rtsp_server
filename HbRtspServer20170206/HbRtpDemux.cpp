#include "StdAfx.h"
#include "HbRtpDemux.h"
#include <algorithm>


bool comp(const rtp_t &a,const rtp_t &b)
{
	return a.seq_num<b.seq_num;
}

	FILE*  fips;
	

CHbRtpDemux::CHbRtpDemux(void)
{
	timestamp=0;
	currnumsamples=0;
	firstframe=true;
	v_rtp_list.clear();


}
CHbRtpDemux::~CHbRtpDemux(void)
{
	if (m_h264ptr!=NULL)
		free(m_h264ptr);
	if (m_audioptr!=NULL)
		free(m_audioptr);
}
bool CHbRtpDemux::open()
{
	m_h264ptr=(char*)malloc(MAX_H264_FRAME_SIZE);
	if (NULL==m_h264ptr)
	    return false;
	else
		memset(m_h264ptr,0,MAX_H264_FRAME_SIZE);


	m_audioptr=(char*)malloc(MAX_AUDIO_FRAME_SIZE);
	if (NULL==m_audioptr)
		return false;
	else
		memset(m_audioptr,0,MAX_AUDIO_FRAME_SIZE);
	//liyang
	fips  =  fopen("11.ps", "wb");

    return true;
}
void CHbRtpDemux::close()
{
	fclose(fips);
}



bool CHbRtpDemux::parse_video_rtp_packet(char* rtp_buffer_ptr,int rtp_buffer_size,char*& h264_frame_ptr,int& h264_frame_size,bool& b_key,int& mstimestamp,int payloadtype,int timecale)
{
	if (BadPtr(m_h264ptr))
	  return false;
	h264_frame_ptr=m_h264ptr;
	int  h264_frame_offset=0;//h264帧数据在rtp包中的起始位置
	unsigned char  paddinglen=0;
	//固定头
	unsigned char version=rtp_buffer_ptr[0]>>7&0x03;
	bool  padding=rtp_buffer_ptr[0]>>5&0x01;
	if (padding)
		paddinglen=rtp_buffer_ptr[rtp_buffer_size-1];
	bool  extension=rtp_buffer_ptr[0]>>4&0x01;
	int   numsofcsrc=rtp_buffer_ptr[0]&0x0f;
	bool  marker=rtp_buffer_ptr[1]>>7&0x01;
	unsigned  char  rtp_payload_type=rtp_buffer_ptr[1]&0x7f;
	if (rtp_payload_type!=payloadtype)
	 return false;
	unsigned  short sn=ntohs(*(unsigned short*)(rtp_buffer_ptr+2));
	unsigned  long  rtptimestamp = ntohl(*(unsigned long*)(rtp_buffer_ptr+4));
	unsigned  long  ssrc=ntohl(*(unsigned long*)(rtp_buffer_ptr+8));
	h264_frame_offset+=12;
	if (numsofcsrc>0)//csrc列表
	  h264_frame_offset+=numsofcsrc*4;
	//扩展头
	if (extension)
	{
		char* exptr=rtp_buffer_ptr+h264_frame_offset;
		unsigned  short extensionID=ntohs(*(unsigned short*)exptr);
		h264_frame_offset+=2;
		unsigned  short extensionLEN=ntohs(*(unsigned short*)(&exptr[2]));
		h264_frame_offset+=2;
		h264_frame_offset+=extensionLEN*4;
	}
	



	if (0==timestamp)
	{
		timestamp=rtptimestamp;
		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size=MAX_MTU_SIZE;  
		else
			tmp.rtpdata_size=rtp_buffer_size;
		memcpy_s(tmp.rtpdata,MAX_MTU_SIZE,rtp_buffer_ptr,tmp.rtpdata_size);
		tmp.h264_frame_offset=h264_frame_offset;
		tmp.h264_frame_len=tmp.rtpdata_size-tmp.h264_frame_offset-paddinglen;
		v_rtp_list.push_back(tmp);
		return false;
	}

	if (rtptimestamp!=timestamp)
	{

		if (firstframe)
		{
			 mstimestamp=0;
			 firstframe=false;
		}else
		     mstimestamp=(rtptimestamp-timestamp)*1.0/timecale*1000;//ms增量


		timestamp=rtptimestamp;
		unsigned char   fu_identity_type;//h264帧数据第一个字节
		unsigned char   fu_header_type;//h264帧数据第二个字节
		int rtp_packet_count=v_rtp_list.size(); 
		if (rtp_packet_count>MAX_RTP_PACKET_COUNT)
		   rtp_packet_count=MAX_RTP_PACKET_COUNT;
		int curr_h264_size=0;
		bool fua_first=true;
		for (int i=0;i<rtp_packet_count;i++)
		{
		
			int offset=v_rtp_list[i].h264_frame_offset;
			unsigned char rtp_1=(unsigned char)v_rtp_list[i].rtpdata[offset];
			unsigned char rtp_2=(unsigned char)v_rtp_list[i].rtpdata[offset+1];
			fu_identity_type=rtp_1&0x1f;
			fu_header_type=rtp_2&0x1f;

			
				
		
			if (NAL_TYPE_SPS==fu_identity_type || NAL_TYPE_PPS==fu_identity_type  || NAL_TYPE_IDR==fu_header_type || NAL_TYPE_SPS==fu_header_type/*明景相机*/)//关键桢
			{

				b_key=true;
				if (PACKET_TYPE_FU_A==fu_identity_type)//fu_a
				{
					if (fua_first)//第一次解析添加起始码，并添加nalheader
					{
						memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
						curr_h264_size+=4;
						unsigned char nal_header=(v_rtp_list[i].rtpdata[offset]&0xe0)|(v_rtp_list[i].rtpdata[offset+1]&0x1f);
						memcpy(h264_frame_ptr+curr_h264_size,&nal_header,1);
						curr_h264_size+=1;
						fua_first=false;
					}
					memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset+2]),v_rtp_list[i].h264_frame_len-2);
					curr_h264_size+=v_rtp_list[i].h264_frame_len-2;

				}else if (NAL_TYPE_SPS==fu_identity_type || NAL_TYPE_PPS==fu_identity_type || NAL_TYPE_SPS==fu_header_type || NAL_TYPE_IDR==fu_identity_type)//忽略fu_b
				{
					memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
					curr_h264_size+=4;
					memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset]),v_rtp_list[i].h264_frame_len);
					curr_h264_size+=v_rtp_list[i].h264_frame_len;
				}
			}else if (NAL_TYPE_SLICE==fu_header_type || NAL_TYPE_SLICE==fu_identity_type/*超低码率*/|| NAL_TYPE_DPA==fu_header_type || NAL_TYPE_DPA==fu_identity_type || NAL_TYPE_DPB==fu_header_type || NAL_TYPE_DPB==fu_identity_type || NAL_TYPE_DPC==fu_header_type || NAL_TYPE_DPC==fu_identity_type)//非关键桢
			{
				b_key=false;
				if (PACKET_TYPE_FU_A==fu_identity_type)//fu_a
				{
					if (fua_first)
					{
						memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
						curr_h264_size+=4;
						unsigned char nal_header=(v_rtp_list[i].rtpdata[offset]&0xe0)|(v_rtp_list[i].rtpdata[offset+1]&0x1f);
						memcpy(h264_frame_ptr+curr_h264_size,&nal_header,1);
						curr_h264_size+=1;
						fua_first=false;
					}
					memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset+2]),v_rtp_list[i].h264_frame_len-2);
					curr_h264_size+=v_rtp_list[i].h264_frame_len-2;

				}else
				{
					memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
					curr_h264_size+=4;
					memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset]),v_rtp_list[i].h264_frame_len);
					curr_h264_size+=v_rtp_list[i].h264_frame_len;

				}
				
			}else if (NAL_TYPE_SEI==fu_identity_type || NAL_TYPE_AUD==fu_identity_type )//附加信息
			{
				memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
				curr_h264_size+=4;
				memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset]),v_rtp_list[i].h264_frame_len);
				curr_h264_size+=v_rtp_list[i].h264_frame_len;
			}
		}
		h264_frame_size=curr_h264_size;

		v_rtp_list.clear();
		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size=MAX_MTU_SIZE;  
		else
			tmp.rtpdata_size=rtp_buffer_size;
		memcpy_s(tmp.rtpdata,MAX_MTU_SIZE,rtp_buffer_ptr,tmp.rtpdata_size);
		tmp.h264_frame_offset=h264_frame_offset;
		tmp.h264_frame_len=tmp.rtpdata_size-tmp.h264_frame_offset-paddinglen;
		v_rtp_list.push_back(tmp);
		return true;
	}else{

		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size=MAX_MTU_SIZE;  
		else
			tmp.rtpdata_size=rtp_buffer_size;
		memcpy_s(tmp.rtpdata,MAX_MTU_SIZE,rtp_buffer_ptr,tmp.rtpdata_size);
		tmp.h264_frame_offset=h264_frame_offset;
		tmp.h264_frame_len=tmp.rtpdata_size-tmp.h264_frame_offset-paddinglen;
		v_rtp_list.push_back(tmp);

		return false;
	}
}
bool CHbRtpDemux::parse_audio_rtp_packet(char* rtp_buffer_ptr,int rtp_buffer_size,char*& audio_frame_ptr,int& audio_frame_size,int& numsamples,int payloadtype)
{

	if (BadPtr(m_audioptr))
		return false;
	audio_frame_ptr=m_audioptr;
	int  audio_frame_offset=0;//音频帧数据在rtp包中的起始位置
	unsigned char  paddinglen=0;
	//固定头
	unsigned char version=rtp_buffer_ptr[0]>>7&0x03;
	bool  padding=rtp_buffer_ptr[0]>>5&0x01;
	if (padding)
		paddinglen=rtp_buffer_ptr[rtp_buffer_size-1];
	bool  extension=rtp_buffer_ptr[0]>>4&0x01;
	int   numsofcsrc=rtp_buffer_ptr[0]&0x0f;
	bool  marker=rtp_buffer_ptr[1]>>7&0x01;
	unsigned  char  rtp_payload_type=rtp_buffer_ptr[1]&0x7f;
	if (rtp_payload_type!=payloadtype)
		return false;
	unsigned  short sn=ntohs(*(unsigned short*)(rtp_buffer_ptr+2));
	unsigned  long  rtptimestamp = ntohl(*(unsigned long*)(rtp_buffer_ptr+4));
	unsigned  long  ssrc=ntohl(*(unsigned long*)(rtp_buffer_ptr+8));
	audio_frame_offset+=12;
	if (numsofcsrc>0)//csrc列表
		audio_frame_offset+=numsofcsrc*4;
	//扩展头
	if (extension)
	{
		char* exptr=rtp_buffer_ptr+audio_frame_offset;
		unsigned  short extensionID=ntohs(*(unsigned short*)exptr);
		audio_frame_offset+=2;
		unsigned  short extensionLEN=ntohs(*(unsigned short*)(&exptr[2]));
		audio_frame_offset+=2;
		audio_frame_offset+=extensionLEN*4;
	}

	audio_frame_size=rtp_buffer_size-audio_frame_offset-paddinglen;
	memcpy_s(audio_frame_ptr,MAX_AUDIO_FRAME_SIZE,rtp_buffer_ptr+audio_frame_offset,audio_frame_size);

	if (0==currnumsamples)
	{
		numsamples=0;
		currnumsamples=rtptimestamp;
	}else
	{
		numsamples=rtptimestamp-currnumsamples;
		currnumsamples=rtptimestamp;
	}

	return true;
}
bool CHbRtpDemux::parse_video_rtp_packet2(char* rtp_buffer_ptr,int rtp_buffer_size,char*& h264_frame_ptr,int& h264_frame_size,bool& b_key,int& mstimestamp,int payloadtype,int timecale)
{
	if (BadPtr(m_h264ptr))
		return false;
	h264_frame_ptr=m_h264ptr;
	int  h264_frame_offset=0;//h264帧数据在rtp包中的起始位置
	unsigned char  paddinglen=0;
	//固定头
	unsigned char version=rtp_buffer_ptr[0]>>7&0x03;
	bool  padding=rtp_buffer_ptr[0]>>5&0x01;
	if (padding)
		paddinglen=rtp_buffer_ptr[rtp_buffer_size-1];
	bool  extension=rtp_buffer_ptr[0]>>4&0x01;
	int   numsofcsrc=rtp_buffer_ptr[0]&0x0f;
	bool  marker=rtp_buffer_ptr[1]>>7&0x01;
	unsigned  char  rtp_payload_type=rtp_buffer_ptr[1]&0x7f;
	if (rtp_payload_type!=payloadtype)
		return false;
	unsigned  short sn=ntohs(*(unsigned short*)(rtp_buffer_ptr+2));
	unsigned  long  rtptimestamp = ntohl(*(unsigned long*)(rtp_buffer_ptr+4));
	unsigned  long  ssrc=ntohl(*(unsigned long*)(rtp_buffer_ptr+8));
	h264_frame_offset+=12;
	if (numsofcsrc>0)//csrc列表
		h264_frame_offset+=numsofcsrc*4;
	//扩展头
	if (extension)
	{
		char* exptr=rtp_buffer_ptr+h264_frame_offset;
		unsigned  short extensionID=ntohs(*(unsigned short*)exptr);
		h264_frame_offset+=2;
		unsigned  short extensionLEN=ntohs(*(unsigned short*)(&exptr[2]));
		h264_frame_offset+=2;
		h264_frame_offset+=extensionLEN*4;
	}




	if (0==timestamp)
	{
		timestamp=rtptimestamp;
		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size=MAX_MTU_SIZE;  
		else
			tmp.rtpdata_size=rtp_buffer_size;
		memcpy_s(tmp.rtpdata,MAX_MTU_SIZE,rtp_buffer_ptr,tmp.rtpdata_size);
		tmp.h264_frame_offset=h264_frame_offset;
		tmp.h264_frame_len=tmp.rtpdata_size-tmp.h264_frame_offset-paddinglen;
		v_rtp_list.push_back(tmp);
		return false;
	}

	if (rtptimestamp!=timestamp)
	{

		mstimestamp=rtptimestamp;



		timestamp=rtptimestamp;
		unsigned char   fu_identity_type;//h264帧数据第一个字节
		unsigned char   fu_header_type;//h264帧数据第二个字节
		int rtp_packet_count=v_rtp_list.size(); 
		if (rtp_packet_count>MAX_RTP_PACKET_COUNT)
			rtp_packet_count=MAX_RTP_PACKET_COUNT;
		int curr_h264_size=0;
		for (int i=0;i<rtp_packet_count;i++)
		{

			int offset=v_rtp_list[i].h264_frame_offset;
			unsigned char rtp_1=(unsigned char)v_rtp_list[i].rtpdata[offset];
			unsigned char rtp_2=(unsigned char)v_rtp_list[i].rtpdata[offset+1];
			fu_identity_type=rtp_1&0x1f;
			fu_header_type=rtp_2&0x1f;

			/*rfc6184*/
			/* Type      Type
			-------------------------------------------------------------
			0        reserved                                     -
			1-23     NAL unit  Single NAL unit packet             5.6
			24       STAP-A    Single-time aggregation packet     5.7.1
			25       STAP-B    Single-time aggregation packet     5.7.1
			26       MTAP16    Multi-time aggregation packet      5.7.2
			27       MTAP24    Multi-time aggregation packet      5.7.2
			28       FU-A      Fragmentation unit                 5.8
			29       FU-B      Fragmentation unit                 5.8
			30-31    reserved                                     -*/
			if (fu_identity_type>=1 && fu_identity_type<=23)//Single NAL unit packet 
			{
				memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
				curr_h264_size+=4;
				memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset]),v_rtp_list[i].h264_frame_len);
				curr_h264_size+=v_rtp_list[i].h264_frame_len;

			}else if (fu_identity_type>=24 && fu_identity_type<=25)//STAP-A,STAP-B
			{
				int h264framelen=v_rtp_list[i].h264_frame_len;
				int parsednalsize=1;
				switch(fu_identity_type)
				 {
				 case  24:
					 while(parsednalsize<h264framelen)
					 {
						 unsigned short temp=*((unsigned short*)(&(v_rtp_list[i].rtpdata[offset+1])));
						 unsigned short nalusize=NetHostBytes(temp);
						 parsednalsize+=2;
						 memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
						 curr_h264_size+=4;
						 memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset+parsednalsize]),nalusize);
						 curr_h264_size+=nalusize;
						 parsednalsize+=nalusize;
					 }
					 break;
				 case  25:

					 break;
				 default:

					 break;
				 }

			}else if (fu_identity_type>=26 && fu_identity_type<=27)//MTAP16 ,MTAP24
			{


			}else if (fu_identity_type>=28 && fu_identity_type<=29)// FU-A  ,FU-B
			{
				 bool  startofnal=((v_rtp_list[i].rtpdata[offset+1]>>7)&0x01);
				// bool  endofnal=((fu_header_type>>6)&0x01);
				 switch(fu_identity_type)
				  {
				  case 28:
					  if (startofnal)
					  {
						  memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
						  curr_h264_size+=4;
						  unsigned char nal_header=(v_rtp_list[i].rtpdata[offset]&0xe0)|(v_rtp_list[i].rtpdata[offset+1]&0x1f);
						  memcpy(h264_frame_ptr+curr_h264_size,&nal_header,1);
						  curr_h264_size+=1;
					  }
					  memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset+2]),v_rtp_list[i].h264_frame_len-2);
					  curr_h264_size+=v_rtp_list[i].h264_frame_len-2;
					  break;
				  case 29:
					  //暂时忽略DON
					  if (startofnal)
					  {
						  memcpy(h264_frame_ptr+curr_h264_size,h264_start_code,4);
						  curr_h264_size+=4;
						  unsigned char nal_header=(v_rtp_list[i].rtpdata[offset]&0xe0)|(v_rtp_list[i].rtpdata[offset+1]&0x1f);
						  memcpy(h264_frame_ptr+curr_h264_size,&nal_header,1);
						  curr_h264_size+=1;
					  }
					  memcpy(h264_frame_ptr+curr_h264_size,&(v_rtp_list[i].rtpdata[offset+4]),v_rtp_list[i].h264_frame_len-4);
					  curr_h264_size+=v_rtp_list[i].h264_frame_len-4;
					  break;
				  default:
					  break;
				  }

			}else
				continue;
		}

		h264_frame_size=curr_h264_size;




		/*判断是否关键帧*/

		for (int i=0;i<h264_frame_size-5;i++)
		{
			if (h264_frame_ptr[i]==0x00 && h264_frame_ptr[i+1]==0x00 && h264_frame_ptr[i+2]==0x00 && h264_frame_ptr[i+3]==0x01)
			{
				unsigned char keytype=h264_frame_ptr[i+4]&0x1f;
				if (keytype==7 || keytype==8 || keytype==5)
				{
					b_key=true;
					break;
				}else if (keytype>=1 && keytype<=4)
				{
					b_key=false;
					break;
				}else
					i+=4;
			}
		}








		v_rtp_list.clear();
		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size=MAX_MTU_SIZE;  
		else
			tmp.rtpdata_size=rtp_buffer_size;
		memcpy_s(tmp.rtpdata,MAX_MTU_SIZE,rtp_buffer_ptr,tmp.rtpdata_size);
		tmp.h264_frame_offset=h264_frame_offset;
		tmp.h264_frame_len=tmp.rtpdata_size-tmp.h264_frame_offset-paddinglen;
		v_rtp_list.push_back(tmp);
		return true;
	}else{

		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size=MAX_MTU_SIZE;  
		else
			tmp.rtpdata_size=rtp_buffer_size;
		memcpy_s(tmp.rtpdata,MAX_MTU_SIZE,rtp_buffer_ptr,tmp.rtpdata_size);
		tmp.h264_frame_offset=h264_frame_offset;
		tmp.h264_frame_len=tmp.rtpdata_size-tmp.h264_frame_offset-paddinglen;
		v_rtp_list.push_back(tmp);

		return false;
	}
}
bool CHbRtpDemux::parse_video_rtp_packet2_mpg2ps(char* rtp_buffer_ptr, int rtp_buffer_size, char*& h264_frame_ptr, int& h264_frame_size, bool& b_key, int& mstimestamp, int payloadtype, int timescale)
{
	if (BadPtr(m_h264ptr))
		return false;
	h264_frame_ptr = m_h264ptr;
	int  h264_frame_offset = 0;//h264帧数据在rtp包中的起始位置
	unsigned char  paddinglen = 0;
	//固定头
	unsigned char version = rtp_buffer_ptr[0] >> 7 & 0x03;
	bool  padding = rtp_buffer_ptr[0] >> 5 & 0x01;
	if (padding)
		paddinglen = rtp_buffer_ptr[rtp_buffer_size - 1];
	bool  extension = rtp_buffer_ptr[0] >> 4 & 0x01;
	int   numsofcsrc = rtp_buffer_ptr[0] & 0x0f;
	bool  marker = rtp_buffer_ptr[1] >> 7 & 0x01;
	unsigned  char  rtp_payload_type = rtp_buffer_ptr[1] & 0x7f;
	if (rtp_payload_type != payloadtype)
		return false;
	unsigned  short sn = ntohs(*(unsigned short*)(rtp_buffer_ptr + 2));
	unsigned  long  rtptimestamp = ntohl(*(unsigned long*)(rtp_buffer_ptr + 4));
	unsigned  long  ssrc = ntohl(*(unsigned long*)(rtp_buffer_ptr + 8));
	h264_frame_offset += 12;
	if (numsofcsrc > 0)//csrc列表
		h264_frame_offset += numsofcsrc * 4;
	//扩展头
	if (extension)
	{
		char* exptr = rtp_buffer_ptr + h264_frame_offset;
		unsigned  short extensionID = ntohs(*(unsigned short*)exptr);
		h264_frame_offset += 2;
		unsigned  short extensionLEN = ntohs(*(unsigned short*)(&exptr[2]));
		h264_frame_offset += 2;
		h264_frame_offset += extensionLEN * 4;
	}

	if (0 == timestamp)
	{
		timestamp = rtptimestamp;
		rtp_t tmp;
		if (rtp_buffer_size > MAX_MTU_SIZE)
			tmp.rtpdata_size = MAX_MTU_SIZE;
		else
			tmp.rtpdata_size = rtp_buffer_size;
		memcpy_s(tmp.rtpdata, MAX_MTU_SIZE, rtp_buffer_ptr, tmp.rtpdata_size);
		tmp.h264_frame_offset = h264_frame_offset;
		tmp.h264_frame_len = tmp.rtpdata_size - tmp.h264_frame_offset - paddinglen;
		//[add liyang]
		tmp.seq_num = sn;

		v_rtp_list.push_back(tmp);
		return false;
	}

	if (rtptimestamp != timestamp/*marker*/)
	{
		timestamp = rtptimestamp;

		//[对rtp包进行排序，乱序处理]
		sort(v_rtp_list.begin(),v_rtp_list.end(),comp);
		/*for (int i = 0; i<v_rtp_list.size();i++)
		{
			m_newList.insert(pair<int,int>(v_rtp_list[i].seq_num));
		}*/


		int curr_h264_size = 0;
		for (int i = 0; i<v_rtp_list.size();i++)
		{
			//[去除音频]
			//if (v_rtp_list[i].rtpdata[14+h264_frame_offset]==0x00 && v_rtp_list[i].rtpdata[15+h264_frame_offset]==0x00 && v_rtp_list[i].rtpdata[16+h264_frame_offset]==0x01)
			//{
			//	if ((unsigned char)(v_rtp_list[i].rtpdata[17+h264_frame_offset])==0xc0)
			//	{
			//		//printf("audio...\r\n");
			//		continue;
			//	}
			//}

			//liyang 
			//fwrite(v_rtp_list[i].rtpdata+h264_frame_offset,1,v_rtp_list[i].h264_frame_len,fips);
			//fflush(fips);



			memcpy(h264_frame_ptr + curr_h264_size,v_rtp_list[i].rtpdata+v_rtp_list[i].h264_frame_offset,v_rtp_list[i].h264_frame_len);
			curr_h264_size += v_rtp_list[i].h264_frame_len;
		}

		h264_frame_size = curr_h264_size;

		vector<rtp_t>().swap(v_rtp_list);
		rtp_t tmp;
		if (rtp_buffer_size>MAX_MTU_SIZE)
			tmp.rtpdata_size = MAX_MTU_SIZE;
		else
			tmp.rtpdata_size = rtp_buffer_size;
		memcpy_s(tmp.rtpdata, MAX_MTU_SIZE, rtp_buffer_ptr, tmp.rtpdata_size);
		tmp.h264_frame_offset = h264_frame_offset;
		tmp.h264_frame_len = tmp.rtpdata_size - tmp.h264_frame_offset - paddinglen;
		tmp.seq_num = sn;
		v_rtp_list.push_back(tmp);
		return true;

	}else{
		rtp_t tmp;
		if (rtp_buffer_size > MAX_MTU_SIZE)
			tmp.rtpdata_size = MAX_MTU_SIZE;
		else
			tmp.rtpdata_size = rtp_buffer_size;
		memcpy_s(tmp.rtpdata, MAX_MTU_SIZE, rtp_buffer_ptr, tmp.rtpdata_size);
		tmp.h264_frame_offset = h264_frame_offset;
		tmp.h264_frame_len = tmp.rtpdata_size - tmp.h264_frame_offset - paddinglen;
		tmp.seq_num = sn;
		v_rtp_list.push_back(tmp);
		return false;
	}
}
unsigned short CHbRtpDemux::NetHostBytes(unsigned short val)
{
	unsigned short  temp=val;
	unsigned short  retval=((val<<8)|(temp>>8&0x00ff));
	return retval;
}


