/* 
 * Copyright Singapore-MIT Alliance for Research and Technology
 * 
 * File:   LuaModel.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on October 10, 2013, 11:21 AM
 */

#include "LuaModel.hpp"
#include <boost/filesystem.hpp>
#include "util/LangHelpers.hpp"
#include <sstream>

using namespace sim_mob;
using namespace sim_mob::lua;
namespace fs = ::boost::filesystem;
using std::string;
using std::list;
using std::runtime_error;

namespace
{
    const string LUA_FILE_EXTENSION = ".lua";

    void closeLua(lua_State* state)
    {
        if (state)
        {
            lua_close(state);
        }
    }

    void loadLuaFile(lua_State* state, const fs::path& filePath)
    {
        // only loads if file exists and is valid
        if (fs::exists(filePath) && fs::is_regular_file(filePath))
        {
            if (filePath.extension() == LUA_FILE_EXTENSION)
            {
                luaL_dofile(state, filePath.string().c_str());
            }
        }
    }
}

LuaModel::LuaModel(): state(luaL_newstate(), closeLua), initialized(false) {}

LuaModel::LuaModel(const LuaModel& orig): state(orig.state.get(), closeLua),files(orig.files), initialized(orig.initialized) {}

LuaModel::~LuaModel()
{
    initialized = false;
}

void LuaModel::initialize()
{
    luaL_openlibs(state.get());
    mapClasses();

    //loads all lua files.
    for (list<string>::iterator it = files.begin(); it != files.end(); ++it)
    {
        fs::path fPath((*it));
        loadLuaFile(state.get(), fPath);
    }
    initialized = true;
}

void LuaModel::loadFile(const std::string& filePath)
{
    fs::path fPath(filePath);
    if (fs::exists(fPath) && fs::is_regular_file(fPath))
    {
        if (fPath.extension() == LUA_FILE_EXTENSION)
        {
            if (initialized)
            {
                loadLuaFile(state.get(), fPath);
            }
            files.push_back(filePath);
        }
        else
        {
        	std::stringstream msg; msg << "Problem with file "<< filePath <<
        		". Your file must have the extension .lua";
            throw runtime_error(msg.str() );
        }
    }
    else
    {
    	std::stringstream msg; msg << "File "<< filePath <<
    	        		" does not exist or is invalid";
    	throw runtime_error(msg.str() );
    }
}

void LuaModel::loadDirectory(const std::string& dirPath)
{
    //load all files which belong to the model
    fs::path dPath(dirPath);
    if (fs::exists(dPath) && fs::is_directory(dPath))
    {
        fs::directory_iterator it(dPath);
        fs::directory_iterator endit;

        while (it != endit)
        {
            files.push_back((*it).path().string());
            ++it;
        }
    }
    else
    {
        throw runtime_error("Model folder value is not a valid directory.");
    }
}
