// 系统头文件
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

// 第三方库
#include "rapidcsv.h"

// LL头文件
#include "version.h"
#include <llapi/LoggerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/ServerAPI.h>
#include <llapi/RemoteCallAPI.h>
#include <Nlohmann/json.hpp>
#include <llapi/KVDBAPI.h>

// 作者插件目录
#define _AUTHOR_PATH_ "./plugins/" PLUGIN_AUTHOR "/"
// 插件目录
#define _PLUGIN_PATH_ _AUTHOR_PATH_ "Web_Log/"
// 配置文件路径
#define _CONFIG_PATH_ _PLUGIN_PATH_ "CSV_PRO.json"

#define _EXPORT_NAMESPACE_ "_CSV_JSON_"

extern Logger logger;
using json = nlohmann::json;

// 是否启用缓存
bool isCache;

// KVDB数据库
std::unique_ptr<KVDB> db;
void initdb()
{
	db = KVDB::create(_PLUGIN_PATH_ "_Cache");
	assert(db != nullptr && *db);
}

/**
 * @brief 释放配置文件
 * @return bool
 */
bool ReleaseConfig()
{
	json Config;
	Config["Cache"] = true;
	std::ofstream file(_CONFIG_PATH_);
	file << Config.dump(4) << std::endl;
	return true;
}

/**
 * @brief 读取配置文件
 * @return bool
 */
bool readConfig()
{
	json Config;
	std::ifstream i(_CONFIG_PATH_);
	i >> Config;
	isCache = Config["Cache"];
	return true;
}

/**
 * @brief 获取文件HASH值
 * @return string hash
 */
std::string getFileHash(std::string filePath)
{
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
	// 转换API
	RemoteCall::exportAs(_EXPORT_NAMESPACE_, "_csvToJson_", [&](std::string name) -> std::vector<std::vector<std::string>>
		{
			// 缓存开启
			logger.debug("缓存状态: {}", isCache);
			if (isCache) {
				// 计算HASH
				std::string currentHash = getFileHash(name);
				std::string value;
				// 检查数据是否存在
				auto status = db->get(name, value);
				logger.debug("文件HASH: {}", currentHash);
				logger.debug("检查缓存：{}", status);
				if (status)
				{
					json cache = json::parse(value);
					if (cache["hash"].get<std::string>() != currentHash)
					{
						// hash不匹配
						auto tmp = csvTo2DArray(name);
						json newCache;
						newCache["hash"] = currentHash;
						newCache["data"] = tmp;
						db->set(name, newCache.dump());
						return tmp;
					}
					return cache["data"];/* .get<std::vector<std::vector<std::string>>>(); */
				}
			}
			// 缓存关闭
			return csvTo2DArray(name);
		}
	);
	return true;
}

void PluginInit()
{
	std::filesystem::exists(_AUTHOR_PATH_ "debug") ? logger.consoleLevel = 5 : NULL;
	// 初始化配置文件
	if (!std::filesystem::exists(_CONFIG_PATH_))
	{
		ReleaseConfig();
		logger.warn("检测到配置文件不存在，正在生成配置文件...");
	}
	// 服务器启动完成事件
	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent)
		{
			auto current_protocol = ll::getServerProtocolVersion();
			if (TARGET_BDS_PROTOCOL_VERSION == current_protocol) {
				exportToJavaScriptAPI() ? logger.info("CsvToJson API已导出") : logger.error("导出API失败");
			}
			// 读取配置文件
			readConfig();
			// 初始化数据库
			initdb();
			return true; });
}