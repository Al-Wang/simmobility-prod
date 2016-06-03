//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "PersonParams.hpp"

#include <sstream>

using namespace std;
using namespace sim_mob;
using namespace medium;

namespace
{
const int NUM_VALID_INCOME_CATEGORIES = 12;
const std::vector<long> EMPTY_VECTOR_OF_LONGS = std::vector<long>();

/**
 * initializes all possible time windows in a day and returns a vector of all windows.
 */
std::vector<TimeWindowAvailability> insertAllTimeWindows()
{
	// Following values are hard coded for now.
	double dayStart = 1; //index of first half hour window in a day
	double dayEnd = 48; //index of last half hour window in a day
	double stepSz = 1;

	std::vector<TimeWindowAvailability> timeWindows;
	for (double i = dayStart; i <= dayEnd; i = i + stepSz)
	{
		for (double j = i; j <= dayEnd; j = j + stepSz)
		{
			timeWindows.push_back(TimeWindowAvailability(i, j));
		}
	}
	return timeWindows;
}

}

TimeWindowAvailability::TimeWindowAvailability() :
		startTime(0), endTime(0), availability(false)
{
}

TimeWindowAvailability::TimeWindowAvailability(double startTime, double endTime, bool availability) :
		startTime(startTime), endTime(endTime), availability(availability)
{
	if (startTime > endTime)
	{
		throw std::runtime_error("Invalid time window; start time cannot be greater than end time");
	}
}

const std::vector<TimeWindowAvailability> TimeWindowAvailability::timeWindowsLookup = insertAllTimeWindows();

double sim_mob::medium::PersonParams::incomeCategoryLowerLimits[] = {};
std::map<int, std::bitset<4> > sim_mob::medium::PersonParams::vehicleCategoryLookup = std::map<int, std::bitset<4> >();
std::map<long, sim_mob::medium::Address> sim_mob::medium::PersonParams::addressLookup = std::map<long, sim_mob::medium::Address>();
std::map<unsigned int, unsigned int> sim_mob::medium::PersonParams::postCodeToNodeMapping = std::map<unsigned int, unsigned int>();
std::map<int, std::vector<long> > sim_mob::medium::PersonParams::zoneAddresses = std::map<int, std::vector<long> >();

sim_mob::medium::PersonParams::PersonParams() :
		personId(""), hhId(""), personTypeId(-1), ageId(-1), isUniversityStudent(-1), studentTypeId(-1), isFemale(-1), incomeId(-1), worksAtHome(-1),
			carOwn(-1), carOwnNormal(-1), carOwnOffpeak(-1), motorOwn(-1), hasFixedWorkTiming(-1), homeLocation(-1), fixedWorkLocation(-1),
			fixedSchoolLocation(-1), stopType(-1), drivingLicence(-1), hhOnlyAdults(-1), hhOnlyWorkers(-1), hhNumUnder4(-1), hasUnder15(-1), workLogSum(0),
			eduLogSum(0), shopLogSum(0), otherLogSum(0), dptLogsum(0), dpsLogsum(0), dpbLogsum(0), genderId(-1), missingIncome(-1), homeAddressId(-1),
			activityAddressId(-1), carLicense(false), motorLicense(false), vanbusLicense(false), fixedWorkplace(false), student(false), hhSize(-1),
			hhNumAdults(-1), hhNumWorkers(-1), hhNumUnder15(-1), householdFactor(-1)
{
	initTimeWindows();
}

sim_mob::medium::PersonParams::~PersonParams()
{
	timeWindowAvailability.clear();
}

void sim_mob::medium::PersonParams::initTimeWindows()
{
	if (!timeWindowAvailability.empty())
	{
		timeWindowAvailability.clear();
	}
	for (double i = 1; i <= 48; i++)
	{
		for (double j = i; j <= 48; j++)
		{
			timeWindowAvailability.push_back(TimeWindowAvailability(i, j, true)); //make all time windows available
		}
	}
}

