#include "../include/WasteManager.h"
#include <cstdio>
#include <ctime>

using namespace std;

WasteManager::WasteManager(string dataDir) {
    this->dataDir = dataDir;
    depot = "Depot";
    comparisonDone = false;

    // If the data files are missing, start with the sample data
    if (!loadFromFiles()) {
        loadSampleData();
        saveToFiles();
    }

    // The routes are kept in their own file so they survive between runs
    routes = FileHandler::loadRoutes(dataDir + "/routes.txt");

    // How many trips were done is simply how many lines the history file has
    completedTrips = (int)FileHandler::loadCollectionLog(dataDir + "/collections.txt").size();
}

WasteManager::~WasteManager() {
    for (int i = 0; i < (int)vehicles.size(); i++)
        delete vehicles[i];
    vehicles.clear();
}

void WasteManager::saveRoutes() {
    FileHandler::saveRoutes(dataDir + "/routes.txt", routes);
}

// ===========================================================================
// SAMPLE DATA
// ===========================================================================
void WasteManager::loadSampleData() {
    routes.clear();
    saveRoutes();

    bins.clear();
    for (int i = 0; i < (int)vehicles.size(); i++) delete vehicles[i];
    vehicles.clear();

    // an empty map again, otherwise roads blocked earlier would stay blocked
    roadGraph = Graph();

    // 10 bins. The comment shows the fill percentage and priority.
    bins.insertEnd(Bin("B101", "GEU Gate",         100, 90,   8,  "Pending")); // 90%  CRITICAL
    bins.insertEnd(Bin("B102", "Hostel",           150, 82.5, 6,  "Pending")); // 55%  MEDIUM
    bins.insertEnd(Bin("B103", "Canteen",          100, 95,   12, "Pending")); // 95%  CRITICAL
    bins.insertEnd(Bin("B104", "Library",           80, 32,   3,  "Pending")); // 40%  LOW
    bins.insertEnd(Bin("B105", "Academic Block",   120, 96,   7,  "Pending")); // 80%  HIGH
    bins.insertEnd(Bin("B106", "Sports Complex",   100, 85,   9,  "Pending")); // 85%  HIGH
    bins.insertEnd(Bin("B107", "Parking",           80, 24,   2,  "Pending")); // 30%  LOW
    bins.insertEnd(Bin("B108", "Market",           150, 138,  14, "Pending")); // 92%  CRITICAL
    bins.insertEnd(Bin("B109", "Main Road",        100, 60,   5,  "Pending")); // 60%  MEDIUM
    bins.insertEnd(Bin("B110", "Residential Area", 120, 54,   4,  "Pending")); // 45%  LOW

    // 4 vehicles of 3 different classes
    vehicles.push_back(new CompactorTruck("V01", 500));      // 625 kg after compaction
    vehicles.push_back(new ElectricMiniTruck("V02", 300));   // 300 kg
    vehicles.push_back(new CompactorTruck("V03", 800));      // 1000 kg after compaction
    vehicles.push_back(new Vehicle("V04", 400));             // 400 kg

    // Road network: 11 locations and 15 roads
    roadGraph.addRoad("Depot",          "GEU Gate",         3.5, "North Link");
    roadGraph.addRoad("Depot",          "Main Road",        4.0, "Central Blvd");
    roadGraph.addRoad("Depot",          "Parking",          2.8, "Service Lane");
    roadGraph.addRoad("GEU Gate",       "Hostel",           2.5, "University Ave");
    roadGraph.addRoad("GEU Gate",       "Market",           3.0, "Commerce Way");
    roadGraph.addRoad("Hostel",         "Canteen",          2.0, "Campus Walk");
    roadGraph.addRoad("Hostel",         "Library",          3.2, "West Pathway");
    roadGraph.addRoad("Canteen",        "Academic Block",   2.2, "Science Walk");
    roadGraph.addRoad("Canteen",        "Library",          1.8, "Central Corridor");
    roadGraph.addRoad("Library",        "Academic Block",   2.5, "Faculty Road");
    roadGraph.addRoad("Academic Block", "Sports Complex",   3.0, "Athletics Way");
    roadGraph.addRoad("Academic Block", "Parking",          4.5, "South Perimeter");
    roadGraph.addRoad("Sports Complex", "Residential Area", 3.8, "Ring Road");
    roadGraph.addRoad("Market",         "Main Road",        2.4, "Market Express");
    roadGraph.addRoad("Main Road",      "Residential Area", 4.2, "South Avenue");
}

