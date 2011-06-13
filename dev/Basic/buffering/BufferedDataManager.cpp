#include "BufferedDataManager.hpp"

using namespace sim_mob;

using std::vector;


////////////////////////////////////////////////////
// Implementation of our simple BufferedBase class
////////////////////////////////////////////////////
sim_mob::BufferedBase::BufferedBase(BufferedDataManager* mgr) : mgr(mgr) {
	if (mgr!=NULL) {
		mgr->add(this);
	}
}

sim_mob::BufferedBase::~BufferedBase() {
	if (mgr!=NULL) {
		mgr->rem(this);
	}
}

BufferedBase& sim_mob::BufferedBase::operator=(const BufferedBase& rhs)
{
	//If we were being managed, we're not any more.
	if (this->mgr!=NULL) {
		mgr->rem(this);
	}

	this->mgr = rhs.mgr;

	//Need to register this class with the new maanger.
	if (this->mgr!=NULL) {
		mgr->add(this);
	}

	return *this;
}


void sim_mob::BufferedBase::migrate(sim_mob::BufferedDataManager* newMgr)
{
	mgr = newMgr;
}




////////////////////////////////////////////////////
// Implementation of our BufferedDataManager
////////////////////////////////////////////////////



void sim_mob::BufferedDataManager::add(BufferedBase* datum)
{
	managedData.push_back(datum);
}

void sim_mob::BufferedDataManager::rem(BufferedBase* datum)
{
	std::vector<BufferedBase*>::iterator it = std::find(managedData.begin(), managedData.end(), datum);
	if (it!=managedData.end()) {
		managedData.erase(it);
	}
}

void sim_mob::BufferedDataManager::flip()
{
	for (vector<BufferedBase*>::iterator it=managedData.begin(); it!=managedData.end(); it++) {
		(*it)->flip();
	}
}



