#include "PjSipSDK.h"
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/loggingmacros.h>
#include <atomic>
#include <map>
#include <mutex>
#include "stringHelper.h"
#ifndef WIN32
#include <sstream>
namespace std {
    std::string to_string(int i) {
        std::ostringstream oss;
        oss << i;
        return oss.str();
    }
}
#endif

static std::atomic_ulong g_pjsipReferce(0);
class pj::Endpoint * ep;

class MyAudioMediaPort :public pj::AudioMediaPort {
public:
    MyAudioMediaPort() {
        this->log = log4cplus::Logger::getInstance("pjsip");
    }
    virtual void onFrameRequested(pj::MediaFrame& frame)
    {
        //PJ_UNUSED_ARG(frame);
        LOG4CPLUS_DEBUG(log, "onFrameRequested " << frame.size);
    }

    virtual void onFrameReceived(pj::MediaFrame& frame)
    {
        //PJ_UNUSED_ARG(frame);
        LOG4CPLUS_DEBUG(log, "onFrameReceived " << frame.size);
    }
private:
    log4cplus::Logger log;
};

class CPEndpoint : public pj::Endpoint {
public:
	CPEndpoint(){
		this->log = log4cplus::Logger::getInstance("pjsip");
	}

	virtual void onNatDetectionComplete(const pj::OnNatDetectionCompleteParam &prm) override
	{
		LOG4CPLUS_DEBUG(log, "nat detect " << prm.natTypeName << ", status:" << prm.reason);
	}
    virtual void onTimer(const pj::OnTimerParam& prm)
    {
        LOG4CPLUS_DEBUG(log, "onTimer " << prm.msecDelay << prm.userData);
    }
private:
	log4cplus::Logger log;

};


class CPCall : public pj::Call
{
public:
	CPCall(CAccount * acc, int call_id = PJSUA_INVALID_ID)
		: Call(*acc, call_id), m_acc(acc)
	{
		this->log = log4cplus::Logger::getInstance("CPCall");
		LOG4CPLUS_DEBUG(log, "New Call");
		std::string micro;
        std::string speaker;

		try {
			const pj::AudioDevInfoVector2 & auddev = pj::Endpoint::instance().audDevManager().enumDev2();
			for (int i = 0; i < auddev.size(); i++) {

				if (auddev[i].inputCount && micro == auddev[i].name) {
					pj::Endpoint::instance().audDevManager().setCaptureDev(i);
				}

				if (auddev[i].outputCount && speaker == auddev[i].name)
				{
					pj::Endpoint::instance().audDevManager().setPlaybackDev(i);
				}
			}
            //pj::MediaFormatAudio fmt;
            //fmt.init(PJMEDIA_FORMAT_PCMU, 8000, 1, 20000, 8, 64000, 64000);
            //m_audioMediaPort.createPort("sim_dev", fmt);
			//pj::Endpoint::instance().audDevManager().getCaptureDevMedia().adjustTxLevel();
			//pj::Endpoint::instance().audDevManager().getPlaybackDevMedia().adjustRxLevel();
		}
		catch (pj::Error& err)
		{
			LOG4CPLUS_ERROR(log, err.info());
		}

	}

	virtual ~CPCall()
	{ 
		LOG4CPLUS_DEBUG(log, "Release Call");
	}

	// Notification when call's state has changed.
	virtual void onCallState(pj::OnCallStateParam &prm) override
	{
		pj::CallInfo ci = getInfo();
		
		m_acc->onCallState(ci);
		if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
			/* Delete the call */
			std::recursive_mutex & mtx = m_acc->m_callsmtx;
			mtx.lock();
			m_acc->m_calls.erase(this->getId());
			//delete this;
			mtx.unlock();
		}
	}

	// Notification when call's media state has changed.
	virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override
	{
		LOG4CPLUS_DEBUG(log, "hasMedia:" << this->hasMedia());
		for (auto & media : this->getInfo().media){
			if (media.type == PJMEDIA_TYPE_AUDIO) {

				pj::AudioMedia *aud_med = (pj::AudioMedia *) this->getMedia(media.index);
				//aud_med->startTransmit(m_audioMediaPort);
    //            m_audioMediaPort.startTransmit(*aud_med);

                pj::AudioMedia& speaker_med = pj::Endpoint::instance().audDevManager().getPlaybackDevMedia();
                aud_med->startTransmit(speaker_med);
                pj::AudioMedia& mic_med = pj::Endpoint::instance().audDevManager().getCaptureDevMedia();
                mic_med.startTransmit(*aud_med);

				//if (m_micro.getPortId() == PJSUA_INVALID_ID) {
					//m_micro.createRecorder(utf8AppDataDir + "\\micro.wav");
				//	mic_med.startTransmit(m_micro);
				//}

				//if (m_speaker.getPortId() == PJSUA_INVALID_ID){
					//m_speaker.createRecorder(utf8AppDataDir + "\\speaker.wav");
				//	aud_med->startTransmit(m_speaker);
				//}
			}
		}
	}

	virtual void onDtmfDigit(pj::OnDtmfDigitParam &prm) override
	{
		m_acc->onDtmfDigit(getInfo().id, prm.digit);
	}

	virtual void onCallTransferStatus(pj::OnCallTransferStatusParam &prm) override
	{
	}

