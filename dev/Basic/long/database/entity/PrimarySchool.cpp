/*
 * PrimarySchool.cpp
 *
 *  Created on: 10 Mar 2016
 *      Author: gishara
 */

#include "PrimarySchool.hpp"

using namespace sim_mob::long_term;

PrimarySchool::PrimarySchool(BigSerial schoolId, BigSerial postcode , double centroidX, double centroidY, std::string schoolName, int giftedProgram, int sapProgram, std::string dgp, BigSerial tazId,int numStudents, int numStudentsCanBeAssigned, double reAllocationProb)
                            :schoolId(schoolId),postcode(postcode), centroidX(centroidX),centroidY(centroidY), schoolName(schoolName), giftedProgram(giftedProgram), sapProgram(sapProgram),dgp(dgp), tazId(tazId),
                             numStudents(numStudents),numStudentsCanBeAssigned(numStudentsCanBeAssigned), reAllocationProb(reAllocationProb){}

PrimarySchool::~PrimarySchool(){}

PrimarySchool::PrimarySchool(const PrimarySchool& source)
{
    this->schoolId = source.schoolId;
    this->postcode = source.postcode;
    this->centroidX = source.centroidX;
    this->centroidY = source.centroidY;
    this->schoolName = source.schoolName;
    this->giftedProgram = source.giftedProgram;
    this->sapProgram = source.sapProgram;
    this->dgp = source.dgp;
    this->tazId = source.tazId;
    this->numStudents = source.numStudents;
    this->numStudentsCanBeAssigned = source.numStudentsCanBeAssigned;
    this->reAllocationProb = source.reAllocationProb;

    for (int i=0; i< source.students.size(); i++)
    {
        this->students[i] = source.students[i];
    }

    for (int i=0; i< source.selectedStudents.size(); i++)
    {
        this->selectedStudents[i] = source.selectedStudents[i];
    }

    for (int i=0; i< source.distanceIndList.size(); i++)
    {
        this->distanceIndList[i] = source.distanceIndList[i];
    }

}

PrimarySchool& PrimarySchool::operator=(const PrimarySchool& source)
{
    this->schoolId = source.schoolId;
    this->postcode = source.postcode;
    this->centroidX = source.centroidX;
    this->centroidY = source.centroidY;
    this->schoolName = source.schoolName;
    this->giftedProgram = source.giftedProgram;
    this->sapProgram = source.sapProgram;
    this->dgp = source.dgp;
    this->tazId = source.tazId;
    this->numStudents = source.numStudents;
    this->numStudentsCanBeAssigned = source.numStudentsCanBeAssigned;
    this->reAllocationProb = source.reAllocationProb;

    for (int i=0; i< source.students.size(); i++)
    {
        this->students[i] = source.students[i];
    }

    for (int i=0; i< source.selectedStudents.size(); i++)
    {
        this->selectedStudents[i] = source.selectedStudents[i];
    }

    for (int i=0; i< source.distanceIndList.size(); i++)
    {
        this->distanceIndList[i] = source.distanceIndList[i];
    }

    return *this;
}

double PrimarySchool::getCentroidX() const
{
    return centroidX;           std::vector<Individual*> students;

}

void PrimarySchool::setCentroidX(double centroidX)
{
    this->centroidX = centroidX;
}

double PrimarySchool::getCentroidY() const
{
    return centroidY;
}

void PrimarySchool::setCentroidY(double centroidY)
{
    this->centroidY = centroidY;
}

const std::string& PrimarySchool::getDgp() const
{
    return dgp;
}

void PrimarySchool::setDgp(const std::string& dgp)
{
    this->dgp = dgp;
}

int PrimarySchool::isGiftedProgram() const
{
    return giftedProgram;
}

void PrimarySchool::setGiftedProgram(int giftedProgram)
{
    this->giftedProgram = giftedProgram;
}

BigSerial PrimarySchool::getPostcode() const
{
    return postcode;
}

void PrimarySchool::setPostcode(BigSerial postcode)
{
    this->postcode = postcode;
}

int PrimarySchool::isSapProgram() const
{
    return sapProgram;
}

void PrimarySchool::setSapProgram(int sapProgram)
{
    this->sapProgram = sapProgram;
}

BigSerial PrimarySchool::getSchoolId() const
{
    return schoolId;
}

void PrimarySchool::setSchoolId(BigSerial schoolId)
{
    this->schoolId = schoolId;
}

const std::string& PrimarySchool::getSchoolName() const
{
    return schoolName;
}

void PrimarySchool::setSchoolName(const std::string& schoolName)
{
    this->schoolName = schoolName;
}

BigSerial PrimarySchool::getTazId() const
{
    return tazId;
}

void PrimarySchool::setTazId(BigSerial tazId)
{
    this->tazId = tazId;
}

int PrimarySchool::getNumStudents() const
{
    return this->numStudents;
}

void PrimarySchool::addStudent(BigSerial studentId)
{
    this->students.push_back(studentId);
    numStudents++;
}

void PrimarySchool::addIndividualDistance(DistanceIndividual &distanceIndividual)
{
    distanceIndList.push_back(distanceIndividual);
}

std::vector<PrimarySchool::DistanceIndividual>  PrimarySchool::getSortedDistanceIndList()
{
    std::sort(distanceIndList.begin(), distanceIndList.end(), PrimarySchool::OrderByDistance());
    return distanceIndList;
}

std::vector<PrimarySchool> getSortedProbSchoolList( std::vector<PrimarySchool> studentsWithProb)
{
    std::sort(studentsWithProb.begin(), studentsWithProb.end(), PrimarySchool::OrderByProbability());
    return studentsWithProb;
}

std::vector<BigSerial> PrimarySchool::getStudents()
{
//  std::vector<BigSerial> studentIdList;
//  std::vector<Individual*>::iterator schoolsItr;
//  for(schoolsItr = students.begin(); schoolsItr != students.end(); schoolsItr++)
//  {
//
//      studentIdList.push_back((*schoolsItr)->getId());
//  }
    return this->students;
}

int PrimarySchool::getNumSelectedStudents()
{
    return this->selectedStudents.size();
}

void PrimarySchool::setSelectedStudentList(std::vector<BigSerial>&selectedStudentsList)
{
    for(BigSerial studentId: selectedStudentsList)
    {
        this->selectedStudents.push_back(studentId);
    }

}

void PrimarySchool::setNumStudentsCanBeAssigned(int numStudents)
{
    this->numStudentsCanBeAssigned = numStudents;
}

int PrimarySchool::getNumStudentsCanBeAssigned()
{
    return this->numStudentsCanBeAssigned;
}

void PrimarySchool::setReAllocationProb(double probability)
{
    this->reAllocationProb = probability;
}

double PrimarySchool::getReAllocationProb()
{
    return this->reAllocationProb;
}

void PrimarySchool::addSelectedStudent(BigSerial &individualId)
{
    this->selectedStudents.push_back(individualId);
}

int  PrimarySchool::getNumOfSelectedStudents()
{
    return this->selectedStudents.size();
}
