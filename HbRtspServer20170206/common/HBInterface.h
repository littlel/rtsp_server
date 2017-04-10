#ifndef HBINTERFACE_H_
#define HBINTERFACE_H_
#include <string>
#include <list>
#ifdef HAIBO_API
#define HAIBO_API extern "C" _declspec(dllimport) 
#else
#define HAIBO_API extern "C" _declspec(dllexport) 
#endif

//----------云台操作---------------//
#define HB_ZOOM_IN		1 //焦距变大(倍率变大) 
#define HB_ZOOM_OUT		2 //焦距变小(倍率变小) 
#define HB_TILT_UP		3 //云台上仰 
#define HB_TILT_DOWN	4 //云台下俯 
#define HB_PAN_LEFT		5 //云台左转 
#define HB_PAN_RIGHT	6 //云台右转 
#define HB_UP_LEFT		7 //云台上仰和左转 
#define HB_UP_RIGHT		8 //云台上仰和右转 
#define HB_DOWN_LEFT	9 //云台下俯和左转 
#define HB_DOWN_RIGHT	10 //云台下俯和右转 
#define HB_PRESET		11 //调用预置位
#define HB_ASSIST		12 //调用辅助位
#define HB_STOP         13 //停止

struct HB_3D_POSITION
{
	long bx;
	long by;
	long ex;
	long ey;
	long rx;
	long ry;
};

/*************************************************
Function:       // HB_VIDEO_CALLBACK
Description:    // 回调函数
Input:          // buffer-数据缓冲区，bufferSize-缓冲区大小
*************************************************/
typedef int (__stdcall *HB_VIDEO_CALLBACK)(long lplayHandle, char* buffer, long bufferSize);

/*************************************************
Function:       // NET_HB_Init
Description:    // SDK初始化
Input:          // 无
Output:         // 无
*************************************************/
HAIBO_API int NET_HB_Init();

/*************************************************
Function:       // NET_HB_Cleanup
Description:    // SDK释放
Input:          // 无
Output:         // 无
*************************************************/
HAIBO_API void NET_HB_Cleanup();

/*************************************************
Function:       // NET_HB_LoginEx
Description:    // 登录系统----内网登录
Input:          // szUser-用户名  szPwd-密码	szUnitIP-流媒体单元IP	Port-流媒体单元端口
Output:         // 无
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_LoginEx(const char* szUser, const char* szPwd, const char* szUnitIP, long Port);

/*************************************************
Function:       // NET_HB_Login
Description:    // 登录系统---公安网登录
Input:          // szUser-用户名	szPwd-密码	szServerIP-配置服务器IP	 Port-配置服务器端口
Output:         // 无
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_Login(const char* szUser, const char* szPwd, const char* szServerIP, long Port);

/*************************************************
Function:       // NET_HB_Logout
Description:    // 退出系统
Input:          // 无
Output:         // 无
Return:         // 无
*************************************************/
HAIBO_API void NET_HB_Logout(void);

/*************************************************
Function:       // NET_HB_GetCamResourceList
Description:    // 获取资源列表---公安网接口
Input:          // 无
Output:         // 无
Return:         // 无
*************************************************/
HAIBO_API int NET_HB_GetCamResourceList(std::string &strCamResourceInfo);

/*************************************************
Function:       // NET_HB_SwitchCam
Description:    // 切换摄像机---公安网接口
Input:          // lCam-摄像机编号
Output:         // 无
Return:         // 成功返回句柄，失败返回错误代码
*************************************************/
HAIBO_API int NET_HB_SwitchCam(long lCam);

/*************************************************
Function:       // NET_HB_SetVideoCallBack
Description:    // 设置回调函数
Input:          // callBack-回调函数
Output:         // 无
Return:         // 无
*************************************************/
HAIBO_API void NET_HB_SetVideoCallBack(HB_VIDEO_CALLBACK callBack);

/*************************************************
Function:       // NET_HB_SwitchCamUrl
Description:    // 切换摄像机---公安网接口
Input:          // lCam--摄像机编号
Output:         // VideoAddress--视频组播地址
Return:         // 成功返回句柄，失败返回错误代码
*************************************************/
HAIBO_API int NET_HB_SwitchCamUrl(long lCam,char *&VideoAddress);

/*************************************************
Function:       // NET_HB_PTZControl
Description:    // 云台控制---公安网接口
Input:          // lPlayHandle-实时调看句柄，lTZCommand-控制命令，lData-控制数据
Output:         // 无
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_PTZControl(long lPlayHandle, long lTZCommand, long lData);

/*************************************************
Function:       // NET_HB_AbandonCam
Description:    // 释放摄像机---公安网接口
Input:          // lPlayHandle-实时调看句柄
Output:         // 无
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_AbandonCam(long lPlayHandle);

/*************************************************
Function:       // NET_HB_AbandonCamUrl
Description:    // 释放摄像机---公安网接口
Input:          // lPlayHandle-实时调看句柄
Output:         // 无
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_AbandonCamUrl(long lPlayHandle);

/*************************************************
Function:       // NET_HB_SetPTZSpeed
Description:    // 设置云台速度
Input:          // lspeed-速度值
Output:         // 无
Return:         // 无
*************************************************/
HAIBO_API int NET_HB_SetPTZSpeed(long lspeed);

/*************************************************
Function:       // NET_HB_PTZPreset
Description:    // 预置位设置
Input:          // lCam-摄像机编号,lNo-预置位编号
Output:         // 无
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_PTZPreset(long lCam, long lNo);

/*************************************************
Function:       // NET_HB_FastPosition
Description:    // 快速定位
Input:          // lPlayHandle-实时调看句柄, Positon-控制坐标
Output:         // 
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_FastPosition(long lPlayHandle, HB_3D_POSITION &Positon);

/*************************************************
Function:       // NET_HB_QueryVideoRecordEx
Description:    // 历史录像查询---内网接口
Input:          // lCam-摄像机编号 ulStarttime-查询的开始时间 ulEndtime-查询的终止时间 szUnitIP-单元IP szUnitPort-单元端口 
Output:         // recordlist-历史文件列表xml
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_QueryVideoRecordEx(long CamID, unsigned long ulStarttime, unsigned long ulEndtime, const char* szUnitIP, int szUnitPort,char *&recordlist);

/*************************************************
Function:       // NET_HB_QueryVideoRecord
Description:    // 历史录像查询---公安网接口
Input:          // lCam-摄像机编号 ulStarttime-查询的开始时间 ulEndtime-查询的终止时间
Output:         // recordlist-历史文件列表xml
Return:         // 成功返回0，其他返回错误代码
*************************************************/
HAIBO_API int NET_HB_QueryVideoRecord(long CamID, unsigned long ulStarttime, unsigned long ulEndtime, char *&recordlist);

/*************************************************
Function:       // NET_HB_GetErrMsg
Description:    // 得到错误信息
Input:          // lError-错误代码
Output:         // 无
Return:         // 错误信息
*************************************************/
HAIBO_API const char* NET_HB_GetErrMsg(int lError);

#endif

