/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "entities/Agent.hpp"
#include "ProfileBuilder.hpp"

//Somewhat hackish way of getting "timespec" defined.
#ifdef SIMMOB_AGENT_UPDATE_PROFILE
#define _XOPEN_SOURCE 700
#include <time.h>
#undef _XOPEN_SOURCE
#endif


using namespace sim_mob;

using std::string;

//Static initialization
std::ofstream ProfileBuilder::LogFile;
boost::mutex ProfileBuilder::profile_mutex;
int ProfileBuilder::ref_count = 0;


void ProfileBuilder::InitLogFile(const string& path)
{
	LogFile.open(path.c_str());
	if (LogFile.fail()) {
		throw std::runtime_error("Couldn't open Profile Builder log file.");
	}
}

int ProfileBuilder::RefCountUpdate(int amount)
{
	{
	boost::mutex::scoped_lock local_lock(profile_mutex);
	ProfileBuilder::ref_count += amount;
	return ProfileBuilder::ref_count;
	}
}


ProfileBuilder::ProfileBuilder()
{
	RefCountUpdate(1);
}

ProfileBuilder::~ProfileBuilder()
{
	//Write any pending output
	flushLogFile();

	//Close the log file?
	int numLeft = RefCountUpdate(-1);
	if (numLeft==0) {
		LogFile.close();
	}
}

void ProfileBuilder::flushLogFile()
{
	//Do nothing if the current log string is empty.
	std::string currLogStr = currLog.str();
	if (currLogStr.empty()) {
		return;
	}

	//Error if the log file isn't in a valid state.
	if (!(LogFile.is_open() && LogFile.good())) {
		throw std::runtime_error("ProfileBuilder can't flush log file; log file is not open.");
	}

	//Write and flush the current buffer.
	boost::mutex::scoped_lock local_lock(profile_mutex);
	LogFile <<currLogStr;
	currLog.str("");
}


#ifdef SIMMOB_AGENT_UPDATE_PROFILE
string ProfileBuilder::GetCurrentTime()
{
	timespec timeres;
	int res;

	{
	//The documentation claims that clock_gettime() is thread-safe
	//boost::mutex::scoped_lock local_lock(profile_mutex);
	res = clock_gettime(CLOCK_REALTIME, &timeres); //For epoch time.
	//res = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeres); //For time since the process started
	}

	//Error?
	if (res!=0) {
		return "<error>";
	}

	//Convert
	std::stringstream msg;
	msg <<"(sec," <<timeres.tv_sec <<"),";
	msg <<"(nano," <<timeres.tv_nsec <<")";
	return msg.str();
}
#else
string ProfileBuilder::GetCurrentTime() { return "<not_supported>"; }
#endif


void ProfileBuilder::logAgentUpdateBegin(const Agent& ag, frame_t tickID)
{
	logAgentUpdateGeneric(ag, "update-begin", &tickID);
}

void ProfileBuilder::logAgentUpdateEnd(const Agent& ag, frame_t tickID)
{
	logAgentUpdateGeneric(ag, "update-end", &tickID);
}

void ProfileBuilder::logAgentCreated(const Agent& ag)
{
	logAgentUpdateGeneric(ag, "constructed");
}

void ProfileBuilder::logAgentException(const Agent& ag, frame_t tickID, const std::exception& ex)
{
	logAgentUpdateGeneric(ag, "exception", &tickID, ex.what());
}

void ProfileBuilder::logAgentDeleted(const Agent& ag)
{
	logAgentUpdateGeneric(ag, "destructed");
}


void ProfileBuilder::logAgentUpdateGeneric(const Agent& ag, const string& action, const frame_t* const tickID, const string& message)
{
	currLog <<"{"
			<<"\"" <<"action" <<"\"" <<":" <<"\"" <<action <<"\"" <<","
			<<"\"" <<"agent" <<"\"" <<":" <<"\"" <<ag.getId() <<"\"" <<","
			<<"\"" <<"worker" <<"\"" <<":" <<"\"" <<ag.currWorker <<"\"" <<",";
	if (tickID) {
		currLog	<<"\"" <<"tick" <<"\"" <<":" <<"\"" <<*tickID <<"\"" <<",";
	}
	currLog <<"\"" <<"real-time" <<"\"" <<":" <<"\"" <<GetCurrentTime() <<"\"" <<",";
	if (!message.empty()) {
		currLog <<"\"" <<"message" <<"\"" <<":" <<"\"" <<message <<"\"" <<",";
	}
	currLog <<"}\n";
}










