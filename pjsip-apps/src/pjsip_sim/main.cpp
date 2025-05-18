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
#include <pjlib-util/json.h>

#define VERSION "1.0.0.0"

std::string sip_server;
int sip_port = 5060;
std::string sip_domain;
std::string sip_userId;
std::string sip_password;
static bool running;

#define SERIAL_PORT_NAME        "/tmp/atcmdtest"

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
    }

}

void httpconfig()
{
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
    std::string data = "{\"cmd\":\"getinfo\",\"mac\" : \"8301002501000720\"}";
    client.Post(url, data, response, headers, http_code, nullptr);
    LOG4CPLUS_INFO(log, url << " " << http_code << " response " << response);

    if(http_code == 200){
        pj_caching_pool caching_pool;
        pj_pool_t* pool;
        pj_json_elem* elem;
        char* out_buf;
        unsigned size = response.size();
        pj_json_err_info err;
        pj_caching_pool_init(&caching_pool, NULL, 0);
        pool = pj_pool_create(&caching_pool.factory, "json", 1000, 1000, NULL);

        elem = pj_json_parse(pool, (char *)response.c_str(), &size, &err);
        if (elem) {
            if (elem->type == PJ_JSON_VAL_OBJ) {
                elem->value.children;
            }
        }



        pj_pool_release(pool);
        return ;
    }
}

static void sigterm_handler(int signo)
{
    running = false;
}