private:
	CAccount * m_acc = nullptr;
	pj::AudioMediaRecorder m_micro;
	pj::AudioMediaRecorder m_speaker;
    MyAudioMediaPort m_audioMediaPort;
	log4cplus::Logger log;
};


void CPjSipSDK::onRegStarted(pj::OnRegStartedParam & prm)
{
	if (prm.renew){
		//this->onRegistered(prm);
	}
	else{ 
		this->onLogOut();
	}
}

void CPjSipSDK::onRegState(pj::OnRegStateParam &prm)
{
	if (prm.code == 200) {
		if (!this->m_Registerd) {
			this->onRegistered(prm);
			this->m_Registerd = true;
		}
	}
	else {
		this->m_Registerd = false;
		this->onRegisterError(prm.code, prm.reason.c_str());
	}
}

void CPjSipSDK::onCallState(const pj::CallInfo & ci)
{
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << "acc_id:" << ci.accId << ", Call " << ci.id << " state=" << ci.stateText);
	this->m_callid = ci.id;

	switch (ci.state) {
	case PJSIP_INV_STATE_CALLING:
		this->onCallProceeding(std::to_string(ci.id).c_str());
		break;
	case PJSIP_INV_STATE_INCOMING: {
		//startRinging();
		std::string remote = ci.remoteUri;
		std::string caller = remote.substr(remote.find(":"), remote.find("@") - remote.find(":"));
        std::string local = ci.localUri;
        std::string called = local.substr(local.find(":") + 1, local.find("@") - local.find(":") - 1);
		//this->onIncomingCallReceived(0, std::to_string(ci.id).c_str(), caller.c_str(), called.c_str());
		}
		break;
	case PJSIP_INV_STATE_EARLY:
		if(ci.role == PJSIP_UAC_ROLE) 
			this->onCallAlerting(std::to_string(ci.id).c_str());
		break;
	case PJSIP_INV_STATE_CONNECTING:
		//g_sipAccount[ci.acc_id]->onCallAlerting(std::to_string(call_id).c_str());
		break;
	case PJSIP_INV_STATE_CONFIRMED:
		//stopRinging();
		this->onCallAnswered(std::to_string(ci.id).c_str());
		break;
	case PJSIP_INV_STATE_DISCONNECTED:
		//stopRinging();
		this->onCallReleased(std::to_string(ci.id).c_str(), ci.lastStatusCode);
		break;
	}
	
}

void CPjSipSDK::onDtmfDigit(pjsua_call_id call_id, const std::string & digit)
{
	this->onDtmfReceived(std::to_string(call_id).c_str(), digit.back());
}


void CPjSipSDK::onIncomingCall(pj::Call * call, const std::string& wholeMsg)
{
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << "Incoming call from " << call->getId() << "  " << wholeMsg);

	this->m_callid = call->getId();
	
	std::string remote = call->getInfo().remoteUri;
	std::string caller = remote.substr(remote.find(":")+1, remote.find("@")- (remote.find(":")+1));
    /*std::string local = call->getInfo().localUri;
    std::string called = local.substr(local.find(":") + 1, local.find("@") - (local.find(":") - 1));*/
    std::string hName = "X-Real-Called-Number:";
    std::string called = wholeMsg.substr(wholeMsg.find(hName) + hName.size() + 1);
    called = called.substr(0, called.find("\r\n"));
    helper::string::trim(called);
	//startRinging();
	this->onIncomingCallReceived(0, std::to_string(call->getInfo().id).c_str(), caller.c_str(), called.c_str());
}

