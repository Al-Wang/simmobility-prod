//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "FlexiBarrier.hpp"


sim_mob::FlexiBarrier::FlexiBarrier(unsigned int count) : m_threshold(count), m_count(count), m_generation(0)
{
    if (count == 0) {
        throw std::runtime_error("FlexiBarrier constructor: count cannot be zero.");
    }
}

bool sim_mob::FlexiBarrier::wait(unsigned int amount)
{
    boost::mutex::scoped_lock lock(m_mutex);
    unsigned int gen = m_generation;
    
    //Can't wait more than the amount that would get us to zero.
    if (amount>m_count) {
       throw std::runtime_error("FlexiBarrier wait() overflow.");
    }
    
    m_count -= amount;
    if (m_count == 0) {
        m_generation++;
        m_count = m_threshold;
        m_cond.notify_all();
        return true;  //Indicates you are the leader.
    }

    while (gen == m_generation) {
        m_cond.wait(lock);
    }
    return false;    //Indicates you are not the leader.
}


bool sim_mob::FlexiBarrier::contribute(unsigned int amount)
{
    
    boost::mutex::scoped_lock lock(m_mutex);
    unsigned int gen = m_generation;

    //Can't wait more than the amount that would get us to zero.
    if (amount>m_count) {
        throw std::runtime_error("FlexiBarrier contribute() overflow.");
    }
    
    //NOTE: The documentation indicates that notify_all doesn't need the mutex, but there is no harm
    //      in it being locked. Might cause some slowdown; we should double-check this.
    m_count -= amount;
    if (m_count == 0) {
        m_generation++;
        m_count = m_threshold;
        m_cond.notify_all();
        return true;  //Indicates you are the leader.
    }

    //No need to wait; return immediately.
    return false;    //Indicates you are not the leader.
}