CPjSipSDK* p_sipsdk = nullptr;
int serialfd = 0;

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
    #ifndef WIN32
        int bytes = read(serialfd, buffer, BUFFSIZE - 1);
    #else
        int bytes = recv(serialfd, buffer, BUFFSIZE - 1, 0);
    #endif

        if (bytes < 1) {
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
        }
        else if (received.find("+CLCC: 1,1,4,0,0") != std::string::npos) {
            auto caller = received.substr(received.find("+CLCC: 1,1,4,0,0") + strlen("+CLCC: 1,1,4,0,0")+2);
            caller = caller.substr(0, caller.find("\""));
            pj::SipHeader h;
            h.hName ="X-Real-Caller-Number";
            h.hValue = caller;
            pj::SipHeaderVector headers ={h};
            p_sipsdk->makeCall(headers, sip_userId);
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


int main(int argc, char* argv[])
{
    log4cplus::initialize();

    {
        running = true;
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
        pj_init();
        pj_log_set_level(1);
        class MyPJSIP : public CPjSipSDK{
        public:
            MyPJSIP(){ log = log4cplus::Logger::getInstance("pjsip"); };
            ~MyPJSIP(){};
            void onRegisterError(int reason, const char* desc) override {
                LOG4CPLUS_ERROR(log, reason << " " << desc << " " << "onRegisterError ");
                timer.add(std::chrono::seconds(60), [=](CppTime::timer_id tid){
                    if (!pj::Endpoint::instance().libIsThreadRegistered()){
                        pj::Endpoint::instance().libRegisterThread("timer");
                    }
                    httpconfig();
                    this->Login(sip_server, sip_port, sip_domain, sip_userId, sip_password); 
                }
                );

            }
            void onRegistered(pj::OnRegStateParam& prm) override
            {
                //this->makeCall("9000");

                LOG4CPLUS_INFO(log, prm.rdata.srcAddress << " " << "onRegistered ");
            }
            void onIncomingCallReceived(int callType, const char* callid, const char* caller, const char * called)  //�к�к���
            {
                LOG4CPLUS_INFO(log, "onIncomingCallReceived callType:" << callType << " callid:" << callid << " caller:" << caller << " called:" << called);
                std::string atcmd = std::string("atd") + called + ";" + "\r\n";
                LOG4CPLUS_INFO(log, "send " << atcmd);
#ifndef WIN32

                int rc = write(serialfd, atcmd.data(), atcmd.size());
#else
                int rc = ::send((SOCKET)serialfd, atcmd.data(), atcmd.size(), 0);
#endif // !WIN32
            }

            void onCallReleased(const char* callid, int reason)				//��йһ�
            {
                LOG4CPLUS_INFO(log, "onCallReleased " << callid);
                std::string atcmd = std::string("ATH") + "\r\n";
                LOG4CPLUS_INFO(log, "send " << atcmd);
#ifndef WIN32
                int rc = write(serialfd, atcmd.data(), atcmd.size());
#else
                int rc = ::send(serialfd, atcmd.data(), atcmd.size(), 0);
#endif
            }
            void onCallAnswered(const char* callid)			//外呼对方应答
            {
                LOG4CPLUS_INFO(log, "onCallAnswered " << callid);
                std::string atcmd = std::string("ATA") + "\r\n";
                LOG4CPLUS_INFO(log, "send " << atcmd);
#ifndef WIN32
                int rc = write(serialfd, atcmd.data(), atcmd.size());
#else
                int rc = ::send(serialfd, atcmd.data(), atcmd.size(), 0);
#endif
            }

            CppTime::Timer timer;
            log4cplus::Logger log;
        };
        std::thread* receiveThread = nullptr;
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
        if(foreground){
            log4cplus::ConfigureAndWatchThread logconfig("log4cplus.properties", 10 * 1000);
            log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
            httpconfig();
#ifndef WIN32
            serialfd = connectUnixSocket(SERIAL_PORT_NAME);

            if (serialfd < 0) {
                LOG4CPLUS_ERROR(log, "ERROR: OPENING DEVICE: " << SERIAL_PORT_NAME);
                return 1;
            }
            else {
                LOG4CPLUS_INFO(log, "open socket:" << SERIAL_PORT_NAME);
            }

            fcntl(serialfd, F_SETFL, O_NONBLOCK);

            tcflush(serialfd, TCIFLUSH);
#endif
            MyPJSIP sipsdk;
            p_sipsdk = &sipsdk;
            receiveThread = new std::thread(ReceiveDataFromChan, serialfd);
            pj_log_set_decor(PJ_LOG_HAS_SENDER | PJ_LOG_HAS_INDENT);
            sipsdk.Login(sip_server, sip_port, sip_domain, sip_userId, sip_password);
            char cmdline[1024];
#ifndef  WIN32
            strcpy(cmdline, "AT+CEREG?\n\r\n");//注册状态
            LOG4CPLUS_INFO(log, "send " << cmdline);
            write(serialfd, cmdline, strlen(cmdline) + 1);
#endif // ! WIN32
            do {
                printf(">");
    #ifndef  WIN32
                malloc_trim(0);
    #endif // ! WIN32
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
    #ifndef WIN32
                    if(running){
                        LOG4CPLUS_INFO(log, "send " << cmdline);
                        cmdline[strlen(cmdline)] = '\r';
                        cmdline[strlen(cmdline)] = '\n';
                        int rc = write(serialfd, cmdline, strlen(cmdline)+1);
                        if (rc < 0) {
                            LOG4CPLUS_ERROR(log, "AT_CHAT_CLIENT: CANNOT SEND DATA");
                        }
                    }
    #endif
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } while (running);
        }
        else {
        #ifndef WIN32
            pid_t pid;
            // �����ӽ���
            pid = fork();
            if (pid < 0) {
                exit(EXIT_FAILURE);
            }
            if (pid > 0) {
                exit(EXIT_SUCCESS); // �������˳�
            }


            // �ر��ļ�������
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            #endif
            log4cplus::ConfigureAndWatchThread logconfig("log4cplus.properties", 10 * 1000);
            log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
            httpconfig();
            LOG4CPLUS_INFO(log, "Run as Daemon");
#ifndef WIN32
            serialfd = connectUnixSocket(SERIAL_PORT_NAME);

            if (serialfd < 0) {
                LOG4CPLUS_ERROR(log, "ERROR: OPENING DEVICE: " << SERIAL_PORT_NAME);
                return 1;
            }
            else {
                LOG4CPLUS_INFO(log, "open socket:" << SERIAL_PORT_NAME);
            }

            fcntl(serialfd, F_SETFL, O_NONBLOCK);

            tcflush(serialfd, TCIFLUSH);
#endif
            MyPJSIP sipsdk;
            p_sipsdk = &sipsdk;
            receiveThread = new std::thread(ReceiveDataFromChan, serialfd);
            pj_log_set_decor(PJ_LOG_HAS_SENDER | PJ_LOG_HAS_INDENT);
            sipsdk.Login(sip_server, sip_port, sip_domain, sip_userId, sip_password);
            while (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        receiveThread->join();
    }
    
    log4cplus::deinitialize();
    return 0;
}
