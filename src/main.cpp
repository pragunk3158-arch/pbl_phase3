#include "../include/Common.h"
#include "../include/WasteManager.h"
#include "../include/TestSuite.h"
#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>

using namespace std;

// ---------------------------------------------------------------
// Small helper functions used by the menus
// ---------------------------------------------------------------

// Throws away whatever is left in the input line
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void pause() {
    cout << endl << "Press Enter to continue...";
    cin.get();
}

void printTitle(string title) {
    cout << endl;
    cout << "====================================================================" << endl;
    cout << "  " << title << endl;
    cout << "====================================================================" << endl;
}

// Reads a number typed by the user. Returns -1 if it was not a number.
// If the input has finished (for example the program was started with a
// file piped into it), 0 is returned so that the menu closes instead of
// repeating for ever.
int readChoice() {
    int value;
    if (!(cin >> value)) {
        if (cin.eof()) return 0;
        clearInput();
        return -1;
    }
    clearInput();
    return value;
}

double readNumber(string question) {
    double value;
    cout << question;
    while (!(cin >> value)) {
        if (cin.eof()) return 0;
        clearInput();
        cout << "Please enter a number: ";
    }
    clearInput();
    return value;
}

string readText(string question) {
    string text;
    cout << question;
    getline(cin, text);
    return text;
}

// ---------------------------------------------------------------
// MENU 1: BIN MANAGEMENT
// ---------------------------------------------------------------
void binMenu(WasteManager& wm) {
    int choice = -1;
    while (choice != 0) {
        printTitle("MODULE 1: BIN MANAGEMENT");
        cout << "1. View all bins" << endl;
        cout << "2. Add a new bin" << endl;
        cout << "3. Update the waste in a bin" << endl;
        cout << "4. Search bins by location" << endl;
        cout << "5. Search a bin by ID" << endl;
        cout << "6. Sort bins by fill percentage (quicksort)" << endl;
        cout << "7. Sort bins by urgency score (quicksort)" << endl;
        cout << "8. Mark a bin as collected" << endl;
        cout << "0. Back to main menu" << endl;
        cout << endl << "Enter choice: ";
        choice = readChoice();

        if (choice == 1) {
            wm.displayBins();
            pause();
        } else if (choice == 2) {
            string id = readText("Bin ID (example B111)     : ");
            string loc = readText("Location (example Library): ");
            double cap = readNumber("Capacity in kg            : ");
            double waste = readNumber("Current waste in kg       : ");
            double rate = readNumber("Fill rate in kg per day   : ");

            if (cap <= 0) {
                cout << "Capacity must be more than 0 kg. Bin not added." << endl;
            } else if (!wm.addBin(Bin(id, loc, cap, waste, rate, "Pending"))) {
                cout << "A bin with the ID " << id << " already exists." << endl;
            } else {
                cout << "Bin " << id << " added." << endl;
                // A bin at a place with no road to it can never be collected
                if (!wm.getGraph().hasLocation(cleanText(loc))) {
                    cout << endl;
                    cout << "Warning: \"" << loc << "\" is not on the road map yet," << endl;
                    cout << "so no truck can reach this bin. Add a road to it from" << endl;
                    cout << "menu 4 (Road network) to include it in the routes." << endl;
                }
            }
            pause();
        } else if (choice == 3) {
            string id = readText("Bin ID to update: ");
            double waste = readNumber("New waste in kg : ");
            if (wm.updateBinFill(id, waste))
                cout << "Bin " << id << " updated." << endl;
            else
                cout << "Bin ID not found." << endl;
            pause();
        } else if (choice == 4) {
            string text = readText("Location to search: ");
            vector<Bin> found = wm.searchBinsByLocation(text);
            cout << "Found " << found.size() << " bin(s)." << endl;
            for (int i = 0; i < (int)found.size(); i++)
                found[i].displayRow();
            pause();
        } else if (choice == 5) {
            string id = readText("Bin ID: ");
            Bin* b = wm.findBin(id);
            if (b != NULL) b->displayDetails();
            else cout << "Bin not found." << endl;
            pause();
        } else if (choice == 6) {
            wm.sortBins(true);
            cout << "Bins sorted by fill percentage (highest first)." << endl;
            wm.displayBins();
            pause();
        } else if (choice == 7) {
            wm.sortBins(false);
            cout << "Bins sorted by urgency score (most urgent first)." << endl;
            wm.displayBins();
            pause();
        } else if (choice == 8) {
            string id = readText("Bin ID to mark collected: ");
            if (wm.markBinCollected(id))
                cout << "Bin marked as collected." << endl;
            else
                cout << "Bin not found." << endl;
            pause();
        }
    }
}

