
#include "hospital_engine.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <utility>

// ---------------- COMMON UTILITY ----------------

namespace
{
    std::string lowerText(std::string text)
    {
        for (char &ch : text)
        {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }

        return text;
    }

    // Original Heap Sort algorithm, sorting by patient ID.

    void heapSortByID(std::vector<Patient> &records)
    {
        auto siftDown = [&](int size, int root)
        {
            while (true)
            {
                int largest = root;
                int left = 2 * root + 1;
                int right = 2 * root + 2;

                if (left < size &&
                    records[left].id > records[largest].id)
                {
                    largest = left;
                }

                if (right < size &&
                    records[right].id > records[largest].id)
                {
                    largest = right;
                }

                if (largest == root)
                    break;

                std::swap(records[root], records[largest]);
                root = largest;
            }
        };

        for (int i = static_cast<int>(records.size()) / 2 - 1;
             i >= 0; --i)
        {
            siftDown(static_cast<int>(records.size()), i);
        }

        for (int end = static_cast<int>(records.size()) - 1;
             end > 0; --end)
        {
            std::swap(records[0], records[end]);
            siftDown(end, 0);
        }
    }
}

// ============================================================
// DOUBLY LINKED LIST
// ============================================================

HospitalEngine::PatientList::~PatientList()
{
    clear();
}

bool HospitalEngine::PatientList::add(const Patient &patient)
{
    if (find(patient.id) != nullptr)
        return false;

    PatientNode *newNode = new PatientNode(patient);

    if (head == nullptr)
    {
        head = tail = newNode;
    }
    else
    {
        tail->next = newNode;
        newNode->previous = tail;
        tail = newNode;
    }

    ++count;
    return true;
}

bool HospitalEngine::PatientList::remove(int id)
{
    PatientNode *current = head;

    while (current != nullptr && current->data.id != id)
        current = current->next;

    if (current == nullptr)
        return false;

    if (current->previous != nullptr)
        current->previous->next = current->next;
    else
        head = current->next;

    if (current->next != nullptr)
        current->next->previous = current->previous;
    else
        tail = current->previous;

    delete current;
    --count;

    return true;
}

Patient *HospitalEngine::PatientList::find(int id)
{
    PatientNode *current = head;

    while (current != nullptr)
    {
        if (current->data.id == id)
            return &current->data;

        current = current->next;
    }

    return nullptr;
}

const Patient *HospitalEngine::PatientList::find(int id) const
{
    PatientNode *current = head;

    while (current != nullptr)
    {
        if (current->data.id == id)
            return &current->data;

        current = current->next;
    }

    return nullptr;
}

std::vector<Patient> HospitalEngine::PatientList::toVector() const
{
    std::vector<Patient> result;

    PatientNode *current = head;

    while (current != nullptr)
    {
        result.push_back(current->data);
        current = current->next;
    }

    return result;
}

int HospitalEngine::PatientList::size() const
{
    return count;
}

void HospitalEngine::PatientList::clear()
{
    PatientNode *current = head;

    while (current != nullptr)
    {
        PatientNode *nextNode = current->next;
        delete current;
        current = nextNode;
    }

    head = nullptr;
    tail = nullptr;
    count = 0;
}

// ============================================================
// AVL TREE
// ============================================================

HospitalEngine::AVLTree::Node::Node(int id, Patient *p)
    : key(id), patient(p), height(1),
      left(nullptr), right(nullptr)
{
}

int HospitalEngine::AVLTree::height(Node *node) const
{
    return node == nullptr ? 0 : node->height;
}

int HospitalEngine::AVLTree::balance(Node *node) const
{
    return node == nullptr
               ? 0
               : height(node->left) - height(node->right);
}

void HospitalEngine::AVLTree::updateHeight(Node *node)
{
    node->height = 1 + std::max(
                           height(node->left),
                           height(node->right));
}

HospitalEngine::AVLTree::Node *
HospitalEngine::AVLTree::rotateRight(Node *top)
{
    Node *newTop = top->left;
    Node *middle = newTop->right;

    newTop->right = top;
    top->left = middle;

    updateHeight(top);
    updateHeight(newTop);

    return newTop;
}

HospitalEngine::AVLTree::Node *
HospitalEngine::AVLTree::rotateLeft(Node *top)
{
    Node *newTop = top->right;
    Node *middle = newTop->left;

    newTop->left = top;
    top->right = middle;

    updateHeight(top);
    updateHeight(newTop);

    return newTop;
}