void CPjSipSDK::onRegistered(pj::OnRegStateParam& prm)
{
    LOG4CPLUS_INFO(log, prm.rdata.srcAddress << " " << "onRegistered ");
}

void CPjSipSDK::onRegisterError(int reason, const char* desc)
{
    LOG4CPLUS_INFO(log, reason << " " << desc << " " << "onRegisterError ");
}

void CPjSipSDK::onIncomingCallReceived(int callType, const char* callid, const char* caller, const char * called)
{
    LOG4CPLUS_INFO(log, callType << " " << callid << " " << caller << ">>" << called);
}

void CPjSipSDK::onCallProceeding(const char* callied)
{
    LOG4CPLUS_INFO(log, "onCallProceeding " << callied);
}

void CPjSipSDK::onCallAlerting(const char* callid)
{
    LOG4CPLUS_INFO(log, "onCallAlerting " << callid);
}

void CPjSipSDK::onCallAnswered(const char* callid)
{
    LOG4CPLUS_INFO(log, "onCallAnswered " << callid);
}

void CPjSipSDK::onMakeCallFailed(const char* callid, int reason)
{
    LOG4CPLUS_INFO(log, "onMakeCallFailed " << callid << " " << reason);
}

void CPjSipSDK::onCallPaused(const char* callid)
{
    LOG4CPLUS_INFO(log, "onCallPaused " << callid);
}


void CPjSipSDK::onCallReleased(const char* callid, int reason)
{
    LOG4CPLUS_INFO(log, "onCallReleased " << callid);
}

void CPjSipSDK::onCallTransfered(const char* callid, const char* destionation)
{
    LOG4CPLUS_INFO(log, "onCallTransfered " << callid << " " << destionation);
}

void CPjSipSDK::onDtmfReceived(const char* callid, char dtmf)
{
    LOG4CPLUS_INFO(log, "onDtmfReceived " << callid << " " << dtmf);
}

void CPjSipSDK::onResumed(const char* callid)
{
    LOG4CPLUS_INFO(log, "onResumed " << callid);
}

void CPjSipSDK::onLogOut()
{
    LOG4CPLUS_INFO(log, "onLogOut ");
}

std::string CPjSipSDK::getHost() const
{
    return std::string();
}

class CPLogWriter :public pj::LogWriter {
private:
	log4cplus::Logger log;
public:
	CPLogWriter(){
		log = log4cplus::Logger::getInstance("pjsip.log");
	}

	virtual void write(const pj::LogEntry &entry)override
	{ 
		std::string msg = entry.msg; 
		if (entry.level == 5) {
			LOG4CPLUS_TRACE(log, msg);
		}
		else if (entry.level == 4) {
			LOG4CPLUS_DEBUG(log, msg);
		}
		else if (entry.level == 3) {
			LOG4CPLUS_INFO(log, msg);
		}
		else if (entry.level == 2) {
			LOG4CPLUS_WARN(log, msg);
		}
		else if (entry.level == 1) {
			LOG4CPLUS_ERROR(log, msg);
		}
		else
			LOG4CPLUS_TRACE(log, msg);
	}
};


std::string CPjSipSDK::getVersion()
{
    return PJSIP_VERSION;
}

CPjSipSDK::CPjSipSDK()
{
	log = log4cplus::Logger::getInstance("CPjSipSDK");
	LOG4CPLUS_INFO(log, "SDK Version:" << getVersion());

	initialize();
	
	//if (!pj::Endpoint::instance().libIsThreadRegistered())
	//	pj::Endpoint::instance().libRegisterThread("CPjSipSDK");

	LOG4CPLUS_TRACE(log, "construction");
}

CPjSipSDK::~CPjSipSDK()
{
	//pjsua_acc_set_user_data(m_acc->getId(), NULL);
    if(m_acc){
    m_acc->shutdown();
	delete m_acc;
    }
	if (m_player)
		delete m_player;
	unInitialize();
	LOG4CPLUS_TRACE(log, "destruction");
}

