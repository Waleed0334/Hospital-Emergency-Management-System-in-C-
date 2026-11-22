
#pragma once

#include <stack>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <queue>
#include <limits>
#include <algorithm>
#include <memory>

#include "hospital_types.h"

// HospitalEngine coordinates the original DSA structures.
// Patient records are owned by the doubly linked list.
// Other structures maintain indexes or references to those records.

class HospitalEngine
{
private:
    // ---------------- DOUBLY LINKED LIST ----------------

    // Max Heap for longest-waiting patients
    std::vector<Patient *> longestWaitingHeap;

    // ---------------- DOCTOR MANAGEMENT ----------------

    std::vector<Doctor> doctors;
    int nextDoctorID = 1;

    // ---------------- APPOINTMENT MANAGEMENT ----------------

    std::vector<Appointment> appointments;
    int nextAppointmentID = 1;

    // ---------------- UNDO STACK ----------------

    struct UndoAction
    {
        std::string type;
        Patient oldRecord;
        std::string previousStatus;
    };

    std::stack<UndoAction> undoStack;

    // ---------------- GRAPH DATA ----------------

    std::vector<std::string> departments;
    std::vector<std::vector<std::pair<int, int>>> departmentGraph;

    // Graph helper functions
    void initializeHospitalGraph();
    int departmentIndex(const std::string &name) const;

    // ---------------- ARRIVAL QUEUE ----------------

    std::vector<int> arrivalQueue;

    // ---------------- MIN HEAP ----------------

    // Stores pointers to waiting patients
    std::vector<Patient *> priorityHeap;

    // Patient ID -> current heap position
    std::unordered_map<int, int> priorityPositions;

    bool higherPriority(const Patient *a, const Patient *b) const;

    // ---------------- HEAP HELPERS ----------------

    void buildLongestWaitingHeap();
    void maxHeapifyDown(int index);

    void swapHeapNodes(int a, int b);

    void siftUp(int index);
    void siftDown(int index);

    // ---------------- PATIENT NODE ----------------

    struct PatientNode
    {
        Patient data;
        PatientNode *previous = nullptr;
        PatientNode *next = nullptr;

        explicit PatientNode(const Patient &patient)
            : data(patient) {}
    };

    // ---------------- PATIENT LINKED LIST ----------------

    class PatientList
    {
    private:
        PatientNode *head = nullptr;
        PatientNode *tail = nullptr;
        int count = 0;

    public:
        ~PatientList();

        PatientList(const PatientList &) = delete;
        PatientList &operator=(const PatientList &) = delete;

        PatientList() = default;

        bool add(const Patient &patient);
        bool remove(int id);

        Patient *find(int id);
        const Patient *find(int id) const;

        std::vector<Patient> toVector() const;

        int size() const;
        void clear();
    };

    // ---------------- AVL TREE ----------------

    class AVLTree
    {
    private:
        struct Node
        {
            int key;
            Patient *patient;
            int height;

            Node *left;
            Node *right;

            Node(int id, Patient *p);
        };

        Node *root = nullptr;

        int height(Node *node) const;
        int balance(Node *node) const;

        void updateHeight(Node *node);

        Node *rotateLeft(Node *node);
        Node *rotateRight(Node *node);

        Node *insertNode(Node *node, int id, Patient *patient);
        Node *deleteNode(Node *node, int id);

        Patient *searchNode(Node *node, int id) const;

        void destroy(Node *node);

    public:
        ~AVLTree();

        AVLTree() = default;

        AVLTree(const AVLTree &) = delete;
        AVLTree &operator=(const AVLTree &) = delete;

        void insert(int id, Patient *patient);
        void erase(int id);

        Patient *search(int id) const;

        void clear();
    };

    // ---------------- HASH TABLE ----------------

    class PatientHashTable
    {
    private:
        struct Entry
        {
            int key;
            Patient *value;
            Entry *next = nullptr;

            Entry(int id, Patient *patient);
        };

        static constexpr int TABLE_SIZE = 101;

        Entry *buckets[TABLE_SIZE]{};

        int hashFunction(int key) const;

    public:
        ~PatientHashTable();

        PatientHashTable() = default;

        PatientHashTable(const PatientHashTable &) = delete;
        PatientHashTable &operator=(const PatientHashTable &) = delete;

        void insert(int id, Patient *patient);

        Patient *search(int id) const;

        void erase(int id);
        void clear();
    };

    // ---------------- NAME TRIE ----------------

