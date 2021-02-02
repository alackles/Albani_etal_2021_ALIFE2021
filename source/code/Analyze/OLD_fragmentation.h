#pragma once


#include <vector>
#include <map>
#include "powerSet.h"
#include "neurocorrelates.h"

/*****************************************************************************
 * 							FRAGMENTATION CODE
 *****************************************************************************/
namespace fragmentation {
	std::vector< int > indicesToValues(std::vector<int>::const_iterator start, const std::vector<int>& indices);

	/**
	 * Gets Fragmentation measure for certain world characteristic
	 * @param stateSet: set of states at each update containing the environment bits (what you want to compare with), and brain bits
	 * @param environmentBits: number of bits in the environment
	 * @param memoryBits: number of bits in the brain
	 * @param threshold: percentage of entropy to acchieve
	 **/
	std::pair<int, double> calcFragmentation(const std::vector<std::vector<int>>& worldStateSet, const std::vector<std::vector<int>>& brainStateSet, std::pair<size_t, size_t> environmentPartitionIndex, double threshold, bool compareToFeature = true, int maxPartitionSize = -1);
	std::vector<std::pair<int, double>> getFragmentationSet(const std::vector<std::vector<int>>& worldStateSet, const std::vector<std::vector<int>>& brainStateSet, std::vector<std::pair<size_t, size_t>> enviromentFeatures, double threshold, bool compareToFeature = true);

	std::pair<std::vector<std::vector<int>>,std::vector<std::vector<double>>> getFragmentationMap(const std::vector<std::vector<int>>& worldStateSet, const std::vector<std::vector<int>>& brainStateSet, std::vector<std::pair<size_t, size_t>> worldFeatures, int maxPartitionSize = 1, std::string compareTo = "source");
}