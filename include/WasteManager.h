#ifndef WASTE_MANAGER_H
#define WASTE_MANAGER_H

#include "Common.h"
#include "Bin.h"
#include "Vehicle.h"
#include "Graph.h"
#include "Route.h"
#include "DataStructures.h"
#include "FileHandler.h"
#include <string>
#include <vector>

// Numbers produced when the old fixed route is compared with the smart plan
struct Comparison {
    double fixedDistance;
    double fixedTime;
    int    fixedBins;
    double fixedWaste;

    double smartDistance;
    double smartTime;
    int    smartBins;
    double smartWaste;
    int    smartTrucks;

    double distanceSaved;
    double timeSaved;
    double fuelSaved;
    double efficiencyGain;   // in percent
};

// Main class of the project. It keeps all the data together and provides
// one function for each module of the system.
class WasteManager {
private:
    BinList bins;
    std::vector<Vehicle*> vehicles;
    Graph roadGraph;
    std::string depot;
    std::string dataDir;

    // One trip per truck. More than one truck is used when a single truck
    // cannot carry all the urgent waste in one go.
    std::vector<Route> routes;

    Comparison lastComparison;
    bool comparisonDone;
    int completedTrips;

    // helpers used while planning
    int chooseVehicle(double wasteToCarry, const std::vector<bool>& alreadyUsed) const;
    Route buildRouteForVehicle(Vehicle* truck, const std::vector<Bin>& waiting,
                               std::vector<bool>& taken);
    void saveRoutes();

public:
    WasteManager(std::string dataDir);
    ~WasteManager();

    // Data loading and saving
    void loadSampleData();
    bool loadFromFiles();
    bool saveToFiles();

    // Module 1: bins
    bool addBin(Bin b);                       // false if the ID is already used
    bool updateBinFill(std::string binId, double waste);
    bool markBinCollected(std::string binId);
    Bin* findBin(std::string binId);
    std::vector<Bin> searchBinsByLocation(std::string text);
    std::vector<std::string> getAllBinIds() const;
    void sortBins(bool byFill);
    void displayBins() const;

    // Module 2: priority queue
    // Returns the bins above the given fill level, most urgent one first
    std::vector<Bin> getUrgentBins(double minFillPercent);

    // Module 3: vehicles
    void addVehicle(Vehicle* v);
    Vehicle* findVehicle(std::string id);
    Vehicle* selectVehicle(double neededCapacity);
    void displayVehicles() const;
    const std::vector<Vehicle*>& getVehicles() const { return vehicles; }

    // Module 4: road network
    void addRoad(std::string from, std::string to, double distance, std::string name);
    bool blockRoad(std::string from, std::string to);
    bool unblockRoad(std::string from, std::string to);
    const Graph& getGraph() const { return roadGraph; }

    // Module 5: route planning.
    // If vehicleId is empty the program uses as many trucks as it needs.
    // If a vehicle is named, only that one truck is sent out.
    std::vector<Route> planRoutes(std::string vehicleId, double threshold);
    Route generateFixedRoute(std::string vehicleId);

    // Module 6: dynamic re-routing.
    // Blocks a road and builds every affected route again.
    bool reRoute(std::string from, std::string to, std::string reason);

    // Module 7: comparison
    Comparison compareRoutes();
    void displayComparison();

    // Module 8: run every planned trip
    bool executeAllRoutes();

    // Module 9: dashboard numbers
    int getTotalBins() const { return bins.size(); }
    int countBinsWithPriority(Priority p) const;
    int getAvailableVehicles() const;
    double getPendingWaste() const;
    void displayDashboard() const;
    void displayRoutes() const;

    const std::vector<Route>& getRoutes() const { return routes; }

    // Used by the web dashboard
    std::string exportJSON() const;
};

#endif
