//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include "entities/misc/TripChain.hpp"
#include <boost/algorithm/string.hpp>

namespace sim_mob
{

template <class PERSON> class Role;

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
template <class PERSON>
class RoleFactory
{
private:
    /**The singleton instance of role factory*/
    static RoleFactory<PERSON> *roleFactory;

    /**Mapping between the roles and prototypes*/
    std::map<std::string, const sim_mob::Role<PERSON> *> prototypes;

    /**Indicates whether the role factory object is created*/
    static bool isRoleFactoryCreated;

public:
    ~RoleFactory()
    {
        typename std::map<std::string, const Role<PERSON> *>::iterator itPrototypes = prototypes.begin();
        while (itPrototypes != prototypes.end())
        {
            if(itPrototypes->second)
            {
                delete itPrototypes->second;
                itPrototypes->second = NULL;
            }
            itPrototypes++;
        }
        prototypes.clear();
    }

    /**
     * Register a role and a prototype.
     *
     * @param name the name of the role
     * @param prototype the prototype to be used when cloning the role
     */
    void registerRole(const std::string& name, const Role<PERSON> *prototype)
    {
        if (prototypes.count(name) > 0)
        {
            std::stringstream msg;
            msg << __func__ << ": Proto-type for role " << name << "has been registered.";
            throw std::runtime_error(msg.str());
        }

        prototypes[name] = prototype;
    }

    /**
     * Indicates whether the role is one that this role factory knows to construct
     *
     * @param roleName the role to be tested
     *
     * @return true, if the factory can construct the given role
     */
    bool isKnownRole(const std::string& roleName) const
    {
        return getPrototype(roleName);
    }

    /**
     * Creates a role based on the given name.
     *
     * @param name the name of the role
     * @param parent the person who will be carrying out the requested role
     *
     * @return the created role
     */
    Role<PERSON>* createRole(const std::string& name, PERSON *parent) const
    {
        const Role<PERSON> *prot = getPrototype(name);

        if (!prot)
        {
            std::stringstream msg;
            msg << __func__ << ": Invalid role: " << name;
            throw std::runtime_error(msg.str());
        }

        Role<PERSON> *role = prot->clone(parent);
        role->make_frame_tick_params(parent->currTick);

        return role;
    }

    /**
     * Creates a role based on the given trip chain item.
     *
     * @param tripChainItem the trip chain item for which the role has to be created
     * @param subTrip the sub trip within the trip chain for which the role has to be created
     * @param parent the person who will play the role
     *
     * @return the created role
     */
    Role<PERSON>* createRole(const TripChainItem *tripChainItem, const SubTrip *subTrip, PERSON *parent) const
    {
        std::string roleName;
        std::string id =parent->getDatabaseId();
        roleName = RoleFactory::getTripChainItemRoleName(tripChainItem, *subTrip);
        return createRole(roleName, parent);
    }

    /**
     * Convert the mode of a trip chain (e.g., "Car", "Walk") to one that we understand (e.g., "driver", "pedestrian").
     * NOTE: These should eventually be unified; for now, we have to do this manually.
     *
     * @param mode the mode of travel
     *
     * @return the role name
     */
    static std::string getRoleName(const std::string mode,bool isTTWalk)
    {
        if (mode == "Walk" || mode == "TravelPedestrian")
        {
            return "pedestrian";
        }
        if (mode == "Bus")
        {
            return "busdriver";
        }
        if (mode == "BusTravel" || mode == "MRT" || mode == "Sharing" || mode == "PrivateBus" ||
                mode == "TaxiTravel" || mode == "SMS_Taxi" || mode == "SMS_Pool_Taxi" || mode == "AMOD_Taxi" ||
                mode == "AMOD_Pool_Taxi" || mode == "Rail_SMS_Taxi" || mode == "Rail_SMS_Pool_Taxi" ||
                mode == "Rail_AMOD_Taxi" || mode == "Rail_AMOD_Pool_Taxi")
        {
            return "passenger";
        }
        if (mode == "WaitingBusActivity")
        {
            return "waitBusActivity";
        }
        if (mode == "WaitingTrainActivity")
        {
            return "waitTrainActivity";
        }
        if (mode == "WaitingTaxiActivity")
        {
            return "waitTaxiActivity";
        }
        if (mode == "Motorcycle" || mode == "Bike")
        {
            return "biker";
        }
        if (mode == "lgv")
        {
            return "truckerLGV";
        }
        if (mode == "hgv")
        {
            return "truckerHGV";
        }
        if (mode == "Activity")
        {
            return "activityRole";
        }
        if (mode == "Car" || mode == "Taxi")
        {
            return "driver";
        }
        if (!mode.empty())
        {
            std::stringstream msg;
            msg << __func__ << ": Invalid mode \'" << mode << "\'  present in activity_schedule ";
            throw std::runtime_error(msg.str());
        }

std::stringstream msg;
        msg << __func__ << ": Empty role name given";       throw std::runtime_error(msg.str());
    }

    /**
     * Gets the role name from the trip chain item
     *
     * @param tripChainItem the trip chain item for which the role name is to be found
     * @param subTrip the sub trip within the trip chain for which the role name is to be found
     *
     * @return the role name
     */
    const std::string getTripChainItemRoleName(const TripChainItem *tripChainItem, const SubTrip& subTrip) const
    {
        if (tripChainItem->itemType == TripChainItem::IT_TRIP)
        {
            return getRoleName(subTrip.getMode(),subTrip.isTT_Walk);
        }
        else if (tripChainItem->itemType == TripChainItem::IT_ACTIVITY)
        {
            return "activityRole";
        }
        else if (tripChainItem->itemType == TripChainItem::IT_BUSTRIP)
        {
            return "busdriver";
        }
        else if(tripChainItem->itemType == TripChainItem::IT_TRAINTRIP)
        {
            return "trainDriver";
        }
        else if (tripChainItem->itemType == TripChainItem::IT_TAXITRIP)
        {
            return "taxidriver";
        }
        else if (tripChainItem->itemType == TripChainItem::IT_ON_HAIL_TRIP)
        {
            return "onHailDriver";
        }
        else if (tripChainItem->itemType == TripChainItem::IT_ON_CALL_TRIP)
        {
            return "onCallDriver";
        }

        throw std::runtime_error("unknown TripChainItem type");
    }

    /**
     * Returns the role prototype corresponding to the given role name
     *
     * @param name the role name
     *
     * @return the role prototype
     */
    const Role<PERSON>* getPrototype(const std::string& name) const
    {
        typename std::map<std::string, const Role<PERSON> *>::const_iterator it = prototypes.find(name);
        if (it != prototypes.end())
        {
            const Role<PERSON> *role = it->second;
            return role;
        }
        return NULL;
    }

    static void setInstance(RoleFactory<PERSON> *rf)
    {
        if(!isRoleFactoryCreated)
        {
            roleFactory = rf;
            isRoleFactoryCreated = true;
        }
        else
        {
            std::stringstream msg;
            msg << __func__ << ": Attempting to replace existing instance of RoleFactory";
            throw std::runtime_error(msg.str());
        }
    }

    static const RoleFactory<PERSON>* getInstance()
    {
        return roleFactory;
    }

    void clear()
    {
        prototypes.clear();
    }
};

template<class PERSON> RoleFactory<PERSON>* RoleFactory<PERSON>::roleFactory = NULL;
template<class PERSON> bool RoleFactory<PERSON>::isRoleFactoryCreated = false;

}
