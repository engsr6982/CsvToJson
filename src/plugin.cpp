#if 0
TMP=$(mktemp -d)
c++ -std=c++11 -I src -o ${TMP}/a.out ${0} && ${TMP}/a.out ${@:1} ; RV=${?}
rm -rf ${TMP}
exit ${RV}
#endif

#include <iostream>
#include <string>
#include <vector>
#include "rapidcsv.h"

#include "version.h"
#include <llapi/LoggerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/ServerAPI.h>
#include <llapi/RemoteCallAPI.h>

extern Logger logger;

std::vector<std::vector<std::string>> csvTo2DArray(const std::string &csvFilePath)
{
    rapidcsv::Document doc(csvFilePath);
    std::vector<std::vector<std::string>> data;

    for (size_t i = 0; i < doc.GetRowCount(); ++i)
    {
        data.push_back(doc.GetRow<std::string>(i));
    }

    return data;
}

void exportToJavaScriptAPI()
{
    RemoteCall::exportAs("_CSV_JSON_", "_csvToJson_", [&](std::string name) -> std::vector<std::vector<std::string>>
                         {
                         logger.info("开始转换文件: {}", name);
                         auto start = std::chrono::high_resolution_clock::now();
                         auto s = csvTo2DArray(name);
                         auto end = std::chrono::high_resolution_clock::now();
                         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                         logger.info("转换文件[{}]完成，耗时: {}ms", name, duration);
                         return s; });
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