bool WasteManager::loadFromFiles() {
    bool a = FileHandler::loadBins(dataDir + "/bins.txt", bins);
    bool b = FileHandler::loadVehicles(dataDir + "/vehicles.txt", vehicles);
    bool c = FileHandler::loadRoads(dataDir + "/roads.txt", roadGraph);
    return a && b && c;
}

bool WasteManager::saveToFiles() {
    bool a = FileHandler::saveBins(dataDir + "/bins.txt", bins);
    bool b = FileHandler::saveVehicles(dataDir + "/vehicles.txt", vehicles);
    bool c = FileHandler::saveRoads(dataDir + "/roads.txt", roadGraph);
    return a && b && c;
}

// ===========================================================================
// MODULE 1: BIN MANAGEMENT
// ===========================================================================
bool WasteManager::addBin(Bin b) {
    // Two bins with the same ID would make findById() unreliable, because it
    // would always return the first one and the second could never be updated.
    if (b.getId() == "" || b.getCapacity() <= 0) return false;
    if (bins.findById(b.getId()) != NULL) return false;

    bins.insertEnd(b);
    saveToFiles();
    return true;
}

bool WasteManager::updateBinFill(string binId, double waste) {
    Bin* b = bins.findById(binId);
    if (b == NULL) return false;

    b->setCurrentWaste(waste);

    // a bin with waste in it waits for a truck, a bin with nothing is empty
    if (b->getCurrentWaste() > 0)
        b->setStatus("Pending");
    else
        b->setStatus("Collected");

    saveToFiles();
    return true;
}

bool WasteManager::markBinCollected(string binId) {
    Bin* b = bins.findById(binId);
    if (b == NULL) return false;

    b->markCollected();
    saveToFiles();
    return true;
}

Bin* WasteManager::findBin(string binId) {
    return bins.findById(binId);
}

vector<Bin> WasteManager::searchBinsByLocation(string text) {
    vector<Bin> result;
    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        if (temp->data.getLocation().find(text) != string::npos)
            result.push_back(temp->data);
        temp = temp->next;
    }
    return result;
}

vector<string> WasteManager::getAllBinIds() const {
    vector<string> ids;
    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        ids.push_back(temp->data.getId());
        temp = temp->next;
    }
    return ids;
}

// Sorts the list using quicksort and puts the sorted bins back
void WasteManager::sortBins(bool byFill) {
    vector<Bin> list = bins.toVector();
    if (list.empty()) return;

    quickSortBins(list, 0, (int)list.size() - 1, byFill);

    bins.clear();
    for (int i = 0; i < (int)list.size(); i++)
        bins.insertEnd(list[i]);
}

void WasteManager::displayBins() const {
    printf("\n=================================================================================\n");
    printf("                              BIN INVENTORY STATUS                               \n");
    printf("=================================================================================\n");
    printf("| %-6s| %-17s| %7s | %7s | %6s | %-9s| %-10s |\n",
           "ID", "Location", "Cap(kg)", "Waste", "Fill %", "Priority", "Status");
    printf("---------------------------------------------------------------------------------\n");

    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        temp->data.displayRow();
        temp = temp->next;
    }
    printf("=================================================================================\n");
}

// ===========================================================================
// MODULE 2: PRIORITY QUEUE
// All matching bins are pushed into the max heap and then popped out,
// which gives them back in order of urgency.
// ===========================================================================
vector<Bin> WasteManager::getUrgentBins(double minFillPercent) {
    BinPriorityQueue pq;

    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        Bin& b = temp->data;

        // A bin is only worth a trip if it is waiting and has some waste in
        // it. Without the waste check, a bin that was set to 0 kg was still
        // "Pending", so the truck drove to empty bins for nothing.
        if (b.getStatus() == "Pending" &&
            b.getCurrentWaste() > 0 &&
            b.getFillPercent() >= minFillPercent) {
            pq.push(b);
        }
        temp = temp->next;
    }

    vector<Bin> result;
    while (!pq.isEmpty())
        result.push_back(pq.pop());
    return result;
}

