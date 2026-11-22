/*
    HOSPITAL MANAGEMENT SYSTEM - DSA PROJECT
    Language: C++17

    IMPORTANT:
    This is an educational/demo system. It is not a clinical decision system.
    Severity values are illustrative: 1 = highest priority, 5 = lowest priority.

    DATA STRUCTURES / ALGORITHMS INCLUDED
    1. Doubly linked list       - stores active patients
    2. AVL tree                 - patient ID search/insert/delete in O(log n)
    3. Hash table               - average O(1) patient ID lookup
    4. Stack                    - undo last register/treat/discharge action
    5. FIFO queue                - records patient arrival order
    6. Min heap                  - selects the highest-priority waiting patient
    7. Heap index map            - supports priority update by patient ID
    8. Max heap                  - displays the longest-waiting patient
    9. Heap sort                 - sorts a copy of patients by ID
    10. Trie                     - patient-name prefix search
    11. Weighted graph + Dijkstra- shortest route between hospital departments
    12. BFS                      - route with the fewest department-to-department moves
    13. Topological sort         - treatment-task dependency ordering
    14. Backtracking             - assigns beds/doctors under simple constraints
    15. File handling            - saves and loads patient records
    16. Doctor directory and appointment scheduling
    17. Bed availability management
    18. Priority aging, benchmarks, and unit tests
    19. Persistent discharge / doctor / appointment records

    Compile:
        g++ -std=c++17 -Wall -Wextra -pedantic hospital_management.cpp -o hospital

    Run:
        ./hospital
*/

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <limits>
#include <cctype>
#include <chrono>
#include <set>

using namespace std;

// ------------------------------ COMMON UTILITIES ------------------------------