int CPjSipSDK::initialize()
{
	if (g_pjsipReferce.fetch_add(1) == 0) {

		log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
		ep = new CPEndpoint();

		// pj create
		pj::EpConfig ep_cfg;
		ep_cfg.logConfig.level = 5;
		ep_cfg.logConfig.writer = new CPLogWriter();

		ep_cfg.medConfig.noVad = true;
		//ep_cfg.medConfig.sndClockRate = 16000;

		try {
			ep->libCreate();
			ep->libInit(ep_cfg);

			// Start the library (worker threads etc)
			ep->libStart();

			for (auto it : ep->codecEnum2()) {
				LOG4CPLUS_DEBUG(log, it.codecId << " priority:" << (uint32_t)it.priority << ",desc:" << it.desc);
			}

			//ep->natDetectType();

			for (auto audiodev : ep->audDevManager().enumDev2()) {
				LOG4CPLUS_DEBUG(log, "audiodevice:" << (audiodev.name));
			}
			pj::Endpoint::instance().audDevManager().setEcOptions(ep->audDevManager().getEcTail(), PJMEDIA_ECHO_WEBRTC | PJMEDIA_ECHO_USE_NOISE_SUPPRESSOR);
			LOG4CPLUS_DEBUG(log, "EcTail:" << ep->audDevManager().getEcTail());
		}
		catch (pj::Error &err){
			LOG4CPLUS_ERROR(log, err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
			return err.status;
		}

	}

	return 0;
}

void CPjSipSDK::setRingFile(const std::string & ringfile)
{
	this->m_ringFile = ringfile;
	LOG4CPLUS_DEBUG(log, "setRingFile:" << this->m_ringFile);
}

void CPjSipSDK::startRinging(bool hasMedia)
{
#ifdef WIN32
	pj::AudioMedia& play_med = ep->audDevManager().getPlaybackDevMedia();
	try{
		if (this->m_player == nullptr) {
			this->m_player = new pj::AudioMediaPlayer();

			this->m_player->createPlayer(m_ringFile);
			this->m_player->startTransmit(play_med);
		}
	}
	catch (pj::Error& err)
	{
		LOG4CPLUS_ERROR(log, this->getHost() << " " << "Error play ringfile :" << err.info());
	}
#endif

    auto it_call = this->m_acc->m_calls.find(this->getCurrentCall());
    pj::Call * call = nullptr;
    if(it_call != this->m_acc->m_calls.end()){
        call = it_call->second;
    }
    if (call) {
        if (hasMedia) {
            pj::CallOpParam cprm(true);
            cprm.statusCode = PJSIP_SC_PROGRESS;
            call->answer(cprm);

            LOG4CPLUS_DEBUG(log, "hasMedia:" << call->hasMedia());
            for (auto& media : call->getInfo().media) {
                if (media.type == PJMEDIA_TYPE_AUDIO) {

                    pj::AudioMedia* aud_med = (pj::AudioMedia*)call->getMedia(media.index);
                    pj::AudioMedia& mic_med = pj::Endpoint::instance().audDevManager().getCaptureDevMedia();
                    mic_med.startTransmit(*aud_med);
                }
            }
        }
        else {
            pj::CallOpParam cprm(true);
            cprm.statusCode = PJSIP_SC_RINGING;
            call->answer(cprm);
        }
    }

}

void CPjSipSDK::stopRinging()
{
	pj::AudioMedia& play_med = ep->audDevManager().getPlaybackDevMedia();

	try
	{
		if (m_player && m_player->getPortId() != PJSUA_INVALID_ID) {
			m_player->stopTransmit(play_med);
			delete m_player;
			m_player = nullptr;
		}
	}
	catch (pj::Error& err)
	{
		LOG4CPLUS_ERROR(log, this->getHost() << " " << "Error stop play ringfile :" << err.info());
	}
}

int CPjSipSDK::Login(std::string server, long port, std::string domain, std::string utf8voipId, std::string utf8voipPwd, int ttl)
{
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " server:" << server << ":"  << port << ", domin:" << domain << ",voipId:" << utf8voipId << ", pwd:" << utf8voipPwd);

	this->m_server = server;
	if (port>0){
		this->m_server += ":" + std::to_string(port);
	}
	if (!domain.empty())
		this->m_domain = domain;
	else
		this->m_domain = m_server;

	this->m_voipid = utf8voipId;
	this->m_voippwd = utf8voipPwd;

	//添加账号...

	pj::AccountConfig acc_cfg;
	if (m_domain != m_server) {
		acc_cfg.sipConfig.proxies.push_back("sip:" + m_server + "");
	}


	acc_cfg.idUri = "<sip:" + this->m_voipid + "@" + this->m_domain + ">";
	acc_cfg.regConfig.registrarUri = "sip:" + this->m_domain + "";
    acc_cfg.regConfig.timeoutSec = ttl;
	acc_cfg.sipConfig.authCreds.push_back(pj::AuthCredInfo("Digest", "*", this->m_voipid, 0, this->m_voippwd));
	//acc_cfg.sipConfig.authCreds.push_back(pj::AuthCredInfo("Digest", "realm=\"realm\",domain=\"sip:domain\",nonce=\"nonce\",opaque=\"opaque\",stale=true,algorithm=MD5,qop=\"auth\"", this->m_voipid, 0, this->m_voippwd));

    pj::TransportConfig tcfg;
    //tcfg.port = 5060;

    try{
        if(this->transport_tcp == PJSUA_INVALID_ID){
            this->transport_tcp = ep->transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
        }
        acc_cfg.sipConfig.transportId = transport_tcp;
    }
    catch (pj::Error& err) {
        LOG4CPLUS_ERROR(log, "Account creation error: " << err.info());
    }

	//"Digest  realm=\"realm\",domain=\"sip:domain\",nonce=\"nonce\",opaque=\"opaque\",stale=true,algorithm=MD5,qop=\"auth\"",

    if (!m_acc) {
        // Create the account
        m_acc = new CAccount(this);
        try {
            m_acc->create(acc_cfg);
        }
        catch (pj::Error& err) {
            LOG4CPLUS_ERROR(log, "Account creation error: " << err.info());
        }
    }
    else {
        // Modify the account
        try {
            //Update the registration
            m_acc->modify(acc_cfg);
            m_acc->setRegistration(true);
        }
        catch (pj::Error& err) {
            LOG4CPLUS_ERROR(log, "Account creation error: " << err.info());
        }
    }

	this->log = log4cplus::Logger::getInstance("CPjSipSDK." + std::to_string(this->m_acc->getId()));

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:");
	return 0;
}

