#include "stdafx.h"
#include "RtspListener.h"
#include <process.h>
#include <ws2ipdef.h>
#include <time.h>


CRtspListener::CRtspListener()
{
	WSADATA ws;
	WSAStartup(MAKEWORD(2,2),&ws);
	m_isExit = 0;
	m_isExit2 = 0;
	InitializeCriticalSection(&m_cric);
}

CRtspListener::~CRtspListener()
{
}

int  CRtspListener::startService(const char* ip, int port, int maxcount, int minrtpPort)
{
	strcpy(m_listenip, ip);
	m_listenport = port;
	m_taskcount = maxcount;

	/*socket*/
	m_listensock = socket(AF_INET, SOCK_STREAM, NULL);
	if (m_listensock == INVALID_SOCKET)
	{
		//LOG(ERROR)<<"监听端口绑定失败";
		return -1;
	}
	m_listenaddr.sin_family = AF_INET;
	m_listenaddr.sin_addr.S_un.S_addr = inet_addr(ip);
	m_listenaddr.sin_port = htons(port);
	int ret = bind(m_listensock, (sockaddr*)&m_listenaddr,sizeof(SOCKADDR_IN));
	if (ret==-1)
	{
		return -2;
	}
	ret = listen(m_listensock, SOMAXCONN);
	if (ret==SOCKET_ERROR)
	{
		return -3;
	}
	
	for (int i = 0; i < maxcount;i++)
	{
		rtsp_t* temp=(rtsp_t*)calloc(1, sizeof(rtsp_t));
		if (temp==NULL)
		{
			return -5;
		}
		temp->arg = this;
		temp->isUsed = 0;
		temp->cli_isExit = 0;
		temp->cli_isUsed2 = 0;
		temp->threadid = i;
		temp->isActive = 0;
		temp->hndRtpExit = CreateEvent(nullptr, false, false, nullptr);
		InitializeCriticalSection(&temp->cric_rtp);
		InitializeCriticalSection(&temp->cric_rtp2);
		InitializeCriticalSection(&temp->cric_rtp3);
		InitializeCriticalSection(&temp->cric_session);

		temp->rtpSock = socket(AF_INET, SOCK_DGRAM, NULL);
		if (temp->rtpSock==INVALID_SOCKET)
		{
			return -6;
		}

		SOCKADDR_IN     m_rtpaddr;
		m_rtpaddr.sin_family = AF_INET;
		m_rtpaddr.sin_addr.S_un.S_addr = inet_addr(ip);
		m_rtpaddr.sin_port = htons(minrtpPort+i*2);
		ret = bind(temp->rtpSock, (sockaddr*)&m_rtpaddr, sizeof(SOCKADDR_IN));
		if (ret == -1)
		{
			return -9;
		}

	//设置发送套接字缓冲区大小[add liyang 20170401]
		int sendbuf_len = MAX_SOCKET_RECV_BUFFER_SIZE;
		int len = sizeof(sendbuf_len);
		ret = setsockopt(temp->rtpSock, SOL_SOCKET, SO_RCVBUF, (const char *)&sendbuf_len, len);

		if (SOCKET_ERROR == ret)
		{
			return -10;
		}
	//[end 0401]

		temp->rtpPort = minrtpPort + i * 2;
		temp->rtcpSock = socket(AF_INET, SOCK_DGRAM, NULL);
		if (temp->rtcpSock == INVALID_SOCKET)
		{
			return -6;
		}
		SOCKADDR_IN     m_rtcpaddr;
		m_rtcpaddr.sin_family = AF_INET;
		m_rtcpaddr.sin_addr.S_un.S_addr = inet_addr(ip);
		m_rtcpaddr.sin_port = htons(temp->rtpPort+1);
		ret = bind(temp->rtcpSock, (sockaddr*)&m_rtcpaddr, sizeof(SOCKADDR_IN));
		if (ret == -1)
		{
			return -9;
		}
		temp->rtcpPort = temp->rtpPort + 1;

		temp->recvbuffer = (char*)calloc(1, max_mtu_size);
		if (temp->recvbuffer==NULL)
		{
			return -7;
		}
		temp->resbuffer = (char*)calloc(1, max_mtu_size);
		if (temp->resbuffer == NULL)
		{
			return -8;
		}
		m_threads.push_back(temp);
	}

	/*线程*/
	for (int i = 0; i < maxcount;i++)
	{
		HANDLE hnd = (HANDLE)_beginthreadex(nullptr, NULL, SessionThread, m_threads[i], NULL, nullptr);
		if (hnd == INVALID_HANDLE_VALUE)
		{
			return -6;
		}
		CloseHandle(hnd);

		hnd = (HANDLE)_beginthreadex(nullptr, NULL, RtpThread, m_threads[i], NULL, nullptr);
		if (hnd == INVALID_HANDLE_VALUE)
		{
			return -6;
		}
		CloseHandle(hnd);

		hnd = (HANDLE)_beginthreadex(nullptr, NULL, RtcpThread, m_threads[i], NULL, nullptr);
		if (hnd == INVALID_HANDLE_VALUE)
		{
			return -6;
		}
		CloseHandle(hnd);
	}
	
	HANDLE hnd = (HANDLE)_beginthreadex(nullptr, NULL, ListenThread, this, NULL, nullptr);
	if (hnd == INVALID_HANDLE_VALUE)
	{
		return -4;
	}
	CloseHandle(hnd);
	return 0;
}