// ===========================================================================
// MODULE 3: VEHICLE MANAGEMENT
// ===========================================================================
void WasteManager::addVehicle(Vehicle* v) {
    vehicles.push_back(v);
    saveToFiles();
}

Vehicle* WasteManager::findVehicle(string id) {
    for (int i = 0; i < (int)vehicles.size(); i++) {
        if (vehicles[i]->getId() == id) return vehicles[i];
    }
    return NULL;
}

// Picks the smallest truck that can still carry the load, so that a big
// truck is not wasted on a small trip. alreadyUsed marks the trucks that
// are already going out on another route today.
int WasteManager::chooseVehicle(double wasteToCarry, const vector<bool>& alreadyUsed) const {
    int best = -1;

    for (int i = 0; i < (int)vehicles.size(); i++) {
        if (alreadyUsed[i]) continue;
        if (!vehicles[i]->isAvailable()) continue;
        if (vehicles[i]->getFreeSpace() < wasteToCarry) continue;

        if (best == -1 || vehicles[i]->getFreeSpace() < vehicles[best]->getFreeSpace())
            best = i;
    }

    // Nothing is big enough for the whole load, so take the biggest free
    // truck. The bins that do not fit go to the next truck after it.
    if (best == -1) {
        for (int i = 0; i < (int)vehicles.size(); i++) {
            if (alreadyUsed[i]) continue;
            if (!vehicles[i]->isAvailable()) continue;

            if (best == -1 || vehicles[i]->getFreeSpace() > vehicles[best]->getFreeSpace())
                best = i;
        }
    }
    return best;
}

Vehicle* WasteManager::selectVehicle(double neededCapacity) {
    vector<bool> noneUsed(vehicles.size(), false);
    int index = chooseVehicle(neededCapacity, noneUsed);
    if (index == -1) return NULL;
    return vehicles[index];
}