// ---------------------------------------------------------------
// MENU 2: VEHICLE MANAGEMENT
// ---------------------------------------------------------------
void vehicleMenu(WasteManager& wm) {
    printTitle("MODULE 2: VEHICLE MANAGEMENT");
    wm.displayVehicles();

    cout << endl;
    cout << "1. Find a suitable vehicle for a given load" << endl;
    cout << "2. Add a new vehicle" << endl;
    cout << "3. Empty all vehicles" << endl;
    cout << "0. Back" << endl;
    cout << endl << "Enter choice: ";
    int choice = readChoice();

    if (choice == 1) {
        double need = readNumber("Waste to carry in kg: ");
        Vehicle* v = wm.selectVehicle(need);
        if (v != NULL)
            cout << "Use vehicle " << v->getId() << " (" << v->getTypeName()
                 << "), free space " << v->getFreeSpace() << " kg." << endl;
        else
            cout << "No vehicle is free right now." << endl;
        pause();
    } else if (choice == 2) {
        string id = readText("Vehicle ID: ");
        cout << "Type (1 = Compactor, 2 = Electric Mini, 3 = Standard): ";
        int type = readChoice();
        double cap = readNumber("Capacity in kg: ");

        if (type == 1)      wm.addVehicle(new CompactorTruck(id, cap));
        else if (type == 2) wm.addVehicle(new ElectricMiniTruck(id, cap));
        else                wm.addVehicle(new Vehicle(id, cap));

        cout << "Vehicle " << id << " added." << endl;
        pause();
    } else if (choice == 3) {
        const vector<Vehicle*>& list = wm.getVehicles();
        for (int i = 0; i < (int)list.size(); i++)
            list[i]->resetLoad();
        wm.saveToFiles();
        cout << "All vehicles are empty and available again." << endl;
        pause();
    }
}

// ---------------------------------------------------------------
// MENU 3: ROUTE PLANNING
// ---------------------------------------------------------------
void routeMenu(WasteManager& wm) {
    printTitle("MODULE 3: SMART ROUTE PLANNING (MULTIPLE TRUCKS)");
    cout << "1. Plan trips for urgent bins (75 percent and above)" << endl;
    cout << "2. Plan trips for medium bins (50 percent and above)" << endl;
    cout << "3. Run the planned trips (collect the waste)" << endl;
    cout << "0. Back" << endl;
    cout << endl << "Enter choice: ";
    int choice = readChoice();

    if (choice == 1 || choice == 2) {
        double threshold = (choice == 1) ? 75.0 : 50.0;

        // An empty answer means the program uses as many trucks as it needs
        cout << endl << "Enter one vehicle ID to send only that truck," << endl;
        string vehicleId = readText("or press Enter to use as many trucks as needed: ");

        vector<Route> plan = wm.planRoutes(vehicleId, threshold);
        if (plan.empty())
            cout << endl << "No bin needs collection right now." << endl;
        else
            wm.displayRoutes();
        pause();
    } else if (choice == 3) {
        if (wm.executeAllRoutes())
            cout << "All trips finished. Bins emptied and logged in data/collections.txt" << endl;
        else
            cout << "No trips planned yet. Plan the routes first." << endl;
        pause();
    }
}

// ---------------------------------------------------------------
// MENU 4: ROAD NETWORK
// ---------------------------------------------------------------
void graphMenu(WasteManager& wm) {
    printTitle("MODULE 4: ROAD NETWORK");
    wm.getGraph().display();

    cout << endl;
    cout << "1. Find the shortest path between two locations" << endl;
    cout << "2. Add a new road" << endl;
    cout << "0. Back" << endl;
    cout << endl << "Enter choice: ";
    int choice = readChoice();

    if (choice == 1) {
        string from = readText("From: ");
        string to = readText("To  : ");
        vector<string> path;
        double distance = wm.getGraph().shortestPath(from, to, path);

        if (distance < 0) {
            cout << "No open path between these two locations." << endl;
        } else {
            cout << "Shortest distance: " << distance << " km" << endl;
            cout << "Path: ";
            for (int i = 0; i < (int)path.size(); i++) {
                cout << path[i];
                if (i + 1 < (int)path.size()) cout << " -> ";
            }
            cout << endl;
        }
        pause();
    } else if (choice == 2) {
        string from = readText("Location 1: ");
        string to = readText("Location 2: ");
        double distance = readNumber("Distance in km: ");
        string name = readText("Road name: ");
        wm.addRoad(from, to, distance, name);
        cout << "Road added." << endl;
        pause();
    }
}