HospitalEngine::AVLTree::Node *
HospitalEngine::AVLTree::insertNode(
    Node *node,
    int id,
    Patient *patient)
{
    if (node == nullptr)
        return new Node(id, patient);

    if (id < node->key)
        node->left = insertNode(node->left, id, patient);

    else if (id > node->key)
        node->right = insertNode(node->right, id, patient);

    else
    {
        node->patient = patient;
        return node;
    }

    updateHeight(node);

    int bf = balance(node);

    if (bf > 1 && id < node->left->key)
        return rotateRight(node);

    if (bf < -1 && id > node->right->key)
        return rotateLeft(node);

    if (bf > 1 && id > node->left->key)
    {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if (bf < -1 && id < node->right->key)
    {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

HospitalEngine::AVLTree::Node *
HospitalEngine::AVLTree::deleteNode(Node *node, int id)
{
    if (node == nullptr)
        return nullptr;

    if (id < node->key)
        node->left = deleteNode(node->left, id);

    else if (id > node->key)
        node->right = deleteNode(node->right, id);

    else
    {
        if (node->left == nullptr || node->right == nullptr)
        {
            Node *child = node->left != nullptr
                              ? node->left
                              : node->right;

            delete node;
            return child;
        }

        Node *successor = node->right;

        while (successor->left != nullptr)
            successor = successor->left;

        node->key = successor->key;
        node->patient = successor->patient;

        node->right = deleteNode(node->right, successor->key);
    }

    updateHeight(node);

    int bf = balance(node);

    if (bf > 1 && balance(node->left) >= 0)
        return rotateRight(node);

    if (bf > 1 && balance(node->left) < 0)
    {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if (bf < -1 && balance(node->right) <= 0)
        return rotateLeft(node);

    if (bf < -1 && balance(node->right) > 0)
    {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

Patient *HospitalEngine::AVLTree::searchNode(
    Node *node,
    int id) const
{
    if (node == nullptr)
        return nullptr;

    if (id == node->key)
        return node->patient;

    if (id < node->key)
        return searchNode(node->left, id);

    return searchNode(node->right, id);
}

void HospitalEngine::AVLTree::destroy(Node *node)
{
    if (node == nullptr)
        return;

    destroy(node->left);
    destroy(node->right);

    delete node;
}

HospitalEngine::AVLTree::~AVLTree()
{
    clear();
}

void HospitalEngine::AVLTree::insert(int id, Patient *patient)
{
    root = insertNode(root, id, patient);
}

void HospitalEngine::AVLTree::erase(int id)
{
    root = deleteNode(root, id);
}

Patient *HospitalEngine::AVLTree::search(int id) const
{
    return searchNode(root, id);
}

void HospitalEngine::AVLTree::clear()
{
    destroy(root);
    root = nullptr;
}

// ============================================================
// HASH TABLE
// ============================================================

HospitalEngine::PatientHashTable::Entry::Entry(
    int id,
    Patient *patient)
    : key(id), value(patient), next(nullptr)
{
}

int HospitalEngine::PatientHashTable::hashFunction(int key) const
{
    return (key % TABLE_SIZE + TABLE_SIZE) % TABLE_SIZE;
}

HospitalEngine::PatientHashTable::~PatientHashTable()
{
    clear();
}

void HospitalEngine::PatientHashTable::insert(
    int id,
    Patient *patient)
{
    int index = hashFunction(id);

    Entry *current = buckets[index];

    while (current != nullptr)
    {
        if (current->key == id)
        {
            current->value = patient;
            return;
        }

        current = current->next;
    }

    Entry *entry = new Entry(id, patient);

    entry->next = buckets[index];
    buckets[index] = entry;
}

Patient *HospitalEngine::PatientHashTable::search(int id) const
{
    int index = hashFunction(id);

    Entry *current = buckets[index];

    while (current != nullptr)
    {
        if (current->key == id)
            return current->value;

        current = current->next;
    }

    return nullptr;
}

void HospitalEngine::PatientHashTable::erase(int id)
{
    int index = hashFunction(id);

    Entry *current = buckets[index];
    Entry *previous = nullptr;

    while (current != nullptr)
    {
        if (current->key == id)
        {
            if (previous == nullptr)
                buckets[index] = current->next;
            else
                previous->next = current->next;

            delete current;
            return;
        }

        previous = current;
        current = current->next;
    }
}

void HospitalEngine::PatientHashTable::clear()
{
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        Entry *current = buckets[i];

        while (current != nullptr)
        {
            Entry *nextEntry = current->next;
            delete current;
            current = nextEntry;
        }

        buckets[i] = nullptr;
    }
}

// ============================================================
// NAME TRIE
// ============================================================

HospitalEngine::NameTrie::TrieNode::~TrieNode()
{
    for (auto &entry : children)
        delete entry.second;
}

HospitalEngine::NameTrie::NameTrie()
{
    root = new TrieNode();
}

HospitalEngine::NameTrie::~NameTrie()
{
    delete root;
}

void HospitalEngine::NameTrie::insert(
    const std::string &name,
    int id)
{
    TrieNode *current = root;

    for (char ch : lowerText(name))
    {
        if (current->children.find(ch) == current->children.end())
            current->children[ch] = new TrieNode();

        current = current->children[ch];
    }

    auto &ids = current->patientIDs;

    if (std::find(ids.begin(), ids.end(), id) == ids.end())
        ids.push_back(id);
}

bool HospitalEngine::NameTrie::erase(
    const std::string &name,
    int id)
{
    std::string key = lowerText(name);

    TrieNode *current = root;

    for (char ch : key)
    {
        auto it = current->children.find(ch);

        if (it == current->children.end())
            return false;

        current = it->second;
    }

    auto &ids = current->patientIDs;

    auto it = std::find(ids.begin(), ids.end(), id);

    if (it == ids.end())
        return false;

    ids.erase(it);

    return true;
}

void HospitalEngine::NameTrie::collect(
    TrieNode *node,
    std::vector<int> &result) const
{
    for (int id : node->patientIDs)
        result.push_back(id);

    for (auto &entry : node->children)
        collect(entry.second, result);
}

std::vector<int> HospitalEngine::NameTrie::searchPrefix(
    const std::string &prefix) const
{
    TrieNode *current = root;

    for (char ch : lowerText(prefix))
    {
        auto it = current->children.find(ch);

        if (it == current->children.end())
            return {};

        current = it->second;
    }

    std::vector<int> result;

    collect(current, result);

    return result;
}

void HospitalEngine::NameTrie::clear()
{
    delete root;
    root = new TrieNode();
}

// ============================================================
// HOSPITAL ENGINE
// ============================================================

HospitalEngine::HospitalEngine() = default;

// ============================================================
// DOCTOR MANAGEMENT
// ============================================================

// Register a new doctor.
bool HospitalEngine::addDoctor(const Doctor &doctor)
{
    Doctor newDoctor = doctor;

    // Validate doctor information.
    if (newDoctor.name.empty() || newDoctor.specialty.empty())
        return false;

    // Automatically generate an ID if one was not provided.
    if (newDoctor.id <= 0)
        newDoctor.id = nextDoctorID;

    // Prevent duplicate doctor IDs.
    if (findDoctorByID(newDoctor.id) != nullptr)
        return false;

    // Store the doctor.
    doctors.push_back(newDoctor);

    // Keep the next available ID updated.
    nextDoctorID = std::max(nextDoctorID, newDoctor.id + 1);

    return true;
}

// Remove a doctor using their ID.
bool HospitalEngine::removeDoctor(int doctorID)
{
    for (const Appointment &appointment : appointments)
    {
        if (appointment.doctorID == doctorID &&
            appointment.status == "Booked")
        {
            return false;
        }
    }

    auto it = std::find_if(
        doctors.begin(),
        doctors.end(),
        [doctorID](const Doctor &doctor)
        {
            return doctor.id == doctorID;
        });

    if (it == doctors.end())
        return false;

    doctors.erase(it);
    return true;
}

// Find a doctor using their ID.
Doctor *HospitalEngine::findDoctorByID(int doctorID)
{
    for (Doctor &doctor : doctors)
    {
        if (doctor.id == doctorID)
            return &doctor;
    }

    return nullptr;
}

// Const version of doctor search.
const Doctor *HospitalEngine::findDoctorByID(int doctorID) const
{
    for (const Doctor &doctor : doctors)
    {
        if (doctor.id == doctorID)
            return &doctor;
    }

    return nullptr;
}

// Update a doctor's availability.
bool HospitalEngine::updateDoctorAvailability(
    int doctorID,
    bool available)
{
    Doctor *doctor = findDoctorByID(doctorID);

    if (doctor == nullptr)
        return false;

    doctor->available = available;

    return true;
}

// Return all registered doctors.
std::vector<Doctor> HospitalEngine::getAllDoctors() const
{
    return doctors;
}

// Return the total number of registered doctors.
int HospitalEngine::doctorCount() const
{
    return static_cast<int>(doctors.size());
}

// ============================================================
// APPOINTMENT MANAGEMENT
// ============================================================

bool HospitalEngine::addAppointment(const Appointment &appointment)
{
    // Both records must exist.
    if (findPatientByID(appointment.patientID) == nullptr)
        return false;

    Doctor *doctor = findDoctorByID(appointment.doctorID);

    if (doctor == nullptr || !doctor->available)
        return false;

    if (appointment.dateTime.empty())
        return false;

    // Prevent duplicate appointment IDs.
    if (appointment.id > 0 &&
        findAppointmentByID(appointment.id) != nullptr)
    {
        return false;
    }

    // Prevent patient and doctor scheduling conflicts.
    for (const Appointment &existing : appointments)
    {
        if (existing.status != "Booked")
            continue;

        if (existing.dateTime == appointment.dateTime &&
            (existing.patientID == appointment.patientID ||
             existing.doctorID == appointment.doctorID))
        {
            return false;
        }
    }

    Appointment newAppointment = appointment;

    if (newAppointment.id <= 0)
        newAppointment.id = nextAppointmentID;

    newAppointment.status = "Booked";

    appointments.push_back(newAppointment);

    nextAppointmentID = std::max(
        nextAppointmentID,
        newAppointment.id + 1);

    return true;
}

bool HospitalEngine::cancelAppointment(int appointmentID)
{
    Appointment *appointment =
        findAppointmentByID(appointmentID);

    if (appointment == nullptr ||
        appointment->status != "Booked")
    {
        return false;
    }

    appointment->status = "Cancelled";
    return true;
}

Appointment *HospitalEngine::findAppointmentByID(int appointmentID)
{
    for (Appointment &appointment : appointments)
    {
        if (appointment.id == appointmentID)
            return &appointment;
    }

    return nullptr;
}

const Appointment *HospitalEngine::findAppointmentByID(
    int appointmentID) const
{
    for (const Appointment &appointment : appointments)
    {
        if (appointment.id == appointmentID)
            return &appointment;
    }

    return nullptr;
}

std::vector<Appointment> HospitalEngine::getAllAppointments() const
{
    return appointments;
}

int HospitalEngine::appointmentCount() const
{
    return static_cast<int>(appointments.size());
}

void HospitalEngine::setAppointments(
    const std::vector<Appointment> &records)
{
    appointments = records;
    nextAppointmentID = 1;

    for (const Appointment &appointment : appointments)
    {
        nextAppointmentID = std::max(
            nextAppointmentID,
            appointment.id + 1);
    }
}

// ============================================================
// BED MANAGEMENT
// ============================================================

// Check whether a bed is occupied.
bool HospitalEngine::isBedOccupied(int bedNumber) const
{
    if (bedNumber < 1 || bedNumber > 100)
        return false;

    for (const Patient &patient : getAllPatients())
    {
        if (patient.bedNumber == bedNumber)
            return true;
    }

    return false;
}

// Assign a bed to a patient.
bool HospitalEngine::assignBed(int patientID, int bedNumber)
{
    if (bedNumber < 1 || bedNumber > 100)
        return false;

    Patient *patient = findPatientByID(patientID);

    if (patient == nullptr)
        return false;

    // Prevent assigning another bed to the same patient.
    if (patient->bedNumber != -1)
        return false;

    // Prevent assigning an occupied bed.
    if (isBedOccupied(bedNumber))
        return false;

    patient->bedNumber = bedNumber;
    return true;
}

// Release a patient's assigned bed.
bool HospitalEngine::releaseBed(int patientID)
{
    Patient *patient = findPatientByID(patientID);

    if (patient == nullptr ||
        patient->bedNumber < 1 ||
        patient->bedNumber > 100)
    {
        return false;
    }

    patient->bedNumber = -1;
    return true;
}

// Return the number of occupied beds.
int HospitalEngine::getOccupiedBedCount() const
{
    int occupied = 0;

    for (const Patient &patient : getAllPatients())
    {
        if (patient.bedNumber >= 1 &&
            patient.bedNumber <= 100)
        {
            ++occupied;
        }
    }

    return occupied;
}

// Return the number of available beds.
int HospitalEngine::getAvailableBedCount() const
{
    return 100 - getOccupiedBedCount();
}

// Return all patients who currently have beds.
std::vector<Patient> HospitalEngine::getPatientsWithBeds() const
{
    std::vector<Patient> result;

    for (const Patient &patient : getAllPatients())
    {
        if (patient.bedNumber >= 1 &&
            patient.bedNumber <= 100)
        {
            result.push_back(patient);
        }
    }

    return result;
}

bool HospitalEngine::registerPatient(const Patient &patient)
{
    // Validate patient information.
    if (patient.id <= 0)
        return false;

    if (patient.name.empty())
        return false;

    if (patient.age < 0 || patient.age > 120)
        return false;

    if (patient.severity < 1 || patient.severity > 5)
        return false;

    // Prevent duplicate patient IDs.
    if (hash.search(patient.id) != nullptr)
        return false;

    // Prepare a copy of the patient.
    Patient newPatient = patient;

    if (newPatient.arrivalOrder <= 0)
        newPatient.arrivalOrder = nextArrivalOrder;

    // Add patient to the linked list.
    if (!patients.add(newPatient))
        return false;

    Patient *stored = patients.find(newPatient.id);

    if (stored == nullptr)
    {
        patients.remove(newPatient.id);
        return false;
    }

    // Update the next arrival sequence after successful insertion.
    nextArrivalOrder = std::max(
        nextArrivalOrder,
        stored->arrivalOrder + 1);

    // Add patient to the AVL Tree.
    avl.insert(stored->id, stored);

    // Add patient to the Hash Table.
    hash.insert(stored->id, stored);

    // Add patient to the Name Trie.
    trie.insert(stored->name, stored->id);

    // Add patient to the FIFO arrival queue.
    rebuildArrivalQueue();

    // Only waiting patients belong in the priority heap.
    if (stored->status == "Waiting")
    {
        insertIntoPriorityHeap(stored);
    }

    // Record registration for Undo.
    UndoAction action;
    action.type = "REGISTER";
    action.oldRecord = *stored;

    undoStack.push(action);

    return true;
}

bool HospitalEngine::removePatient(int id)
{
    // Find the patient in the Hash Table.
    Patient *patient = hash.search(id);

    if (patient == nullptr)
        return false;

    // Preserve the patient record before deleting it.
    Patient oldRecord = *patient;

    // Confirm that the patient exists in the linked list.
    if (patients.find(id) == nullptr)
        return false;

    // Remove the patient from the linked list.
    if (!patients.remove(id))
        return false;

    // Remove the patient from all indexes.
    trie.erase(oldRecord.name, id);
    avl.erase(id);
    hash.erase(id);

    // Rebuild the FIFO arrival queue.
    rebuildArrivalQueue();

    // Rebuild the priority heap.
    rebuildPriorityHeap();

    // Record successful discharge for Undo.
    UndoAction action;
    action.type = "DISCHARGE";
    action.oldRecord = oldRecord;

    undoStack.push(action);

    return true;
}

Patient *HospitalEngine::findPatientByID(int id)
{
    return hash.search(id);
}

const Patient *HospitalEngine::findPatientByID(int id) const
{
    return hash.search(id);
}

Patient *HospitalEngine::findPatientAVL(int id)
{
    return avl.search(id);
}

std::vector<Patient> HospitalEngine::getAllPatients() const
{
    return patients.toVector();
}

std::vector<Patient> HospitalEngine::searchPatientsByPrefix(
    const std::string &prefix) const
{
    std::vector<Patient> result;

    std::vector<int> ids = trie.searchPrefix(prefix);

    for (int id : ids)
    {
        const Patient *patient = hash.search(id);

        if (patient != nullptr)
            result.push_back(*patient);
    }

    return result;
}

std::vector<Patient> HospitalEngine::getPatientsSortedByID() const
{
    std::vector<Patient> result = patients.toVector();

    heapSortByID(result);

    return result;
}

int HospitalEngine::patientCount() const
{
    return patients.size();
}

void HospitalEngine::rebuildIndexes(const std::vector<Patient> &records)
{
    // Clear existing records and all associated structures.
    clear();

    for (const Patient &patient : records)
    {
        // Validate patient ID.
        if (patient.id <= 0)
            continue;

        // Validate patient name.
        if (patient.name.empty())
            continue;

        // Validate patient age.
        if (patient.age < 0 || patient.age > 120)
            continue;

        // Validate patient severity.
        if (patient.severity < 1 || patient.severity > 5)
            continue;

        // Insert patient into the linked list.
        // Duplicate IDs will be rejected.
        if (!patients.add(patient))
            continue;

        // Retrieve the actual stored patient.
        Patient *stored = patients.find(patient.id);

        if (stored == nullptr)
            continue;

        // Rebuild AVL Tree.
        avl.insert(stored->id, stored);

        // Rebuild Hash Table.
        hash.insert(stored->id, stored);

        // Rebuild Name Trie.
        trie.insert(stored->name, stored->id);

        // Assign a valid arrival order if missing.
        if (stored->arrivalOrder <= 0)
        {
            stored->arrivalOrder = nextArrivalOrder;
        }

        // Maintain the next arrival sequence.
        nextArrivalOrder = std::max(
            nextArrivalOrder,
            stored->arrivalOrder + 1);
    }

    // Reconstruct the FIFO arrival queue.
    rebuildArrivalQueue();

    // Reconstruct the priority min-heap.
    rebuildPriorityHeap();
}

void HospitalEngine::clear()
{
    // 1. Clear structures containing patient pointers.
    clearPriorityHeap();
    longestWaitingHeap.clear();

    // 2. Clear the arrival queue.
    arrivalQueue.clear();

    // 3. Clear previous undo operations.
    while (!undoStack.empty())
    {
        undoStack.pop();
    }

    // 4. Clear pointer-based indexes.
    avl.clear();
    hash.clear();
    trie.clear();

    // 5. Destroy patient records.
    patients.clear();

    // 6. Reset arrival sequence.
    nextArrivalOrder = 1;
}

void HospitalEngine::rebuildArrivalQueue()
{
    arrivalQueue.clear();

    // Retrieve all current patients.
    std::vector<Patient> allPatients = patients.toVector();

    // Sort patients according to their original arrival order.
    std::sort(
        allPatients.begin(),
        allPatients.end(),
        [](const Patient &a, const Patient &b)
        {
            if (a.arrivalOrder != b.arrivalOrder)
                return a.arrivalOrder < b.arrivalOrder;

            return a.id < b.id;
        });

    // Reconstruct the FIFO queue.
    for (const Patient &patient : allPatients)
    {
        arrivalQueue.push_back(patient.id);
    }
}

std::vector<int> HospitalEngine::getPatientsInArrivalOrder() const
{
    return arrivalQueue;
}

bool HospitalEngine::higherPriority(const Patient *a, const Patient *b) const
{
    // Lower severity means higher priority.
    if (a->severity != b->severity)
        return a->severity < b->severity;

    // If severity is equal, earlier arrival gets priority.
    return a->arrivalOrder < b->arrivalOrder;
}

void HospitalEngine::swapHeapNodes(int a, int b)
{
    std::swap(priorityHeap[a], priorityHeap[b]);

    // Update the index map after swapping.
    priorityPositions[priorityHeap[a]->id] = a;
    priorityPositions[priorityHeap[b]->id] = b;
}

void HospitalEngine::siftUp(int index)
{
    // Move the patient upward while they have higher priority.
    while (index > 0)
    {
        int parent = (index - 1) / 2;

        if (!higherPriority(priorityHeap[index], priorityHeap[parent]))
            break;

        swapHeapNodes(index, parent);
        index = parent;
    }
}

void HospitalEngine::siftDown(int index)
{
    int size = static_cast<int>(priorityHeap.size());

    while (true)
    {
        int best = index;

        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < size &&
            higherPriority(priorityHeap[left], priorityHeap[best]))
        {
            best = left;
        }

        if (right < size &&
            higherPriority(priorityHeap[right], priorityHeap[best]))
        {
            best = right;
        }

        if (best == index)
            break;

        swapHeapNodes(index, best);
        index = best;
    }
}

void HospitalEngine::insertIntoPriorityHeap(Patient *patient)
{
    if (patient == nullptr)
        return;

    // Only waiting patients belong in the priority heap.
    if (patient->status != "Waiting")
        return;

    // Prevent duplicate entries.
    if (priorityPositions.find(patient->id) != priorityPositions.end())
        return;

    // Insert at the end of the heap.
    priorityHeap.push_back(patient);

    int index = static_cast<int>(priorityHeap.size()) - 1;

    // Store the patient's current heap position.
    priorityPositions[patient->id] = index;

    // Restore min heap order.
    siftUp(index);
}

Patient *HospitalEngine::getHighestPriorityPatient()
{
    if (priorityHeap.empty())
        return nullptr;

    Patient *highest = priorityHeap.front();

    // Remove the patient's ID from the index map.
    priorityPositions.erase(highest->id);

    // If this was the only patient, the heap is now empty.
    if (priorityHeap.size() == 1)
    {
        priorityHeap.pop_back();
        return highest;
    }

    // Move the last patient to the root.
    priorityHeap[0] = priorityHeap.back();
    priorityHeap.pop_back();

    // Update the index of the new root.
    priorityPositions[priorityHeap[0]->id] = 0;

    // Restore heap order.
    siftDown(0);

    return highest;
}

// Update a patient's priority.
bool HospitalEngine::updatePatientPriority(int patientID, int newSeverity)
{
    if (newSeverity < 1 || newSeverity > 5)
        return false;

    Patient *patient = hash.search(patientID);

    if (patient == nullptr)
        return false;

    // Record the old priority before changing it
    UndoAction action;
    action.type = "PRIORITY";
    action.oldRecord = *patient;
    undoStack.push(action);

    patient->severity = newSeverity;

    rebuildPriorityHeap();

    return true;
}

// Rebuild the heap using only active waiting patients.
void HospitalEngine::rebuildPriorityHeap()
{
    // Clear the existing heap and its position map.
    clearPriorityHeap();

    // Retrieve all current patient records.
    std::vector<Patient> records = patients.toVector();

    for (const Patient &record : records)
    {
        // Find the actual patient stored in the linked list.
        Patient *actual = patients.find(record.id);

        if (actual == nullptr)
            continue;

        // Only waiting patients should be in the priority heap.
        if (actual->status == "Waiting")
        {
            insertIntoPriorityHeap(actual);
        }
    }
}

// Clear heap and index map.
void HospitalEngine::clearPriorityHeap()
{
    priorityHeap.clear();
    priorityPositions.clear();
}

// Return waiting patients in priority order without changing the heap.
std::vector<Patient> HospitalEngine::getPatientsByPriority() const
{
    std::vector<Patient> result;

    for (const Patient *patient : priorityHeap)
    {
        if (patient != nullptr)
            result.push_back(*patient);
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const Patient &a, const Patient &b)
        {
            if (a.severity != b.severity)
                return a.severity < b.severity;

            return a.arrivalOrder < b.arrivalOrder;
        });

    return result;
}

void HospitalEngine::maxHeapifyDown(int index)
{
    int size = static_cast<int>(longestWaitingHeap.size());

    while (true)
    {
        int oldest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < size &&
            longestWaitingHeap[left]->arrivalOrder <
                longestWaitingHeap[oldest]->arrivalOrder)
        {
            oldest = left;
        }

        if (right < size &&
            longestWaitingHeap[right]->arrivalOrder <
                longestWaitingHeap[oldest]->arrivalOrder)
        {
            oldest = right;
        }

        if (oldest == index)
            break;

        std::swap(longestWaitingHeap[index],
                  longestWaitingHeap[oldest]);

        index = oldest;
    }
}

void HospitalEngine::buildLongestWaitingHeap()
{
    longestWaitingHeap.clear();

    for (const Patient &record : patients.toVector())
    {
        Patient *patient = hash.search(record.id);

        if (patient != nullptr && patient->status == "Waiting")
            longestWaitingHeap.push_back(patient);
    }

    for (int i = static_cast<int>(longestWaitingHeap.size()) / 2 - 1;
         i >= 0; --i)
    {
        maxHeapifyDown(i);
    }
}

Patient *HospitalEngine::getLongestWaitingPatient()
{
    buildLongestWaitingHeap();

    if (longestWaitingHeap.empty())
        return nullptr;

    return longestWaitingHeap.front();
}

bool HospitalEngine::treatNextPatient()
{
    // Ensure the priority heap is synchronized.
    rebuildPriorityHeap();

    // Retrieve the highest-priority waiting patient.
    Patient *patient = getHighestPriorityPatient();

    if (patient == nullptr)
        return false;

    // Save the patient's previous state for Undo.
    UndoAction action;
    action.type = "TREAT";
    action.oldRecord = *patient;

    undoStack.push(action);

    // Update patient status.
    patient->status = "In Treatment";

    // Remove the treated patient from the waiting heap.
    rebuildPriorityHeap();

    return true;
}

bool HospitalEngine::undoLastAction()
{
    if (undoStack.empty())
        return false;

    // Read the latest action without removing it yet.
    const UndoAction &action = undoStack.top();

    if (action.type == "REGISTER")
    {
        int id = action.oldRecord.id;

        Patient *patient = hash.search(id);

        if (patient == nullptr)
            return false;

        // Remove the patient from all indexes.
        trie.erase(patient->name, id);
        avl.erase(id);
        hash.erase(id);

        // Remove the patient from the linked list.
        if (!patients.remove(id))
            return false;

        // Reconstruct the FIFO queue.
        rebuildArrivalQueue();
    }
    else if (action.type == "TREAT" || action.type == "PRIORITY")
    {
        Patient *patient = hash.search(action.oldRecord.id);

        if (patient == nullptr)
            return false;

        // Restore the patient's previous record.
        *patient = action.oldRecord;
    }
    else if (action.type == "DISCHARGE")
    {
        Patient restored = action.oldRecord;

        // Restore the patient to the linked list.
        if (!patients.add(restored))
            return false;

        Patient *patient = patients.find(restored.id);

        if (patient == nullptr)
        {
            patients.remove(restored.id);
            return false;
        }

        // Restore all indexes.
        avl.insert(patient->id, patient);
        hash.insert(patient->id, patient);
        trie.insert(patient->name, patient->id);

        // Rebuild FIFO queue using original arrival order.
        rebuildArrivalQueue();
    }
    else
    {
        // Unknown action: retain it in the undo stack.
        return false;
    }

    // Remove the action only after successful restoration.
    undoStack.pop();

    // Synchronize the priority heap.
    rebuildPriorityHeap();

    return true;
}

void HospitalEngine::initializeHospitalGraph()
{
    // Avoid initializing the graph more than once.
    if (!departments.empty())
        return;

    departments = {
        "Reception",
        "Emergency",
        "Laboratory",
        "Radiology",
        "Pharmacy",
        "Ward"};

    departmentGraph.resize(departments.size());

    auto addRoad = [this](const std::string &from,
                          const std::string &to,
                          int weight)
    {
        int a = departmentIndex(from);
        int b = departmentIndex(to);

        if (a == -1 || b == -1 || weight < 0)
            return;

        departmentGraph[a].push_back({b, weight});
        departmentGraph[b].push_back({a, weight});
    };

    addRoad("Reception", "Emergency", 2);
    addRoad("Reception", "Laboratory", 5);
    addRoad("Emergency", "Radiology", 3);
    addRoad("Emergency", "Ward", 6);
    addRoad("Laboratory", "Radiology", 2);
    addRoad("Radiology", "Ward", 4);
    addRoad("Ward", "Pharmacy", 3);
    addRoad("Laboratory", "Pharmacy", 7);
}

int HospitalEngine::departmentIndex(const std::string &name) const
{
    for (int i = 0; i < static_cast<int>(departments.size()); i++)
    {
        if (departments[i] == name)
            return i;
    }

    return -1;
}

std::vector<std::string> HospitalEngine::getDepartments()
{
    initializeHospitalGraph();
    return departments;
}

RouteResult HospitalEngine::getShortestRoute(
    const std::string &start,
    const std::string &destination)
{
    initializeHospitalGraph();

    RouteResult result;

    int source = departmentIndex(start);
    int target = departmentIndex(destination);

    if (source == -1 || target == -1)
        return result;

    const int INF = std::numeric_limits<int>::max();

    std::vector<int> distance(departments.size(), INF);
    std::vector<int> previous(departments.size(), -1);

    using Node = std::pair<int, int>;

    std::priority_queue<
        Node,
        std::vector<Node>,
        std::greater<Node>>
        pq;

    distance[source] = 0;
    pq.push({0, source});

    while (!pq.empty())
    {
        auto current = pq.top();
        pq.pop();

        int currentDistance = current.first;
        int node = current.second;

        if (currentDistance != distance[node])
            continue;

        for (const auto &edge : departmentGraph[node])
        {
            int next = edge.first;
            int weight = edge.second;

            if (distance[node] != INF &&
                distance[node] + weight < distance[next])
            {
                distance[next] = distance[node] + weight;
                previous[next] = node;

                pq.push({distance[next], next});
            }
        }
    }

    if (distance[target] == INF)
        return result;

    std::vector<int> path;

    for (int at = target; at != -1; at = previous[at])
        path.push_back(at);

    std::reverse(path.begin(), path.end());

    for (int index : path)
        result.departments.push_back(departments[index]);

    result.totalWeight = distance[target];
    result.found = true;

    return result;
}

RouteResult HospitalEngine::getFewestMovesRoute(
    const std::string &start,
    const std::string &destination)
{
    initializeHospitalGraph();

    RouteResult result;

    int source = departmentIndex(start);
    int target = departmentIndex(destination);

    if (source == -1 || target == -1)
        return result;

    std::vector<bool> visited(departments.size(), false);
    std::vector<int> previous(departments.size(), -1);

    std::queue<int> pending;

    visited[source] = true;
    pending.push(source);

    while (!pending.empty())
    {
        int node = pending.front();
        pending.pop();

        if (node == target)
            break;

        for (const auto &edge : departmentGraph[node])
        {
            int next = edge.first;

            if (!visited[next])
            {
                visited[next] = true;
                previous[next] = node;
                pending.push(next);
            }
        }
    }

    if (!visited[target])
        return result;

    std::vector<int> path;

    for (int at = target; at != -1; at = previous[at])
        path.push_back(at);

    std::reverse(path.begin(), path.end());

    for (int index : path)
        result.departments.push_back(departments[index]);

    // For BFS, totalWeight represents the number of moves.
    result.totalWeight = static_cast<int>(path.size()) - 1;
    result.found = true;

    return result;
}

TreatmentOrderResult HospitalEngine::getTreatmentOrder()
{
    TreatmentOrderResult result;

    std::vector<std::string> tasks = {
        "Initial Assessment",
        "Lab Tests",
        "Imaging",
        "Doctor Review",
        "Treatment Plan",
        "Pharmacy"};

    std::vector<std::vector<int>> edges(tasks.size());
    std::vector<int> indegree(tasks.size(), 0);

    auto addDependency = [&](int before, int after)
    {
        edges[before].push_back(after);
        indegree[after]++;
    };

    // Treatment dependencies from the original project.
    addDependency(0, 1); // Assessment -> Lab Tests
    addDependency(0, 2); // Assessment -> Imaging
    addDependency(1, 3); // Lab Tests -> Doctor Review
    addDependency(2, 3); // Imaging -> Doctor Review
    addDependency(3, 4); // Doctor Review -> Treatment Plan
    addDependency(4, 5); // Treatment Plan -> Pharmacy

    std::queue<int> ready;

    for (int i = 0; i < static_cast<int>(tasks.size()); i++)
    {
        if (indegree[i] == 0)
            ready.push(i);
    }

    while (!ready.empty())
    {
        int current = ready.front();
        ready.pop();

        result.tasks.push_back(tasks[current]);

        for (int next : edges[current])
        {
            indegree[next]--;

            if (indegree[next] == 0)
                ready.push(next);
        }
    }

    // If some tasks remain, a dependency cycle exists.
    result.valid = (result.tasks.size() == tasks.size());

    if (!result.valid)
        result.tasks.clear();

    return result;
}

namespace
{
    bool allocateRecursively(
        const std::vector<Patient> &selectedPatients,
        int index,
        int bedCount,
        int doctorCount,
        std::vector<bool> &bedsUsed,
        std::vector<bool> &doctorsUsed,
        std::vector<ResourceAllocation> &answer)
    {
        // All selected patients have been assigned.
        if (index == static_cast<int>(selectedPatients.size()))
            return true;

        // Try every available bed.
        for (int bed = 1; bed <= bedCount; bed++)
        {
            if (bedsUsed[bed])
                continue;

            // Try every available doctor.
            for (int doctor = 1; doctor <= doctorCount; doctor++)
            {
                if (doctorsUsed[doctor])
                    continue;

                // Make a tentative assignment.
                bedsUsed[bed] = true;
                doctorsUsed[doctor] = true;

                answer.push_back({selectedPatients[index].id,
                                  bed,
                                  doctor});

                // Recursively assign the next patient.
                if (allocateRecursively(
                        selectedPatients,
                        index + 1,
                        bedCount,
                        doctorCount,
                        bedsUsed,
                        doctorsUsed,
                        answer))
                {
                    return true;
                }

                // Backtrack if this combination fails.
                answer.pop_back();
                bedsUsed[bed] = false;
                doctorsUsed[doctor] = false;
            }
        }

        return false;
    }
}

AllocationResult HospitalEngine::generateAllocationProposal()
{
    AllocationResult result;

    // Get a copy of the current patient records.
    std::vector<Patient> allPatients = patients.toVector();

    // Select up to three waiting patients.
    std::vector<Patient> selectedPatients;

    for (const Patient &patient : allPatients)
    {
        if (patient.status == "Waiting")
        {
            selectedPatients.push_back(patient);

            if (selectedPatients.size() == 3)
                break;
        }
    }

    if (selectedPatients.empty())
    {
        result.message = "There are no waiting patients to allocate.";
        return result;
    }

    // Three demonstration beds and three demonstration doctor slots.
    const int bedCount = 3;
    const int doctorCount = 3;

    std::vector<bool> bedsUsed(bedCount + 1, false);
    std::vector<bool> doctorsUsed(doctorCount + 1, false);

    if (allocateRecursively(
            selectedPatients,
            0,
            bedCount,
            doctorCount,
            bedsUsed,
            doctorsUsed,
            result.assignments))
    {
        result.success = true;
        result.message =
            "A demonstration allocation proposal was generated successfully.";
    }
    else
    {
        result.success = false;
        result.assignments.clear();
        result.message =
            "No valid allocation was found with the available demonstration resources.";
    }

    return result;
}

int HospitalEngine::applyPriorityAging()
{
    std::time_t now = std::time(nullptr);
    int changed = 0;

    std::vector<Patient> records = patients.toVector();

    for (const Patient &copy : records)
    {
        Patient *patient = hash.search(copy.id);

        if (patient == nullptr || patient->status != "Waiting")
            continue;

        // Parse the original arrival timestamp.
        std::tm parsed = {};

        std::istringstream input(patient->arrivalTime);

        input >> std::get_time(
                     &parsed,
                     "%Y-%m-%d %H:%M:%S");

        if (input.fail())
            continue;

        std::time_t arrival = std::mktime(&parsed);

        if (arrival == -1 || arrival > now)
            continue;

        // One complete interval equals 30 minutes.
        long long completedIntervals =
            static_cast<long long>(
                std::difftime(now, arrival) / 1800.0);

        long long newlyCompleted =
            std::max(0LL, completedIntervals - patient->agingIntervals);

        int newSeverity = std::max(
            1,
            patient->severity - static_cast<int>(newlyCompleted));

        if (newSeverity < patient->severity)
        {
            // Preserve the previous record for Undo.
            UndoAction action;
            action.type = "PRIORITY";
            action.oldRecord = *patient;
            undoStack.push(action);

            patient->severity = newSeverity;
            ++changed;
        }

        // Remember the number of intervals already processed.
        patient->agingIntervals = std::max(
            patient->agingIntervals,
            completedIntervals);
    }

    rebuildPriorityHeap();

    return changed;
}

BenchmarkResults HospitalEngine::runBenchmarks() const
{
    BenchmarkResults result;

    // Retrieve patient records.
    std::vector<Patient> records = patients.toVector();

    if (records.empty())
        return result;

    // Use the last patient as the search target.
    int targetID = records.back().id;

    // Prevent search results from being ignored.
    long long searchChecksum = 0;

    // Generic search benchmarking function.
    auto measure = [&](auto searchFunction)
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < result.searchIterations; ++i)
        {
            const Patient *found = searchFunction(targetID);

            if (found != nullptr)
                searchChecksum += found->id;
        }

        auto finish = std::chrono::high_resolution_clock::now();

        return std::chrono::duration<double, std::micro>(
                   finish - start)
            .count();
    };

    // 1. Linked List Search.
    result.linkedListSearchMicroseconds = measure(
        [&](int id)
        {
            return patients.find(id);
        });

    // 2. AVL Tree Search.
    result.avlSearchMicroseconds = measure(
        [&](int id)
        {
            return avl.search(id);
        });

    // 3. Hash Table Search.
    result.hashSearchMicroseconds = measure(
        [&](int id)
        {
            return hash.search(id);
        });

    // 4. Compare Heap Sort and Standard Sort.
    if (records.size() >= 2)
    {
        // Prepare identical copies outside the timed sections.
        std::vector<Patient> heapRecords = records;
        std::vector<Patient> standardRecords = records;

        // Heap Sort.
        auto startHeap = std::chrono::high_resolution_clock::now();

        heapSortByID(heapRecords);

        auto finishHeap = std::chrono::high_resolution_clock::now();

        result.heapSortMicroseconds =
            std::chrono::duration<double, std::micro>(
                finishHeap - startHeap)
                .count();

        // Standard Library Sort.
        auto startStandard = std::chrono::high_resolution_clock::now();

        std::sort(
            standardRecords.begin(),
            standardRecords.end(),
            [](const Patient &a, const Patient &b)
            {
                return a.id < b.id;
            });

        auto finishStandard = std::chrono::high_resolution_clock::now();

        result.standardSortMicroseconds =
            std::chrono::duration<double, std::micro>(
                finishStandard - startStandard)
                .count();
    }

    // Keep the checksum observable.
    (void)searchChecksum;

    return result;
}

