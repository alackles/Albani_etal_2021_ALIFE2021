#include "fragmentation.h"
#include <iostream>


#include <bitset>


/*****************************************************************************
 * 							FRAGMENTATION CODE
 *****************************************************************************/

// takes an iterator to a vector (start) and a list of indices from that vector and returns
// a vector with the values at those indices
std::vector< int > fragmentation::indicesToValues(std::vector<int>::const_iterator start, const std::vector<int> & indices)
{
	std::vector<int> values;
	values.reserve(indices.size());

	for (const auto& index : indices)
	{
		values.push_back(*(start+index));
	}
	
	return values;
}

/**
 * Gets Fragmentation measure for one feature (of arbatrary size)
 * returns pair with
 *    (first) size of smallest partition with at least threshold % shared entropy with feature
 *      a value of -1 in (first) indicates that no partition contains atleast threshold shared entropy relitive to feature
 *    (second) the ratio of total shared entropy between the entire source and featrue
 * @param source: vector of states containing the source data (i.e. what is used to make prediction)
 *     source and fettures must be the same size AND each element of source must be the same size AND each element of features must be the same size
 * @param features: vector of states containing the data we are want to predict (i.e. we will ask how well source predicts features)
 * @param featureIndexPair: the positions in the features that demark the start and end of this feature
 * @param threshold: the first partition of source that has atleast this amount of shared entropy with feature as compaired with features total entropy will trigger a return
 * @param compareToFeature: If true, function works as decribed. If false, threshold comparison is made agaist max shared entropy as aposed to feature entropy (i.e. it will always succed unless feature entropy is 0)
 * @param maxPartitionSize: max size of partitions of source to consider, if -1 (defaut) consider all partitions
 **/
std::pair<int,double> fragmentation::calcFragmentation(const std::vector<std::vector<int>>& source, const std::vector<std::vector<int>>& features, std::pair<size_t, size_t> featureIndexPair, double threshold, bool compareToFeature, int maxPartitionSize) {

	if (source.size() != features.size()) {
		std::cout << "  in fragmentation::calcFragmentation :: worldStateSet.size() != brainStateSet.size() - this is a problem! exiting..." << std::endl;
		exit(1);
	}

	double updateWeight = 1.0 / (source.size()); // ratio of all samples that any one sample represents

	std::map<int,int> featureProb;
	std::map<int, int> wholeBrainProb;
	std::map<int, int> maxJointProb;
	for (int j = 0; j < source.size(); j++) {
		featureProb[neurocorrelates::vector1PartToInt(source[j].begin() + featureIndexPair.first, source[j].begin() + featureIndexPair.second + 1)] ++;
		wholeBrainProb[neurocorrelates::vector1PartToInt(features[j].begin(), features[j].end())] ++;
		maxJointProb[neurocorrelates::vector2PartsToInt(source[j].begin() + featureIndexPair.first, source[j].begin() + featureIndexPair.second + 1,
			features[j].begin(), features[j].end())]++;
	}
	double featrueEntropy = neurocorrelates::calcEntropy(featureProb, updateWeight);
	double wholeBrainEntropy = neurocorrelates::calcEntropy(wholeBrainProb, updateWeight); // this is the entorpy of the whole brain
	double maxJointEntropy = neurocorrelates::calcEntropy(maxJointProb, updateWeight);
	double maxSharedEntropy = (featrueEntropy + wholeBrainEntropy) - maxJointEntropy; // this is the max known by the brain about this feature

	double maxEntropyRatio = 0;
	if (featrueEntropy > 0) {
		maxEntropyRatio = maxSharedEntropy/featrueEntropy;
	}
	else {
		return { -1,0 }; // there is no entropy in feature, so we can just stop now
	}

	//std::cout << "running fragmentation::calcFragmentation - feature index pair: " << featureIndexPair.first << ", " << featureIndexPair.second << std::endl;
	//std::cout << "     featrueEntropy = " << featrueEntropy << "   maxJointEntropy = " << maxJointEntropy << std::endl;
	//std::cout << "     maxSharedEntropy = ( " << featrueEntropy << " + " << wholeBrainEntropy << " ) - " << maxJointEntropy << " = " << maxSharedEntropy << std::endl;
	//std::cout << "     maxEntropyRatio = " << maxSharedEntropy << " / " << featrueEntropy << " = " << maxEntropyRatio << std::endl;

	if (maxPartitionSize == -1 || maxPartitionSize > source[0].size()) {
		maxPartitionSize = source[0].size();
	}

	// get power set for all combinations of source (partitions)
	PowerSet ps;
	auto indexSets = ps.getPowerSet(maxPartitionSize);

	std::map<int, int> featureProbs; // map of frequency of values from feature
	std::map<int, int> jointProbs; // map of frequency of values from source partition and features

	// test each power set until we find a powerset with suffect shared entropy
	for (const auto& indexSet : indexSets){

		featureProbs.clear();
		jointProbs.clear();

		for (int i = 0; i < source.size(); i++) {
			// create probability map for this feature
			std::vector<int> featureRow = indicesToValues(features[i].begin(), indexSet);
			featureProbs[neurocorrelates::vector1PartToInt(featureRow.begin(), featureRow.end())] ++;

			// create probability map for joint for this source patition and feature
			std::vector<int> jointList = { source[i].begin() + featureIndexPair.first, source[i].begin() + featureIndexPair.second + 1 };
			jointList.insert(jointList.end(), featureRow.begin(), featureRow.end());
			jointProbs[neurocorrelates::vector1PartToInt(jointList.begin(), jointList.end())]++;
		}

		double brainEntropy = neurocorrelates::calcEntropy(featureProbs, updateWeight);
		double jointEntropy = neurocorrelates::calcEntropy(jointProbs, updateWeight);

		if (compareToFeature) {
			if ((brainEntropy + featrueEntropy) - jointEntropy >= threshold * featrueEntropy) { // if what we have left after we remove joint entorpy is = featrue entropy then...
				return { indexSet.size(), maxEntropyRatio };
			}
		}
		else { // !compareToFeature, i.e. compare to maxSharedEntropy
			if ((brainEntropy + featrueEntropy) - jointEntropy >= threshold * maxSharedEntropy) { // if what this brain partition knows about everything the brain knows about the feature then...
				return { indexSet.size(), maxEntropyRatio };
			}
		}
	}

	// if we don't find a good partition... so return -1 to indicate a fail and the max shared entropy between source and feature (i.e. min to get a positive result)
	return {-1, maxEntropyRatio};
}

