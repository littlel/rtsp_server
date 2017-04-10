// HbRtspServer20170206.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "RtspListener.h"
#include <windows.h>
#include "common/HBInterface.h"


#pragma comment(lib,"HBNetSDK")





//获取当前程序的运行目录
string GetAppPathA()
{
	char szExePath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szExePath, MAX_PATH);
	char *pstr = strrchr(szExePath, '\\');
	memset(pstr + 1, 0, 1);
	string strAppPath(szExePath);
	return strAppPath;
}



int _tmain(int argc, _TCHAR* argv[])
{

	//日志对象初始化[add liyang 170110]
	//PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("log4cplus.cfg"));//载入配置
	//gvlog = Logger::getInstance(LOG4CPLUS_TEXT("HBRTSP_SERVER"));
	//SharedAppenderPtr pConsoleAppender(new ConsoleAppender());
	//gvlog.addAppender(pConsoleAppender);
	//LOG4CPLUS_DEBUG(gvlog, "～日志对象初始化完成～");
	

	system("COLOR B");

	/*初始化*/
	char ip[256] = { 0 };
	GetPrivateProfileStringA("net", "listenip", "127.0.0.1", ip, 256, "./HbRtspServer20170206.ini");
	int port=GetPrivateProfileIntA("net", "listenport", 554, "./HbRtspServer20170206.ini");
	int maxtask = GetPrivateProfileIntA("net", "maxtask", 8, "./HbRtspServer20170206.ini");
	int minrtpPort = GetPrivateProfileIntA("net", "minrtpPort",30000, "./HbRtspServer20170206.ini");
	//LOG(INFO) << "本地监听端口：" <<port<< " 最大任务数量："<<maxtask<<" 最小rtp端口："<<minrtpPort;

	/*平台登录*/
	char login_ip[256] = { 0 };
	GetPrivateProfileStringA("login", "ip", "127.0.0.1", login_ip, 256, "./HbRtspServer20170206.ini");
	int login_port = GetPrivateProfileIntA("login", "port", 8080, "./HbRtspServer20170206.ini");
	char login_user[256] = { 0 };
	GetPrivateProfileStringA("login", "user", "admin", login_user, 256, "./HbRtspServer20170206.ini");
	char login_password[256] = { 0 };
	GetPrivateProfileStringA("login", "password", "admin", login_password, 256, "./HbRtspServer20170206.ini");
	
	NET_HB_Init();

	int ret = NET_HB_Login(login_user, login_password, login_ip, login_port);
	if (ret!=0)
	{
		exit(0);
	}

	/*启动rtsp服务*/
	CRtspListener myRtspService;
	ret=myRtspService.startService(ip, port, maxtask,minrtpPort);
	if (-1==ret)
	{
		exit(0);
	}
	while (true)
	{
		Sleep(100);
	}
	return 0;
}