void CRtspListener::closeService()
{

}

unsigned int _stdcall CRtspListener::ListenThread(void* arg)
{
	CRtspListener* pthis = (CRtspListener*)arg;
	
	while (!pthis->m_isExit)
	{
		SOCKET  sock = accept(pthis->m_listensock, nullptr, nullptr);
		if (sock == INVALID_SOCKET)
		{
			Sleep(20);
			continue;
		}

		SOCKADDR_IN from_addr = { 0 };
		int fromlen = sizeof(from_addr);
		getpeername(sock, (sockaddr*)&from_addr,&fromlen);
		char cli_ip[32] = { 0 };
		strcpy(cli_ip, inet_ntoa(from_addr.sin_addr));

		EnterCriticalSection(&pthis->m_cric);
		for (int i = 0; i < pthis->m_threads.size();i++)
		{
			if (!pthis->m_threads[i]->isUsed)
			{
				
				ULONG ul = 0;//阻塞
				int ret = ioctlsocket(sock, FIONBIO, (unsigned long*)&ul);
				if (ret == SOCKET_ERROR)
				{
					closesocket(sock);
					break;
				}
				// 接收缓冲区
				int nRecvBuf = 32 * 1024;//设置为32K
				ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
				if (ret == SOCKET_ERROR)
				{
					closesocket(sock);
					break;
				}
				//发送缓冲区
				int nSendBuf = 32 * 1024;//设置为32K
				ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));
				if (ret == SOCKET_ERROR)
				{
					closesocket(sock);
					break;
				}

				strcpy(pthis->m_threads[i]->cli_ip, cli_ip);
				closesocket(pthis->m_threads[i]->connSock);
				pthis->m_threads[i]->connSock = sock;

				EnterCriticalSection(&pthis->m_threads[i]->cric_session);
				pthis->m_threads[i]->isUsed = 1;
				LeaveCriticalSection (&pthis->m_threads[i]->cric_session);

				break;
			}else if (i==pthis->m_threads.size()-1)
			{
				closesocket(sock);
			}
		}
		LeaveCriticalSection(&pthis->m_cric);
	}
	return 0;
}

