//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "conf/settings/DisableMPI.h"

#include "PackageUtils.hpp"

#ifndef SIMMOB_DISABLE_MPI

#include "util/GeomHelpers.hpp"

using namespace sim_mob;

std::string sim_mob::PackageUtils::getPackageData() {
    return buffer.str().data();
}

sim_mob::PackageUtils::PackageUtils()
{
    package = new boost::archive::text_oarchive(buffer);
}

sim_mob::PackageUtils::~PackageUtils()
{
    buffer.clear();
    safe_delete_item(package);
}

void sim_mob::PackageUtils::operator<<(double value) {
    if (value != value) {
        throw std::runtime_error("Double value is NAN.");
    }
    (*package) & value;
}


#endif
