/* 
 * Copyright Singapore-MIT Alliance for Research and Technology
 * 
 * File:   LuaProxy.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on October 9, 2013, 4:39 PM
 */

#include <stdexcept>
#include <string>
#include "boost/thread/tss.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/RawConfigParams.hpp"
#include "path/PT_RouteChoiceLuaProvider.hpp"

using namespace sim_mob;
using namespace sim_mob::lua;

namespace
{
    struct ModelContext
    {
        PT_RouteChoiceLuaModel ptrcModel;
    };

    boost::thread_specific_ptr<ModelContext> threadContext;

    void ensureContext()
    {
        if (!threadContext.get())
        {
            try
            {
                const ModelScriptsMap& extScripts = ConfigManager::GetInstance().FullConfig().luaScriptsMap;
                const std::string& scriptsPath = extScripts.getPath();
                ModelContext* modelCtx = new ModelContext();
                modelCtx->ptrcModel.loadFile(scriptsPath + extScripts.getScriptFileName("logit"));
                modelCtx->ptrcModel.loadFile(scriptsPath + extScripts.getScriptFileName("ptrc"));
                modelCtx->ptrcModel.initialize();
                threadContext.reset(modelCtx);
            }
            catch (const std::runtime_error& ex)
            {
                throw ex;
            }
        }
    }
}

PT_RouteChoiceLuaModel& PT_RouteChoiceLuaProvider::getPTRC_Model()
{
    ensureContext();
    return threadContext.get()->ptrcModel;
}