void WasteManager::displayVehicles() const {
    printf("\n=============================================================================================================\n");
    printf("                                               VEHICLE FLEET                                                 \n");
    printf("=============================================================================================================\n");
    printf("| %-6s| %-20s| %8s | %8s | %7s | %9s | %-4s| %-14s| %-10s |\n",
           "ID", "Type", "Cap(kg)", "Eff.Cap", "Loaded", "FreeSpace", "Free", "Location", "Status");
    printf("-------------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < (int)vehicles.size(); i++)
        vehicles[i]->displayRow();

    printf("=============================================================================================================\n");
}

// ===========================================================================
// MODULE 4: ROAD NETWORK
// ===========================================================================
void WasteManager::addRoad(string from, string to, double distance, string name) {
    roadGraph.addRoad(from, to, distance, name);
    saveToFiles();
}

bool WasteManager::blockRoad(string from, string to) {
    bool done = roadGraph.blockRoad(from, to);
    if (done) saveToFiles();
    return done;
}

bool WasteManager::unblockRoad(string from, string to) {
    bool done = roadGraph.unblockRoad(from, to);
    if (done) saveToFiles();
    return done;
}

// ===========================================================================
// MODULE 5: ROUTE PLANNING
//
// Builds one trip for one truck:
//   1. take the waiting bins that still fit inside this truck
//   2. from the depot, always drive to the nearest bin that is left
//      (nearest neighbour), the distances come from Dijkstra
//   3. come back to the depot
// The bins that are really visited are marked as taken, so the next truck
// does not go to them again.
// ===========================================================================
Route WasteManager::buildRouteForVehicle(Vehicle* truck, const vector<Bin>& waiting,
                                         vector<bool>& taken) {
    double freeSpace = truck->getFreeSpace();

    // 1. which bins fit in this truck
    vector<int> chosen;
    double planned = 0;
    for (int i = 0; i < (int)waiting.size(); i++) {
        if (taken[i]) continue;
        if (planned + waiting[i].getCurrentWaste() > freeSpace) continue;

        chosen.push_back(i);
        planned = planned + waiting[i].getCurrentWaste();
    }
    if (chosen.empty()) return Route();

    // 2. nearest neighbour order
    vector<bool> visited(chosen.size(), false);
    vector<string> stops;
    vector<string> binIds;
    vector<string> binLocations;
    double load = 0;

    string current = depot;
    stops.push_back(depot);

    for (int step = 0; step < (int)chosen.size(); step++) {
        int nearest = -1;
        double nearestDistance = 0;

        for (int i = 0; i < (int)chosen.size(); i++) {
            if (visited[i]) continue;

            vector<string> path;
            double d = roadGraph.shortestPath(current, waiting[chosen[i]].getLocation(), path);
            if (d < 0) continue;              // no road to that bin

            if (nearest == -1 || d < nearestDistance) {
                nearestDistance = d;
                nearest = i;
            }
        }
        if (nearest == -1) break;             // the rest cannot be reached

        visited[nearest] = true;
        int binIndex = chosen[nearest];
        taken[binIndex] = true;

        current = waiting[binIndex].getLocation();
        stops.push_back(current);
        binIds.push_back(waiting[binIndex].getId());
        binLocations.push_back(waiting[binIndex].getLocation());
        load = load + waiting[binIndex].getCurrentWaste();
    }

    if (binIds.empty()) return Route();

    // 3. back to the depot
    stops.push_back(depot);

    vector<string> fullPath;
    double distance = roadGraph.buildTour(stops, fullPath);

    return Route(truck->getId(), fullPath, binIds, binLocations, distance, load);
}

// Plans the trips for today. One truck is sent out, and if urgent bins are
// still waiting another truck is sent after it, until no bin or no truck
// is left.
vector<Route> WasteManager::planRoutes(string vehicleId, double threshold) {
    routes.clear();

    vector<Bin> waiting = getUrgentBins(threshold);
    if (waiting.empty()) waiting = getUrgentBins(50);   // nothing urgent, take medium bins
    if (waiting.empty()) waiting = getUrgentBins(0);    // still nothing, take every pending bin

    if (waiting.empty()) {
        saveRoutes();
        return routes;
    }

    vector<bool> taken(waiting.size(), false);
    vector<bool> truckUsed(vehicles.size(), false);

    while (true) {
        // how much waste is still waiting for a truck
        double leftOver = 0;
        for (int i = 0; i < (int)waiting.size(); i++) {
            if (!taken[i]) leftOver = leftOver + waiting[i].getCurrentWaste();
        }
        if (leftOver <= 0) break;             // every bin has a truck

        // which truck goes next
        int index = -1;
        if (vehicleId != "") {
            if (!routes.empty()) break;       // a named truck makes one trip only
            for (int i = 0; i < (int)vehicles.size(); i++) {
                if (vehicles[i]->getId() == vehicleId) index = i;
            }
        } else {
            index = chooseVehicle(leftOver, truckUsed);
        }
        if (index == -1) break;               // no truck is free any more

        truckUsed[index] = true;
        Route trip = buildRouteForVehicle(vehicles[index], waiting, taken);
        if (trip.getBinIds().empty()) break;  // this truck could not take anything

        routes.push_back(trip);
    }

    saveRoutes();
    return routes;
}

// Old style route: one truck follows the same order every single day and
// stops at every location, no matter how full the bins are.
Route WasteManager::generateFixedRoute(string vehicleId) {
    vector<string> stops;
    stops.push_back("Depot");
    stops.push_back("GEU Gate");
    stops.push_back("Hostel");
    stops.push_back("Canteen");
    stops.push_back("Library");
    stops.push_back("Academic Block");
    stops.push_back("Sports Complex");
    stops.push_back("Residential Area");
    stops.push_back("Main Road");
    stops.push_back("Market");
    stops.push_back("Parking");
    stops.push_back("Depot");

    vector<string> fullPath;
    double distance = roadGraph.buildTour(stops, fullPath);

    vector<string> binIds;
    vector<string> binLocations;
    double totalWaste = 0;

    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        binIds.push_back(temp->data.getId());
        binLocations.push_back(temp->data.getLocation());
        totalWaste = totalWaste + temp->data.getCurrentWaste();
        temp = temp->next;
    }

    string chosenId = (vehicleId == "") ? "V01" : vehicleId;
    return Route(chosenId, fullPath, binIds, binLocations, distance, totalWaste);
}

