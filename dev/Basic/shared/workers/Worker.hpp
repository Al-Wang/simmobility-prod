//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <ostream>
#include <vector>
#include <set>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include "buffering/BufferedDataManager.hpp"
#include "metrics/Frame.hpp"
#include "event/EventPublisher.hpp"
#include "event/SystemEvents.hpp"
#include "event/args/EventArgs.hpp"
//#include "event/EventListener.hpp"

namespace sim_mob {

class FlexiBarrier;
class ProfileBuilder;
class ControlManager;
class WorkGroup;
class Entity;
class PathSetManager;

//subclassed Eventpublisher coz its destructor is pure virtual
class UpdatePublisher: public sim_mob::event::EventPublisher  {
public:
    UpdatePublisher() {
        registerEvent(sim_mob::event::EVT_CORE_AGENT_UPDATED);
    }

    virtual ~UpdatePublisher(){}
};



class UpdateEventArgs: public sim_mob::event::EventArgs {
    const sim_mob::Entity *entity;
public:
    const Entity * GetEntity()const;
    UpdateEventArgs(const sim_mob::Entity *agent);
    virtual ~UpdateEventArgs();
};

/**
 * A "WorkerProvider" is a restrictive parent class of Worker that provides Worker-related
 * functionality to Agents. This prevents the Agent from requiring full access to the Worker.
 * All methods are abstract virtual; the Worker is expected to fill them in.
 *
 * \note
 * See the Worker class for documentation on these functions.
 */
class WorkerProvider : public BufferedDataManager
{
protected:
    /**Worker specific random number generator*/
    boost::mt19937 gen;

public:
    //NOTE: Allowing access to the BufferedDataManager is somewhat risky; we need it for Roles, but we might
    //      want to organize this differently.
    virtual ~WorkerProvider() {}

    virtual std::ostream* getLogFile() const = 0;

    virtual void scheduleForBred(Entity* entity) = 0;

    virtual const std::set<Entity*>& getEntities() const = 0;

    virtual ProfileBuilder* getProfileBuilder() const = 0;

    /**
     * Note: Calling this function from another worker is extremely dangerous if you don't know what you're doing
     */
    boost::mt19937& getGenerator()
    {
        return gen;
    }
};


/**
 * A "worker" performs a task asynchronously. Workers are managed by WorkGroups, which are
 *   themselves managed by the WorkGroupManager. You usually don't need to deal with
 *   Workers directly.
 *
 * Workers can be run in "singleThreaded" mode (by specifying this parameter to the WorkGroupManager).
 *   This will cause them to use no threads or barriers, and to simply be stepped through one-by-one by
 *   their parent WorkGroups.
 *
 * \author Seth N. Hetu
 * \author LIM Fung Chai
 * \author Xu Yan
 */
class Worker : public WorkerProvider {
private:
    friend class WorkGroup;
    static UpdatePublisher  updatePublisher;

    /**
     * Create a Worker object.
     *
     * \param tickStep How many ticks to advance per update(). It is beneficial to have one WorkGroup where
     *        this value is 1, since any WorkGroup with a greater value will have to wait 2 times (due to
     *        the way we synchronize data).
     * \param logFile Pointer to the file (or cout, cerr) that will receive all logging for this Worker's agents.
     *        Note that this stream should *not* require logging, so any shared ostreams should be on the same
     *        thread (usually that means on the same worker).
     */

    Worker(WorkGroup* parent, std::ostream* logFile, sim_mob::FlexiBarrier* frame_tick, sim_mob::FlexiBarrier* buff_flip, sim_mob::FlexiBarrier* aura_mgr,
            boost::barrier* macro_tick, std::vector<Entity*>* entityRemovalList, std::vector<Entity*>* entityBredList, uint32_t endTick, uint32_t tickStep, uint32_t simulationStart = 0);

    void start();
    void interrupt();  ///<Note: I am not sure how this will work with multiple granularities. ~Seth
    void join();       ///<Note: This will probably only work if called at the end of the main simulation loop.

    void scheduleForAddition(Entity* entity);
    int getAgentSize(bool includeToBeAdded=false);
    void migrateAllOut();

public:
    virtual ~Worker();
    static UpdatePublisher & GetUpdatePublisher();
    //Removing entities and scheduling them for removal is allowed (but adding is restricted).
    const std::set<Entity*>& getEntities() const;
    void remEntity(Entity* entity);
    void scheduleForRemoval(Entity* entity);
    void scheduleForBred(Entity* entity);