// ---------------------------------------------------------------
// MENU 5: DYNAMIC RE-ROUTING
// ---------------------------------------------------------------
void reRouteMenu(WasteManager& wm) {
    printTitle("MODULE 5: DYNAMIC RE-ROUTING");
    cout << "1. Block a road and build the route again" << endl;
    cout << "2. Open a blocked road" << endl;
    cout << "0. Back" << endl;
    cout << endl << "Enter choice: ";
    int choice = readChoice();

    if (choice == 1) {
        string from = readText("Location 1 (example Hostel) : ");
        string to = readText("Location 2 (example Canteen): ");
        string reason = readText("Reason (example Road Repair): ");
        if (reason == "") reason = "Road blocked";

        // add up the distance of every truck before and after the block
        double oldTotal = 0;
        for (int i = 0; i < (int)wm.getRoutes().size(); i++)
            oldTotal = oldTotal + wm.getRoutes()[i].getTotalDistance();

        if (wm.reRoute(from, to, reason)) {
            double newTotal = 0;
            for (int i = 0; i < (int)wm.getRoutes().size(); i++)
                newTotal = newTotal + wm.getRoutes()[i].getTotalDistance();

            cout << endl << "The affected trips were planned again." << endl;
            cout << "Old total distance : " << oldTotal << " km" << endl;
            cout << "New total distance : " << newTotal << " km" << endl;
            cout << "Extra travel       : " << (newTotal - oldTotal) << " km" << endl;
            wm.displayRoutes();
        } else {
            cout << "Road is blocked now, but no planned trip used it." << endl;
        }
        pause();
    } else if (choice == 2) {
        string from = readText("Location 1: ");
        string to = readText("Location 2: ");
        if (wm.unblockRoad(from, to))
            cout << "Road is open again." << endl;
        else
            cout << "That road was not found." << endl;
        pause();
    }
}

// ---------------------------------------------------------------
// MENU 8: FILES
// ---------------------------------------------------------------
void fileMenu(WasteManager& wm) {
    printTitle("MODULE 8: DATA FILES");
    cout << "1. Save all data to the text files" << endl;
    cout << "2. Load the data again from the text files" << endl;
    cout << "3. Reset everything to the sample data" << endl;
    cout << "0. Back" << endl;
    cout << endl << "Enter choice: ";
    int choice = readChoice();

    if (choice == 1) {
        wm.saveToFiles();
        cout << "Saved to data/bins.txt, data/vehicles.txt and data/roads.txt" << endl;
        pause();
    } else if (choice == 2) {
        wm.loadFromFiles();
        cout << "Data loaded from the files." << endl;
        pause();
    } else if (choice == 3) {
        wm.loadSampleData();
        wm.saveToFiles();
        cout << "Sample data restored." << endl;
        pause();
    }
}

// ---------------------------------------------------------------
// DEMO: runs the full story in one go, useful during the viva
// ---------------------------------------------------------------
void runDemo(WasteManager& wm) {
    printTitle("DEMO SCENARIO");

    cout << "Step 1: bins loaded in the system" << endl;
    wm.displayBins();

    cout << endl << "Step 2: priority queue puts the urgent bins first" << endl;
    vector<Bin> urgent = wm.getUrgentBins(50);
    for (int i = 0; i < (int)urgent.size(); i++) {
        cout << "  " << (i + 1) << ". " << urgent[i].getId() << " at "
             << urgent[i].getLocation() << " - "
             << urgent[i].getFillPercent() << "% ("
             << priorityToString(urgent[i].getPriority()) << ")" << endl;
    }

    cout << endl << "Step 3: the trips are planned, one for each truck needed" << endl;
    wm.planRoutes("", 75);
    wm.displayRoutes();

    cout << endl << "Step 4: the road Hostel - Canteen gets blocked" << endl;
    wm.reRoute("Hostel", "Canteen", "Road repair at Campus Walk");
    wm.displayRoutes();

    cout << endl << "Step 5: fixed route compared with the smart route" << endl;
    wm.displayComparison();

    // open the road again so the next demo starts from a clean state
    wm.unblockRoad("Hostel", "Canteen");

    cout << endl << "Demo finished." << endl;
}

