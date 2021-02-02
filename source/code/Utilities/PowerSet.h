#pragma once
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <iostream>

class PowerSet {
    // a static container that holds all the power set we have made so far
    static std::unordered_map<int, std::vector<std::vector<int>>> PowerSetCollection;

    // create a power set from a vector of elements
    void createPowerSet(std::vector<std::vector<int>>& powerSet, std::vector<int> start, std::vector<int> current = std::vector<int>(), int index = -1);
public:

    // given size, return the power set for the vector [0,size-1]
    // this function uses PowerSetCollection so it will not recrate power sets, but rather, just return the prior result
    const std::vector<std::vector<int>>& getPowerSet(int size);
};