std::string CPjSipSDK::makeCall(const pj::SipHeaderVector headers, std::string strCalled) {

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " called:" << strCalled);
	
	if (ep->utilVerifySipUri(strCalled) != PJ_SUCCESS){
		strCalled = "sip:" + strCalled + "@" + this->m_domain;
	}

	pj::Call * call = nullptr;
	try {
		m_acc->makeCall(headers, strCalled, &call);
	}
	catch (pj::Error &err) {
		this->onMakeCallFailed("", err.status);
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		return "";
	}
	
	m_callid = call->getId();

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << m_callid);
	return std::to_string(m_callid);
}

int CPjSipSDK::sendIM(const std::string& content)
{

    LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " content:" << content);


    pj::Call* call = nullptr;
    try {
        m_acc->sendIM(content, &call);
    }
    catch (pj::Error& err) {
        LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
        return 0;
    }

    LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << m_callid);
    return m_callid;
}

int CPjSipSDK::acceptCall(int callid) {
	pj::Call * call = pj::Call::lookup(callid);
	pj_status_t status = PJ_SUCCESS;
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid);

	try {
		if (call && (call->getInfo().state == PJSIP_INV_STATE_INCOMING || call->getInfo().state == PJSIP_INV_STATE_EARLY))
		{
			pj::CallOpParam cprm;
			cprm.statusCode = PJSIP_SC_OK;
			cprm.opt.audioCount = 1;
			call->answer(cprm);
		}
		else
			status = PJ_EINVAL;
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

int CPjSipSDK::rejectCall(int callid, int reason) {
	pj::Call * call = pj::Call::lookup(callid);
	pj_status_t status = PJ_SUCCESS;
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid << ", reason:" << reason);
	try {
		if (call) {
			call->hangup(true);
		}
		else
			status = PJ_EINVAL;
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

int CPjSipSDK::pauseCall(int callid)
{
	pj::Call * call = pj::Call::lookup(callid);
	pj_status_t status = PJ_SUCCESS;

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid);
	try {
		if (call) {
			call->setHold(true);
		}
		else
			status = PJ_EINVAL;
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	if (status == PJ_SUCCESS){
		this->onCallPaused(std::to_string(callid).c_str());
	}
	return status;
}

int CPjSipSDK::resumeCall(int callid)
{
	pj::Call * call = pj::Call::lookup(callid);
	pj_status_t status = PJ_SUCCESS;

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid);
	try {
		if (call) {
			pj::CallOpParam rpm(true);
			rpm.opt.flag = PJSUA_CALL_UNHOLD;
			call->reinvite(rpm);
		}
		else
			status = PJ_EINVAL;
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	if (status == PJ_SUCCESS) {
		this->onResumed(std::to_string(callid).c_str());
	}
	return status;
}

int CPjSipSDK::transferCall(int callid, std::string number, int type)
{
	pj::Call * call = pj::Call::lookup(callid);
	pj_status_t status = PJ_SUCCESS;

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid << ",dest:" << number);
	if (ep->utilVerifySipUri(number) != PJ_SUCCESS) {
		number = "sip:" + number + "@" + this->m_domain;
	}

	try {
		if (call) {
			call->xfer(number, true);
		}
		else
			status = PJ_EINVAL;
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);

	if (status == PJ_SUCCESS)
		this->onCallTransfered(std::to_string(callid).c_str(), number.c_str());

	return status;
}

int CPjSipSDK::sendDTMF(int callid, const char dtmf)
{
	pj::Call * call = pj::Call::lookup(callid);
	pj_status_t status = PJ_SUCCESS;

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid << ",dtmf:" << dtmf);
	try {
		if (call) {
			std::string digit;
			digit.push_back(dtmf);
			call->dialDtmf(digit);
		}
		else
			status = PJ_EINVAL;
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

int CPjSipSDK::releaseCall(int callid) {

	pj_status_t status = PJ_SUCCESS;
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " callid:" << callid);

	if (callid < 0 && this->m_acc){
		this->m_acc->m_callsmtx.lock();
		for (auto & call : this->m_acc->m_calls)
		{
			delete call.second;
		}
		this->m_acc->m_calls.clear();
		this->m_acc->m_callsmtx.unlock();
	}
	else {
		pj::Call * call = pj::Call::lookup(callid);

		try {
			if (call) {
				call->hangup(true);
			}
			else
				status = PJ_EINVAL;
		}
		catch (pj::Error &err) {
			LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
			status = err.status;
		}
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

const int CPjSipSDK::getCurrentCall() const
{
	return m_callid;
}

int CPjSipSDK::Logout()
{
	pj_status_t status = PJ_SUCCESS;
	try
	{
		if (this->m_acc->isValid()) {
			this->m_acc->setRegistration(false);
		}
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

int CPjSipSDK::unInitialize()
{
	if (g_pjsipReferce.fetch_sub(1) == 1) {
		ep->libDestroy();
		//LOG4CPLUS_DEBUG(log, __FUNCTION__ );
		delete ep;
		ep = nullptr;
	}
	return 0;
}

int CPjSipSDK::setCodecEnabled(int codecid, int enabled)
{
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " codec:" << codecid << ",enabled :" << enabled);

	std::string strcodec_id = getCodecs(codecid);

	pj_status_t status = PJ_SUCCESS;
	try {
		ep->codecSetPriority(strcodec_id, enabled ? PJMEDIA_CODEC_PRIO_NORMAL : PJMEDIA_CODEC_PRIO_DISABLED);
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}
	
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

int CPjSipSDK::getCodecEnabled(int codecid)
{
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " codec:" << codecid);

	std::string strcodec_id = getCodecs(codecid);

	pj_status_t status = PJ_SUCCESS;
	try {
		
		for (auto it : ep->codecEnum2()) {
			if (it.codecId == strcodec_id) {
				status = it.priority != PJMEDIA_CODEC_PRIO_DISABLED;
				LOG4CPLUS_DEBUG(log, this->getHost() << " " << it.codecId << " priority:" << (uint8_t)it.priority << ",desc:" << it.desc);
			}
		}
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = 0;
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return status;
}

int CPjSipSDK::setMute(bool on)
{
	pj_status_t status = PJ_SUCCESS;
	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " on:" << on);
	on = !on;
	try {
		ep->audDevManager().getCaptureDevMedia().adjustTxLevel((float)on / 100);
	}
	catch (pj::Error &err) {
		LOG4CPLUS_ERROR(log, this->getHost() << " " << err.info() << ":" << err.reason << ";" << err.srcFile << ":" << err.srcLine);
		status = err.status;
	}

	LOG4CPLUS_DEBUG(log, this->getHost() << " " << __FUNCTION__ << " result:" << status);
	return 0;
}

std::string CPjSipSDK::getCodecs(int type)
{
	std::string ret = "";
	switch (type)
	{
	case 0:	ret = "opus/16000/1";	break;	//Opus 16 kHz
	case 1:	ret = "opus/24000/1";	break;	//Opus 24 kHz
	case 2:	ret = "opus/48000/1";	break;	//Opus 48 kHz
	case 3:	ret = "PCMA/8000/1";	break;	//G.711 A-law
	case 4:	ret = "PCMU/8000/1";	break;	//G.711 u-law
	case 5:	ret = "G722/16000/1";	break;	//G.722 16 kHz
	case 6:	ret = "G729/8000/1";	break;	//G.729 8 kHz
	case 7:	ret = "GSM/8000/1";		break;	//GSM 8 kHz
	case 8:	ret = "AMR/8000/1";		break;	//AMR 8 kHz
	case 9:	ret = "iLBC/8000/1";	break;	//iLBC 8 kHz
	case 10:	ret = "speex/32000/1";	break;	//Speex 32 kHz
	case 11:	ret = "speex/16000/1";	break;	//Speex 16 kHz
	case 12:	ret = "speex/8000/1";	break;	//Speex 8 kHz
	case 13:	ret = "SILK/24000/1";	break;	//SILK 24 kHz
	case 14:	ret = "SILK/16000/1";	break;	//SILK 16 kHz
	case 15:	ret = "SILK/12000/1";	break;	//SILK 12 kHz
	case 16:	ret = "SILK/8000/1";	break;	//SILK 8 kHz
	case 17:	ret = "L16/44100/1";	break;	//LPCM 44 kHz
	case 18:	ret = "L16/44100/2";	break;	//LPCM 44 kHz Stereo
	case 19:	ret = "L16/16000/1";	break;	//LPCM 16 kHz
	case 20:	ret = "L16/16000/2";	break;	//LPCM 16 kHz Stereo
	case 21:	ret = "L16/8000/1";		break;	//LPCM 8 kHz
	case 22:	ret = "L16/8000/2";		break;	//LPCM 8 kHz Stereo
	default:
		break;
	}
	return ret;
}

CAccount::CAccount(CPjSipSDK * plugin):m_Plugin(plugin)
{
	this->log = log4cplus::Logger::getInstance("CAccount");
}

CAccount::~CAccount()
{
	this->m_callsmtx.lock();
	for (auto & call : m_calls)
	{
		delete call.second;
	}
	m_calls.clear();
	this->m_callsmtx.unlock();
}

void CAccount::onRegStarted(pj::OnRegStartedParam & prm)
{
	m_Plugin->onRegStarted(prm);
}

void CAccount::onRegState(pj::OnRegStateParam & prm)
{
	m_Plugin->onRegState(prm);
}

void CAccount::onCallState(const pj::CallInfo & ci)
{
	m_Plugin->onCallState(ci);
}

void CAccount::onDtmfDigit(pjsua_call_id call_id, const std::string & dtmf)
{
	m_Plugin->onDtmfDigit(call_id, dtmf);
}

void CAccount::onIncomingCall(pj::OnIncomingCallParam & prm)
{
	pj::Call *call = new CPCall(this, prm.callId);

	std::unique_lock<std::recursive_mutex >lck(this->m_callsmtx);
	for (auto & precall : this->m_calls) {
		precall.second->hangup(true);
	}
	this->m_calls[prm.callId] = call;
    lck.unlock();
	//pj::CallOpParam cprm(true);
	//cprm.statusCode = PJSIP_SC_RINGING;
	//call->answer(cprm);
	m_Plugin->onIncomingCall(call, prm.rdata.wholeMsg);
}

void CAccount::makeCall(const pj::SipHeaderVector headers, const std::string & strCalled, pj::Call ** pcall)
{
	pj::Call * call = new CPCall(this);
	*pcall = call;
	pj::CallOpParam prm(true);
	prm.opt.audioCount = 1;
    prm.txOption.headers = headers;
	call->makeCall(strCalled, prm);
    std::unique_lock<std::recursive_mutex >lck(this->m_callsmtx);
	this->m_calls[call->getId()] = call;
	return;
}

void CAccount::sendIM(const std::string& content, pj::Call** pcall)
{
    pj::Call* call = new CPCall(this);
    pj::SendInstantMessageParam imParam;
    imParam.content = content;
    call->sendInstantMessage(imParam);
    return;
}
