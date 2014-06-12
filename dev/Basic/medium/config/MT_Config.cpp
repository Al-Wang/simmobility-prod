/*
 * MTConfig.cpp
 *
 *  Created on: 2 Jun, 2014
 *      Author: zhang
 */

#include "MT_Config.hpp"
#include "util/LangHelpers.hpp"

namespace sim_mob
{
namespace medium
{

ModelScriptsMap::ModelScriptsMap(const std::string& scriptFilesPath, const std::string& scriptsLang) : path(scriptFilesPath), scriptLanguage(scriptsLang) {}

MongoCollectionsMap::MongoCollectionsMap(const std::string& dbName) : dbName(dbName) {}

MT_Config::MT_Config() : pedestrianWalkSpeed(0), numPredayThreads(0) {}

MT_Config::~MT_Config() {}

MT_Config* MT_Config::instance(nullptr);

MT_Config& MT_Config::GetInstance()
{
	if (!instance)
	{
		instance = new MT_Config();
	}
	return *instance;
}



}
}

