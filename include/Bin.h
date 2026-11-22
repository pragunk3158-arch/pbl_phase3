#ifndef BIN_H
#define BIN_H

#include "Common.h"
#include <string>

// A single waste bin.
// Shows encapsulation: data is private, outside code uses getters/setters.
class Bin {
private:
    std::string id;
    std::string location;
    double capacity;        // kg
    double currentWaste;    // kg
    double fillPercent;     // %
    double fillRate;        // kg per day
    Priority priority;
    double urgencyScore;    // used by the priority queue
    std::string status;     // "Pending" or "Collected"

public:
    Bin();
    Bin(std::string id, std::string location, double capacity,
        double currentWaste, double fillRate, std::string status);

    // Recalculates fill percent, priority and urgency score
    void calculatePriority();

    // Getters
    std::string getId() const { return id; }
    std::string getLocation() const { return location; }
    double getCapacity() const { return capacity; }
    double getCurrentWaste() const { return currentWaste; }
    double getFillPercent() const { return fillPercent; }
    double getFillRate() const { return fillRate; }
    Priority getPriority() const { return priority; }
    double getUrgencyScore() const { return urgencyScore; }
    std::string getStatus() const { return status; }

    // Setters
    void setCurrentWaste(double waste);
    void setStatus(std::string newStatus) { status = newStatus; }

    // Empties the bin
    void markCollected();

    // Display
    void displayRow() const;
    void displayDetails() const;

    // File handling
    std::string toCSV() const;
    static Bin fromCSV(std::string line);
};

#endif