// ===========================================================================
// MODULE 6: DYNAMIC RE-ROUTING
//
// Nothing is printed inside this function on purpose. The web dashboard
// reads the JSON output of the program, and any extra text would break it.
// ===========================================================================
bool WasteManager::reRoute(string from, string to, string reason) {
    reason = cleanText(reason);
    blockRoad(from, to);

    bool changed = false;

    for (int i = 0; i < (int)routes.size(); i++) {
        if (!routes[i].usesRoad(from, to)) continue;   // this truck is not affected
        changed = true;

        // Build the trip again for the same bins. Dijkstra skips blocked
        // roads, so it finds a way around the closed road by itself.
        vector<string> stops;
        stops.push_back(depot);
        vector<string> locations = routes[i].getBinLocations();
        for (int j = 0; j < (int)locations.size(); j++)
            stops.push_back(locations[j]);
        stops.push_back(depot);

        vector<string> newPath;
        double newDistance = roadGraph.buildTour(stops, newPath);

        if (newDistance < 0) {
            routes[i].setReRouted(true, "No way around the closed road " + from + "-" + to);
            continue;
        }

        routes[i] = Route(routes[i].getVehicleId(), newPath,
                          routes[i].getBinIds(), routes[i].getBinLocations(),
                          newDistance, routes[i].getTotalWaste(),
                          true, reason + " (" + from + " - " + to + ")");
    }

    if (changed) saveRoutes();
    return changed;
}

// ===========================================================================
// MODULE 7: FIXED ROUTE VS SMART PLAN
// ===========================================================================
Comparison WasteManager::compareRoutes() {
    // planRoutes() replaces the plan, so keep a copy of what is there now
    vector<Route> planBefore = routes;

    Route fixed = generateFixedRoute("V01");
    vector<Route> smart = planRoutes("", 75);

    Comparison c;
    c.fixedDistance = fixed.getTotalDistance();
    c.fixedTime     = fixed.getEstimatedTime();
    c.fixedBins     = (int)fixed.getBinIds().size();
    c.fixedWaste    = fixed.getTotalWaste();

    // the smart side is the total of every truck that goes out
    c.smartDistance = 0;
    c.smartTime     = 0;
    c.smartBins     = 0;
    c.smartWaste    = 0;
    c.smartTrucks   = (int)smart.size();

    for (int i = 0; i < (int)smart.size(); i++) {
        c.smartDistance = c.smartDistance + smart[i].getTotalDistance();
        c.smartTime     = c.smartTime + smart[i].getEstimatedTime();
        c.smartBins     = c.smartBins + (int)smart[i].getBinIds().size();
        c.smartWaste    = c.smartWaste + smart[i].getTotalWaste();
    }

    c.distanceSaved = c.fixedDistance - c.smartDistance;
    c.timeSaved     = c.fixedTime - c.smartTime;
    c.fuelSaved     = c.distanceSaved * 0.28;   // a compactor truck uses about 0.28 L/km

    if (c.fixedDistance > 0)
        c.efficiencyGain = (c.distanceSaved / c.fixedDistance) * 100.0;
    else
        c.efficiencyGain = 0;

    lastComparison = c;
    comparisonDone = true;

    // put the old plan back
    routes = planBefore;
    saveRoutes();
    return c;
}

