#include "PjSipSDK.h"
#include <log4cplus/configurator.h>
#include <log4cplus/log4cplus.h>
#include "tinyxml2.h"

int main(int argc, char* argv[])
{
    log4cplus::initialize();
    log4cplus::ConfigureAndWatchThread logconfig("log4cplus.properties", 10 * 1000);
    log4cplus::Logger log = log4cplus::Logger::getInstance("pjsip");
    std::string sip_server;
    int sip_port = 5060;
    std::string sip_domain;
    std::string sip_userId;
    std::string sip_password;
    using namespace tinyxml2;
    tinyxml2::XMLDocument config;
    if (config.LoadFile("pjsip.xml") != XMLError::XML_SUCCESS)
    {
        LOG4CPLUS_ERROR(log, "load config file error:" << config.ErrorName() << ":" << config.GetErrorStr1());
        return -1;
    }

    if (tinyxml2::XMLElement* eConfig = config.FirstChildElement("Config")) {

        if (tinyxml2::XMLElement* server = eConfig->FirstChildElement("Server")) {
            if (server && server->GetText()) {
                sip_server = server->GetText();
            }
        }
        if (tinyxml2::XMLElement* port = eConfig->FirstChildElement("Port")) {
            if (port && port->GetText()) {
                sip_port = std::stoi(port->GetText());
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

    {

        pj_log_set_level(1);
        CPjSipSDK sipsdk;
        sipsdk.Login(sip_server, sip_port, sip_domain, sip_userId, sip_password);

        std::getchar();

    }
    log4cplus::deinitialize();
    return 0;
}
