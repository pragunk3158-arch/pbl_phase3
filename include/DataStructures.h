#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "Bin.h"
#include <vector>

// ===========================================================================
// DATA STRUCTURE 1: SINGLY LINKED LIST OF BINS
// Every bin is a node created with new, and the nodes are joined by the
// next pointer. Used because bins are added and removed often.
// ===========================================================================
struct BinNode {
    Bin data;
    BinNode* next;
};

class BinList {
private:
    BinNode* head;
    int count;

public:
    BinList() {
        head = NULL;
        count = 0;
    }

    ~BinList() { clear(); }

    // Copying is switched off on purpose. The list owns nodes created with
    // new, so a copy would point at the same nodes and both destructors
    // would delete them. That is the classic "rule of three" problem.
    BinList(const BinList& other) = delete;
    BinList& operator=(const BinList& other) = delete;

    // Adds a bin at the end of the list
    void insertEnd(Bin b) {
        BinNode* node = new BinNode;
        node->data = b;
        node->next = NULL;

        if (head == NULL) {
            head = node;
        } else {
            BinNode* temp = head;
            while (temp->next != NULL)
                temp = temp->next;
            temp->next = node;
        }
        count++;
    }

    // Linear search by bin id. Returns NULL if not found.
    Bin* findById(std::string id) {
        BinNode* temp = head;
        while (temp != NULL) {
            if (temp->data.getId() == id)
                return &(temp->data);
            temp = temp->next;
        }
        return NULL;
    }

    void clear() {
        while (head != NULL) {
            BinNode* temp = head;
            head = head->next;
            delete temp;
        }
        count = 0;
    }

    std::vector<Bin> toVector() const {
        std::vector<Bin> list;
        BinNode* temp = head;
        while (temp != NULL) {
            list.push_back(temp->data);
            temp = temp->next;
        }
        return list;
    }

    BinNode* getHead() const { return head; }
    int size() const { return count; }
};

// ===========================================================================
// DATA STRUCTURE 2: MAX HEAP (PRIORITY QUEUE)
// The bin with the highest urgency score always stays at index 0, so the
// most urgent bin is collected first.
//   parent of i = (i - 1) / 2,  children of i = 2i + 1 and 2i + 2
// ===========================================================================
class BinPriorityQueue {
private:
    std::vector<Bin> heap;

    // Moves a new bin up until its parent is more urgent
    void heapifyUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (heap[parent].getUrgencyScore() >= heap[i].getUrgencyScore())
                break;
            Bin temp = heap[i];
            heap[i] = heap[parent];
            heap[parent] = temp;
            i = parent;
        }
    }

    // Moves the bin at index i down until both children are less urgent
    void heapifyDown(int i) {
        int n = (int)heap.size();
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int largest = i;

            if (left < n && heap[left].getUrgencyScore() > heap[largest].getUrgencyScore())
                largest = left;
            if (right < n && heap[right].getUrgencyScore() > heap[largest].getUrgencyScore())
                largest = right;
            if (largest == i) break;

            Bin temp = heap[i];
            heap[i] = heap[largest];
            heap[largest] = temp;
            i = largest;
        }
    }

public:
    void push(Bin b) {
        heap.push_back(b);
        heapifyUp((int)heap.size() - 1);
    }

    // Removes and returns the most urgent bin
    Bin pop() {
        Bin top = heap[0];
        heap[0] = heap[heap.size() - 1];
        heap.pop_back();
        if (!heap.empty()) heapifyDown(0);
        return top;
    }

    bool isEmpty() const { return heap.empty(); }
    int size() const { return (int)heap.size(); }
};

// ===========================================================================
// SORTING: QUICKSORT ON A VECTOR OF BINS
// byFill = true  -> sorts on fill percentage
// byFill = false -> sorts on urgency score
// Both are sorted in descending order (fullest / most urgent first).
// ===========================================================================
inline double sortKey(const Bin& b, bool byFill) {
    return byFill ? b.getFillPercent() : b.getUrgencyScore();
}

inline int partitionBins(std::vector<Bin>& arr, int low, int high, bool byFill) {
    double pivot = sortKey(arr[high], byFill);
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (sortKey(arr[j], byFill) > pivot) {   // ">" gives descending order
            i++;
            Bin temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    Bin temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    return i + 1;
}

inline void quickSortBins(std::vector<Bin>& arr, int low, int high, bool byFill) {
    if (low < high) {
        int p = partitionBins(arr, low, high, byFill);
        quickSortBins(arr, low, p - 1, byFill);
        quickSortBins(arr, p + 1, high, byFill);
    }
}

#endif