    void processMultiUpdateEntities(uint32_t currTick);

    virtual std::ostream* getLogFile() const;

//  /// return current worker's path set manager
//  virtual sim_mob::PathSetManager *getPathSetMgr();

    virtual ProfileBuilder* getProfileBuilder() const;

protected:
    ///Simple struct that holds all of the params used throughout threaded_function_loop().
    struct MgmtParams {
        MgmtParams();

        ///Helper: returns true if the "extra" (macro) tick is active.
        bool extraActive(uint32_t endTick) const;


        ///TODO: Using ConfigParams here is risky, since unit-tests may not have access to an actual config file.
        ///      We might want to remove this later, but most of our simulator relies on ConfigParams anyway, so
        ///      this will be a major cleanup effort anyway.

        uint32_t msPerFrame;  ///< How long is each frame tick in ms.
        sim_mob::ControlManager* ctrlMgr;  ///< The Control Manager (if any)
        uint32_t currTick;    ///< The current time tick.
        bool active;          ///< Is the simulation currently active?
    };

    //These functions encapsulate all of the non-initialization, non-barrier behavior of threaded_function_loop().
    //This allows us to call these functions individually if singleThreaded is set to true.
    //Some minor hacking is done via the MgmtParams struct to make this possible.
    void perform_frame_tick();
    void perform_buff_flip();
    //void perform_aura_mgr();  //Would do nothing (by chance).
    //void perform_macro();     //Would do nothing (by definition).

private:
    //The function that forms the basis of each Worker thread.
    void threaded_function_loop();

    //Helper functions for various update functionality.
    virtual void update_entities(timeslice currTime);

    void migrateOut(Entity& ent);
    void migrateIn(Entity& ent);

    //Entity management. Adding is restricted (use WorkGroups).
    void addEntity(Entity* entity);

    //Helper methods
    void addPendingEntities();
    void removePendingEntities();
    void breedPendingEntities();


protected:
    //Our various barriers.
    sim_mob::FlexiBarrier* frame_tick_barr;
    sim_mob::FlexiBarrier* buff_flip_barr;
    sim_mob::FlexiBarrier* aura_mgr_barr;
    boost::barrier* macro_tick_barr;

    //Time management
    uint32_t endTick;
    uint32_t tickStep;

    //Saved
    WorkGroup* const parent;

    //Assigned by the parent.
    std::vector<Entity*>* entityRemovalList;
    std::vector<Entity*>* entityBredList;

    //For migration. The first array is accessed by WorkGroup in the flip() phase, and should be
    //  emptied by this worker at the beginning of the update() phase.
    //  The second array is accessed by Agents (rather, the *action function) in update() and should
    //  be cleared by this worker some time before the next update. For now we clear it right after
    //  update(), but it might make sense to clear directly before update(), so that the WorkGroup
    //  has the ability to schedule Agents for deletion in flip().
    std::vector<Entity*> toBeAdded;
    std::vector<Entity*> toBeRemoved;
    std::vector<Entity*> toBeBred;


private:
    ///Logging
    std::ostream* logFile;

    ///The main thread which this Worker wraps
    boost::thread main_thread;

    ///All parameters used by main_thread. Maintained by the object so that we don't need to use boost::coroutines.
    MgmtParams loop_params;

    ///Simple Entities managed by this worker
    std::set<Entity*> managedEntities;

    ///Some Entities need to be updated multiple times in each time step.
    ///This typically happens when part of the update of an Entity depends on the partial update of other entities.
    ///Confluxes in mid-term are a good example of multi-update entities
    ///NOTE: The entities in this set also belong to managedEntities set.
    ///      In other words, managedMultiUpdateEntities is a subset of managedEntities containing only multi-update entities
    std::set<Entity*> managedMultiUpdateEntities;

    ///If non-null, used for profiling.
    sim_mob::ProfileBuilder* profile;
    //int thread_id;
    //static int auto_matical_thread_id;

    uint32_t simulationStartDay;

public:

    /// each worker has its own path set manager
    sim_mob::PathSetManager *pathSetMgr;
};

}


