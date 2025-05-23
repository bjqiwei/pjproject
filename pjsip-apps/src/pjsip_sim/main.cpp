#include "PjSipSDK.h"
#include <log4cplus/configurator.h>
#include <log4cplus/log4cplus.h>
#include "tinyxml2.h"
#include <signal.h>
#include <thread>
#include "cpptimer.h"
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h> /* close */
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <audio_api/utilities.h>
#include <getopt.h>
#endif
#include <malloc.h>
#include "httpclient.h"
#include "stringHelper.h"
#include <jsoncpp-1.9.5/include/json/json.h>

#define VERSION "1.1.0.0"

#ifndef WIN32
#define WRITE(fd, buf, len) write(fd, buf, len)
#else
#define WRITE(fd, buf, len) ::send(fd, buf, len, 0)
#endif // !WIN32

std::string sip_server;
int sip_port = 5060;
std::string sip_domain;
std::string sip_userId;
std::string sip_password;
int sip_ttl = 300;
std::string mac_id;
static bool running;
static int serialfd = 0;

class MyPJSIP : public CPjSipSDK {
public:
    MyPJSIP() { log = log4cplus::Logger::getInstance("pjsip"); };
    ~MyPJSIP() {};
    void onRegisterError(int reason, const char* desc) override {
        LOG4CPLUS_ERROR(log, reason << " " << desc << " " << "onRegisterError ");
    }
    void onRegistered(pj::OnRegStateParam& prm) override
    {
        //this->makeCall("9000");

        LOG4CPLUS_INFO(log, prm.rdata.srcAddress << " " << "onRegistered ");
    }
    void onIncomingCallReceived(int callType, const char* callid, const char* caller, const char* called)  //�к�к���
    {
        LOG4CPLUS_INFO(log, "onIncomingCallReceived callType:" << callType << " callid:" << callid << " caller:" << caller << " called:" << called);
        if (called == nullptr || strlen(called) == 0) {
            this->rejectCall(this->getCurrentCall(), 505);
        }
        else{
        std::string atcmd = std::string("atd") + called + ";" + "\r\n";
        LOG4CPLUS_INFO(log, "send " << atcmd.size()<< " >>" << atcmd);
        int rc = WRITE(serialfd, atcmd.c_str(), atcmd.size());
    }
    }

    void onCallReleased(const char* callid, int reason)				//��йһ�
    {
        LOG4CPLUS_INFO(log, "onCallReleased " << callid);
        std::string atcmd = std::string("ATH") + "\r\n";
        LOG4CPLUS_INFO(log, "send " << atcmd.size() << " >>" << atcmd);
        int rc = WRITE(serialfd, atcmd.c_str(), atcmd.size());
    }
    void onCallAnswered(const char* callid)			//外呼对方应答
    {
        LOG4CPLUS_INFO(log, "onCallAnswered " << callid);
        std::string atcmd = std::string("ATA") + "\r\n";
        LOG4CPLUS_INFO(log, "send " << atcmd.size() << " >>" << atcmd);
        int rc = WRITE(serialfd, atcmd.c_str(), atcmd.size());
    }

    CppTime::Timer timer;
    log4cplus::Logger log;
};

MyPJSIP * p_sipsdk = nullptr;
#define SERIAL_PORT_NAME        "/tmp/atcmdtest"

namespace Json{
typedef struct JsonParsed_ {
    bool is_success = false;
    Json::Value json_obj;
    std::string err_msg;
}JsonParsed;

inline bool parse(const std::string& stream, Json::Value* root, Json::String* jerr)
{
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    return reader->parse(stream.c_str(), stream.c_str() + stream.length(), root, jerr);
}

inline JsonParsed parse(const std::string& stream) {
    JsonParsed result;
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    result.is_success = reader->parse(stream.c_str(), stream.c_str() + stream.length(), &result.json_obj, &result.err_msg);
    return result;
}
}