void sim_mob::medium::PersonParams::blockTime(double startTime, double endTime)
{
	if (startTime <= endTime)
	{
		for (std::vector<TimeWindowAvailability>::iterator i = timeWindowAvailability.begin(); i != timeWindowAvailability.end(); i++)
		{
			TimeWindowAvailability& twa = (*i);
			double start = twa.getStartTime();
			double end = twa.getEndTime();
			if ((start >= startTime && start <= endTime) || (end >= startTime && end <= endTime))
			{
				twa.setAvailability(false);
			}
		}
	}
	else
	{
		std::stringstream errStream;
		errStream << "invalid time window was passed for blocking" << " |start: " << startTime << " |end: " << endTime << std::endl;
		throw std::runtime_error(errStream.str());
	}
}

int PersonParams::getTimeWindowAvailability(size_t timeWnd, int mode) const
{
	const sim_mob::medium::TimeWindowAvailability& timeWndwAvail = timeWindowAvailability[timeWnd - 1];
	//anytime before 6AM cannot is not a valid start time for tour's primary activity with PT modes
	if((mode == 1 || mode == 2) && timeWndwAvail.getStartTime() <= 6)
	{
		return 0;
	}
	else
	{
		return timeWindowAvailability[timeWnd - 1].getAvailability();
	}
}

void sim_mob::medium::PersonParams::setIncomeIdFromIncome(double income)
{
	int i = 0;
	while (i < NUM_VALID_INCOME_CATEGORIES && income >= incomeCategoryLowerLimits[i])
	{
		i++;
	}
	i = i - 1; //income id is the index of the greatest element in incomeCategoryLowerLimits lower than or equal to income
	setIncomeId((i > 0) ? i : NUM_VALID_INCOME_CATEGORIES); //lua models expect 12 to be the id for no income
}

void sim_mob::medium::PersonParams::setVehicleOwnershipFromCategoryId(int vehicleCategoryId)
{
	std::map<int, std::bitset<4> >::const_iterator it = vehicleCategoryLookup.find(vehicleCategoryId);
	if (it == vehicleCategoryLookup.end())
	{
		throw std::runtime_error("Invalid vehicle category");
	}
	const std::bitset<4>& vehOwnershipBits = it->second;
	setCarOwn(vehOwnershipBits[0]);
	setCarOwnNormal(vehOwnershipBits[1]);
	setCarOwnOffpeak(vehOwnershipBits[2]);
	setMotorOwn(vehOwnershipBits[3]);
}

std::string sim_mob::medium::PersonParams::print()
{
	std::stringstream printStrm;
	printStrm << personId << "," << personTypeId << "," << ageId << "," << isUniversityStudent << "," << hhOnlyAdults << "," << hhOnlyWorkers << ","
			<< hhNumUnder4 << "," << hasUnder15 << "," << isFemale << "," << incomeId << "," << missingIncome << "," << worksAtHome << "," << carOwn << ","
			<< carOwnNormal << "," << carOwnOffpeak << "," << motorOwn << "," << workLogSum << "," << eduLogSum << "," << shopLogSum << "," << otherLogSum
			<< std::endl;
	return printStrm.str();
}

void sim_mob::medium::PersonParams::fixUpParamsForLtPerson()
{
	if(incomeId >= 12)
	{
		//in preday models, income value of 0 (12 - No income categroy) is considered as missing income
		setMissingIncome(1);
	}
	else
	{
		setMissingIncome(0);
	}
	setHouseholdFactor(1); // no scaling of persons when generating day activity schedule
	setHomeLocation(getTAZCodeForAddressId(homeAddressId));
	setFixedSchoolLocation(0);
	setFixedWorkLocation(0);
	if (fixedWorkplace)
	{
		setFixedWorkLocation(getTAZCodeForAddressId(activityAddressId));
	}
	if (student)
	{
		setFixedSchoolLocation(getTAZCodeForAddressId(activityAddressId));
	}
	setHasDrivingLicence(getCarLicense() || getVanbusLicense());
	setIsUniversityStudent(studentTypeId == 4);
	setIsFemale(genderId == 2);
	setHH_OnlyAdults(hhNumAdults == hhSize);
	setHH_OnlyWorkers(hhNumWorkers == hhSize);
	setHH_HasUnder15(hhNumUnder15 > 0);
}

