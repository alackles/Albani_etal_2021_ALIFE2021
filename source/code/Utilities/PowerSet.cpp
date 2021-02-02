#include "PowerSet.h"

// instantiate the static power set collection
std::unordered_map<int, std::vector<std::vector<int>>> PowerSet::PowerSetCollection;

// create a power set recursively from elements in start
void PowerSet::createPowerSet(std::vector<std::vector<int>>& powerSet, std::vector<int> start, std::vector<int> current, int index) {
    if (index == start.size()) { // termination case... do nothing now
        return;
    }

    if (current.size()) { // if current is not empty
        powerSet.push_back(std::vector<int>(current));
    }

    for (int i = index + 1; i < start.size(); i++) { // recursion magic...
        current.push_back(start[i]);
        createPowerSet(powerSet, start, current, i);

        current.pop_back();
    }

    return;
}

// given size, return the power set for the vector [0,size-1]
// this function uses PowerSetCollection so it will not recrate power sets, but rather, just return the prior result
//
// check to see if powerset has been created, if not, create it, sort it and add to PowerSetCollection
// then, return powerset
// note: the empty set is not included in the result
const std::vector<std::vector<int>>& PowerSet::getPowerSet(int size) {

    // check to see if we have already made this power set
    if (PowerSetCollection.find(size) == PowerSetCollection.end()) {
        // make a new powerset
        std::vector<int> start(size);
        std::iota(start.begin(), start.end(), 0); // make a vector with elements [0,1,...,size-2,size-1]
        std::vector<std::vector<int>> powerSet;   // container to hold the power set
        createPowerSet(powerSet, start);

        // sort powerSet elements by size and values in each set
        std::sort(powerSet.begin(), powerSet.end(), [size](const std::vector<int>& a, const std::vector<int>& b) 
            {
                if (a.size() == b.size()) {
                    int v_a = 1;
                    for (auto e : a) {
                        v_a = (v_a * size) + e; // use size here as the 'base' to insure that 'keys' are unique
                    }
                    int v_b = 1;
                    for (auto e : b) {
                        v_b = (v_b * size) + e;
                    }
                    return v_a < v_b;
                }
                else {
                    return a.size() < b.size();
                }
            });

        // add this powerSet to the collection
        PowerSetCollection[size] = powerSet;
    }
    return(PowerSetCollection[size]);
}