UnitTestResult HospitalEngine::runUnitTests() const
{
    UnitTestResult result;

    // Helper to record test results.
    auto check = [&](bool condition, const std::string &label)
    {
        result.total++;

        if (condition)
            result.passed++;

        result.tests.push_back({label, condition});
    };

    // --------------------------------------------------
    // TEST 1: Actual Heap Sort implementation
    // --------------------------------------------------

    std::vector<Patient> sample(3);

    sample[0].id = 30;
    sample[1].id = 10;
    sample[2].id = 20;

    heapSortByID(sample);

    bool heapSortCorrect =
        sample[0].id == 10 &&
        sample[1].id == 20 &&
        sample[2].id == 30;

    check(heapSortCorrect, "Heap Sort ordering");

    // --------------------------------------------------
    // TEST 2: Dijkstra shortest route between departments
    // --------------------------------------------------

    auto departments =
        const_cast<HospitalEngine *>(this)->getDepartments();

    bool dijkstraTest = false;

    for (size_t i = 0; i < departments.size(); i++)
    {
        for (size_t j = 0; j < departments.size(); j++)
        {
            if (i == j)
                continue;

            RouteResult route =
                const_cast<HospitalEngine *>(this)->getShortestRoute(
                    departments[i],
                    departments[j]);

            if (route.found &&
                route.departments.size() >= 2 &&
                route.departments.front() == departments[i] &&
                route.departments.back() == departments[j])
            {
                dijkstraTest = true;
                break;
            }
        }

        if (dijkstraTest)
            break;
    }

    check(dijkstraTest, "Dijkstra inter-department route");

    // --------------------------------------------------
    // TEST 3: BFS route between departments
    // --------------------------------------------------

    bool bfsTest = false;

    for (size_t i = 0; i < departments.size(); i++)
    {
        for (size_t j = 0; j < departments.size(); j++)
        {
            if (i == j)
                continue;

            RouteResult route =
                const_cast<HospitalEngine *>(this)->getFewestMovesRoute(
                    departments[i],
                    departments[j]);

            if (route.found &&
                route.departments.size() >= 2 &&
                route.departments.front() == departments[i] &&
                route.departments.back() == departments[j])
            {
                bfsTest = true;
                break;
            }
        }

        if (bfsTest)
            break;
    }

    check(bfsTest, "BFS inter-department route");

    // --------------------------------------------------
    // TEST 4: Topological treatment order
    // --------------------------------------------------

    TreatmentOrderResult order =
        const_cast<HospitalEngine *>(this)->getTreatmentOrder();

    bool orderCorrect =
        order.valid && order.tasks.size() == 6;

    // Verify that no task appears more than once.
    if (orderCorrect)
    {
        for (size_t i = 0; i < order.tasks.size(); i++)
        {
            for (size_t j = i + 1; j < order.tasks.size(); j++)
            {
                if (order.tasks[i] == order.tasks[j])
                {
                    orderCorrect = false;
                    break;
                }
            }

            if (!orderCorrect)
                break;
        }
    }

    check(orderCorrect, "Topological treatment order");

    // --------------------------------------------------
    // TEST 5: Patient AVL and Hash Table consistency
    // --------------------------------------------------

    std::vector<Patient> currentPatients = patients.toVector();

    bool indexesCorrect = true;

    for (const Patient &patient : currentPatients)
    {
        const Patient *hashRecord = hash.search(patient.id);
        const Patient *avlRecord = avl.search(patient.id);

        if (hashRecord == nullptr ||
            avlRecord == nullptr ||
            hashRecord->id != patient.id ||
            avlRecord->id != patient.id)
        {
            indexesCorrect = false;
            break;
        }
    }

    check(indexesCorrect, "Patient Hash and AVL index consistency");

    // --------------------------------------------------
    // TEST 6: Patient ID sorting correctness
    // --------------------------------------------------

    bool sortedCorrect = true;

    std::vector<Patient> sortedPatients = patients.toVector();

    heapSortByID(sortedPatients);

    for (size_t i = 1; i < sortedPatients.size(); i++)
    {
        if (sortedPatients[i - 1].id > sortedPatients[i].id)
        {
            sortedCorrect = false;
            break;
        }
    }

    check(sortedCorrect, "Patient records sorted by ID");

    return result;
}
