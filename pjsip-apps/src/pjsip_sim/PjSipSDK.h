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

	void makeCall(const pj::SipHeaderVector headers, const std::string & strCalled, pj::Call ** call);
private:
	log4cplus::Logger log;
	class CPjSipSDK * m_Plugin = nullptr;
	std::recursive_mutex m_callsmtx;
	std::map<pjsua_call_id, pj::Call *>m_calls;
	friend class CPCall;
	friend class CPjSipSDK;
};

class CBuddy : public pj::Buddy
{
public:
    CBuddy();
    ~CBuddy() {}

    virtual void onBuddyState();
private:
    log4cplus::Logger log;
};

class CPjSipSDK
{
public:
	virtual void onRegStarted(pj::OnRegStartedParam & prm);// 注册或注销登记时通知申请
	virtual void onRegState(pj::OnRegStateParam &prm);//登录状态改变回调
	virtual void onCallState(const pj::CallInfo & ci);//通话状态改变回调
	virtual void onDtmfDigit(pjsua_call_id call_id, const std::string & dtmf);
	virtual void onIncomingCall(pj::Call *call, const std::string& wholeMsg);

    bool IsRegisterd(){ return m_Registerd;}

private:// 虚函数   
	virtual void onRegistered(pj::OnRegStateParam& prm);					//与云通讯平台连接成功
	virtual void onRegisterError(int reason, const char * desc);		//与云通讯平台连接断开或者出错
	virtual void onIncomingCallReceived(int callType, const char *callid, const char *caller, const char * called);  //有呼叫呼入
	virtual void onCallProceeding(const char*callied);		//呼叫已经被云通讯平台处理
	virtual void onCallAlerting(const char *callid);			//呼叫振铃
	virtual void onCallAnswered(const char *callid);			//外呼对方应答
	virtual void onMakeCallFailed(const char *callid, int reason);//外呼失败
	virtual void onCallPaused(const char* callid);				//本地Pause呼叫成功
	virtual void onCallReleased(const char *callid, int reason);				//呼叫挂机
	virtual void onCallTransfered(const char *callid, const char *destionation); //呼叫被转接
	virtual void onDtmfReceived(const char *callid, char dtmf);		//收到DTMF按键时的回调
																																												//void (*onGroupTextMessageReceived)(const char* sender, const char* groupid, const char *message) =0; //收到群组文本短消息
	virtual void onResumed(const char* callid);
	virtual void onLogOut();
    virtual std::string getHost() const;

protected:
	log4cplus::Logger log;
private:
	std::string m_server;
	std::string m_domain;
	std::string m_voipid;
	std::string m_voippwd;
    pj::TransportId transport_tcp = PJSUA_INVALID_ID;
	pjsua_call_id m_callid = PJSUA_INVALID_ID;
	bool m_Registerd = false;

	std::string m_ringFile;
	pj::AudioMediaPlayer * m_player = nullptr;
	
	std::string getCodecs(int type);

public:
	CAccount * m_acc = nullptr;
    CBuddy  m_buddy;

public:
    CPjSipSDK();
	~CPjSipSDK();

	static std::string getVersion();
	static int initialize();
	static int unInitialize();

	void setRingFile(const std::string & ringfile);
	void startRinging(bool hasMedia);
	void stopRinging();
	int Login(std::string server, long port, std::string domain, std::string utf8voipId, std::string utf8voipPwd, int ttl);
	std::string makeCall(const pj::SipHeaderVector headers, std::string strCalled);
    int sendIM(const pj::SipHeaderVector headers, const std::string& utf8Text);
	int acceptCall(int callid);
	int rejectCall(int callid, int reason);
	int pauseCall(int callid);
	int resumeCall(int callid);
	int transferCall(int callid, std::string number, int type);
	int sendDTMF(int callid, const char dtmf);
	int releaseCall(int callid);
	const int getCurrentCall() const;
	int Logout();
	int setCodecEnabled(int codecid, int enabled);
	int getCodecEnabled(int codecid);
	int setMute(bool on);
    void setCaptureDev(int dev);
    void setPlaybackDev(int dev);

};

