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
#endif
#include <malloc.h>


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

static void sigterm_handler(int signo)
{
    running = false;
}

#ifndef WIN32
void ReceiveDataFromChan(int serialfd)
{
    //pthread_detach(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    log4cplus::Logger log = log4cplus::Logger::getInstance("ReceiveDataFromChan");
    LOG4CPLUS_INFO(log, "ReceiveDataFromChan Start");
    int BUFFSIZE = 512;
    char buffer[BUFFSIZE];

    while (running) {

        int bytes = read(serialfd, buffer, BUFFSIZE - 1);

        if (bytes < 1) {
            usleep(1);
            continue;
        }

        buffer[bytes] = '\0';
        LOG4CPLUS_INFO(log, "<<" << buffer);
    }
    LOG4CPLUS_INFO(log, "ReceiveDataFromChan end");
}
#endif // !WIN32

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
    log4cplus::ConfigureAndWatchThread logconfig("log4cplus.properties", 10 * 1000);
    log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
    loadconfig();

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

        pj_log_set_level(1);
        class MyPJSIP : public CPjSipSDK{
        public:
            MyPJSIP(){};
            ~MyPJSIP(){};
            void onRegisterError(int reason, const char* desc) override {
                LOG4CPLUS_ERROR(log, reason << " " << desc << " " << "onRegisterError ");
                timer.add(std::chrono::seconds(60), [=](CppTime::timer_id tid){
                    if (!pj::Endpoint::instance().libIsThreadRegistered()){
                        pj::Endpoint::instance().libRegisterThread("timer");
                    }
                    loadconfig();
                    this->Login(sip_server, sip_port, sip_domain, sip_userId, sip_password); 
                }
                );

            }
            void onRegistered(pj::OnRegStateParam& prm) override
            {
                //this->makeCall("9000");

                LOG4CPLUS_INFO(log, prm.rdata.srcAddress << " " << "onRegistered ");
            }
            CppTime::Timer timer;
        }
        sipsdk;
#ifndef WIN32
        int serialfd = connectUnixSocket(SERIAL_PORT_NAME);

        if (serialfd < 0) {
            LOG4CPLUS_ERROR(log, "ERROR: OPENING DEVICE: " << SERIAL_PORT_NAME);
            return 1;
        }
        else {
            LOG4CPLUS_INFO(log, "open socket:" << SERIAL_PORT_NAME);
        }

        fcntl(serialfd, F_SETFL, O_NONBLOCK);

        tcflush(serialfd, TCIFLUSH);

        std::thread receiveThread = std::thread(ReceiveDataFromChan, serialfd);
#endif

        pj_log_set_decor(PJ_LOG_HAS_SENDER | PJ_LOG_HAS_INDENT);
        sipsdk.Login(sip_server, sip_port, sip_domain, sip_userId, sip_password);
        char cmdline[1024];
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
                    int rc = write(serialfd, cmdline, strlen(cmdline)+1);
                    if (rc < 0) {
                        LOG4CPLUS_ERROR(log, "AT_CHAT_CLIENT: CANNOT SEND DATA");
                    }
                }
#endif
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } while (running);
#ifndef WIN32
        receiveThread.join();
#endif // WIN32
    }
    
    log4cplus::deinitialize();
    return 0;
}
