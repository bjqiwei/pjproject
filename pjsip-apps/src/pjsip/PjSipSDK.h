#pragma once
#include <string>
#include <log4cplus/logger.h>
#include <pjsua2.hpp>
#include <mutex>

#define PJSIP_VERSION "1.0.0.0"

class CAccount :public pj::Account {
public:
	CAccount(class CPjSipSDK* plugin);
	~CAccount();
	virtual void onRegStarted(pj::OnRegStartedParam & prm) override;// 注册或注销登记时通知申请
	virtual void onRegState(pj::OnRegStateParam &prm) override;//登录状态改变回调
	virtual void onCallState(const pj::CallInfo & ci);//通话状态改变回调
	virtual void onDtmfDigit(pjsua_call_id call_id, const std::string & dtmf);
	virtual void onIncomingCall(pj::OnIncomingCallParam &prm) override;

	void makeCall(const std::string & strCalled, pj::Call ** call);
private:
	log4cplus::Logger log;
	class CPjSipSDK * m_Plugin = nullptr;
	std::recursive_mutex m_callsmtx;
	std::map<pjsua_call_id, pj::Call *>m_calls;
	friend class CPCall;
	friend class CPjSipSDK;
};

class CPjSipSDK
{
public:
	virtual void onRegStarted(pj::OnRegStartedParam & prm);// 注册或注销登记时通知申请
	virtual void onRegState(pj::OnRegStateParam &prm);//登录状态改变回调
	virtual void onCallState(const pj::CallInfo & ci);//通话状态改变回调
	virtual void onDtmfDigit(pjsua_call_id call_id, const std::string & dtmf);
	virtual void onIncomingCall(pj::Call *call);

private:// 虚函数   
	virtual void onGetCapabilityToken() = 0;			//由APP提供的获取能力token的回调方法
	virtual void onConnected() = 0;					//与云通讯平台连接成功
	virtual void onConnectError(int reason, const char * desc) = 0;		//与云通讯平台连接断开或者出错
	virtual void onIncomingCallReceived(int callType, const char *callid, const char *caller) = 0;  //有呼叫呼入
	virtual void onCallProceeding(const char*callied) = 0;		//呼叫已经被云通讯平台处理
	virtual void onCallAlerting(const char *callid) = 0;			//呼叫振铃
	virtual void onCallAnswered(const char *callid) = 0;			//外呼对方应答
	virtual void onMakeCallFailed(const char *callid, int reason) = 0;//外呼失败
	virtual void onCallPaused(const char* callid) = 0;				//本地Pause呼叫成功
	virtual void onCallPausedByRemote(const char *callid) = 0;		//呼叫被被叫pasue
	virtual void onCallReleased(const char *callid, int reason) = 0;				//呼叫挂机
	virtual void onCallTransfered(const char *callid, const char *destionation) = 0; //呼叫被转接
	virtual void onDtmfReceived(const char *callid, char dtmf) = 0;		//收到DTMF按键时的回调
	virtual void onTextMessageReceived(const char *sender, const char *receiver, const char *sendtime, const char *msgid, const char *message, const char *userdata) = 0;		//收到文本短消息
																																												//void (*onGroupTextMessageReceived)(const char* sender, const char* groupid, const char *message) =0; //收到群组文本短消息
	virtual void onMessageSendReport(const char*msgid, const char*time, int status) = 0; //发送消息结果
	virtual void onLogInfo(const char* loginfo) = 0; // 用于接收底层的log信息,调试出现的问题.
	virtual void onResumed(const char* callid) = 0;
	virtual void onNotifyGeneralEvent(const char*callid, int eventType, const char*userdata, int intdata) = 0;	//通用事件通知
	virtual void onCallMediaUpdateRequest(const char*callid, int request) = 0; // 收到对方请求的更新媒体 request：0  请求增加视频（需要响应） 1:请求删除视频（不需要响应）
	virtual void onCallMediaUpdateResponse(const char*callid, int response) = 0;  // 本地请求更新媒体后，更新后的媒体状态 0 有视频 1 无视频
	virtual void onDeliverVideoFrame(const char*callid, unsigned char*buf, int size, int width, int height) = 0; //视频通话过程中，如果请求本地视频，视频数据通过这个函数上报。视频格式是RGB24
	virtual void onRecordVoiceStatus(const char *callid, const char *fileName, int status) = 0; //通话录音结束或者出现错误，上报事件。filenName是上层传下的文件名。 status是录音状态：0： 成功  -1：失败，录音文件删除  -2：写文件失败，保留已经保存的录音。
	virtual void onAudioData(const char *callid, const void *inData, int inLen, void *outData, int &outLen, bool send) = 0; //在音频数据发送之前，将音频数据返回给上层处理，然后将上层处理后的数据返回来。
	virtual void onOriginalAudioData(const char *callid, const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send) = 0; //将原始音频数据抛到上层。
	virtual void onMessageRemoteVideoRotate(const char *degree) = 0;//当远端视频发生旋转时，将旋转的角度上报，degree为向左旋转的度数（0，90，180，270）。
	virtual void onRequestSpecifiedVideoFailed(const char *callid, const char *sip, int reason) = 0;//视频会议时，请求视频数据失败
	virtual void onStopSpecifiedVideoResponse(const char *callid, const char *sip, int response, void *window) = 0;//视频会议时，取消视频数据响应
	virtual void onEnableSrtp(const char *sip, bool isCaller) = 0;//设置srtp加密属性
	virtual void onRemoteVideoRatioChanged(const char *callid, int width, int height, bool isVideoConference, const char *sipNo) = 0;//远端视频媒体分辨率变化时上报
	virtual void onLogOut() = 0;
	virtual void oneXosipThreadStop() = 0;
	virtual std::string getHost() const = 0;

protected:
	log4cplus::Logger log;
private:
	std::string m_server;
	std::string m_domain;
	std::string m_voipid;
	std::string m_voippwd;
	pjsua_call_id m_callid = PJSUA_INVALID_ID;
	bool m_Registerd = false;

	std::string m_ringFile;
	pj::AudioMediaPlayer * m_player = nullptr;
	
	std::string getCodecs(int type);

public:
	CAccount * m_acc = nullptr;
public:
    CPjSipSDK();
	~CPjSipSDK();

	static std::string getVersion();
	static int initialize();
	static int unInitialize();

	void setRingFile(const std::string & ringfile);
	void startRinging();
	void stopRinging();
	int connectToCCP(std::string server, LONG port, std::string domain, std::string utf8voipId, std::string utf8voipPwd);
	std::string makeCall(std::string strCalled);
	int acceptCall(int callid);
	int rejectCall(int callid, int reason);
	int pauseCall(int callid);
	int resumeCall(int callid);
	int transferCall(int callid, std::string number, int type);
	int sendDTMF(int callid, const char dtmf);
	int releaseCall(int callid);
	const int getCurrentCall() const;
	int disConnectToCCP();
	int setCodecEnabled(int codecid, int enabled);
	int getCodecEnabled(int codecid);
	int setMute(bool on);

};