int sim_mob::medium::PersonParams::getTAZCodeForAddressId(long addressId) const
{
	std::map<long, sim_mob::medium::Address>::const_iterator addressIdIt = addressLookup.find(addressId);
	if (addressIdIt == addressLookup.end())
	{
		throw std::runtime_error("invalid address id");
	}
	return addressIdIt->second.getTazCode();
}

unsigned int sim_mob::medium::PersonParams::getSimMobNodeForAddressId(long addressId) const
{
	std::map<long, sim_mob::medium::Address>::const_iterator addressIdIt = addressLookup.find(addressId);
	if (addressIdIt == addressLookup.end())
	{
		throw std::runtime_error("invalid address id " + std::to_string(addressId));
	}
	unsigned int postcode = addressIdIt->second.getPostcode();
	std::map<unsigned int, unsigned int>::const_iterator postcodeIt = postCodeToNodeMapping.find(postcode);
	if(postcodeIt == postCodeToNodeMapping.end())
	{
		throw std::runtime_error("invalid postcode " + std::to_string(postcode));
	}
	return postcodeIt->second;
}

int sim_mob::medium::ZoneAddressParams::getNumAddresses() const
{
	return numAddresses;
}

double sim_mob::medium::ZoneAddressParams::getDistanceMRT(int addressIdx) const
{
	if(addressIdx > numAddresses || addressIdx <= 0)
	{
		throw std::runtime_error("Invalid address index passed to getDistanceMRT()");
	}
	long addressId = zoneAddresses[addressIdx-1];
	std::map<long, sim_mob::medium::Address>::const_iterator addressIt = addressLookup.find(addressId);
	if (addressIt == addressLookup.end())
	{
		throw std::runtime_error("invalid address id " + std::to_string(addressId));
	}
	return addressIt->second.getDistanceMrt();
}

double sim_mob::medium::ZoneAddressParams::getDistanceBus(int addressIdx) const
{
	if(addressIdx > numAddresses || addressIdx <= 0)
	{
		throw std::runtime_error("Invalid address index passed to getDistanceBus()");
	}
	long addressId = zoneAddresses[addressIdx-1];
	std::map<long, sim_mob::medium::Address>::const_iterator addressIt = addressLookup.find(addressId);
	if (addressIt == addressLookup.end())
	{
		throw std::runtime_error("invalid address id " + std::to_string(addressId));
	}
	return addressIt->second.getDistanceBus();
}

sim_mob::medium::ZoneAddressParams::ZoneAddressParams(const std::map<long, sim_mob::medium::Address>& addressLkp, const std::vector<long>& znAddresses)
	: addressLookup(addressLkp), zoneAddresses(znAddresses), numAddresses(znAddresses.size())
{
}

sim_mob::medium::ZoneAddressParams::~ZoneAddressParams()
{
}

long sim_mob::medium::ZoneAddressParams::getAddressId(int addressIdx) const
{
	if(addressIdx > numAddresses || addressIdx <= 0)
	{
		throw std::runtime_error("Invalid address index passed to getAddress()");
	}
	return zoneAddresses[addressIdx-1];
}

const std::vector<long>& sim_mob::medium::PersonParams::getAddressIdsInZone(int zoneCode) const
{
	std::map<int, std::vector<long> >::const_iterator znAddressIt = zoneAddresses.find(zoneCode);
	if(znAddressIt == zoneAddresses.end())
	{
		return EMPTY_VECTOR_OF_LONGS;
	}
	return znAddressIt->second;
}