void WasteManager::displayComparison() {
    Comparison c = compareRoutes();

    printf("\n=================================================================\n");
    printf("            FIXED ROUTE  vs  SMART PLAN COMPARISON               \n");
    printf("=================================================================\n");
    printf("| %-19s | %16s | %20s |\n", "Metric", "Fixed Route", "Smart Plan");
    printf("-----------------------------------------------------------------\n");
    printf("| %-19s | %13.1f km | %17.1f km |\n", "Travel Distance", c.fixedDistance, c.smartDistance);
    printf("| %-19s | %11.1f mins | %15.1f mins |\n", "Travel Time", c.fixedTime, c.smartTime);
    printf("| %-19s | %16d | %20d |\n", "Bins Visited", c.fixedBins, c.smartBins);
    printf("| %-19s | %13.1f kg | %17.1f kg |\n", "Waste Collected", c.fixedWaste, c.smartWaste);
    printf("| %-19s | %16d | %20d |\n", "Trucks Used", 1, c.smartTrucks);
    printf("=================================================================\n");
    printf("  Distance saved  : %.1f km\n", c.distanceSaved);
    printf("  Time saved      : %.1f mins\n", c.timeSaved);
    printf("  Fuel saved      : %.2f litres\n", c.fuelSaved);
    printf("  Efficiency gain : %.1f %%\n", c.efficiencyGain);
    printf("=================================================================\n");
    printf("  The fixed route sends one truck to all %d bins in the same\n", c.fixedBins);
    printf("  order every day. The smart plan sends %d truck(s) to the %d\n", c.smartTrucks, c.smartBins);
    printf("  urgent bins only, so it is shorter, but the bins that are not\n");
    printf("  full yet are left for a later day.\n");
    printf("  The travel time is the total of all trucks. In real life the\n");
    printf("  trucks drive at the same time, so the day would finish sooner.\n");
    printf("=================================================================\n");
}

// ===========================================================================
// MODULE 8: RUN THE PLANNED TRIPS
// ===========================================================================
bool WasteManager::executeAllRoutes() {
    if (routes.empty()) return false;

    time_t now = time(NULL);
    char timeText[32];
    strftime(timeText, sizeof(timeText), "%Y-%m-%d %H:%M", localtime(&now));

    for (int i = 0; i < (int)routes.size(); i++) {
        if (routes[i].getBinIds().empty()) continue;

        Vehicle* v = findVehicle(routes[i].getVehicleId());
        if (v != NULL) {
            v->loadWaste(routes[i].getTotalWaste());
            // The truck drives back to the depot and empties itself there,
            // so it is ready for the next day.
            v->resetLoad();
        }

        // The bins are emptied here directly instead of calling
        // markBinCollected(), because that function saves all the files
        // after every single bin.
        vector<string> ids = routes[i].getBinIds();
        for (int j = 0; j < (int)ids.size(); j++) {
            Bin* b = bins.findById(ids[j]);
            if (b != NULL) b->markCollected();
        }

        string log = string(timeText) + " | Vehicle: " + routes[i].getVehicleId() +
                     " | Distance: " + numberToText(routes[i].getTotalDistance()) + " km" +
                     " | Waste: " + numberToText(routes[i].getTotalWaste()) + " kg" +
                     " | Bins: " + FileHandler::joinList(ids);

        FileHandler::addCollectionLog(dataDir + "/collections.txt", log);
        completedTrips++;
    }

    // the trips are over, so there is no plan any more
    routes.clear();
    saveRoutes();
    saveToFiles();
    return true;
}

// ===========================================================================
// MODULE 9: DASHBOARD
// ===========================================================================
int WasteManager::countBinsWithPriority(Priority p) const {
    int count = 0;
    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        if (temp->data.getPriority() == p && temp->data.getStatus() == "Pending")
            count++;
        temp = temp->next;
    }
    return count;
}

int WasteManager::getAvailableVehicles() const {
    int count = 0;
    for (int i = 0; i < (int)vehicles.size(); i++) {
        if (vehicles[i]->isAvailable()) count++;
    }
    return count;
}

double WasteManager::getPendingWaste() const {
    double total = 0;
    BinNode* temp = bins.getHead();
    while (temp != NULL) {
        if (temp->data.getStatus() == "Pending")
            total = total + temp->data.getCurrentWaste();
        temp = temp->next;
    }
    return total;
}

