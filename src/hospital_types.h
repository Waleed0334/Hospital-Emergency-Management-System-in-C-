
#pragma once

#include <string>
#include <vector>

// ============================================================
// PATIENT RECORD
// ============================================================

struct Patient
{
    int id = 0;
    std::string name;
    int age = 0;

    // 1 = highest priority, 5 = lowest priority
    int severity = 5;

    std::string status = "Waiting";

    // -1 means no bed assigned
    int bedNumber = -1;

    std::string arrivalTime;
    std::string lastUpdate;

    long long arrivalOrder = 0;
    long long agingIntervals = 0;
};

// ============================================================
// DOCTOR RECORD
// ============================================================

struct Doctor
{
    int id = 0;
    std::string name;
    std::string specialty;
    bool available = true;
};

// ============================================================
// APPOINTMENT RECORD
// ============================================================

struct Appointment
{
    int id = 0;
    int patientID = 0;
    int doctorID = 0;

    std::string dateTime;
    std::string status = "Booked";
};

// ============================================================
// DISCHARGE RECORD
// ============================================================

struct DischargeEntry
{
    Patient patient;
    std::string dischargeTime;
};

// ============================================================
// DASHBOARD STATISTICS
// ============================================================

struct HospitalStatistics
{
    int totalPatients = 0;
    int waitingPatients = 0;
    int treatedPatients = 0;
    int dischargedPatients = 0;

    int totalDoctors = 0;
    int availableDoctors = 0;

    int totalBeds = 100;
    int occupiedBeds = 0;
    int availableBeds = 100;
};

struct RouteResult
{
    bool found = false;
    std::vector<std::string> departments;
    int totalWeight = 0;
};

struct TreatmentOrderResult
{
    bool valid = false;
    std::vector<std::string> tasks;
};

struct ResourceAllocation
{
    int patientID = 0;
    int bedNumber = 0;
    int doctorNumber = 0;
};

struct AllocationResult
{
    bool success = false;
    std::vector<ResourceAllocation> assignments;
    std::string message;
};

struct BenchmarkResults
{
    double linkedListSearchMicroseconds = 0;
    double avlSearchMicroseconds = 0;
    double hashSearchMicroseconds = 0;
    double heapSortMicroseconds = 0;
    double standardSortMicroseconds = 0;
    int searchIterations = 20000;
};

struct UnitTestResult
{
    int passed = 0;
    int total = 0;
    std::vector<std::pair<std::string, bool>> tests;
};