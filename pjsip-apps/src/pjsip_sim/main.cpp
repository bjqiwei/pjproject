#include "PjSipSDK.h"
#include <log4cplus/configurator.h>
#include <log4cplus/log4cplus.h>
#include "tinyxml2.h"
#include <signal.h>
#include <thread>
#include "cpptimer.h"
#ifndef WIN32
#include <stdlib.h>
#endif
#include <malloc.h>


std::string sip_server;
int sip_port = 5060;
std::string sip_domain;
std::string sip_userId;
std::string sip_password;
static bool running;

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

static bool cmdline_process(char* cmdline)
{
    bool result = true;
    char* name;
    char* last;
    name = strtok(cmdline, " ");


    if (strcasecmp(name, "exit") == 0 || strcmp(name, "quit") == 0) {
        result = false;
    }

    else if (strcasecmp(name, "help") == 0) {
        printf("usage:\n");
        printf("- ... quit\n");
        printf("- quit, exit\n");
    }
    else {
        printf("unknown command: %s (input help for usage)\n", name);
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
                this->makeCall("9000");

                LOG4CPLUS_INFO(log, prm.rdata.srcAddress << " " << "onRegistered ");
            }
            CppTime::Timer timer;
        }
        sipsdk;

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
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } while (running);

    }
    log4cplus::deinitialize();
    return 0;
}