void WasteManager::displayDashboard() const {
    printf("\n====================================================================\n");
    printf("     INTELLIGENT WASTE COLLECTION AND ROUTE OPTIMIZATION SYSTEM     \n");
    printf("====================================================================\n");
    printf(" Total Bins : %-4d |  Critical : %-4d |  High : %d\n",
           getTotalBins(), countBinsWithPriority(CRITICAL), countBinsWithPriority(HIGH));
    printf(" Free Trucks: %d of %-2d |  Pending Waste : %.0f kg |  Trips Done : %d\n",
           getAvailableVehicles(), (int)vehicles.size(), getPendingWaste(), completedTrips);
    printf("====================================================================\n");

    if (!routes.empty()) {
        printf(" Planned today: %d truck(s)\n", (int)routes.size());
        for (int i = 0; i < (int)routes.size(); i++) {
            printf("   %s : %.1f km, %d bins%s\n",
                   routes[i].getVehicleId().c_str(), routes[i].getTotalDistance(),
                   (int)routes[i].getBinIds().size(),
                   routes[i].isReRouted() ? "  [RE-ROUTED]" : "");
        }
        printf("====================================================================\n");
    }
}

void WasteManager::displayRoutes() const {
    if (routes.empty()) {
        printf("\nNo trips are planned. Use the route planning menu first.\n");
        return;
    }

    double distance = 0;
    double waste = 0;
    for (int i = 0; i < (int)routes.size(); i++) {
        routes[i].displaySummary();
        routes[i].displayPath();
        distance = distance + routes[i].getTotalDistance();
        waste = waste + routes[i].getTotalWaste();
    }

    printf("\nTOTAL: %d truck(s), %.2f km, %.1f kg of waste\n",
           (int)routes.size(), distance, waste);
}

// Puts quotes around every item so that it can be written in a JSON list
static vector<string> quoteAll(const vector<string>& list) {
    vector<string> result;
    for (int i = 0; i < (int)list.size(); i++)
        result.push_back("\"" + list[i] + "\"");
    return result;
}

