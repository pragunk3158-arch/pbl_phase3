#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <cstdio>
#include <cstdlib>

// Priority of a bin, decided from its fill percentage
//   90% and above -> CRITICAL
//   75% to 89%    -> HIGH
//   50% to 74%    -> MEDIUM
//   below 50%     -> LOW
enum Priority { LOW, MEDIUM, HIGH, CRITICAL };

inline std::string priorityToString(Priority p) {
    if (p == CRITICAL) return "CRITICAL";
    if (p == HIGH) return "HIGH";
    if (p == MEDIUM) return "MEDIUM";
    return "LOW";
}

// ===========================================================================
// SMALL HELPERS USED ALL OVER THE PROJECT
// ===========================================================================

// Turns text into a number and says whether it worked.
// stod() stops the whole program if the text is not a number, so this is
// used instead. One damaged line in a data file should not crash the system.
inline bool textToNumber(std::string text, double& value) {
    if (text == "") return false;

    for (int i = 0; i < (int)text.size(); i++) {
        char c = text[i];
        bool allowed = (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+';
        if (!allowed) return false;
    }

    value = atof(text.c_str());
    return true;
}

// Turns a number into short text: 100 stays "100" and 82.5 stays "82.5",
// instead of the "100.000000" that to_string() would give.
inline std::string numberToText(double value) {
    char buffer[32];
    sprintf(buffer, "%g", value);
    return std::string(buffer);
}

// Removes the characters that would damage the data files or the JSON
// output. A location written as "Block A, Phase 2" would otherwise be read
// back as two different fields and break every later run of the program.
inline std::string cleanText(std::string text) {
    std::string result = "";
    for (int i = 0; i < (int)text.size(); i++) {
        char c = text[i];
        if (c == ',' || c == '"' || c == '\\' || c == '\r' || c == '\n') continue;
        result = result + c;
    }
    return result;
}

// Breaks a line like "B101,Hostel,100" into parts and returns how many
// parts were found. Used everywhere instead of stringstream.
inline int splitLine(std::string line, std::string parts[], int maxParts) {
    int count = 0;
    std::string word = "";

    for (int i = 0; i < (int)line.size(); i++) {
        char c = line[i];
        if (c == '\r') continue;

        if (c == ',') {
            if (count < maxParts) parts[count] = word;
            count++;
            word = "";
        } else {
            word = word + c;
        }
    }

    if (count < maxParts) parts[count] = word;
    count++;
    return count;
}

#endif
