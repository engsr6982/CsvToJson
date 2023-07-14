// 系统头文件 避免玄学，优先导入
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "rapidcsv.h"

// LL头文件
#include "version.h"
#include <llapi/LoggerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/ServerAPI.h>
#include <llapi/RemoteCallAPI.h>
#include <Nlohmann/json.hpp>

extern Logger logger;
using json = nlohmann::json;

// 是否启用缓存
bool isCache;
// 配置文件路径
std::string config_path = "./plugins/PPOUI/Web_Log/CSV_PRO.json";

// 缓存转换后的二维数组 key: 文件名 value: 二维数组
std::unordered_map<std::string, std::vector<std::vector<std::string>>> fileCache;
// 缓存文件的SHA1
std::unordered_map<std::string, std::string> fileHASH;


/**
 * @brief 释放配置文件
 * @return bool
 */
bool ReleaseConfig() {
	json Config;
	Config["Cache"] = true;
	std::ofstream file(config_path);
	file << Config << std::endl;
	return true;
}

/**
 * @brief 读取配置文件
 * @return bool
 */
bool readConfig()
{
	json Config;
	std::ifstream i(config_path);
	i >> Config;
	isCache = Config["Cache"];
	return true;
}

/**
 * @brief 获取文件HASH值
 * @return string hash
*/
std::string getFileHash(std::string filePath) {
	std::ifstream file(filePath);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::hash<std::string> hasher;
	return std::to_string(hasher(content));
}


/**
 * @brief 转换数据
 * @return vector<vector<string>> 二维数组
 */
std::vector<std::vector<std::string>> csvTo2DArray(const std::string& csvFilePath)
{
	// 记录开始时间
	logger.info("开始转换文件: {}", csvFilePath);
	auto start = std::chrono::high_resolution_clock::now();
	// 开始转换
	rapidcsv::Document doc(csvFilePath);
	std::vector<std::vector<std::string>> data;
	for (size_t i = 0; i < doc.GetRowCount(); ++i)
	{
		data.push_back(doc.GetRow<std::string>(i));
	}
	// 记录结束时间
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	logger.info("转换文件[{}]完成，耗时: {}ms", csvFilePath, duration);
	// 返回数据
	return data;
}


bool exportToJavaScriptAPI()
{
	std::string title = "_CSV_JSON_";
	RemoteCall::exportAs(title, "_csvToJson_", [&](std::string name) -> std::vector<std::vector<std::string>>
		{
			if (isCache) {
				// 缓存已开启, 检查文件是否变动(SHA1)
				std::string currentHash = getFileHash(name);
				if (fileHASH[name] != currentHash) {
					// 文件有变动，调用转换函数并更新缓存
					fileHASH[name] = currentHash;
					fileCache[name] = csvTo2DArray(name);
					return fileCache[name];
				}
				// 若未变动，直接返回缓存中的二维数组
				return fileCache[name];
			}
			else
			{
				// 缓存关闭，直接调用转换函数，并返回二维数组
				return csvTo2DArray(name);
			}
		}
	);
	// 增加API
	// 1. 获取所有SHA1缓存
	// 2. 获取所有缓存二维数组
	// 3. 清除缓存(二维数组 / SHA1)

	return true;
}



void PluginInit()
{
	// 初始化日志等级
	std::filesystem::exists("./plugins/PPOUI/debug") ? logger.consoleLevel = 5 : NULL;
	// 初始化配置文件
	std::filesystem::exists(config_path) ? readConfig() : ReleaseConfig();
	// 服务器启动完成事件
	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent)
		{
			auto current_protocol = ll::getServerProtocolVersion();
			if (TARGET_BDS_PROTOCOL_VERSION == current_protocol) {
				exportToJavaScriptAPI() ? logger.info("CsvToJson API已导出") : logger.error("导出API失败");
			}
			logger.debug("DeBug模式已启用");
			return true; });
}