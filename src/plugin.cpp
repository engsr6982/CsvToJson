#include <llapi/LoggerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/ServerAPI.h>

#include <iostream>
#include "version.h"

extern Logger logger;

void exportToJavaScriptAPI()
{
}

void PluginInit()
{
    std::filesystem::exists("./plugins/PPOUI/debug") ? logger.consoleLevel = 5 : NULL;
    Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent)
                                         {   
        auto current_protocol = ll::getServerProtocolVersion();
        if (TARGET_BDS_PROTOCOL_VERSION == current_protocol) {
            exportToJavaScriptAPI();
            logger.info("CsvToJson API已导出");
        } else {
            logger.error("协议版本错误！" + current_protocol);
        }
        logger.debug("DeBug模式已启用");
        return true; });
}