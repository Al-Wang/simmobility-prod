//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <string>
#include <map>

namespace sim_mob
{

class Role;
class Person;
class TripChainItem;
class SubTrip;


/**
 * Class which handles the creation of Roles.
 *
 * \author Seth N. Hetu
 *
 * This class allows one to create Roles without the knowledge of where those Roles are
 * implemented. This is particularly useful when two roles have the same identity (short + medium) but
 * very different functionality.
 *
 * In addition, it does the following:
 *   \li Allows for future language-independence (e.g., a Role written in Python/Java)
 *   \li Allows the association of Roles with strings (needed by the config file).
 *   \li Allows full delayed loading of Roles, by saving the actual config string.
 */
class RoleFactory {
public:
	///Register a Role, and a prototype we can clone to create members of this Role.
	void registerRole(const std::string& name, const sim_mob::Role* prototype);
	void registerRole(const sim_mob::Role* prototype);

	///Is this a Role that our Factory knows how to construct?
	bool isKnownRole(const std::string& roleName) const;

	///Return a map of required attributes, with a flag on each set to false.
	std::map<std::string, bool> getRequiredAttributes(const std::string& roleName) const;

	///Create a role based on its name
	sim_mob::Role* createRole(const std::string& name, sim_mob::Person* parent) const;

	///Create a Role based on the current TripChain item.
	Role* createRole(const TripChainItem* currTripChainItem, const sim_mob::SubTrip& subTrip, Person* parent) const;

	///Workaround: Convert the mode of a trip chain (e.g., "Car", "Walk") to one that
	///            we understand (e.g., "driver", "pedestrian"). These should eventually
	///            be unified; for now, we have to do this manually.
	static std::string GetRoleName(const std::string mode);
	const std::string GetTripChainItemRoleName(const sim_mob::TripChainItem *tripChainItem, const sim_mob::SubTrip& subTrip) const;

	void clear() { prototypes.clear(); }

public:
	//Helper
	const sim_mob::Role* getPrototype(const std::string& name) const;

private:
	//List of Roles and prototypes
	std::map<std::string, const sim_mob::Role*> prototypes;



};


}