    class NameTrie
    {
    private:
        struct TrieNode
        {
            std::unordered_map<char, TrieNode *> children;
            std::vector<int> patientIDs;

            ~TrieNode();
        };

        TrieNode *root = nullptr;

        void collect(
            TrieNode *node,
            std::vector<int> &result) const;

    public:
        NameTrie();
        ~NameTrie();

        NameTrie(const NameTrie &) = delete;
        NameTrie &operator=(const NameTrie &) = delete;

        void insert(const std::string &name, int id);

        bool erase(const std::string &name, int id);

        std::vector<int> searchPrefix(
            const std::string &prefix) const;

        void clear();
    };

    // ---------------- ENGINE DATA ----------------

    // Actual objects, not pointers
    PatientList patients;
    AVLTree avl;
    PatientHashTable hash;
    NameTrie trie;

    long long nextArrivalOrder = 1;

public:
    // ---------------- CONSTRUCTOR / DESTRUCTOR ----------------

    HospitalEngine();
    ~HospitalEngine() = default;

    HospitalEngine(const HospitalEngine &) = delete;
    HospitalEngine &operator=(const HospitalEngine &) = delete;

    // ---------------- PATIENT OPERATIONS ----------------

    bool registerPatient(const Patient &patient);

    bool removePatient(int id);

    Patient *findPatientByID(int id);
    const Patient *findPatientByID(int id) const;

    Patient *findPatientAVL(int id);

    std::vector<Patient> getAllPatients() const;

    std::vector<Patient> searchPatientsByPrefix(
        const std::string &prefix) const;

    int patientCount() const;

    // Rebuild indexes after loading records from storage
    void rebuildIndexes(const std::vector<Patient> &records);

    // Remove all in-memory patient records and indexes
    void clear();

    // ---------------- PATIENT PRIORITY ----------------

    bool updatePatientPriority(int patientID, int newSeverity);

    void rebuildPriorityHeap();
    void clearPriorityHeap();

    void insertIntoPriorityHeap(Patient *patient);

    Patient *getHighestPriorityPatient();

    std::vector<Patient> getPatientsByPriority() const;

    // ---------------- ARRIVAL QUEUE ----------------

    void rebuildArrivalQueue();

    std::vector<int> getPatientsInArrivalOrder() const;

    // ---------------- TREATMENT / UNDO ----------------

    Patient *getLongestWaitingPatient();

    bool treatNextPatient();

    bool undoLastAction();

    // ---------------- DOCTOR OPERATIONS ----------------

    bool addDoctor(const Doctor &doctor);

    bool removeDoctor(int doctorID);

    Doctor *findDoctorByID(int doctorID);
    const Doctor *findDoctorByID(int doctorID) const;

    bool updateDoctorAvailability(int doctorID, bool available);

    std::vector<Doctor> getAllDoctors() const;

    int doctorCount() const;

    // ---------------- APPOINTMENT MANAGEMENT ----------------

    bool addAppointment(const Appointment &appointment);

    bool cancelAppointment(int appointmentID);

    Appointment *findAppointmentByID(int appointmentID);
    const Appointment *findAppointmentByID(int appointmentID) const;

    std::vector<Appointment> getAllAppointments() const;

    int appointmentCount() const;

    void setAppointments(const std::vector<Appointment> &records);

    // ---------------- BED MANAGEMENT ----------------

    bool assignBed(int patientID, int bedNumber);

    bool releaseBed(int patientID);

    bool isBedOccupied(int bedNumber) const;

    int getOccupiedBedCount() const;

    int getAvailableBedCount() const;

    std::vector<Patient> getPatientsWithBeds() const;

    // ---------------- RESOURCE ALLOCATION ----------------

    AllocationResult generateAllocationProposal();

    // ---------------- PRIORITY AGING ----------------

    int applyPriorityAging();

    // ---------------- HEAP SORT ----------------

    std::vector<Patient> getPatientsSortedByID() const;

    // ---------------- BENCHMARKS AND UNIT TESTS ----------------

    BenchmarkResults runBenchmarks() const;

    UnitTestResult runUnitTests() const;

    // ---------------- GRAPH ALGORITHMS ----------------

    std::vector<std::string> getDepartments();

    RouteResult getShortestRoute(
        const std::string &start,
        const std::string &destination);

    RouteResult getFewestMovesRoute(
        const std::string &start,
        const std::string &destination);

    TreatmentOrderResult getTreatmentOrder();
};
