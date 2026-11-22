#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "Common.h"
#include "Bin.h"
#include "Vehicle.h"
#include "Graph.h"
#include "Route.h"
#include "DataStructures.h"
#include <fstream>
#include <string>
#include <vector>

// Saves and loads the data in plain text files.
// No database is used, only ifstream and ofstream.
// Lines starting with # are comments and are skipped.
class FileHandler {
public:
    static bool loadBins(std::string path, BinList& bins) {
        std::ifstream file(path.c_str());
        if (!file.is_open()) return false;

        bins.clear();
        std::string line;
        while (getline(file, line)) {
            if (line == "" || line[0] == '#') continue;
            Bin b = Bin::fromCSV(line);
            if (b.getId() != "") bins.insertEnd(b);
        }
        file.close();
        return true;
    }

    static bool saveBins(std::string path, const BinList& bins) {
        std::ofstream file(path.c_str());
        if (!file.is_open()) return false;

        file << "# BinID,Location,Capacity(kg),CurrentWaste(kg),FillRate(kg/day),Status\n";
        BinNode* temp = bins.getHead();
        while (temp != NULL) {
            file << temp->data.toCSV() << "\n";
            temp = temp->next;
        }
        file.close();
        return true;
    }

    static bool loadVehicles(std::string path, std::vector<Vehicle*>& vehicles) {
        std::ifstream file(path.c_str());
        if (!file.is_open()) return false;

        // free the old objects before loading new ones
        for (int i = 0; i < (int)vehicles.size(); i++) delete vehicles[i];
        vehicles.clear();

        std::string line;
        while (getline(file, line)) {
            if (line == "" || line[0] == '#') continue;
            Vehicle* v = Vehicle::fromCSV(line);
            if (v != NULL) vehicles.push_back(v);
        }
        file.close();
        return true;
    }

    static bool saveVehicles(std::string path, const std::vector<Vehicle*>& vehicles) {
        std::ofstream file(path.c_str());
        if (!file.is_open()) return false;

        file << "# VehicleID,Type,Capacity,CurrentLoad,Available,Location,Status,Extra\n";
        for (int i = 0; i < (int)vehicles.size(); i++)
            file << vehicles[i]->toCSV() << "\n";
        file.close();
        return true;
    }

    static bool loadRoads(std::string path, Graph& graph) {
        std::ifstream file(path.c_str());
        if (!file.is_open()) return false;

        std::string line;
        while (getline(file, line)) {
            if (line == "" || line[0] == '#') continue;
            graph.addRoadFromCSV(line);
        }
        file.close();
        return true;
    }

    static bool saveRoads(std::string path, const Graph& graph) {
        std::ofstream file(path.c_str());
        if (!file.is_open()) return false;

        file << "# From,To,Distance(km),Blocked,RoadName\n";
        std::vector<std::string> lines = graph.toCSVLines();
        for (int i = 0; i < (int)lines.size(); i++)
            file << lines[i] << "\n";
        file.close();
        return true;
    }

    // ---------------------------------------------------------------
    // The planned routes have to be saved too, because the web dashboard
    // starts the program again for every click. Without this file the new
    // run would not know which routes were planned a moment ago.
    //
    // File layout: the first line is how many routes follow, and then
    // every route takes 8 lines.
    // ---------------------------------------------------------------

    static bool saveRoutes(std::string path, const std::vector<Route>& routes) {
        std::ofstream file(path.c_str());
        if (!file.is_open()) return false;

        file << routes.size() << "\n";
        for (int i = 0; i < (int)routes.size(); i++) {
            file << routes[i].getVehicleId() << "\n";
            file << numberToText(routes[i].getTotalDistance()) << "\n";
            file << numberToText(routes[i].getTotalWaste()) << "\n";
            file << (routes[i].isReRouted() ? 1 : 0) << "\n";
            file << routes[i].getReRouteReason() << "\n";
            file << joinList(routes[i].getPath()) << "\n";
            file << joinList(routes[i].getBinIds()) << "\n";
            file << joinList(routes[i].getBinLocations()) << "\n";
        }
        file.close();
        return true;
    }

    static std::vector<Route> loadRoutes(std::string path) {
        std::vector<Route> routes;

        std::ifstream file(path.c_str());
        if (!file.is_open()) return routes;

        std::string countLine;
        getline(file, countLine);

        double howMany;
        if (!textToNumber(countLine, howMany)) return routes;

        for (int i = 0; i < (int)howMany; i++) {
            std::string vehicleId, distance, waste, reRouted, reason;
            std::string pathLine, binIdLine, binLocationLine;

            getline(file, vehicleId);
            getline(file, distance);
            getline(file, waste);
            getline(file, reRouted);
            getline(file, reason);
            getline(file, pathLine);
            getline(file, binIdLine);
            getline(file, binLocationLine);

            double km, kg;
            if (!textToNumber(distance, km)) break;
            if (!textToNumber(waste, kg)) kg = 0;

            routes.push_back(Route(cleanText(vehicleId), splitList(pathLine),
                                   splitList(binIdLine), splitList(binLocationLine),
                                   km, kg, reRouted == "1", cleanText(reason)));
        }
        file.close();
        return routes;
    }

    // Joins {"A","B"} into "A,B"
    static std::string joinList(const std::vector<std::string>& list) {
        std::string text = "";
        for (int i = 0; i < (int)list.size(); i++) {
            if (i > 0) text = text + ",";
            text = text + list[i];
        }
        return text;
    }

    // Splits "A,B" back into {"A","B"}
    static std::vector<std::string> splitList(std::string text) {
        std::vector<std::string> list;
        std::string word = "";
        for (int i = 0; i < (int)text.size(); i++) {
            if (text[i] == '\r') continue;
            if (text[i] == ',') {
                list.push_back(word);
                word = "";
            } else {
                word = word + text[i];
            }
        }
        if (word != "") list.push_back(word);
        return list;
    }

    // Adds one line at the end of the history file
    static bool addCollectionLog(std::string path, std::string record) {
        std::ofstream file(path.c_str(), std::ios::app);
        if (!file.is_open()) return false;
        file << record << "\n";
        file.close();
        return true;
    }

    static std::vector<std::string> loadCollectionLog(std::string path) {
        std::vector<std::string> history;
        std::ifstream file(path.c_str());
        if (!file.is_open()) return history;

        std::string line;
        while (getline(file, line)) {
            if (line != "" && line[0] != '#') history.push_back(line);
        }
        file.close();
        return history;
    }
};

#endif