// ---------------------------------------------------------------
// COMMAND LINE MODE
// The web server starts the program with these options and reads the
// JSON that is printed. Returns true if an option was handled.
// ---------------------------------------------------------------
bool handleCommandLine(WasteManager& wm, int argc, char* argv[]) {
    string option = argv[1];

    if (option == "--test") {
        TestSuite::runAllTests();
        return true;
    }
    if (option == "--demo") {
        runDemo(wm);
        return true;
    }
    if (option == "--json") {
        cout << wm.exportJSON() << endl;
        return true;
    }
    if (option == "--api") {
        string action = (argc > 2) ? argv[2] : "";

        if (action == "smart")
            wm.planRoutes((argc >= 4) ? argv[3] : "", 75);
        else if (action == "block" && argc >= 5)
            wm.blockRoad(argv[3], argv[4]);
        else if (action == "unblock" && argc >= 5)
            wm.unblockRoad(argv[3], argv[4]);
        else if (action == "reroute" && argc >= 5)
            wm.reRoute(argv[3], argv[4], (argc >= 6) ? argv[5] : "Road blocked");
        else if (action == "update" && argc >= 5)
            wm.updateBinFill(argv[3], atof(argv[4]));
        else if (action == "addbin" && argc >= 7)
            wm.addBin(Bin(argv[3], argv[4], atof(argv[5]), atof(argv[6]), 5, "Pending"));
        else if (action == "collect")
            wm.executeAllRoutes();
        else if (action == "compare")
            wm.compareRoutes();
        else if (action == "reset") {
            wm.loadSampleData();
            wm.saveToFiles();
        }

        cout << wm.exportJSON() << endl;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------
int main(int argc, char* argv[]) {
    WasteManager wm("data");

    if (argc > 1) {
        if (handleCommandLine(wm, argc, argv)) return 0;
    }

    int choice = -1;
    while (choice != 0) {
        wm.displayDashboard();

        cout << endl << "MAIN MENU" << endl;
        cout << "  1. Bin management" << endl;
        cout << "  2. Vehicle management" << endl;
        cout << "  3. Smart route planning" << endl;
        cout << "  4. Road network" << endl;
        cout << "  5. Dynamic re-routing" << endl;
        cout << "  6. Fixed route vs smart route" << endl;
        cout << "  7. Collection history" << endl;
        cout << "  8. Data files" << endl;
        cout << "  9. Run the test cases" << endl;
        cout << " 10. Run the demo scenario" << endl;
        cout << "  0. Exit" << endl;
        cout << endl << "Enter option (0-10): ";

        choice = readChoice();

        if (choice == 1) binMenu(wm);
        else if (choice == 2) vehicleMenu(wm);
        else if (choice == 3) routeMenu(wm);
        else if (choice == 4) graphMenu(wm);
        else if (choice == 5) reRouteMenu(wm);
        else if (choice == 6) {
            printTitle("MODULE 6: FIXED ROUTE VS SMART ROUTE");
            wm.displayComparison();
            pause();
        }
        else if (choice == 7) {
            printTitle("MODULE 7: COLLECTION HISTORY");
            vector<string> history = FileHandler::loadCollectionLog("data/collections.txt");
            if (history.empty()) {
                cout << "No trips have been recorded yet." << endl;
            } else {
                for (int i = 0; i < (int)history.size(); i++)
                    cout << "  " << history[i] << endl;
            }
            pause();
        }
        else if (choice == 8) fileMenu(wm);
        else if (choice == 9) {
            TestSuite::runAllTests();
            pause();
        }
        else if (choice == 10) {
            runDemo(wm);
            pause();
        }
        else if (choice == 0) {
            cout << endl << "Closing the system. Goodbye!" << endl;
        }
        else {
            cout << "Invalid option. Please enter a number from 0 to 10." << endl;
            pause();
        }
    }

    return 0;
}