void loadconfig()
{
    using namespace tinyxml2;
    tinyxml2::XMLDocument config;
    log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
    if (config.LoadFile("pjsip.xml") != XMLError::XML_SUCCESS)
    {
        LOG4CPLUS_ERROR(log, "load config file error:" << config.ErrorName() << ":" << config.GetErrorStr1());
        return;
    }

    if (tinyxml2::XMLElement* eConfig = config.FirstChildElement("Config")) {

        if (tinyxml2::XMLElement* server = eConfig->FirstChildElement("Server")) {
            if (server && server->GetText()) {
                sip_server = server->GetText();
            }
        }
        if (tinyxml2::XMLElement* port = eConfig->FirstChildElement("Port")) {
            if (port && port->GetText()) {
            #ifdef WIN32
                sip_port = std::stoi(port->GetText());
            #else
                sip_port = atoi(port->GetText());
            #endif
            }
        }
        if (tinyxml2::XMLElement* domain = eConfig->FirstChildElement("Domain")) {
            if (domain && domain->GetText()) {
                sip_domain = domain->GetText();
            }
        }

        if (tinyxml2::XMLElement* userId = eConfig->FirstChildElement("UserID")) {
            if (userId && userId->GetText()) {
                sip_userId = userId->GetText();
            }
        }

        if (tinyxml2::XMLElement* password = eConfig->FirstChildElement("Password")) {
            if (password && password->GetText()) {
                sip_password = password->GetText();
            }
        }

        if (tinyxml2::XMLElement* mac = eConfig->FirstChildElement("Mac")) {
            if (mac && mac->GetText()) {
                mac_id = mac->GetText();
            }
        }
    }

}

void httpconfig()
{
#ifdef WIN32
    loadconfig();
#endif // WIN32
    sip_server.clear();
    sip_userId.clear();
    sip_password.clear();
    sip_domain.clear();
    sip_port = 5060;
    sip_ttl = 300;

    log4cplus::Logger log = log4cplus::Logger::getInstance("Http");
    std::string url = "https://www.dimld.com/reg_auth_server.txt";
    HttpClient client;
    std::vector<std::string> headers;
    std::string response;
    long http_code = 0;
    client.Get(url, response, headers, http_code);
    LOG4CPLUS_INFO(log, url <<  " " << http_code << " response " << response);

    url = helper::string::trim(response);
    response.clear();
    headers= {"Content-Type:application/json; charset=utf-8"};
    std::string data = "{\"cmd\":\"getinfo\",\"mac\" : \"";
    data.append(mac_id).append("\"}");
    client.Post(url, data, response, headers, http_code, nullptr);
    LOG4CPLUS_INFO(log, url << " send >>" << data  << http_code << " response<<" << response);

    if(http_code == 200){
        auto j_resp = Json::parse(response).json_obj;
        if(j_resp["code"].asInt() == 0){
            if(j_resp["data"]["ip"].isString())
                sip_server = j_resp["data"]["ip"].asString();

            if (j_resp["data"]["port"].isInt())
                sip_port = j_resp["data"]["port"].asInt();

            if (j_resp["data"]["sip_id"].isString())
                sip_userId = j_resp["data"]["sip_id"].asString();

            if (j_resp["data"]["sip_pwd"].isString())
                sip_password = j_resp["data"]["sip_pwd"].asString();

            if (j_resp["data"]["ttl"].isInt())
                sip_ttl = j_resp["data"]["ttl"].asInt();

            sip_domain = sip_server;
        }
    }
}

static void sigterm_handler(int signo)
{
    running = false;
}

