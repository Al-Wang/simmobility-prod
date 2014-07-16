#include "Profiler.hpp"
#include "logging/Log.hpp"
int sim_mob::Profiler::totalProfilers = 0;

///Constructor + start profiling if init is true
sim_mob::Profiler::Profiler(bool init, std::string id_, std::string path){
	reset();
	start = stop = totalTime = 0;
	started = false;
	output.clear();
	outputSize = 0;
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		index = totalProfilers ++;
	}
	id = id_;
	if(path.size()){
		InitLogFile(path);
	}
	if(init && !isStarted()){
		startProfiling();
	}
}
sim_mob::Profiler::~Profiler(){
	if(LogFile.is_open()){
		flushLog();
		LogFile.close();
	}
}


///whoami
std::string sim_mob::Profiler::getId(){
	return id;
}
///whoami
int sim_mob::Profiler::getIndex(){
	return index;
}

bool sim_mob::Profiler::isStarted(){
	return started;
}

///like it suggests, store the start time of the profiling
void sim_mob::Profiler::startProfiling(){
	started = true;

	struct timeval  tv;
	struct timezone tz;
	struct tm      *tm;

	gettimeofday(&tv, &tz);
	tm = localtime(&tv.tv_sec);

	start = tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
		tm->tm_sec * 1000 + tv.tv_usec / 1000;
}

///save the ending time ...and .. if add==true add the value to the total time;
uint32_t sim_mob::Profiler::endProfiling(bool addToTotalTime_){
	if(!started){
		throw std::runtime_error("Profiler Ended before Starting");
	}

	struct timeval  tv;
	struct timezone tz;
	struct tm      *tm;

	gettimeofday(&tv, &tz);
	tm = localtime(&tv.tv_sec);

	stop = tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
		tm->tm_sec * 1000 + tv.tv_usec / 1000;

	uint32_t elapsed = stop - start;
	if(addToTotalTime_){
		addToTotalTime(elapsed);
	}
	return elapsed;
}

///add the given time to the total time
void sim_mob::Profiler::addToTotalTime(uint32_t value){
	boost::unique_lock<boost::mutex> lock(mutexTotalTime);
//		Print() << "Profiler "  << "[" << index << ":" << id << "] Adding " << value << " seconds to total time " << std::endl;
	totalTime+=value;
}
std::stringstream & sim_mob::Profiler::outPut(){
	boost::unique_lock<boost::mutex> lock(mutexOutput);
	return output;
}

//	void sim_mob::Profiler::addOutPut(std::stringstream & s){
////		Print() << "Dump: " << s.str() <<  std::endl;
//		boost::unique_lock<boost::mutex> lock(mutexOutput);
//		outputSize+=s.str().size();
//		output << s.str();
////		Print() << "Dump-so-far: " << output.str() <<  std::endl;
//		if(outputSize > 1000000){
//			flushLog();
//		}
//	}
void sim_mob::Profiler::flushLog(){
		if ((LogFile.is_open() && LogFile.good())) {
			boost::unique_lock<boost::mutex> lock(mutexOutput);
			LogFile << output.str();
			LogFile.flush();
			output.str("");
			outputSize = 0;
		}
		else{
			Warn() << "pathset profiler log ignored" << std::endl;
		}

}
unsigned int & sim_mob::Profiler::getTotalTime(){
	boost::unique_lock<boost::mutex> lock(mutexTotalTime);
	return totalTime;
}

void printTime(struct tm *tm, struct timeval & tv, std::string id){
	sim_mob::Print() << "TIMESTAMP:\t  " << tm->tm_hour << std::setw(2) << ":" <<tm->tm_min << ":" << ":" <<  tm->tm_sec << ":" << tv.tv_usec << std::endl;
}

void sim_mob::Profiler::reset(){
	start = stop = totalTime = 0;
	started = false;
	output.clear();
	id = "";
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		index = totalProfilers ++;
	}
}
sim_mob::Profiler & sim_mob::Profiler::getInstance(){
	static Profiler instance(true, "overall profiler", "profiler.txt");
	return instance;
}

void  sim_mob::Profiler::InitLogFile(const std::string& path)
{
	//1. first type of implementation
	LogFile.open(path.c_str());
	if (LogFile.fail()) {
		log_handle =  &std::cout;
	}
	log_handle = (!LogFile.fail() ? &LogFile : &std::cout);
}

///////////////////////////////////////////////////////////////////////////
std::ostream*   log_handle;
//Type of cout.
typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
//Type of std::endl and some other manipulators.
typedef CoutType& (*StandardEndLine)(CoutType&);

sim_mob::Profiler&  sim_mob::Profiler::operator<<(StandardEndLine manip) {
		manip(output);
	return *this;
}

