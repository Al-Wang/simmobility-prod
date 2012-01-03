/**
 * \file GenConfig.h
 * Configuration parameters and constants. 
 * 
 * Please do not edit the file GenConfig.h; instead, edit GenConfig.h.in, which the header
 *  file is generated from.
 *  
 * Also note that parameters like SIMMOB_DISABLE_MPI should be set in your cmake cache file.
 *  Do not simply override the defaults in CMakeLists.txt
 *
 * \par
 * ~Seth
 */

#pragma once

//Cmake-related constants
#define SIMMOB_VERSION_MAJOR "0"
#define SIMMOB_VERSION_MINOR "1"

//Flags imported from cmake
/* #undef SIMMOB_DISABLE_MPI */
/* #undef SIMMOB_DISABLE_DYNAMIC_DISPATCH */
/* #undef SIMMOB_LATEST_STANDARD */


///Sizes of workgroups. (Note that enums are allowed to overlap values)
///\todo
///See if we can define these via cmake, or in the config file. Hardcoding them
///  doesn't make a lot of sense.
enum WORKGROUP_SIZES {
	WG_TRIPCHAINS_SIZE = 4,       ///<Number of trip chain workers in group.
	WG_CREATE_AGENT_SIZE = 3,     ///<Number of agent creation workers in group.
	WG_CHOICESET_SIZE = 6,        ///<Number of choice set workers in group.
	WG_AGENTS_SIZE = 5,           ///<Number of agent workers in group.
	WG_SIGNALS_SIZE = 2,          ///<Number of signal workers in group.
	WG_SHORTEST_PATH_SIZE = 10,   ///<Number of shortest path workers in group.
};