static void open_socket()
{
    log4cplus::Logger log = log4cplus::Logger::getInstance("open_socket");
#ifndef WIN32
    serialfd = connectUnixSocket(SERIAL_PORT_NAME);

    if (serialfd < 0) {
        LOG4CPLUS_ERROR(log, "ERROR: OPENING DEVICE: " << SERIAL_PORT_NAME);
        return ;
    }
    else {
        LOG4CPLUS_INFO(log, "open socket:" << SERIAL_PORT_NAME);
    }

    fcntl(serialfd, F_SETFL, O_NONBLOCK);

    tcflush(serialfd, TCIFLUSH);
#endif
}
void ReceiveDataFromChan(int serialfd)
{
    log4cplus::Logger log = log4cplus::Logger::getInstance("ReceiveDataFromChan");
    LOG4CPLUS_INFO(log, "ReceiveDataFromChan Start");
#ifndef WIN32
    //pthread_detach(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif
    if (!pj::Endpoint::instance().libIsThreadRegistered()) {
        pj::Endpoint::instance().libRegisterThread("ReceiveDataFromChan");
    }
    const int BUFFSIZE = 512;
    char buffer[BUFFSIZE];

    while (running) {
        memset(buffer,0, BUFFSIZE);
    #ifndef WIN32
        int bytes = read(serialfd, buffer, BUFFSIZE - 1);
    #else
        int bytes = recv(serialfd, buffer, BUFFSIZE - 1, 0);
    #endif

        if (bytes < 1) {
            //LOG4CPLUS_ERROR(log, bytes << " <<" << buffer);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        buffer[bytes] = '\0';
        LOG4CPLUS_INFO(log, "<<" << buffer);
        std::string received = buffer;
        if (received.find("+CLCC: 1,0,2,0,0") != std::string::npos) {
            p_sipsdk->startRinging(false);
        }
        else if (received.find("RINGBACK") != std::string::npos) {
            p_sipsdk->startRinging(true);
        }
        else if (received.find("CALLDISCONNECT") != std::string::npos) {
            p_sipsdk->releaseCall(p_sipsdk->getCurrentCall());
        }
        else if (received.find("CONNECT") != std::string::npos) {
            p_sipsdk->acceptCall(p_sipsdk->getCurrentCall());
            //呼入设备上，必需在应答后打开设备才能采集到声音
            p_sipsdk->setCaptureDev(PJSUA_SND_DEFAULT_CAPTURE_DEV);
            p_sipsdk->setPlaybackDev(PJSUA_SND_DEFAULT_PLAYBACK_DEV);
        }
        else if (received.find("+CLCC: 1,1,4,0,0") != std::string::npos) {
            auto caller = received.substr(received.find("+CLCC: 1,1,4,0,0") + strlen("+CLCC: 1,1,4,0,0")+2);
            caller = caller.substr(0, caller.find("\""));
            pj::SipHeader h;
            h.hName ="X-Real-Caller-Number";
            h.hValue = caller;
            pj::SipHeaderVector headers ={h};
            if(p_sipsdk->IsRegisterd()){
                //呼入设备上，必需在应答后打开设备才能采集到声音，先选择空设备
                p_sipsdk->setCaptureDev(PJSUA_SND_NULL_DEV);
                p_sipsdk->setPlaybackDev(PJSUA_SND_NULL_DEV);
                p_sipsdk->makeCall(headers, sip_userId);
            }
            else {
                std::string atcmd = std::string("ATH") + "\r\n";
                LOG4CPLUS_INFO(log, "send " << atcmd.size() << " >>" << atcmd);
                int rc = WRITE(serialfd, atcmd.c_str(), atcmd.size());
            }
        }
        else if (received.find("CSIM:20,\"") != std::string::npos) {
            auto deviceId = received.substr(received.find("CSIM:20,\"") + strlen("CSIM:20,\""));
            deviceId = deviceId.substr(0, 16);//16位
            mac_id = deviceId;
            LOG4CPLUS_INFO(log, "deviceId " << mac_id);
        }
    }
    LOG4CPLUS_INFO(log, "ReceiveDataFromChan end");
}


static void usage()
{
    printf(
        "\n"
        "Usage:\n"
        "   -d [--daemon]            : Run as a daemon.\n"
        "\n"
        "   -w [--without-cmdline]   : Run without command-line.\n"
        "\n"
        "   -v [--version]           : Show the version.\n"
        "\n"
        "   -h [--help]              : Show the help.\n"
        "\n");
}
static bool cmdline_process(char* cmdline)
{
    bool result = true;
    char* name;
    char* last;
    name = strtok(cmdline, " ");


    if (strcasecmp(name, "exit") == 0 || strcmp(name, "quit") == 0 || strcmp(name, "...") == 0) {
        result = false;
    }

    else if (strcasecmp(name, "help") == 0) {
        printf("usage:\n");
        printf("- ... quit\n");
        printf("- quit, exit\n");
    }
    return result;
}

std::thread* receiveThread = nullptr;
int start()
{
    log4cplus::initialize();
    log4cplus::ConfigureAndWatchThread logconfig("log4cplus.properties", 10 * 1000);
    log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
    pj_init();
    pj_log_set_level(1);
    running = true;
    p_sipsdk = new MyPJSIP();
    open_socket();
    receiveThread = new std::thread(ReceiveDataFromChan, serialfd);
    pj_log_set_decor(PJ_LOG_HAS_SENDER | PJ_LOG_HAS_INDENT);
    p_sipsdk->timer.add(std::chrono::seconds(10), [=](CppTime::timer_id tid) {
        if (!pj::Endpoint::instance().libIsThreadRegistered()) {
            pj::Endpoint::instance().libRegisterThread("timer");
        }
        if (!p_sipsdk->IsRegisterd()) {
            if(!mac_id.empty()){
                httpconfig();
            }

            if(!sip_userId.empty()){
                p_sipsdk->Login(sip_server, sip_port, sip_domain, sip_userId, sip_password, sip_ttl);
            }
        }
        }, std::chrono::seconds(30)
    );
    char cmdline[1024];
    strcpy(cmdline, "AT+CEREG?\r\n");//注册状态
    LOG4CPLUS_INFO(log, "send " << strlen(cmdline) << " >>" << cmdline);
    WRITE(serialfd, cmdline, strlen(cmdline));

    strcpy(cmdline, "AT+CPIN?\r\n");
    LOG4CPLUS_INFO(log, "send " << strlen(cmdline) << " >>" << cmdline);
    WRITE(serialfd, cmdline, strlen(cmdline));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::string atcmd = "AT+CSIM=10,\"0001028208\"\r\n";//获取设备ID
    LOG4CPLUS_INFO(log, "send " << atcmd.size() << " >>" << atcmd);
    WRITE(serialfd, atcmd.c_str(), atcmd.size());
    return 0;
}

void stop()
{
    delete p_sipsdk;
    receiveThread->join();
    log4cplus::deinitialize();
}

int main(int argc, char* argv[])
{

    signal(SIGINT, sigterm_handler);
#ifdef SIGTSTP
    signal(SIGTSTP, sigterm_handler);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, sigterm_handler);
#endif
#ifdef SIGTERM
    signal(SIGTERM, sigterm_handler);
#endif
    int opt = 'w';
    bool foreground = true;
#ifndef WIN32
    while ((opt = getopt(argc, argv, "dhwv")) != -1)
#endif // !WIN32
    {
        switch (opt) {
        case 'd':
            foreground = false;
            break;
        case 'h':
            usage();
            return 0;
        case 'w':
            foreground = true;
            break;
        case 'v':
            printf("%s", VERSION);
            return 0;
        default:
            printf("Unknown option: %c\n", opt);
            foreground = true;
            break;
        }
    }
    if (foreground) {
        start();
        log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
        char cmdline[1024];
        do {
            printf(">");
            memset(&cmdline, 0, sizeof(cmdline));
            for (size_t i = 0; i < sizeof(cmdline); i++) {
                cmdline[i] = (char)getchar();
                if (cmdline[i] == '\n') {
                    cmdline[i] = '\0';
                    break;
                }
            }
            if (*cmdline) {
                running = cmdline_process(cmdline);
                if (running) {
                    cmdline[strlen(cmdline)] = '\r';
                    cmdline[strlen(cmdline)] = '\n';
                    LOG4CPLUS_INFO(log, "send " << strlen(cmdline) << " >>" << cmdline);
                    int rc = WRITE(serialfd, cmdline, strlen(cmdline));
                    if (rc < 0) {
                        LOG4CPLUS_ERROR(log, "AT_CHAT_CLIENT: CANNOT SEND DATA");
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } while (running);
    }
    else {
#ifndef WIN32
        pid_t pid;
        //
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS); //
        }


        //
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
#endif
        printf("Run as Daemon");
        start();
        log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
        LOG4CPLUS_INFO(log, "Run as Daemon");
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    stop();
}