// takes stateSet (vect<vect> int with environment and brain state per world update), a list of feature sizes, the size of brain state
// and threshold (a cutoff for fragmentation).
// Returns a vect with the fragmentation value for each feature
std::vector<std::pair<int, double>> fragmentation::getFragmentationSet(const std::vector<std::vector<int>>& worldStateSet, const std::vector<std::vector<int>>& brainStateSet, std::vector<std::pair<size_t, size_t>> enviromentFeatures, double threshold, bool compareToFeature) {
	std::vector<std::pair<int, double>> fragSet(enviromentFeatures.size());
	for (int i = 0; i < enviromentFeatures.size(); i++) {
		fragSet[i] = calcFragmentation(worldStateSet, brainStateSet, enviromentFeatures[i], threshold, compareToFeature);
	}
	return(fragSet);
}

// this function takes two information streams "source" and "features" and calculates how much each partition of source knows about the features.
//
// given a featureSet, a sourceSet and a list identifing which elements in the featuresSet should be considered together (featuresIndexPairs)
// first create a power set of index lists for all partitions of sourceSet (excluding the empty set)
// next determin the shared entropy between each partition of sourceSet and each feature as defined by featuresSet and featuresIndexPairs
// return the power set and the matrix of shared entropies
std::pair<std::vector<std::vector<int>>, std::vector<std::vector<double>>> fragmentation::getFragmentationMap(const std::vector<std::vector<int>>& featuresSet, const std::vector<std::vector<int>>& sourceSet, std::vector<std::pair<size_t, size_t>> featuresIndexPairs, int maxPartitionSize, std::string compareTo) {
	std::cout << "creating Fragmentation Map" << std::endl;
	
	if (featuresSet.size() != sourceSet.size()) {
		std::cout << "  in fragmentation::getFragmentationMap :: featuresSet.size() != sourceSet.size() - this is a problem! exiting..." << std::endl;
		exit(1);
	}
	
	// get the source partitions (power set, excluding sets with more then maxPartitionSize)
	PowerSet ps;
	std::vector<std::vector<int>> sourcePartitions = ps.getPowerSet(sourceSet[0].size());
	int i = 0;
	while (sourcePartitions[i].size() <= maxPartitionSize) {
		i++;
	}
	sourcePartitions.resize(std::max(i, (int)sourcePartitions.size()));


	std::vector<std::vector<double>> FragMap; // a matrix used to how the shared info for each partition and feature


	double updateWeight = 1.0 / (featuresSet.size()); // 1/(number of samples), this is the weight per sample used in calcEntropy

	// collect counts for the number of times each unique state is observed in the source data.
	std::map<int, int> wholeSourceStatesCounts;
	for (int sample = 0; sample < sourceSet.size(); sample++) {
		wholeSourceStatesCounts[neurocorrelates::vector1PartToInt(sourceSet[sample].begin(), sourceSet[sample].end())] ++;
	}
	double wholeSourceEntropy = neurocorrelates::calcEntropy(wholeSourceStatesCounts, updateWeight); // this is the entorpy of the whole source


	// for each feature - where a featurs is one or more elements from featuresSet as determined by featuresIndexPairs
	for (int featureID = 0; featureID < featuresIndexPairs.size(); featureID++) {

		FragMap.push_back({}); // add a row to FragMap

		// get entropies related to this feature
		// collect counts for the number of times each unique state is observed in the feature isolated
		// AND
		// collect counts for the number of times each unique state is observed in the feature joint with the whole source ( neede to get the max joint entropy ) 
		std::map<int, int> featureStatesCounts;
		std::map<int, int> maxJointStatesCounts;
		for (int sample = 0; sample < featuresSet.size(); sample++) {
			featureStatesCounts[neurocorrelates::vector1PartToInt(featuresSet[sample].begin() + featuresIndexPairs[featureID].first, featuresSet[sample].begin() + featuresIndexPairs[featureID].second + 1)] ++;
			maxJointStatesCounts[neurocorrelates::vector2PartsToInt(
				featuresSet[sample].begin() + featuresIndexPairs[featureID].first, featuresSet[sample].begin() + featuresIndexPairs[featureID].second + 1, // feature values
				sourceSet[sample].begin(), sourceSet[sample].end())]++; // source values

			/*
			std::cout << " XXX " << featuresSet[sample][featureID] << "   ";
			for (auto e : sourceSet[sample]) {
				std::cout << e << "  ";
			}
			std::cout << std::endl;
			auto temptemp = neurocorrelates::vector2PartsToInt(
				featuresSet[sample].begin() + featuresIndexPairs[featureID].first, featuresSet[sample].begin() + featuresIndexPairs[featureID].second + 1, // feature values
				sourceSet[sample].begin(), sourceSet[sample].end());
			std::cout << " YYY " << std::bitset<8 * sizeof(temptemp)>(temptemp) << std::endl;
			*/

		}

		double featrueEntropy = neurocorrelates::calcEntropy(featureStatesCounts, updateWeight);
		double maxJointEntropy = neurocorrelates::calcEntropy(maxJointStatesCounts, updateWeight);
		double maxSharedEntropy = (featrueEntropy + wholeSourceEntropy) - maxJointEntropy; // this is the max known by the source about this feature

		/*
		for (auto e : featureStatesCounts) {
			std::cout << "  "  << std::bitset<8 * sizeof(e.first)>(e.first) << " appears " << e.second << " times in featureStatesCounts" << std::endl;
		}
		for (auto e : maxJointStatesCounts) {
			std::cout << "  " << std::bitset<8 * sizeof(e.first)>(e.first) << " appears " << e.second << " times in maxJointStatesCounts" << std::endl;
		}
		*/
		std::cout << "  working on feature " << featureID << "  (" << featuresIndexPairs[featureID].first << "," << featuresIndexPairs[featureID].second << ")" <<
			"\n    samples: " << featuresSet.size() << "  feautre vector size = " << featuresSet[0].size() << "  source data size = " << sourceSet[0].size() <<
			"\n    featureEntropy = " << featrueEntropy << "  maxSharedEntropy between feature and source = " << maxSharedEntropy << std::endl;

		std::map<int, int> partitionStatesCounts; // map with values for this source partition
		std::map<int, int> jointStatesCounts; // map with values for this feature and this source partition 
		// test each power set
		for (const auto& partitionIndexSet : sourcePartitions) {
			partitionStatesCounts.clear();
			jointStatesCounts.clear();

			for (int sample = 0; sample < sourceSet.size(); sample++) {
				// create StatesCounts map for this source partition
				std::vector<int> partitionRow = indicesToValues(sourceSet[sample].begin(), partitionIndexSet); // extract values from source[sample] for this partition
				partitionStatesCounts[neurocorrelates::vector1PartToInt(partitionRow.begin(), partitionRow.end())] ++;

				// create StatesCounts map for joint for this source partition and this feature
				std::vector<int> jointList = { featuresSet[sample].begin() + featuresIndexPairs[featureID].first, featuresSet[sample].begin() + featuresIndexPairs[featureID].second + 1 };
				jointList.insert(jointList.end(), partitionRow.begin(), partitionRow.end());
				jointStatesCounts[neurocorrelates::vector1PartToInt(jointList.begin(), jointList.end())]++;
			}

			double partitionEntropy = neurocorrelates::calcEntropy(partitionStatesCounts, updateWeight);
			double jointEntropy = neurocorrelates::calcEntropy(jointStatesCounts, updateWeight);
			if (compareTo == "feature") {
				if (featrueEntropy > 0) { // nomalize to featrueEntropy
					FragMap.back().push_back(((partitionEntropy + featrueEntropy) - jointEntropy) / featrueEntropy);
				}
				else {
					FragMap.back().push_back(0.0);
				}
			}
			else if (compareTo == "source") {  // nomalize to maxSharedEntropy (i.e. what the source knows about the feature)
				if (maxSharedEntropy > 0) {
					FragMap.back().push_back(((partitionEntropy + featrueEntropy) - jointEntropy) / maxSharedEntropy);
				}
				else {
					FragMap.back().push_back(0.0);
				}
			}
			else {
				std::cout << "  In fragmentation::getFragmentationMap recived bad compairTo method: " << compareTo << " . exiting..." << std::endl;
				exit(1);
			}
		}
		// last 2 elemets of fag map will always be the entropy of this feature and
		// the max shared entropy of this featrue and whole source (i.e. largest partition of source)
		FragMap.back().push_back(featrueEntropy);
		FragMap.back().push_back(maxSharedEntropy);
	}
	std::pair<std::vector<std::vector<int>>, std::vector<std::vector<double>>> result({ sourcePartitions, FragMap });
	std::cout << "  done" << std::endl;
	return(result);
}