unsigned int _stdcall CRtspListener::SessionThread(void* arg)
{
	rtsp_t*  ctx = (rtsp_t*)arg;
	CRtspListener* pthis = (CRtspListener*)ctx->arg;
	fd_set   fd;
	timeval  ti;
	ti.tv_sec = 0;
	ti.tv_usec = 20000;//20ms
	CRtspAnalyzer rtspAnalyzer;
	while (!pthis->m_isExit2)
	{
		if (!ctx->isUsed)
		{
			Sleep(20);
			continue;
		}

		SYSTEMTIME st;
		GetLocalTime(&st);
		printf("%d)   <%04d-%02d-%02d %02d:%02d:%02d:%03d>sessionThread->1...<cli_ip:%s>\r\n",ctx->threadid,st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,ctx->cli_ip);

		int    isSetip = 0;
		char*  videoaddr;
		int    is_bye = 0;
		SOCKADDR_IN  fromaddr;
		int          fromlen = sizeof(fromaddr);
		memset(&fromaddr, 0, fromlen);

		int isKeepalive = GetPrivateProfileIntA("keepalive", "enable", 1, "./HbRtspServer20170206.ini");
		int Timeout = GetPrivateProfileIntA("keepalive", "timeout",5, "./HbRtspServer20170206.ini");
		rtspAnalyzer.setTimeout(Timeout);
		time_t KeepaliveTime;
		time_t KeepaliveTime2;
		time(&KeepaliveTime);
		int   videohandle = 0;
		while (!pthis->m_isExit2 && !is_bye)
		{
			/*
			  心跳超时
			*/
			time(&KeepaliveTime2);
			if (KeepaliveTime2-KeepaliveTime>Timeout+1)
			{
				is_bye = 1;
				printf("......timeout\r\n");
				continue;
			}
			
			FD_ZERO(&fd);
			FD_SET(ctx->connSock, &fd);
			int ret = select(0, &fd, nullptr, nullptr, &ti);
			if (ret>0 && FD_ISSET(ctx->connSock,&fd))//接收到rtsp报文
			{
				int recvlen = recvfrom(ctx->connSock, ctx->recvbuffer, max_mtu_size, NULL,(sockaddr*)&fromaddr,&fromlen);
				if (recvlen<=0 || recvlen>max_mtu_size)
				{
					if (recvlen<=0)
					{
						printf("......disconnect  %d\r\n",GetLastError());
						is_bye = 1;
					}
					continue;
				}
				if (!isSetip)
				{
					rtspAnalyzer.setip(pthis->m_listenip, ctx->rtpPort, ctx->rtcpPort,ctx->cli_ip);
					isSetip = 1;
				}

				e_rtsp_method method=rtspAnalyzer.analyzeMessage(ctx->recvbuffer,ctx->resbuffer,max_mtu_size);
				int reslen = strlen(ctx->resbuffer);
				if (reslen == 0)
				{
					continue;
				}
				/*解析*/
				switch (method)
				{
				case rtsp_describe:
					/*开始接入视频流*/
					ctx->cli_logicid = rtspAnalyzer.getRtspChannelNum();
					videohandle = NET_HB_SwitchCamUrl(ctx->cli_logicid, videoaddr);


					//liyang_test
					//videohandle = 20000;


					if (videohandle<10000)
					{
						is_bye = 1;//相机打开失败
						rtspAnalyzer.setStateNode(404,ctx->resbuffer,max_mtu_size);//not found
						printf("......相机打开失败%d<%d>\r\n",ret,ctx->cli_logicid);
					}
					else{
						/*解析url udp://239.3.4.5:3000*/

						//liyang
						//videoaddr = "udp://239.7.1.33:5000";

						memset(ctx->video_multicast_recvip, 0, 32);
						string addr = videoaddr;
						addr = addr.substr(6);
						int pos = addr.find(":");
						if (pos>0 && pos < addr.length())
						{
							strncpy(ctx->video_multicast_recvip, videoaddr + 6, pos);
							string port = addr.substr(pos + 1);
							ctx->video_multicast_recvport = atoi(port.c_str());

							SYSTEMTIME st;
							GetLocalTime(&st);
							printf("%d)   <%04d-%02d-%02d %02d:%02d:%02d:%03d>sessionThread->video recving...\r\n\t\t<logicid:%d><recvip:%s><recvport:%d>\r\n", ctx->threadid,st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,ctx->cli_logicid,ctx->video_multicast_recvip,ctx->video_multicast_recvport);

						}
					}
					break;
				case rtsp_play:
					rtspAnalyzer.getCliContext(ctx->cli_ip, ctx->cli_rtpPort, ctx->cli_rtcpPort);
					ctx->cli_ssrc = rtspAnalyzer.getssrc();
					EnterCriticalSection(&ctx->cric_rtp);
					ctx->cli_isUsed2 = 1;//开始转发
					LeaveCriticalSection(&ctx->cric_rtp);
					break;
				case rtsp_teardown:
					is_bye = 1;//rtsp退出
					break;
				case rtsp_get_parameter:
					time(&KeepaliveTime);
					break;
				default:
					break;
				}
				/*回复*/
				ret = send(ctx->connSock, ctx->resbuffer, reslen, NULL);
			}
		}

		EnterCriticalSection(&ctx->cric_rtp2);
		ctx->cli_isBye = 1;//rtp退出
		LeaveCriticalSection(&ctx->cric_rtp2);
		if (ctx->isActive)
		{
			WaitForSingleObject(ctx->hndRtpExit, INFINITE);
			EnterCriticalSection(&ctx->cric_rtp3);
			ctx->isActive = 0;
			LeaveCriticalSection(&ctx->cric_rtp3);
		}

		EnterCriticalSection(&ctx->cric_session);
		ctx->isUsed = 0;
		LeaveCriticalSection(&ctx->cric_session);
		NET_HB_AbandonCamUrl(videohandle);


		GetLocalTime(&st);
		printf("%d)   <%04d-%02d-%02d %02d:%02d:%02d:%03d>sessionThread->2...\r\n", ctx->threadid,st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	}

	SYSTEMTIME st;
	GetLocalTime(&st);
	printf("sessionThread->3...<%04d-%02d-%02d %02d:%02d:%02d:%03d>\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	closesocket(ctx->connSock);
	free(ctx->recvbuffer);
	free(ctx->resbuffer);
	free(ctx);
	return 0;
}

//RTP处理线程
unsigned int _stdcall CRtspListener::RtpThread(void* arg)
{
	rtsp_t*  ctx = (rtsp_t*)arg;
	CRtspListener* pthis = (CRtspListener*)ctx->arg;
	SOCKET   rtpSock = ctx->rtpSock;

//ps stream
	CHbRtpDemux rtpobj;
	if (!rtpobj.open())
		return NULL;
	char* h264_frame_ptr;//h264帧指针
	int   h264_frame_len;//h264帧长度
	bool  keyframe;
	int   mstimestamp = 0;

	char*  m_esbuffer;
	int    m_esbufferLen;
	CHbMpeg2psDemux m_psdemux;

	m_esbuffer = (char*)calloc(1, max_mpeg2ps_len);

	if (m_esbuffer == NULL)
	{
		return false;
	}
	m_esbufferLen = 0;

	CHbRtpMux m_rtpmux;
	rtpMuxer_udp m_rtpmux2;
//ps_stream  end



	while (!ctx->cli_isExit)
	{

	

		if (!ctx->cli_isUsed2)
		{
			Sleep(20);
			continue;
		}
		if (!m_rtpmux2.open())
		{
			printf("rtp mux err\r\n");
			continue;
		}
		

		SYSTEMTIME st;
		GetLocalTime(&st);
		printf("%d)   <%04d-%02d-%02d %02d:%02d:%02d:%03d>rtpthread->1...<cli_ip:%s>\r\n",ctx->threadid,st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,ctx->cli_ip);

		EnterCriticalSection(&ctx->cric_rtp);
		ctx->cli_isUsed2 = 0;
		LeaveCriticalSection(&ctx->cric_rtp);

		EnterCriticalSection(&ctx->cric_rtp2);
		ctx->cli_isBye = 0;
		LeaveCriticalSection(&ctx->cric_rtp2);

		EnterCriticalSection(&ctx->cric_rtp3);
		ctx->isActive = 1;
		LeaveCriticalSection(&ctx->cric_rtp3);
		/*socket 相关*/
		SOCKET recv_sock = socket(AF_INET, SOCK_DGRAM, NULL);
		if (recv_sock==INVALID_SOCKET)
		{
			goto fail;
		}
		int     opt = 1;
		int ret = setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
		if (SOCKET_ERROR == ret)
		{
			goto fail;
		}
		SOCKADDR_IN  recv_addr;
		recv_addr.sin_addr.S_un.S_addr = inet_addr(pthis->m_listenip);
		recv_addr.sin_family = AF_INET;
		recv_addr.sin_port = htons(ctx->video_multicast_recvport);
		ret = bind(recv_sock, (sockaddr*)&recv_addr, sizeof(SOCKADDR_IN));
		if (ret == SOCKET_ERROR)
		{
			printf("bind socket error\r\n");
			goto fail;
		}

		int rcvbuf_len = MAX_SOCKET_RECV_BUFFER_SIZE;
		int len = sizeof(rcvbuf_len);
		ret = setsockopt(recv_sock, SOL_SOCKET, SO_RCVBUF, (const char *)&rcvbuf_len, len);
		
		if (SOCKET_ERROR == ret)
		{
			printf("set rec sbuffer error\r\n");
			goto fail;
		}
		
		ip_mreq mcast;
		mcast.imr_interface.S_un.S_addr = inet_addr(pthis->m_listenip);
		mcast.imr_multiaddr.S_un.S_addr = inet_addr(ctx->video_multicast_recvip);
		ret = setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mcast, sizeof(mcast));
		if (SOCKET_ERROR == ret)
		{
			printf("set rec socketopt error\r\n");
			goto fail;
		}
		char   recv_buffer[max_mtu_size];
		SOCKADDR_IN send_addr;
		send_addr.sin_family = AF_INET;
		send_addr.sin_addr.S_un.S_addr = inet_addr(ctx->cli_ip);
		send_addr.sin_port = htons(ctx->cli_rtpPort);
		fd_set fd;
		timeval ti;
		ti.tv_sec = 0;
		ti.tv_usec = 20000;//20ms


		
		//保存文件
		/*FILE*  fih264;
		fopen_s(&fih264, "11.h264", "wb");*/
		SYSTEMTIME sys1,sys2; 
		


		while (!ctx->cli_isExit && !ctx->cli_isBye)
		{
			FD_ZERO(&fd);
			FD_SET(recv_sock, &fd);
			ret = select(recv_sock+1, &fd, nullptr, nullptr, &ti);
			if (ret > 0 && FD_ISSET(recv_sock, &fd))
			{
			
				/*接收video*/
				ret = recv(recv_sock, recv_buffer, max_mtu_size, NULL);
				if (ret>0 && ret<max_mtu_size)
				{
					unsigned int  ssrc = htonl(ctx->cli_ssrc);
					int* ssrc2 = (int*)(recv_buffer + 8);
					*ssrc2 = ssrc;
					/*ps2es*/
					bool bret = rtpobj.parse_video_rtp_packet2_mpg2ps(recv_buffer, ret, h264_frame_ptr, h264_frame_len, keyframe, mstimestamp,96, 90000);
				
					//printf("bret:%d\n",bret);

					if (bret)
					{	
						GetLocalTime( &sys1 );
						int type = 0;
						/*ps解复用*/
						m_esbufferLen =m_psdemux.Mpeg2psFrame_Demux((uint8_t*)h264_frame_ptr, h264_frame_len, (uint8_t*)m_esbuffer, max_mpeg2ps_len, type);

						//[add liyang 170405]
						//if(type != 1 )//过滤掉音频，只封装视频
						//{
							if (m_esbufferLen > 0)
							{
								rtp6_t* rtpList;
								int rtpcount;
								//rtp打包(强转效率低)
								//rtpcount = m_rtpmux.PackVideoFrame((uint8_t*)m_esbuffer, m_esbufferLen, rtpList, 3600, st_h264, keyframe,ssrc);
								rtpcount = m_rtpmux2.pack_video(rtpList, m_esbuffer, m_esbufferLen, keyframe, 0, ssrc);
								//printf("rtpcount:%d\n",rtpcount);
								for (int i = 0; i < rtpcount;i++)
								{
									/*转发video*/
									sendto(rtpSock, (char*)rtpList[i].buffer, rtpList[i].len, NULL, (sockaddr*)&send_addr, sizeof(SOCKADDR_IN));
								}
							}
						//}
						GetLocalTime( &sys2);

					//printf("一帧耗时：%3d ms\r\n",sys2.wMilliseconds - sys1.wMilliseconds);
						
					}
					if (ssrc > max_rtp_num)
						ssrc = 0;
				}
				else
				{
					printf("recv error：%d\r\n",ret);
				}
				
			}
			/*if (fih264)
			{
				fclose(fih264);
			}*/
		}
	fail:
		closesocket(recv_sock);
		GetLocalTime(&st);
		printf("%d)   <%04d-%02d-%02d %02d:%02d:%02d:%03d>rtpthread->2...\r\n", ctx->threadid, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		SetEvent(ctx->hndRtpExit);

		m_rtpmux2.close();
	}

	SYSTEMTIME st;
	GetLocalTime(&st);
	printf("rtpthread->3...<%04d-%02d-%02d %02d:%02d:%02d:%03d>\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);


	


	free(m_esbuffer);
	closesocket(rtpSock);
	return NULL;
}

unsigned int _stdcall CRtspListener::RtcpThread(void* arg)
{
	rtsp_t*  ctx = (rtsp_t*)arg;
	CRtspListener* pthis = (CRtspListener*)ctx->arg;
	SOCKET   rtcpSock = ctx->rtcpSock;
	while (!ctx->cli_isExit)
	{
		Sleep(200);
	}
	closesocket(rtcpSock);
	return NULL;
}
