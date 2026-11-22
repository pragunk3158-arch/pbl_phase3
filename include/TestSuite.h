#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include "WasteManager.h"
#include <iostream>
#include <string>
#include <vector>

// Test cases that check the main rules of the system.
// Each test prints PASS or FAIL and the total is shown at the end.

// Prints the result of one test and updates the counters
inline void check(bool condition, std::string name, int& passed, int& total) {
    total++;
    if (condition) {
        passed++;
        std::cout << "[PASS] Test " << total << ": " << name << std::endl;
    } else {
        std::cout << "[FAIL] Test " << total << ": " << name << std::endl;
    }
}

class TestSuite {
public:
    // Returns true only if every test passed
    static bool runAllTests() {
        int passed = 0;
        int total = 0;

        std::cout << std::endl;
        std::cout << "======================================================" << std::endl;
        std::cout << "                 RUNNING TEST CASES                   " << std::endl;
        std::cout << "======================================================" << std::endl;

        // Tests 1 to 4: priority is decided correctly from the fill percentage
        Bin empty("T1", "Lab", 100, 0, 5, "Pending");
        check(empty.getFillPercent() == 0 && empty.getPriority() == LOW,
              "Empty bin is LOW priority", passed, total);

        Bin half("T2", "Lab", 100, 50, 5, "Pending");
        check(half.getFillPercent() == 50 && half.getPriority() == MEDIUM,
              "50 percent full bin is MEDIUM priority", passed, total);

        Bin high("T3", "Lab", 100, 80, 5, "Pending");
        check(high.getFillPercent() == 80 && high.getPriority() == HIGH,
              "80 percent full bin is HIGH priority", passed, total);

        Bin critical("T4", "Lab", 100, 95, 5, "Pending");
        check(critical.getFillPercent() == 95 && critical.getPriority() == CRITICAL,
              "95 percent full bin is CRITICAL priority", passed, total);

        // Test 5: a vehicle that is not available cannot be used
        Vehicle offline("T5", 500, 0, false);
        check(offline.canCarry(50) == false,
              "Unavailable vehicle refuses the load", passed, total);

        // Test 6: a vehicle that is nearly full cannot take more waste
        Vehicle nearlyFull("T6", 200, 180, true);
        check(nearlyFull.canCarry(50) == false,
              "Vehicle without free space refuses the load", passed, total);

        // Test 7: Dijkstra picks the cheaper path, not the direct road
        Graph g1;
        g1.addRoad("A", "B", 4, "r1");
        g1.addRoad("B", "C", 3, "r2");
        g1.addRoad("A", "C", 10, "r3");
        std::vector<std::string> p1;
        double d1 = g1.shortestPath("A", "C", p1);
        check(d1 == 7 && p1.size() == 3 && p1[1] == "B",
              "Dijkstra finds shortest path A-B-C of 7 km", passed, total);

        // Test 8: blocking a road is stored in the graph
        Graph g2;
        g2.addRoad("A", "B", 4, "r1");
        g2.blockRoad("A", "B");
        check(g2.isRoadBlocked("A", "B") == true,
              "Blocked road is marked in the graph", passed, total);

        // Test 9: when a road is blocked, a longer open path is used
        Graph g3;
        g3.addRoad("A", "B", 2, "r1");
        g3.addRoad("B", "C", 2, "r2");
        g3.addRoad("A", "D", 3, "r3");
        g3.addRoad("D", "C", 3, "r4");
        g3.blockRoad("B", "C");
        std::vector<std::string> p3;
        double d3 = g3.shortestPath("A", "C", p3);
        check(d3 == 6 && p3.size() == 3 && p3[1] == "D",
              "Re-routing finds the bypass A-D-C of 6 km", passed, total);

        // Test 10: if the only road is blocked there is no path at all
        Graph g4;
        g4.addRoad("A", "B", 5, "r1");
        g4.blockRoad("A", "B");
        std::vector<std::string> p4;
        double d4 = g4.shortestPath("A", "B", p4);
        check(d4 == -1 && p4.empty(),
              "No path is reported when the only road is blocked", passed, total);

        // Test 11: collecting a bin empties it
        Bin full("T11", "Gym", 100, 90, 5, "Pending");
        full.markCollected();
        check(full.getCurrentWaste() == 0 && full.getStatus() == "Collected" &&
              full.getPriority() == LOW,
              "Collected bin becomes empty and LOW priority", passed, total);

        // Test 12: the data files are read correctly
        WasteManager wm("data");
        check(wm.getTotalBins() >= 10 && wm.getVehicles().size() >= 4 &&
              wm.getGraph().getLocationCount() >= 10,
              "Data files are loaded from disk", passed, total);

        // Test 13: a bin cannot hold more than its capacity
        Bin small("T13", "Lab", 80, 0, 5, "Pending");
        small.setCurrentWaste(500);
        check(small.getCurrentWaste() == 80 && small.getFillPercent() == 100,
              "Waste above the capacity is cut down to the capacity", passed, total);

        // Test 14: two bins cannot share the same ID
        WasteManager manager("data");
        std::vector<std::string> ids = manager.getAllBinIds();
        std::string existingId = ids.empty() ? "B101" : ids[0];
        check(!ids.empty() &&
              manager.addBin(Bin(existingId, "Library", 100, 50, 5, "Pending")) == false,
              "A bin with a repeated ID is refused", passed, total);

        // Test 15: a damaged line in a file is skipped, it does not crash
        Bin damaged = Bin::fromCSV("B999,Lab,abc,10,5,Pending");
        check(damaged.getId() == "",
              "A line with a wrong number is skipped instead of crashing", passed, total);

        // Test 16: a comma inside a name is removed, otherwise the file breaks
        Bin comma("T16", "Block A, Phase 2", 100, 50, 5, "Pending");
        check(comma.getLocation() == "Block A Phase 2",
              "A comma inside a location name is removed", passed, total);

        // Test 17: a road from a place to itself is not accepted
        Graph g5;
        g5.addRoad("A", "A", 5, "self");
        check(g5.getRoadCount() == 0,
              "A road from a location to itself is refused", passed, total);

        // Test 18: when one truck is not enough, a second truck is sent.
        // This manager is given a folder that does not exist, so it works on
        // the sample data in memory only. The tests below change bins and run
        // trips, and that must never touch the real files in the data folder.
        WasteManager fleet("folder_that_does_not_exist");
        std::vector<std::string> allIds = fleet.getAllBinIds();
        for (int i = 0; i < (int)allIds.size(); i++)
            fleet.updateBinFill(allIds[i], 9999);      // fill every bin completely

        std::vector<Route> plan = fleet.planRoutes("", 75);
        double carried = 0;
        for (int i = 0; i < (int)plan.size(); i++)
            carried = carried + plan[i].getTotalWaste();

        check(plan.size() >= 2 && carried > 1000,
              "More than one truck is used when the waste does not fit in one",
              passed, total);

        // Test 19: naming one truck gives exactly one trip
        std::vector<Route> single = fleet.planRoutes("V01", 75);
        check(single.size() == 1 && single[0].getVehicleId() == "V01",
              "Naming a vehicle sends only that one truck", passed, total);

        // Test 20: after the trips are run, the next plan never contains a bin
        // that was just emptied, and those bins are 0 kg
        fleet.loadSampleData();
        std::vector<Route> first = fleet.planRoutes("", 75);
        std::vector<std::string> emptied;
        for (int i = 0; i < (int)first.size(); i++) {
            std::vector<std::string> ids = first[i].getBinIds();
            for (int j = 0; j < (int)ids.size(); j++) emptied.push_back(ids[j]);
        }
        fleet.executeAllRoutes();

        bool allZero = true;
        for (int i = 0; i < (int)emptied.size(); i++) {
            Bin* b = fleet.findBin(emptied[i]);
            if (b == NULL || b->getCurrentWaste() != 0) allZero = false;
        }

        bool repeated = false;
        std::vector<Route> second = fleet.planRoutes("", 75);
        for (int i = 0; i < (int)second.size(); i++) {
            std::vector<std::string> ids = second[i].getBinIds();
            for (int j = 0; j < (int)ids.size(); j++) {
                for (int k = 0; k < (int)emptied.size(); k++) {
                    if (ids[j] == emptied[k]) repeated = true;
                }
            }
        }
        check(allZero && !repeated && !second.empty(),
              "Emptied bins are 0 kg and never come back in the next plan",
              passed, total);

        // Test 21: an empty bin is never part of a route
        fleet.loadSampleData();
        std::vector<std::string> every = fleet.getAllBinIds();
        for (int i = 0; i < (int)every.size(); i++) fleet.updateBinFill(every[i], 0);
        fleet.updateBinFill(every[0], 10);              // only one bin has waste
        std::vector<Route> tiny = fleet.planRoutes("", 75);
        check(tiny.size() == 1 && tiny[0].getBinIds().size() == 1,
              "A route only goes to bins that have waste in them", passed, total);

        std::cout << "------------------------------------------------------" << std::endl;
        std::cout << "RESULT: " << passed << " of " << total << " tests passed" << std::endl;
        std::cout << "======================================================" << std::endl;

        return passed == total;
    }
};

#endif