string currentTime()
{
    // Get the current system time.
    time_t now = time(nullptr);

    // Convert it to local calendar time.
    tm *local = localtime(&now);

    // Format the time in a readable form.
    stringstream output;
    output << put_time(local, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

string lowerText(string text)
{
    // Convert every character to lowercase for case-insensitive name searching.
    for (char &ch : text)
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    return text;
}

int readInt(const string &message, int minimum, int maximum)
{
    // Keep asking until the user enters a valid integer in the requested range.
    while (true)
    {
        cout << message;
        int value;

        if (cin >> value && value >= minimum && value <= maximum)
        {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }

        // Clear the failed input state so the next input can be read.
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Enter a value from " << minimum
             << " to " << maximum << ".\n";
    }
}

string readLine(const string &message)
{
    // Read a complete line so names can contain spaces.
    cout << message;
    string value;
    getline(cin, value);
    return value;
}

// ------------------------------ PATIENT RECORD ------------------------------

struct Patient
{
    int id = 0;
    string name;
    int age = 0;
    int severity = 5;          // 1 is the most urgent; 5 is the least urgent.
    string status = "Waiting";
    int bedNumber = -1;         // -1 means no bed has been assigned.
    string arrivalTime;
    string lastUpdate;
    long long arrivalOrder = 0; // Used to break equal-priority ties fairly.
    long long agingIntervals = 0; // Completed 30-minute intervals already applied to priority.
};

// ------------------------------ DOUBLY LINKED LIST ------------------------------

class PatientList
{
private:
    struct Node
    {
        Patient data;
        Node *previous;
        Node *next;

        // Store the patient and initialize both links as empty.
        Node(const Patient &patient) : data(patient), previous(nullptr), next(nullptr) {}
    };

    Node *head;
    Node *tail;
    int count;

public:
    PatientList() : head(nullptr), tail(nullptr), count(0) {}

    ~PatientList()
    {
        // Release every node when the list object is destroyed.
        clear();
    }

    void clear()
    {
        // Walk through the list and delete each dynamically allocated node.
        Node *current = head;
        while (current != nullptr)
        {
            Node *nextNode = current->next;
            delete current;
            current = nextNode;
        }

        // Reset both ends because the list is now empty.
        head = tail = nullptr;
        count = 0;
    }

    bool add(const Patient &patient)
    {
        // Prevent duplicate IDs in the primary patient list.
        if (find(patient.id) != nullptr)
            return false;

        // Allocate a new node containing a copy of the patient.
        Node *newNode = new Node(patient);

        // If the list is empty, the new node becomes both head and tail.
        if (head == nullptr)
            head = tail = newNode;
        else
        {
            // Connect the old tail to the new node in both directions.
            tail->next = newNode;
            newNode->previous = tail;

            // Move the tail pointer to the newly inserted node.
            tail = newNode;
        }

        count++;
        return true;
    }

    bool remove(int id, Patient &removedPatient)
    {
        // Search for the node that contains the requested patient ID.
        Node *node = head;
        while (node != nullptr && node->data.id != id)
            node = node->next;

        // Return false when the patient does not exist.
        if (node == nullptr)
            return false;

        // Copy the record before deleting its node.
        removedPatient = node->data;

        // If there is a previous node, bypass the node being removed.
        if (node->previous != nullptr)
            node->previous->next = node->next;
        else
            head = node->next; // The removed node was the head.

        // If there is a next node, connect it to the previous node.
        if (node->next != nullptr)
            node->next->previous = node->previous;
        else
            tail = node->previous; // The removed node was the tail.

        // Free the removed node and update the patient count.
        delete node;
        count--;
        return true;
    }

    Patient *find(int id)
    {
        // Traverse the list until the matching ID is found.
        Node *current = head;
        while (current != nullptr)
        {
            if (current->data.id == id)
                return &current->data;
            current = current->next;
        }
        return nullptr;
    }

    vector<Patient> toVector() const
    {
        // Create a temporary copy for algorithms that need indexed access.
        vector<Patient> result;
        Node *current = head;

        while (current != nullptr)
        {
            result.push_back(current->data);
            current = current->next;
        }
        return result;
    }

    void printAll() const
    {
        // Display all active patients from first registered to last registered.
        if (head == nullptr)
        {
            cout << "No active patients.\n";
            return;
        }

        cout << "\nID\tName\t\tAge\tPriority\tStatus\t\tBed\n";
        cout << "---------------------------------------------------------------\n";

        Node *current = head;
        while (current != nullptr)
        {
            const Patient &p = current->data;
            cout << p.id << "\t" << left << setw(16) << p.name
                 << right << p.age << "\t" << p.severity << "\t\t"
                 << p.status << "\t\t";

            if (p.bedNumber == -1)
                cout << "Not assigned";
            else
                cout << p.bedNumber;

            cout << '\n';
            current = current->next;
        }
    }

    int size() const { return count; }
};

// ------------------------------ AVL TREE ------------------------------

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

        // Each AVL node stores the patient ID and a pointer to the main record.
        Node(int id, Patient *p)
            : key(id), patient(p), height(1), left(nullptr), right(nullptr) {}
    };

    Node *root = nullptr;

    int height(Node *node) const
    {
        // An empty subtree has height zero.
        return node == nullptr ? 0 : node->height;
    }

    int balance(Node *node) const
    {
        // Balance factor is left-subtree height minus right-subtree height.
        return node == nullptr ? 0 : height(node->left) - height(node->right);
    }

    void updateHeight(Node *node)
    {
        // The height is one plus the larger child-subtree height.
        node->height = 1 + max(height(node->left), height(node->right));
    }

    Node *rotateRight(Node *top)
    {
        // Perform a right rotation to repair a left-heavy subtree.
        Node *newTop = top->left;
        Node *middle = newTop->right;

        newTop->right = top;
        top->left = middle;

        // Update the lower node first, then the new subtree root.
        updateHeight(top);
        updateHeight(newTop);
        return newTop;
    }

    Node *rotateLeft(Node *top)
    {
        // Perform a left rotation to repair a right-heavy subtree.
        Node *newTop = top->right;
        Node *middle = newTop->left;

        newTop->left = top;
        top->right = middle;

        // Recalculate heights after changing the links.
        updateHeight(top);
        updateHeight(newTop);
        return newTop;
    }

    Node *insertNode(Node *node, int key, Patient *patient)
    {
        // Insert like a normal binary search tree first.
        if (node == nullptr)
            return new Node(key, patient);

        if (key < node->key)
            node->left = insertNode(node->left, key, patient);
        else if (key > node->key)
            node->right = insertNode(node->right, key, patient);
        else
        {
            // An existing key is updated rather than duplicated.
            node->patient = patient;
            return node;
        }

        // Update height and inspect the balance factor on the way back up.
        updateHeight(node);
        int bf = balance(node);

        // Left-Left case: rotate right.
        if (bf > 1 && key < node->left->key)
            return rotateRight(node);

        // Right-Right case: rotate left.
        if (bf < -1 && key > node->right->key)
            return rotateLeft(node);

        // Left-Right case: rotate left child, then rotate right.
        if (bf > 1 && key > node->left->key)
        {
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }

        // Right-Left case: rotate right child, then rotate left.
        if (bf < -1 && key < node->right->key)
        {
            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        return node;
    }

    Node *minimumNode(Node *node) const
    {
        // The smallest key is the leftmost node in a BST subtree.
        while (node != nullptr && node->left != nullptr)
            node = node->left;
        return node;
    }

    Node *deleteNode(Node *node, int key)
    {
        // Search for the node to delete using the BST ordering rule.
        if (node == nullptr)
            return nullptr;

        if (key < node->key)
            node->left = deleteNode(node->left, key);
        else if (key > node->key)
            node->right = deleteNode(node->right, key);
        else
        {
            // Case 1 and 2: node has zero or one child.
            if (node->left == nullptr || node->right == nullptr)
            {
                Node *child = node->left != nullptr ? node->left : node->right;
                delete node;
                return child;
            }

            // Case 3: replace with the in-order successor (smallest right key).
            Node *successor = minimumNode(node->right);
            node->key = successor->key;
            node->patient = successor->patient;
            node->right = deleteNode(node->right, successor->key);
        }

        // Rebalance ancestors after deletion.
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

    Patient *searchNode(Node *node, int key) const
    {
        // Follow the smaller or larger child according to the key.
        if (node == nullptr)
            return nullptr;
        if (key == node->key)
            return node->patient;
        if (key < node->key)
            return searchNode(node->left, key);
        return searchNode(node->right, key);
    }

    void inOrder(Node *node) const
    {
        // In-order traversal prints patient IDs in ascending order.
        if (node == nullptr)
            return;
        inOrder(node->left);
        if (node->patient != nullptr)
            cout << node->key << " - " << node->patient->name << '\n';
        inOrder(node->right);
    }

    void destroy(Node *node)
    {
        // Post-order deletion safely releases every AVL node.
        if (node == nullptr)
            return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    ~AVLTree() { destroy(root); }

    void insert(int id, Patient *patient) { root = insertNode(root, id, patient); }
    void erase(int id) { root = deleteNode(root, id); }
    Patient *search(int id) const { return searchNode(root, id); }

    void displaySorted() const
    {
        cout << "\nPatients in ascending ID order:\n";
        inOrder(root);
    }
};

// ------------------------------ HASH TABLE ------------------------------

class PatientHashTable
{
private:
    // Each bucket uses a linked list (separate chaining) to handle collisions.
    struct Entry
    {
        int key;
        Patient *value;
        Entry *next;
        Entry(int id, Patient *p) : key(id), value(p), next(nullptr) {}
    };

    static const int TABLE_SIZE = 101;
    Entry *buckets[TABLE_SIZE]{};

    int hashFunction(int key) const
    {
        // Modulo maps any patient ID to a valid bucket index.
        return (key % TABLE_SIZE + TABLE_SIZE) % TABLE_SIZE;
    }

public:
    ~PatientHashTable()
    {
        // Delete hash entries; patient records are owned by PatientList.
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            Entry *current = buckets[i];
            while (current != nullptr)
            {
                Entry *nextEntry = current->next;
                delete current;
                current = nextEntry;
            }
        }
    }

    void insert(int id, Patient *patient)
    {
        // Update an existing entry if the ID is already present.
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

        // Add a new entry at the front of the bucket chain.
        Entry *entry = new Entry(id, patient);
        entry->next = buckets[index];
        buckets[index] = entry;
    }

    Patient *search(int id) const
    {
        // Search only the bucket selected by the hash function.
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

    void erase(int id)
    {
        // Remove the matching entry while preserving the remaining chain.
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
};

// ------------------------------ ARRIVAL FIFO QUEUE ------------------------------

class ArrivalQueue
{
private:
    // The STL queue provides FIFO behavior for this arrival-order demonstration.
    queue<int> ids;

public:
    void add(int id) { ids.push(id); }

    void remove(int id)
    {
        // Rebuild the queue without the selected ID because std::queue has no erase.
        queue<int> temporary;
        while (!ids.empty())
        {
            int current = ids.front();
            ids.pop();
            if (current != id)
                temporary.push(current);
        }
        ids.swap(temporary);
    }

    void display(const PatientHashTable &table) const
    {
        // Display arrival order using IDs and current patient records.
        queue<int> copy = ids;
        cout << "\nArrival order (oldest first):\n";

        if (copy.empty())
            cout << "The arrival queue is empty.\n";

        while (!copy.empty())
        {
            int id = copy.front();
            copy.pop();
            Patient *p = table.search(id);
            if (p != nullptr)
                cout << p->id << " - " << p->name << " (" << p->status << ")\n";
        }
    }
};

// ------------------------------ PRIORITY HEAP ------------------------------

class PatientPriorityHeap
{
private:
    vector<Patient *> heap;
    unordered_map<int, int> position; // Patient ID -> current heap index.

    bool higherPriority(Patient *a, Patient *b) const
    {
        // Lower severity number means higher priority; arrival order breaks ties.
        if (a->severity != b->severity)
            return a->severity < b->severity;
        return a->arrivalOrder < b->arrivalOrder;
    }

    void swapNodes(int a, int b)
    {
        // Swap heap pointers and immediately repair both ID-to-index entries.
        swap(heap[a], heap[b]);
        position[heap[a]->id] = a;
        position[heap[b]->id] = b;
    }

    void siftUp(int index)
    {
        // Move a node toward the root while it outranks its parent.
        while (index > 0)
        {
            int parent = (index - 1) / 2;
            if (!higherPriority(heap[index], heap[parent]))
                break;
            swapNodes(index, parent);
            index = parent;
        }
    }

    void siftDown(int index)
    {
        // Move a node down until it outranks both children.
        int n = static_cast<int>(heap.size());

        while (true)
        {
            int best = index;
            int left = 2 * index + 1;
            int right = 2 * index + 2;

            if (left < n && higherPriority(heap[left], heap[best]))
                best = left;
            if (right < n && higherPriority(heap[right], heap[best]))
                best = right;

            if (best == index)
                break;

            swapNodes(index, best);
            index = best;
        }
    }

public:
    void clear()
    {
        // Clear both the heap array and its index lookup table.
        heap.clear();
        position.clear();
    }

    void insert(Patient *patient)
    {
        // Avoid adding the same patient twice.
        if (position.count(patient->id))
            return;

        heap.push_back(patient);
        int index = static_cast<int>(heap.size()) - 1;
        position[patient->id] = index;
        siftUp(index);
    }

    Patient *peek() const
    {
        // The root always contains the highest-priority waiting patient.
        return heap.empty() ? nullptr : heap[0];
    }

    Patient *pop()
    {
        // Remove and return the highest-priority patient.
        if (heap.empty())
            return nullptr;

        Patient *answer = heap[0];
        position.erase(answer->id);

        if (heap.size() == 1)
        {
            heap.pop_back();
            return answer;
        }

        heap[0] = heap.back();
        heap.pop_back();
        position[heap[0]->id] = 0;
        siftDown(0);
        return answer;
    }

    void updatePriority(int id, int newSeverity)
    {
        // Use the index map to find a patient without scanning the whole heap.
        if (!position.count(id))
            return;

        int index = position[id];
        int oldSeverity = heap[index]->severity;
        heap[index]->severity = newSeverity;
        heap[index]->lastUpdate = currentTime();

        // A smaller severity value moves upward; a larger value moves downward.
        if (newSeverity < oldSeverity)
            siftUp(index);
        else
            siftDown(index);
    }

    void heapify(const vector<Patient *> &patients)
    {
        // Build the heap from a collection in linear time using bottom-up sift-down.
        clear();
        for (Patient *p : patients)
        {
            heap.push_back(p);
            position[p->id] = static_cast<int>(heap.size()) - 1;
        }

        for (int i = static_cast<int>(heap.size()) / 2 - 1; i >= 0; i--)
            siftDown(i);
    }

    void displaySortedCopy() const
    {
        // Pop from a copy so the original waiting heap remains unchanged.
        PatientPriorityHeap copy;
        vector<Patient *> records = heap;
        copy.heapify(records);

        cout << "\nWaiting patients by priority:\n";
        while (copy.peek() != nullptr)
        {
            Patient *p = copy.pop();
            cout << "ID: " << p->id << " | " << p->name
                 << " | Priority: " << p->severity << '\n';
        }
    }
};

// ------------------------------ LONGEST-WAITING MAX HEAP ------------------------------

class LongestWaitingHeap
{
private:
    vector<Patient *> data;

    // Compare arrival order; the smallest order number has waited the longest.
    bool longerWaiting(Patient *a, Patient *b) const
    {
        return a->arrivalOrder < b->arrivalOrder;
    }

public:
    void build(const vector<Patient *> &patients)
    {
        // Copy all active patient pointers, then build a max-style heap.
        data = patients;

        for (int i = static_cast<int>(data.size()) / 2 - 1; i >= 0; i--)
        {
            int index = i;
            while (true)
            {
                int best = index;
                int left = 2 * index + 1;
                int right = 2 * index + 2;

                if (left < static_cast<int>(data.size()) &&
                    longerWaiting(data[left], data[best]))
                    best = left;

                if (right < static_cast<int>(data.size()) &&
                    longerWaiting(data[right], data[best]))
                    best = right;

                if (best == index)
                    break;

                swap(data[index], data[best]);
                index = best;
            }
        }
    }

    Patient *top() const
    {
        // Root is the patient with the earliest arrival order.
        return data.empty() ? nullptr : data[0];
    }
};

// ------------------------------ HEAP SORT ------------------------------

void heapSortByID(vector<Patient> &records)
{
    // Build a max heap so the largest ID can be moved to the array's end.
    auto siftDown = [&](int size, int root)
    {
        while (true)
        {
            int largest = root;
            int left = 2 * root + 1;
            int right = 2 * root + 2;

            if (left < size && records[left].id > records[largest].id)
                largest = left;
            if (right < size && records[right].id > records[largest].id)
                largest = right;

            if (largest == root)
                break;

            swap(records[root], records[largest]);
            root = largest;
        }
    };

    // Heapify all non-leaf nodes from bottom to top.
    for (int i = static_cast<int>(records.size()) / 2 - 1; i >= 0; i--)
        siftDown(static_cast<int>(records.size()), i);

    // Repeatedly move the maximum element to its final sorted position.
    for (int end = static_cast<int>(records.size()) - 1; end > 0; end--)
    {
        swap(records[0], records[end]);
        siftDown(end, 0);
    }
}

// ------------------------------ NAME TRIE ------------------------------

class NameTrie
{
private:
    struct TrieNode
    {
        unordered_map<char, TrieNode *> children;
        vector<int> patientIDs;
        // Recursively release all child nodes when a trie node is destroyed.
        ~TrieNode() { for (auto &entry : children) delete entry.second; }
    };
    TrieNode *root = new TrieNode();

    // Remove one patient ID and prune nodes that are no longer needed.
    bool eraseRecursive(TrieNode *node, const string &key, size_t depth, int id, bool &removed)
    {
        if (depth == key.size())
        {
            auto it = find(node->patientIDs.begin(), node->patientIDs.end(), id);
            if (it == node->patientIDs.end()) return false;
            node->patientIDs.erase(it);
            removed = true;
        }
        else
        {
            auto it = node->children.find(key[depth]);
            if (it == node->children.end()) return false;
            if (eraseRecursive(it->second, key, depth + 1, id, removed))
            {
                delete it->second;
                node->children.erase(it);
            }
        }
        return node != root && node->patientIDs.empty() && node->children.empty();
    }

    // Collect IDs stored at this node and every descendant node.
    void collect(TrieNode *node, vector<int> &result) const
    {
        for (int id : node->patientIDs) result.push_back(id);
        for (auto &entry : node->children) collect(entry.second, result);
    }

public:
    // Release the complete trie when the object is destroyed.
    ~NameTrie() { delete root; }

    // Insert a patient ID at the node representing the normalized full name.
    void insert(const string &name, int id)
    {
        TrieNode *current = root;
        for (char ch : lowerText(name))
        {
            if (!current->children.count(ch)) current->children[ch] = new TrieNode();
            current = current->children[ch];
        }
        if (find(current->patientIDs.begin(), current->patientIDs.end(), id) == current->patientIDs.end())
            current->patientIDs.push_back(id);
    }

    // Delete one patient's name entry while preserving shared prefixes.
    bool erase(const string &name, int id)
    {
        bool removed = false;
        eraseRecursive(root, lowerText(name), 0, id, removed);
        return removed;
    }

    // Return all patient IDs whose names begin with the supplied prefix.
    vector<int> searchPrefix(const string &prefix) const
    {
        TrieNode *current = root;
        for (char ch : lowerText(prefix))
        {
            if (!current->children.count(ch)) return {};
            current = current->children[ch];
        }
        vector<int> result;
        collect(current, result);
        return result;
    }
};

// ------------------------------ HOSPITAL DEPARTMENT GRAPH ------------------------------

class HospitalGraph
{
private:
    vector<string> names;
    vector<vector<pair<int, int>>> adjacency;

    int indexOf(const string &name) const
    {
        // Find a department's numeric index in the graph.
        for (int i = 0; i < static_cast<int>(names.size()); i++)
            if (lowerText(names[i]) == lowerText(name))
                return i;
        return -1;
    }

public:
    HospitalGraph()
    {
        // Create example hospital departments.
        addDepartment("Reception");
        addDepartment("Emergency");
        addDepartment("Laboratory");
        addDepartment("Radiology");
        addDepartment("Pharmacy");
        addDepartment("Ward");

        // Add illustrative weighted connections (distance/time units are examples).
        addRoad("Reception", "Emergency", 2);
        addRoad("Reception", "Laboratory", 5);
        addRoad("Emergency", "Radiology", 3);
        addRoad("Emergency", "Ward", 6);
        addRoad("Laboratory", "Radiology", 2);
        addRoad("Radiology", "Ward", 4);
        addRoad("Ward", "Pharmacy", 3);
        addRoad("Laboratory", "Pharmacy", 7);
    }

    void addDepartment(const string &name)
    {
        // Add a department only if its name is not already present.
        if (indexOf(name) != -1)
            return;
        names.push_back(name);
        adjacency.push_back({});
    }

    void addRoad(const string &from, const string &to, int weight)
    {
        // Add an undirected weighted connection between two known departments.
        int a = indexOf(from);
        int b = indexOf(to);

        if (a == -1 || b == -1 || weight < 0)
            return;

        adjacency[a].push_back({b, weight});
        adjacency[b].push_back({a, weight});
    }

    void dijkstra(const string &startName, const string &endName) const
    {
        // Find the shortest weighted route using a min-priority queue.
        int start = indexOf(startName);
        int finish = indexOf(endName);

        if (start == -1 || finish == -1)
        {
            cout << "Unknown department name.\n";
            return;
        }

        const int INF = numeric_limits<int>::max();
        vector<int> distance(names.size(), INF);
        vector<int> previous(names.size(), -1);

        // Store (distance, node); smallest distance is processed first.
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
        distance[start] = 0;
        pq.push({0, start});

        while (!pq.empty())
        {
            auto current = pq.top();
            pq.pop();

            int dist = current.first;
            int node = current.second;

            // Ignore an outdated queue entry.
            if (dist != distance[node])
                continue;

            for (auto edge : adjacency[node])
            {
                int next = edge.first;
                int weight = edge.second;

                // Relax the edge if this route is shorter.
                if (distance[node] != INF && distance[node] + weight < distance[next])
                {
                    distance[next] = distance[node] + weight;
                    previous[next] = node;
                    pq.push({distance[next], next});
                }
            }
        }

        if (distance[finish] == INF)
        {
            cout << "No route exists.\n";
            return;
        }

        // Reconstruct the path by following predecessor links backward.
        vector<int> path;
        for (int at = finish; at != -1; at = previous[at])
            path.push_back(at);
        reverse(path.begin(), path.end());

        cout << "Shortest route: ";
        for (int i = 0; i < static_cast<int>(path.size()); i++)
        {
            if (i) cout << " -> ";
            cout << names[path[i]];
        }
        cout << "\nTotal route weight: " << distance[finish] << '\n';
    }

    void bfsFewestMoves(const string &startName, const string &endName) const
    {
        // BFS finds a path with the fewest edges when every edge counts as one move.
        int start = indexOf(startName);
        int finish = indexOf(endName);

        if (start == -1 || finish == -1)
        {
            cout << "Unknown department name.\n";
            return;
        }

        vector<int> previous(names.size(), -1);
        vector<bool> visited(names.size(), false);
        queue<int> pending;

        visited[start] = true;
        pending.push(start);

        while (!pending.empty())
        {
            int node = pending.front();
            pending.pop();

            for (auto edge : adjacency[node])
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

        if (!visited[finish])
        {
            cout << "No route exists.\n";
            return;
        }

        // Reconstruct and display the fewest-edge path.
        vector<int> path;
        for (int at = finish; at != -1; at = previous[at])
            path.push_back(at);
        reverse(path.begin(), path.end());

        cout << "Fewest-moves route: ";
        for (int i = 0; i < static_cast<int>(path.size()); i++)
        {
            if (i) cout << " -> ";
            cout << names[path[i]];
        }
        cout << "\nNumber of moves: " << path.size() - 1 << '\n';
    }

    void topologicalTreatmentOrder() const
    {
        // This is a separate directed dependency graph for example treatment tasks.
        // A task may appear only after all of its prerequisite tasks.
        vector<string> tasks = {
            "Initial Assessment", "Lab Tests", "Imaging",
            "Doctor Review", "Treatment Plan", "Pharmacy"
        };

        vector<vector<int>> edges(tasks.size());
        vector<int> indegree(tasks.size(), 0);

        auto addDependency = [&](int before, int after)
        {
            edges[before].push_back(after);
            indegree[after]++;
        };

        // Example dependencies: assessment before tests/imaging; review after results.
        addDependency(0, 1);
        addDependency(0, 2);
        addDependency(1, 3);
        addDependency(2, 3);
        addDependency(3, 4);
        addDependency(4, 5);

        // Kahn's algorithm begins with every task having no prerequisites.
        queue<int> ready;
        for (int i = 0; i < static_cast<int>(tasks.size()); i++)
            if (indegree[i] == 0)
                ready.push(i);

        vector<string> order;
        while (!ready.empty())
        {
            int task = ready.front();
            ready.pop();
            order.push_back(tasks[task]);

            // Removing a task reduces the prerequisite count of its dependents.
            for (int next : edges[task])
                if (--indegree[next] == 0)
                    ready.push(next);
        }

        if (order.size() != tasks.size())
        {
            cout << "A dependency cycle exists; no valid order is possible.\n";
            return;
        }

        cout << "\nExample treatment workflow order:\n";
        for (const string &task : order)
            cout << " -> " << task << '\n';
    }

    void showDepartments() const
    {
        // Display all department names so the user can enter valid route endpoints.
        cout << "Departments: ";
        for (const string &name : names)
            cout << name << " | ";
        cout << '\n';
    }
};

// ------------------------------ BACKTRACKING BED / DOCTOR ALLOCATION ------------------------------

struct Allocation
{
    int patientID;
    int bed;
    int doctor;
};

bool allocateRecursively(const vector<Patient> &patients, int index,
                         int bedCount, int doctorCount,
                         vector<bool> &bedsUsed, vector<bool> &doctorsUsed,
                         vector<Allocation> &answer)
{
    // Base case: every patient in this small scheduling batch has been assigned.
    if (index == static_cast<int>(patients.size()))
        return true;

    // Try every free bed for the current patient.
    for (int bed = 1; bed <= bedCount; bed++)
    {
        if (bedsUsed[bed])
            continue;

        // Try every free doctor; one doctor and one bed can be used once in this batch.
        for (int doctor = 1; doctor <= doctorCount; doctor++)
        {
            if (doctorsUsed[doctor])
                continue;

            // Make a tentative choice.
            bedsUsed[bed] = true;
            doctorsUsed[doctor] = true;
            answer.push_back({patients[index].id, bed, doctor});

            // Continue with the next patient; success means all constraints were met.
            if (allocateRecursively(patients, index + 1, bedCount, doctorCount,
                                    bedsUsed, doctorsUsed, answer))
                return true;

            // Backtrack: undo the tentative choice and try another combination.
            answer.pop_back();
            bedsUsed[bed] = false;
            doctorsUsed[doctor] = false;
        }
    }

    // No available combination could be found for this patient.
    return false;
}

void demonstrateBacktrackingAllocation(const vector<Patient> &waiting)
{
    // Schedule at most three waiting patients to keep the demonstration manageable.
    vector<Patient> batch;
    for (const Patient &p : waiting)
    {
        if (p.status == "Waiting")
            batch.push_back(p);
        if (batch.size() == 3)
            break;
    }

    if (batch.empty())
    {
        cout << "There are no waiting patients to allocate.\n";
        return;
    }

    const int beds = 3;
    const int doctors = 3;
    vector<bool> bedsUsed(beds + 1, false);
    vector<bool> doctorsUsed(doctors + 1, false);
    vector<Allocation> answer;

    // Start recursive assignment at the first patient.
    if (allocateRecursively(batch, 0, beds, doctors, bedsUsed, doctorsUsed, answer))
    {
        cout << "\nBacktracking allocation proposal (demo only):\n";
        for (const Allocation &a : answer)
            cout << "Patient " << a.patientID << " -> Bed " << a.bed
                 << ", Doctor " << a.doctor << '\n';
        cout << "This is a proposal; it does not reserve real hospital resources.\n";
    }
    else
        cout << "No valid allocation was found with the available demo resources.\n";
}

// ------------------------------ UNDO STACK ------------------------------

struct UndoAction
{
    string type;       // Identifies the operation that can be reversed.
    Patient oldRecord; // Previous patient record when relevant.
    int number = 0;
    int second = 0;
    string text;
};

class UndoStack
{
private:
    // The stack's last-in-first-out order makes the most recent action undo first.
    stack<UndoAction> actions;

public:
    void push(const UndoAction &action) { actions.push(action); }

    bool pop(UndoAction &action)
    {
        // Return false when there is no operation available to undo.
        if (actions.empty())
            return false;

        action = actions.top();
        actions.pop();
        return true;
    }

    bool empty() const { return actions.empty(); }
};

// ------------------------------ DISCHARGE LOG ------------------------------

class DischargeLog
{
private:
    vector<pair<Patient, string>> records;

public:
    // Add a discharged patient and record the current date and time.
    void add(const Patient &patient) { records.push_back({patient, currentTime()}); }

    // Load historical discharge entries from a text file without clearing current entries.
    void load(const string &fileName)
    {
        ifstream file(fileName);
        string line;
        while (getline(file, line))
        {
            stringstream ss(line); string field; Patient p; string when;
            try {
                if (!getline(ss, field, '|')) continue;
                p.id = stoi(field);
                if (!getline(ss, p.name, '|')) continue;
                if (!getline(ss, field, '|')) continue;
                p.age = stoi(field);
                if (!getline(ss, field, '|')) continue;
                p.severity = stoi(field);
                if (!getline(ss, p.status, '|')) continue;
                if (!getline(ss, field, '|')) continue;
                p.bedNumber = stoi(field);
                if (!getline(ss, p.arrivalTime, '|')) continue;
                if (!getline(ss, p.lastUpdate, '|')) continue;
                if (!getline(ss, field, '|')) continue;
                p.arrivalOrder = stoll(field);
                if (!getline(ss, field, '|')) continue;
                try { p.agingIntervals = stoll(field); if (!getline(ss, when)) continue; }
                catch (...) { when = field; }
                if (p.id > 0) records.push_back({p, when});
            } catch (...) { continue; }
        }
    }

    // Save all discharge history so it remains available after restarting the program.
    void save(const string &fileName) const
    {
        ofstream file(fileName);
        for (const auto &entry : records)
        {
            const Patient &p = entry.first;
            file << p.id << '|' << p.name << '|' << p.age << '|' << p.severity << '|'
                 << p.status << '|' << p.bedNumber << '|' << p.arrivalTime << '|'
                 << p.lastUpdate << '|' << p.arrivalOrder << '|' << p.agingIntervals << '|' << entry.second << '\n';
        }
    }

    // Remove the most recent matching entry when a discharge is undone.
    void removeLatest(int id)
    {
        for (auto it = records.rbegin(); it != records.rend(); ++it)
            if (it->first.id == id) { records.erase(next(it).base()); return; }
    }

    // Display archived discharge entries without modifying the history.
    void display() const
    {
        if (records.empty()) { cout << "No discharge records.\n"; return; }
        cout << "\nDischarge History:\n";
        for (const auto &entry : records)
            cout << "ID: " << entry.first.id << " | Name: " << entry.first.name
                 << " | Discharged: " << entry.second << '\n';
    }
};

// ------------------------------ DOCTOR / APPOINTMENT / BED MODULES ------------------------------

struct Doctor
{
    int id = 0;
    string name;
    string specialty;
    bool available = true;
};

struct Appointment
{
    int id = 0;
    int patientID = 0;
    int doctorID = 0;
    string dateTime;
    string status = "Booked";
};

class DoctorManager
{
private:
    vector<Doctor> doctors;
public:
    // Add a doctor unless another doctor already uses the same ID.
    bool add(const Doctor &doctor)
    {
        if (find(doctor.id) != nullptr) return false;
        doctors.push_back(doctor); return true;
    }
    // Find a doctor by ID and return a pointer to the stored record.
    Doctor *find(int id)
    {
        for (Doctor &d : doctors) if (d.id == id) return &d;
        return nullptr;
    }
    // Restore a doctor's availability flag when undoing an availability change.
    bool setAvailability(int id, bool value)
    {
        Doctor *d = find(id);
        if (!d) return false;
        d->available = value;
        return true;
    }
    // Remove a doctor by ID when undoing a doctor registration.
    bool remove(int id)
    {
        for (auto it = doctors.begin(); it != doctors.end(); ++it)
            if (it->id == id) { doctors.erase(it); return true; }
        return false;
    }
    // Display all doctors and their current availability.
    void display() const
    {
        if (doctors.empty()) { cout << "No doctors registered.\n"; return; }
        for (const Doctor &d : doctors)
            cout << d.id << " | " << d.name << " | " << d.specialty
                 << " | " << (d.available ? "Available" : "Unavailable") << '\n';
    }
    // Save doctor records to a persistent text file.
    void save(const string &fileName) const
    {
        ofstream f(fileName); for (const Doctor &d : doctors)
            f << d.id << '|' << d.name << '|' << d.specialty << '|' << d.available << '\n';
    }
    // Load doctors from disk, ignoring malformed and duplicate records.
    void load(const string &fileName)
    {
        ifstream f(fileName); string line;
        while (getline(f, line)) { stringstream ss(line); Doctor d; string flag;
            if (getline(ss, line, '|')) { try { d.id = stoi(line); } catch (...) { continue; } }
            else continue;
            if (!getline(ss, d.name, '|') || !getline(ss, d.specialty, '|') || !getline(ss, flag)) continue;
            d.available = flag == "1"; add(d);
        }
    }
    // Return all doctor records for appointment validation and statistics.
    const vector<Doctor> &all() const { return doctors; }
};

class AppointmentManager
{
private:
    vector<Appointment> appointments;
public:
    // Book an appointment only if its ID is unique and the doctor has no booking at that exact time.
    bool book(const Appointment &a)
    {
        for (const Appointment &old : appointments)
            if (old.id == a.id || (old.doctorID == a.doctorID && old.dateTime == a.dateTime && old.status == "Booked"))
                return false;
        appointments.push_back(a); return true;
    }
    // Cancel an active appointment by ID and return the previous record for undo.
    bool cancel(int id, Appointment &old)
    {
        for (Appointment &a : appointments) if (a.id == id && a.status == "Booked")
        { old = a; a.status = "Cancelled"; return true; }
        return false;
    }
    // Restore an appointment to its previous booked state during undo.
    bool restore(const Appointment &a)
    {
        for (Appointment &item : appointments) if (item.id == a.id) { item = a; return true; }
        appointments.push_back(a); return true;
    }
    // Erase an appointment record when undoing its creation.
    bool erase(int id)
    {
        for (auto it = appointments.begin(); it != appointments.end(); ++it)
            if (it->id == id) { appointments.erase(it); return true; }
        return false;
    }
    // Display appointment records including cancelled bookings.
    void display() const
    {
        if (appointments.empty()) { cout << "No appointments.\n"; return; }
        for (const Appointment &a : appointments)
            cout << "Appointment " << a.id << " | Patient " << a.patientID << " | Doctor "
                 << a.doctorID << " | " << a.dateTime << " | " << a.status << '\n';
    }
    // Return the largest saved appointment ID so new IDs remain unique after restart.
    int maxID() const
    {
        int result = 0;
        for (const Appointment &a : appointments) result = max(result, a.id);
        return result;
    }

    // Persist appointment records to disk.
    void save(const string &fileName) const
    {
        ofstream f(fileName); for (const Appointment &a : appointments)
            f << a.id << '|' << a.patientID << '|' << a.doctorID << '|' << a.dateTime << '|' << a.status << '\n';
    }
    // Load appointment records and ignore malformed entries.
    void load(const string &fileName)
    {
        ifstream f(fileName); string line;
        while (getline(f, line)) { stringstream ss(line); Appointment a; string field;
            try { if (!getline(ss, field, '|')) continue; a.id = stoi(field);
                if (!getline(ss, field, '|')) continue;
                a.patientID = stoi(field);
                if (!getline(ss, field, '|')) continue;
                a.doctorID = stoi(field);
            } catch (...) { continue; }
            if (!getline(ss, a.dateTime, '|') || !getline(ss, a.status)) continue;
            bool duplicate = false; for (const auto &x : appointments) if (x.id == a.id) duplicate = true;
            if (!duplicate && a.id > 0) appointments.push_back(a);
        }
    }
};

class BedManager
{
private:
    static const int BED_COUNT = 100;
    vector<int> occupant;
public:
    // Initialize all beds as unoccupied.
    BedManager() : occupant(BED_COUNT + 1, 0) {}
    // Rebuild bed occupancy from active patient records after loading data.
    void rebuild(const vector<Patient> &patients)
    {
        fill(occupant.begin(), occupant.end(), 0);
        for (const Patient &p : patients)
            if (p.bedNumber >= 1 && p.bedNumber <= BED_COUNT) occupant[p.bedNumber] = p.id;
    }
    // Assign a requested bed if it is within range and currently free.
    bool assign(int bed, int patientID)
    {
        if (bed < 1 || bed > BED_COUNT || occupant[bed] != 0) return false;
        occupant[bed] = patientID; return true;
    }
    // Release a bed only when it is occupied by the expected patient.
    bool release(int bed, int patientID)
    {
        if (bed < 1 || bed > BED_COUNT || occupant[bed] != patientID) return false;
        occupant[bed] = 0; return true;
    }
    // Print every available bed number.
    void showAvailable() const
    {
        int count = 0; cout << "Available beds: ";
        for (int i = 1; i <= BED_COUNT; ++i) if (occupant[i] == 0) { cout << i << ' '; ++count; }
        cout << "\nTotal available: " << count << '/' << BED_COUNT << '\n';
    }
};

// ------------------------------ HOSPITAL SYSTEM CONTROLLER ------------------------------

class HospitalSystem
{
private:
    PatientList patients;
    AVLTree avl;
    PatientHashTable hash;
    ArrivalQueue arrivalQueue;
    PatientPriorityHeap priorityHeap;
    NameTrie trie;
    HospitalGraph graph;
    UndoStack undo;
    DischargeLog dischargeLog;
    DoctorManager doctors;
    AppointmentManager appointments;
    BedManager beds;
    long long nextArrivalOrder = 1;
    int nextAppointmentID = 1;

    void rebuildPriorityHeap()
    {
        // Rebuild from active waiting patients so no deleted pointer remains in the heap.
        vector<Patient *> waiting;
        for (Patient copy : patients.toVector())
        {
            Patient *actual = hash.search(copy.id);
            if (actual != nullptr && actual->status == "Waiting")
                waiting.push_back(actual);
        }
        priorityHeap.heapify(waiting);
    }

    void addToIndexes(Patient *patient)
    {
        // Keep all fast-search structures synchronized with the primary linked list.
        avl.insert(patient->id, patient);
        hash.insert(patient->id, patient);
        trie.insert(patient->name, patient->id);
        arrivalQueue.add(patient->id);
    }

    void removeFromIndexes(int id)
    {
        // Remove the ID from every index that supports deletion.
        avl.erase(id);
        hash.erase(id);
        arrivalQueue.remove(id);
        rebuildPriorityHeap();
    }

public:
    void registerPatient()
    {
        // Collect patient details before inserting the record into any structure.
        Patient p;
        p.id = readInt("Enter patient ID: ", 1, 1000000000);

        if (hash.search(p.id) != nullptr)
        {
            cout << "That patient ID already exists.\n";
            return;
        }

        p.name = readLine("Enter patient name: ");
        if (p.name.empty())
        {
            cout << "Name cannot be empty.\n";
            return;
        }

        p.age = readInt("Enter age: ", 0, 120);
        p.severity = readInt("Enter priority (1=highest, 5=lowest): ", 1, 5);
        p.status = "Waiting";
        p.bedNumber = -1;
        p.arrivalTime = currentTime();
        p.lastUpdate = p.arrivalTime;
        p.arrivalOrder = nextArrivalOrder++;

        // Add to the owning linked list first; its stored address becomes the shared record.
        if (!patients.add(p))
        {
            cout << "Could not register patient.\n";
            return;
        }

        Patient *stored = hash.search(p.id);
        // The pointer is obtained from the list after adding indexes below.
        (void)stored;

        Patient *actual = patients.find(p.id);
        addToIndexes(actual);
        priorityHeap.insert(actual);

        // Save an undo action so registration can be reversed.
        UndoAction action; action.type = "REGISTER"; action.oldRecord = p; undo.push(action);

        cout << "Patient registered successfully.\n";
    }

    void searchPatient()
    {
        // Hash lookup gives average O(1) search by ID.
        int id = readInt("Enter patient ID: ", 1, 1000000000);
        Patient *p = hash.search(id);

        if (p == nullptr)
        {
            cout << "Patient not found.\n";
            return;
        }

        cout << "\nPatient Details\n"
             << "ID: " << p->id << '\n'
             << "Name: " << p->name << '\n'
             << "Age: " << p->age << '\n'
             << "Priority: " << p->severity << '\n'
             << "Status: " << p->status << '\n'
             << "Arrival: " << p->arrivalTime << '\n'
             << "Last update: " << p->lastUpdate << '\n'
             << "Bed: " << (p->bedNumber == -1 ? string("Not assigned") : to_string(p->bedNumber))
             << '\n';
    }

    void searchAVL()
    {
        // Demonstrate the AVL tree's guaranteed logarithmic search.
        int id = readInt("Enter patient ID: ", 1, 1000000000);
        Patient *p = avl.search(id);

        if (p == nullptr)
            cout << "Patient not found in AVL tree.\n";
        else
            cout << "AVL found: " << p->id << " - " << p->name
                 << " | Status: " << p->status << '\n';
    }

    void searchNamePrefix()
    {
        // Trie returns IDs whose names begin with the entered prefix.
        string prefix = readLine("Enter the beginning of the patient's name: ");
        vector<int> ids = trie.searchPrefix(prefix);

        cout << "\nPrefix matches:\n";
        bool found = false;

        for (int id : ids)
        {
            Patient *p = hash.search(id);
            // Check the hash table because a trie entry may belong to a discharged patient.
            if (p != nullptr)
            {
                cout << p->id << " - " << p->name << " (" << p->status << ")\n";
                found = true;
            }
        }

        if (!found)
            cout << "No active patient matched that prefix.\n";
    }

    void treatNextPatient()
    {
        // Rebuild first so the heap reflects current priority and active waiting status.
        rebuildPriorityHeap();
        Patient *p = priorityHeap.pop();

        if (p == nullptr)
        {
            cout << "No waiting patients.\n";
            return;
        }

        // Store the previous record so this status change can be undone.
        Patient old = *p;
        p->status = "In Treatment";
        p->lastUpdate = currentTime();

        UndoAction action; action.type = "TREAT"; action.oldRecord = old; undo.push(action);
        rebuildPriorityHeap();

        cout << "Patient " << p->name << " is now marked In Treatment.\n";
    }

    void dischargePatient()
    {
        // Find the active patient before removing the owning linked-list node.
        int id = readInt("Enter patient ID to discharge: ", 1, 1000000000);
        Patient *p = hash.search(id);

        if (p == nullptr)
        {
            cout << "Patient not found.\n";
            return;
        }

        // Copy the record because deleting the linked-list node invalidates its pointer.
        Patient old = *p;
        string previousStatus = p->status;
        old.status = "Discharged";
        old.lastUpdate = currentTime();

        // Keep a discharge-history copy, then save the undo information.
        dischargeLog.add(old);
        UndoAction action; action.type = "DISCHARGE"; action.oldRecord = old; action.text = previousStatus; undo.push(action);

        if (old.bedNumber != -1) beds.release(old.bedNumber, id);
        trie.erase(old.name, id);
        Patient removed;
        patients.remove(id, removed);
        removeFromIndexes(id);

        cout << "Patient discharged and removed from active records.\n";
    }

    void undoLastAction()
    {
        // Reverse the newest action because the undo structure is LIFO.
        UndoAction action;
        if (!undo.pop(action))
        {
            cout << "There is no action to undo.\n";
            return;
        }

        if (action.type == "REGISTER")
        {
            // Undo registration by removing the newly registered patient.
            trie.erase(action.oldRecord.name, action.oldRecord.id);
            Patient removed;
            patients.remove(action.oldRecord.id, removed);
            removeFromIndexes(action.oldRecord.id);
            cout << "Last registration undone.\n";
        }
        else if (action.type == "TREAT")
        {
            // Undo treatment by restoring the previous patient status and priority.
            Patient *p = hash.search(action.oldRecord.id);
            if (p != nullptr)
            {
                *p = action.oldRecord;
                rebuildPriorityHeap();
                cout << "Last treatment-status change undone.\n";
            }
        }
        else if (action.type == "DISCHARGE")
        {
            // Undo discharge by restoring the copied patient record.
            Patient restored = action.oldRecord;
            dischargeLog.removeLatest(restored.id);
            restored.status = action.text.empty() ? "Waiting" : action.text;
            restored.lastUpdate = currentTime();

            if (patients.add(restored))
            {
                Patient *actual = patients.find(restored.id);
                addToIndexes(actual);
                rebuildPriorityHeap();
                beds.rebuild(patients.toVector());
                cout << "Last discharge undone; patient restored as Waiting.\n";
            }
            else
                cout << "Could not restore patient because the ID is already active.\n";
        }
        else if (action.type == "PRIORITY")
        {
            Patient *p = hash.search(action.oldRecord.id);
            if (p) { *p = action.oldRecord; rebuildPriorityHeap(); cout << "Priority change undone.\n"; }
        }
        else if (action.type == "ASSIGN_BED")
        {
            Patient *p = hash.search(action.oldRecord.id);
            if (p) { beds.release(action.number, p->id); p->bedNumber = action.oldRecord.bedNumber; cout << "Bed assignment undone.\n"; }
        }
        else if (action.type == "RELEASE_BED")
        {
            Patient *p = hash.search(action.oldRecord.id);
            if (p && beds.assign(action.number, p->id)) { p->bedNumber = action.number; cout << "Bed release undone.\n"; }
            else cout << "Could not restore the bed assignment.\n";
        }
        else if (action.type == "ADD_DOCTOR")
        {
            doctors.remove(action.number); doctors.save("doctors.txt"); cout << "Doctor registration undone.\n";
        }
        else if (action.type == "DOCTOR_AVAILABILITY")
        {
            doctors.setAvailability(action.number, action.second == 1); doctors.save("doctors.txt"); cout << "Doctor availability change undone.\n";
        }
        else if (action.type == "BOOK_APPOINTMENT")
        {
            appointments.erase(action.number); appointments.save("appointments.txt"); cout << "Appointment booking undone.\n";
        }
        else if (action.type == "CANCEL_APPOINTMENT")
        {
            Appointment a; a.id = action.number; a.patientID = action.second; a.doctorID = action.oldRecord.id;
            a.dateTime = action.text; a.status = "Booked"; appointments.restore(a);
            appointments.save("appointments.txt"); cout << "Appointment cancellation undone.\n";
        }
    }

    void updatePatientPriority()
    {
        // Update the record and heap priority using the patient's ID.
        int id = readInt("Enter patient ID: ", 1, 1000000000);
        Patient *p = hash.search(id);

        if (p == nullptr)
        {
            cout << "Patient not found.\n";
            return;
        }

        int severity = readInt("New priority (1=highest, 5=lowest): ", 1, 5);
        UndoAction action; action.type = "PRIORITY"; action.oldRecord = *p; undo.push(action);
        p->severity = severity;
        p->lastUpdate = currentTime();

        // Rebuild ensures the priority queue is correct even for non-waiting records.
        rebuildPriorityHeap();
        cout << "Priority updated.\n";
    }

    void displayWaitingPriority()
    {
        // Show the current waiting order without changing the live heap.
        rebuildPriorityHeap();
        priorityHeap.displaySortedCopy();
    }

    void displayLongestWaiting()
    {
        // Build a max heap over active waiting patients using arrival order.
        vector<Patient *> waiting;
        for (const Patient &copy : patients.toVector())
        {
            Patient *p = hash.search(copy.id);
            if (p != nullptr && p->status == "Waiting")
                waiting.push_back(p);
        }

        LongestWaitingHeap maxHeap;
        maxHeap.build(waiting);
        Patient *p = maxHeap.top();

        if (p == nullptr)
            cout << "No waiting patients.\n";
        else
            cout << "Longest waiting: " << p->name << " (ID " << p->id << ")\n";
    }

    void displayArrivalQueue()
    {
        // Display patients in the order they originally arrived.
        arrivalQueue.display(hash);
    }

    void displayPatients()
    {
        // The doubly linked list is the main active-patient storage.
        patients.printAll();
    }

    void displaySortedByID()
    {
        // Use heap sort on a copy so the original linked list is not rearranged.
        vector<Patient> records = patients.toVector();
        heapSortByID(records);

        cout << "\nPatients sorted by ID using Heap Sort:\n";
        for (const Patient &p : records)
            cout << p.id << " - " << p.name << " - " << p.status << '\n';
    }

    void displayStatistics()
    {
        // Count active patients by status and priority.
        int waiting = 0;
        int treating = 0;
        int priorityCounts[6] = {};

        for (const Patient &p : patients.toVector())
        {
            if (p.status == "Waiting") waiting++;
            if (p.status == "In Treatment") treating++;
            priorityCounts[p.severity]++;
        }

        cout << "\nHospital Statistics\n"
             << "Active patients: " << patients.size() << '\n'
             << "Waiting: " << waiting << '\n'
             << "In treatment: " << treating << '\n';

        for (int i = 1; i <= 5; i++)
            cout << "Priority " << i << ": " << priorityCounts[i] << '\n';
    }

    void showDischargeHistory() const
    {
        // Display archived discharge entries.
        dischargeLog.display();
    }

    void routeMenu()
    {
        // Let the user select departments for graph demonstrations.
        graph.showDepartments();
        string from = readLine("Starting department: ");
        string to = readLine("Destination department: ");

        cout << "\n1. Dijkstra (lowest total route weight)\n";
        cout << "2. BFS (fewest department moves)\n";
        int choice = readInt("Choose route algorithm: ", 1, 2);

        if (choice == 1)
            graph.dijkstra(from, to);
        else
            graph.bfsFewestMoves(from, to);
    }

    void showTreatmentOrder()
    {
        // Demonstrate topological ordering of a separate task-dependency graph.
        graph.topologicalTreatmentOrder();
    }

    void allocateBedsDoctors()
    {
        // Run the backtracking scheduling demonstration on current waiting records.
        demonstrateBacktrackingAllocation(patients.toVector());
    }

    // Register a doctor with a unique ID and persist the updated doctor list.
    void addDoctor()
    {
        Doctor d;
        d.id = readInt("Doctor ID: ", 1, 1000000000);
        d.name = readLine("Doctor name: ");
        d.specialty = readLine("Specialty: ");
        if (d.name.empty() || d.specialty.empty() || !doctors.add(d))
            cout << "Doctor could not be added (empty field or duplicate ID).\n";
        else { UndoAction action; action.type = "ADD_DOCTOR"; action.number = d.id; undo.push(action); doctors.save("doctors.txt"); cout << "Doctor added.\n"; }
    }

    // Search for a doctor by ID and display the matching record.
    void searchDoctor()
    {
        int id = readInt("Doctor ID: ", 1, 1000000000);
        Doctor *d = doctors.find(id);
        if (!d) cout << "Doctor not found.\n";
        else cout << d->id << " | " << d->name << " | " << d->specialty
                  << " | " << (d->available ? "Available" : "Unavailable") << '\n';
    }

    // Display the complete doctor directory.
    void listDoctors() { doctors.display(); }

    // Change doctor availability; appointments cannot be booked with unavailable doctors.
    void setDoctorAvailability()
    {
        int id = readInt("Doctor ID: ", 1, 1000000000);
        Doctor *d = doctors.find(id);
        if (!d) { cout << "Doctor not found.\n"; return; }
        int value = readInt("Availability (1=available, 0=unavailable): ", 0, 1);
        UndoAction action; action.type = "DOCTOR_AVAILABILITY"; action.number = id; action.second = d->available ? 1 : 0; undo.push(action);
        d->available = (value == 1);
        doctors.save("doctors.txt");
        cout << "Doctor availability updated.\n";
    }

    // Book an appointment after validating the patient, doctor, and exact time-slot conflict.
    void bookAppointment()
    {
        Appointment a;
        a.id = nextAppointmentID++;
        a.patientID = readInt("Patient ID: ", 1, 1000000000);
        if (!hash.search(a.patientID)) { cout << "Patient not found.\n"; return; }
        a.doctorID = readInt("Doctor ID: ", 1, 1000000000);
        Doctor *d = doctors.find(a.doctorID);
        if (!d || !d->available) { cout << "Doctor not found or unavailable.\n"; return; }
        a.dateTime = readLine("Appointment date/time (YYYY-MM-DD HH:MM): ");
        if (a.dateTime.empty() || !appointments.book(a)) { cout << "Time slot is already booked or appointment ID conflict.\n"; return; }
        UndoAction action; action.type = "BOOK_APPOINTMENT"; action.number = a.id; undo.push(action);
        appointments.save("appointments.txt");
        cout << "Appointment booked with ID " << a.id << ".\n";
    }

    // Cancel an appointment while retaining its record in the persistent history.
    void cancelAppointment()
    {
        int id = readInt("Appointment ID: ", 1, 1000000000); Appointment old;
        if (!appointments.cancel(id, old)) cout << "Active appointment not found.\n";
        else { UndoAction action; action.type = "CANCEL_APPOINTMENT"; action.number = old.id; action.second = old.patientID; action.oldRecord.id = old.doctorID; action.text = old.dateTime; undo.push(action); appointments.save("appointments.txt"); cout << "Appointment cancelled.\n"; }
    }

    // Display all booked and cancelled appointments.
    void listAppointments() { appointments.display(); }

    // Assign a free bed to an active patient and update the patient's record.
    void assignBed()
    {
        int id = readInt("Patient ID: ", 1, 1000000000); Patient *p = hash.search(id);
        if (!p) { cout << "Patient not found.\n"; return; }
        if (p->bedNumber != -1) { cout << "Patient already has a bed.\n"; return; }
        int bed = readInt("Bed number (1-100): ", 1, 100);
        if (!beds.assign(bed, id)) { cout << "Bed is occupied.\n"; return; }
        UndoAction action; action.type = "ASSIGN_BED"; action.oldRecord = *p; action.number = bed; undo.push(action);
        p->bedNumber = bed; p->lastUpdate = currentTime();
        cout << "Bed " << bed << " assigned to patient " << id << ".\n";
    }

    // Release a patient's assigned bed and update the active patient record.
    void releaseBed()
    {
        int id = readInt("Patient ID: ", 1, 1000000000); Patient *p = hash.search(id);
        if (!p) { cout << "Patient not found.\n"; return; }
        if (p->bedNumber == -1 || !beds.release(p->bedNumber, id)) { cout << "No matching bed assignment.\n"; return; }
        UndoAction action; action.type = "RELEASE_BED"; action.oldRecord = *p; action.number = p->bedNumber; undo.push(action);
        p->bedNumber = -1; p->lastUpdate = currentTime(); cout << "Bed released.\n";
    }

    // Show bed occupancy totals and available bed numbers.
    void showBeds() { beds.showAvailable(); }

    // Improve waiting priority by one level for each complete 30-minute waiting interval.
    void applyPriorityAging()
    {
        time_t now = time(nullptr); int changed = 0;
        for (const Patient &copy : patients.toVector())
        {
            Patient *p = hash.search(copy.id);
            if (!p || p->status != "Waiting") continue;
            tm parsed = {}; istringstream input(p->arrivalTime); input >> get_time(&parsed, "%Y-%m-%d %H:%M:%S");
            if (input.fail()) continue;
            time_t arrived = mktime(&parsed);
            long long intervals = max(0LL, static_cast<long long>(difftime(now, arrived) / 1800.0));
            long long newlyCompleted = max(0LL, intervals - p->agingIntervals);
            int newSeverity = max(1, p->severity - static_cast<int>(newlyCompleted));
            if (newSeverity < p->severity)
            {
                UndoAction action; action.type = "PRIORITY"; action.oldRecord = *p; undo.push(action);
                p->severity = newSeverity; p->lastUpdate = currentTime(); ++changed;
            }
            p->agingIntervals = max(p->agingIntervals, intervals);
        }
        rebuildPriorityHeap();
        cout << "Priority aging updated " << changed << " patient(s).\n";
    }

    // Benchmark ID lookup in the linked list, AVL tree, and hash table using the same records.
    void benchmarkSearch()
    {
        vector<Patient> records = patients.toVector();
        if (records.empty()) { cout << "Register patients before benchmarking.\n"; return; }
        int target = records.back().id;
        auto measure = [&](auto searchFunction) {
            auto begin = chrono::high_resolution_clock::now();
            for (int i = 0; i < 20000; ++i) searchFunction(target);
            auto finish = chrono::high_resolution_clock::now();
            return chrono::duration<double, micro>(finish - begin).count();
        };
        double listTime = measure([&](int id) { for (const Patient &p : records) if (p.id == id) break; });
        double avlTime = measure([&](int id) { (void)avl.search(id); });
        double hashTime = measure([&](int id) { (void)hash.search(id); });
        cout << "20,000 lookup iterations (microseconds):\nLinked-list scan: " << listTime
             << "\nAVL tree: " << avlTime << "\nHash table: " << hashTime << '\n';
    }

    // Compare heap sort and the standard library sort on identical patient copies.
    void benchmarkSorting()
    {
        vector<Patient> original = patients.toVector();
        if (original.size() < 2) { cout << "Register at least two patients first.\n"; return; }
        vector<Patient> a = original, b = original;
        auto t1 = chrono::high_resolution_clock::now(); heapSortByID(a); auto t2 = chrono::high_resolution_clock::now();
        sort(b.begin(), b.end(), [](const Patient &x, const Patient &y) { return x.id < y.id; });
        auto t3 = chrono::high_resolution_clock::now();
        cout << "Heap sort: " << chrono::duration<double, micro>(t2-t1).count()
             << " microseconds\nstd::sort: " << chrono::duration<double, micro>(t3-t2).count() << " microseconds\n";
    }

    // Run small repeatable checks for core structures and report pass/fail results.
    void runUnitTests()
    {
        int passed = 0, total = 0;
        auto check = [&](bool condition, const string &label) {
            ++total; if (condition) { ++passed; cout << "[PASS] " << label << '\n'; }
            else cout << "[FAIL] " << label << '\n';
        };
        vector<Patient> sample(3); sample[0].id = 30; sample[1].id = 10; sample[2].id = 20;
        heapSortByID(sample);
        check(sample[0].id == 10 && sample[1].id == 20 && sample[2].id == 30, "Heap sort ordering");
        NameTrie testTrie; testTrie.insert("Arqam", 7); testTrie.insert("Aria", 8);
        check(testTrie.searchPrefix("ar").size() == 2, "Trie prefix search");
        check(testTrie.erase("Arqam", 7) && testTrie.searchPrefix("ar").size() == 1, "Trie deletion");
        BedManager testBeds; check(testBeds.assign(1, 99), "Bed assignment");
        check(!testBeds.assign(1, 100), "Occupied bed rejection");
        check(testBeds.release(1, 99), "Bed release");
        Patient avlA; avlA.id = 41; Patient avlB; avlB.id = 19;
        AVLTree testAVL; testAVL.insert(avlA.id, &avlA); testAVL.insert(avlB.id, &avlB);
        check(testAVL.search(19) == &avlB, "AVL insertion and search");
        PatientHashTable testHash; testHash.insert(avlA.id, &avlA);
        check(testHash.search(41) == &avlA, "Hash table insertion and search");
        DoctorManager testDoctors; Doctor d; d.id = 1; d.name = "Test Doctor"; d.specialty = "General";
        check(testDoctors.add(d) && !testDoctors.add(d), "Doctor duplicate ID validation");
        AppointmentManager testAppointments; Appointment a; a.id=1; a.patientID=1; a.doctorID=1; a.dateTime="2030-01-01 09:00";
        check(testAppointments.book(a), "Appointment booking");
        Appointment b=a; b.id=2; check(!testAppointments.book(b), "Doctor time conflict validation");
        cout << "Unit tests: " << passed << '/' << total << " passed.\n";
    }

    void saveToFile()
    {
        // Save active patient data in a simple delimiter-separated text format.
        ofstream file("patients.txt");
        if (!file)
        {
            cout << "Could not open patients.txt for writing.\n";
            return;
        }

        for (const Patient &p : patients.toVector())
        {
            file << p.id << '|' << p.name << '|' << p.age << '|'
                 << p.severity << '|' << p.status << '|' << p.bedNumber << '|'
                 << p.arrivalTime << '|' << p.lastUpdate << '|'
                 << p.arrivalOrder << '|' << p.agingIntervals << '\n';
        }

        dischargeLog.save("discharges.txt");
        doctors.save("doctors.txt");
        appointments.save("appointments.txt");
        cout << "Patient, discharge, doctor, and appointment data saved.\n";
    }

    void loadFromFile()
    {
        // Load supporting persistent records before the patient indexes are rebuilt.
        dischargeLog.load("discharges.txt");
        doctors.load("doctors.txt");
        appointments.load("appointments.txt");
        nextAppointmentID = appointments.maxID() + 1;
        // Load records at startup; skip malformed lines and duplicate IDs.
        ifstream file("patients.txt");
        if (!file)
        {
            cout << "No patients.txt file found; starting with an empty system.\n";
            return;
        }

        string line;
        int loaded = 0;

        while (getline(file, line))
        {
            stringstream ss(line);
            string field;
            Patient p;

            try
            {
                if (!getline(ss, field, '|')) continue;
                p.id = stoi(field);
                if (!getline(ss, p.name, '|')) continue;
                if (!getline(ss, field, '|')) continue;
                p.age = stoi(field);
                if (!getline(ss, field, '|')) continue;
                p.severity = stoi(field);
                if (!getline(ss, p.status, '|')) continue;
                if (!getline(ss, field, '|')) continue;
                p.bedNumber = stoi(field);
                if (!getline(ss, p.arrivalTime, '|')) continue;
                if (!getline(ss, p.lastUpdate, '|')) continue;
                if (!getline(ss, field, '|')) continue;
                p.arrivalOrder = stoll(field);
                if (getline(ss, field, '|')) p.agingIntervals = stoll(field);
            }
            catch (...)
            {
                // Ignore a line whose numeric fields cannot be converted.
                continue;
            }

            // Validate basic fields before accepting the record.
            if (p.id <= 0 || p.age < 0 || p.age > 120 ||
                p.severity < 1 || p.severity > 5 || hash.search(p.id) != nullptr)
                continue;

            if (patients.add(p))
            {
                Patient *actual = patients.find(p.id);
                addToIndexes(actual);
                nextArrivalOrder = max(nextArrivalOrder, p.arrivalOrder + 1);
                loaded++;
            }
        }

        rebuildPriorityHeap();
        beds.rebuild(patients.toVector());
        cout << loaded << " patient record(s) loaded.\n";
    }

    void run()
    {
        // Load saved data before showing the main menu.
        loadFromFile();

        int choice;
        do
        {
            // Keep the main function simple: this controller owns the menu loop.
            cout << "\n========== HOSPITAL MANAGEMENT SYSTEM ==========\n"
                 << "1. Register patient\n"
                 << "2. Search patient by ID (Hash Table)\n"
                 << "3. Search patient by ID (AVL Tree)\n"
                 << "4. Search patient by name prefix (Trie)\n"
                 << "5. Display all active patients\n"
                 << "6. Display patients sorted by ID (Heap Sort)\n"
                 << "7. Display arrival FIFO queue\n"
                 << "8. Display waiting priority order (Min Heap)\n"
                 << "9. Treat next highest-priority patient\n"
                 << "10. Update patient priority\n"
                 << "11. Display longest-waiting patient (Max Heap)\n"
                 << "12. Discharge patient\n"
                 << "13. Undo last supported action (Stack)\n"
                 << "14. Display hospital statistics\n"
                 << "15. Display discharge history\n"
                 << "16. Find department route (Dijkstra / BFS)\n"
                 << "17. Display treatment task order (Topological Sort)\n"
                 << "18. Demonstrate bed/doctor allocation (Backtracking)\n"
                 << "19. Save all records\n"
                 << "20. Add doctor\n"
                 << "21. Search doctor\n"
                 << "22. List doctors\n"
                 << "23. Book appointment\n"
                 << "24. Cancel appointment\n"
                 << "25. List appointments\n"
                 << "26. Assign bed\n"
                 << "27. Release bed\n"
                 << "28. Show bed availability\n"
                 << "29. Apply priority aging\n"
                 << "30. Benchmark searches and sorting\n"
                 << "31. Run unit tests\n"
                 << "32. Change doctor availability\n"
                 << "0. Save and exit\n";

            choice = readInt("Enter choice: ", 0, 32);

            // Dispatch each menu choice to its corresponding function.
            switch (choice)
            {
                case 1: registerPatient(); break;
                case 2: searchPatient(); break;
                case 3: searchAVL(); break;
                case 4: searchNamePrefix(); break;
                case 5: displayPatients(); break;
                case 6: displaySortedByID(); break;
                case 7: displayArrivalQueue(); break;
                case 8: displayWaitingPriority(); break;
                case 9: treatNextPatient(); break;
                case 10: updatePatientPriority(); break;
                case 11: displayLongestWaiting(); break;
                case 12: dischargePatient(); break;
                case 13: undoLastAction(); break;
                case 14: displayStatistics(); break;
                case 15: showDischargeHistory(); break;
                case 16: routeMenu(); break;
                case 17: showTreatmentOrder(); break;
                case 18: allocateBedsDoctors(); break;
                case 19: saveToFile(); break;
                case 20: addDoctor(); break;
                case 21: searchDoctor(); break;
                case 22: listDoctors(); break;
                case 23: bookAppointment(); break;
                case 24: cancelAppointment(); break;
                case 25: listAppointments(); break;
                case 26: assignBed(); break;
                case 27: releaseBed(); break;
                case 28: showBeds(); break;
                case 29: applyPriorityAging(); break;
                case 30: benchmarkSearch(); benchmarkSorting(); break;
                case 31: runUnitTests(); break;
                case 32: setDoctorAvailability(); break;
                case 0:
                    saveToFile();
                    cout << "Exiting Hospital Management System.\n";
                    break;
            }
        } while (choice != 0);
    }
};

// ------------------------------ MAIN FUNCTION ------------------------------

int main()
{
    // Create one controller object; it owns and coordinates all DSA components.
    HospitalSystem hospital;

    // Start the menu-based hospital management application.
    hospital.run();

    return 0;
}