// ===========================================================================
// JSON OUTPUT FOR THE WEB DASHBOARD
// The web page runs the program and reads this text.
// Every name was already cleaned when it was typed in, so there are no
// quotes inside it that could break the JSON.
// ===========================================================================
string WasteManager::exportJSON() const {
    string js = "{";

    js = js + "\"metrics\":{";
    js = js + "\"totalBins\":" + numberToText(getTotalBins()) + ",";
    js = js + "\"criticalBins\":" + numberToText(countBinsWithPriority(CRITICAL)) + ",";
    js = js + "\"highBins\":" + numberToText(countBinsWithPriority(HIGH)) + ",";
    js = js + "\"availableVehicles\":" + numberToText(getAvailableVehicles()) + ",";
    js = js + "\"pendingWaste\":" + numberToText(getPendingWaste()) + ",";
    js = js + "\"completedTrips\":" + numberToText(completedTrips) + "},";

    // bins
    js = js + "\"bins\":[";
    BinNode* temp = bins.getHead();
    bool first = true;
    while (temp != NULL) {
        if (!first) js = js + ",";
        first = false;
        js = js + "{\"id\":\"" + temp->data.getId() + "\",";
        js = js + "\"location\":\"" + temp->data.getLocation() + "\",";
        js = js + "\"capacity\":" + numberToText(temp->data.getCapacity()) + ",";
        js = js + "\"currentWaste\":" + numberToText(temp->data.getCurrentWaste()) + ",";
        js = js + "\"fillPercent\":" + numberToText(temp->data.getFillPercent()) + ",";
        js = js + "\"priority\":\"" + priorityToString(temp->data.getPriority()) + "\",";
        js = js + "\"status\":\"" + temp->data.getStatus() + "\"}";
        temp = temp->next;
    }
    js = js + "],";

    // vehicles
    js = js + "\"vehicles\":[";
    for (int i = 0; i < (int)vehicles.size(); i++) {
        if (i > 0) js = js + ",";
        js = js + "{\"id\":\"" + vehicles[i]->getId() + "\",";
        js = js + "\"type\":\"" + vehicles[i]->getTypeName() + "\",";
        js = js + "\"capacity\":" + numberToText(vehicles[i]->getCapacity()) + ",";
        js = js + "\"effectiveCapacity\":" + numberToText(vehicles[i]->getEffectiveCapacity()) + ",";
        js = js + "\"currentLoad\":" + numberToText(vehicles[i]->getCurrentLoad()) + ",";
        js = js + "\"available\":" + string(vehicles[i]->isAvailable() ? "true" : "false") + ",";
        js = js + "\"location\":\"" + vehicles[i]->getLocation() + "\",";
        js = js + "\"status\":\"" + vehicles[i]->getStatus() + "\"}";
    }
    js = js + "],";

    // roads
    js = js + "\"roads\":[";
    first = true;
    for (int i = 0; i < roadGraph.getLocationCount(); i++) {
        for (int j = i + 1; j < roadGraph.getLocationCount(); j++) {
            if (roadGraph.getDistance(i, j) <= 0) continue;
            if (!first) js = js + ",";
            first = false;
            js = js + "{\"from\":\"" + roadGraph.getLocationName(i) + "\",";
            js = js + "\"to\":\"" + roadGraph.getLocationName(j) + "\",";
            js = js + "\"distance\":" + numberToText(roadGraph.getDistance(i, j)) + ",";
            js = js + "\"blocked\":" + string(roadGraph.getBlocked(i, j) ? "true" : "false") + ",";
            js = js + "\"name\":\"" + roadGraph.getRoadName(i, j) + "\"}";
        }
    }
    js = js + "],";

    // the planned trips, one for each truck
    double totalDistance = 0;
    double totalWaste = 0;

    js = js + "\"routes\":[";
    for (int i = 0; i < (int)routes.size(); i++) {
        if (i > 0) js = js + ",";
        js = js + "{\"vehicleId\":\"" + routes[i].getVehicleId() + "\",";
        js = js + "\"totalDistance\":" + numberToText(routes[i].getTotalDistance()) + ",";
        js = js + "\"estimatedTime\":" + numberToText(routes[i].getEstimatedTime()) + ",";
        js = js + "\"totalWaste\":" + numberToText(routes[i].getTotalWaste()) + ",";
        js = js + "\"reRouted\":" + string(routes[i].isReRouted() ? "true" : "false") + ",";
        js = js + "\"reRouteReason\":\"" + routes[i].getReRouteReason() + "\",";
        js = js + "\"path\":[" + FileHandler::joinList(quoteAll(routes[i].getPath())) + "],";
        js = js + "\"bins\":[" + FileHandler::joinList(quoteAll(routes[i].getBinIds())) + "]}";

        totalDistance = totalDistance + routes[i].getTotalDistance();
        totalWaste = totalWaste + routes[i].getTotalWaste();
    }
    js = js + "],";

    js = js + "\"routeTotals\":{\"trucks\":" + numberToText((int)routes.size()) + ",";
    js = js + "\"distance\":" + numberToText(totalDistance) + ",";
    js = js + "\"waste\":" + numberToText(totalWaste) + "},";

    // comparison, only if it was calculated in this run
    js = js + "\"comparison\":";
    if (!comparisonDone) {
        js = js + "null";
    } else {
        js = js + "{\"fixedDistance\":" + numberToText(lastComparison.fixedDistance) + ",";
        js = js + "\"fixedTime\":" + numberToText(lastComparison.fixedTime) + ",";
        js = js + "\"fixedBins\":" + numberToText(lastComparison.fixedBins) + ",";
        js = js + "\"fixedWaste\":" + numberToText(lastComparison.fixedWaste) + ",";
        js = js + "\"smartDistance\":" + numberToText(lastComparison.smartDistance) + ",";
        js = js + "\"smartTime\":" + numberToText(lastComparison.smartTime) + ",";
        js = js + "\"smartBins\":" + numberToText(lastComparison.smartBins) + ",";
        js = js + "\"smartWaste\":" + numberToText(lastComparison.smartWaste) + ",";
        js = js + "\"smartTrucks\":" + numberToText(lastComparison.smartTrucks) + ",";
        js = js + "\"distanceSaved\":" + numberToText(lastComparison.distanceSaved) + ",";
        js = js + "\"timeSaved\":" + numberToText(lastComparison.timeSaved) + ",";
        js = js + "\"fuelSaved\":" + numberToText(lastComparison.fuelSaved) + ",";
        js = js + "\"efficiencyGain\":" + numberToText(lastComparison.efficiencyGain) + "}";
    }

    js = js + "}";
    return js;
}
