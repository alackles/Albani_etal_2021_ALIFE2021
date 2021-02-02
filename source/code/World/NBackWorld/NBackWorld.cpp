//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

// Evaluates agents on how many '1's they can output. This is a purely fixed
// task
// that requires to reactivity to stimuli.
// Each correct '1' confers 1.0 point to score, or the decimal output determined
// by 'mode'.

#include "NBackWorld.h"

std::shared_ptr<ParameterLink<int>> NBackWorld::evaluationsPerGenerationPL =
Parameters::register_parameter("WORLD_NBACK-evaluationsPerGeneration", 10, "Number of times to evaluate each agent per generation");

std::shared_ptr<ParameterLink<int>> NBackWorld::testsPerEvaluationPL =
Parameters::register_parameter("WORLD_NBACK-testsPerEvaluation", 10, "Number of times to test each agent per evaluation");

std::shared_ptr<ParameterLink<std::string>> NBackWorld::NsListsPL =
Parameters::register_parameter("WORLD_NBACK-NsList", (std::string)"1,2,3:100|2,3,4:-1", "comma seperated list of n values followed by ':' and a time\n"
	"more then one list can be defined seperated by '|'. The last list time must be -1 (i.e. forever)\n"
	"eg: 1,2,3:100|2,3,4:-1");

std::shared_ptr<ParameterLink<int>> NBackWorld::scoreMultPL =
Parameters::register_parameter("WORLD_NBACK-scoreMult", 1, "score multiplier");

std::shared_ptr<ParameterLink<int>> NBackWorld::RMultPL =
Parameters::register_parameter("WORLD_NBACK-RMult", 1, "score R multiplier");

std::shared_ptr<ParameterLink<int>> NBackWorld::delayOutputEvalPL =
Parameters::register_parameter("WORLD_NBACK-delayOutputEval", 0, "generation delay for ouput evalutation");

std::shared_ptr<ParameterLink<std::string>> NBackWorld::groupNamePL =
Parameters::register_parameter("WORLD_NBACK-groupNameSpace", (std::string) "root::", "namespace of group to be evaluated");
std::shared_ptr<ParameterLink<std::string>> NBackWorld::brainNamePL =
Parameters::register_parameter("WORLD_NBACK-brainNameSpace", (std::string) "root::", "namespace for parameters used to define brain");

NBackWorld::NBackWorld(std::shared_ptr<ParametersTable> PT_) : AbstractWorld(PT_) {
	std::vector<std::string> NListsBreakDown1; // used to parse nLists
	std::vector<std::string> NListsBreakDown2; // used to parse nLists

	convertCSVListToVector(NsListsPL->get(PT), NListsBreakDown1, '|'); // lists (i.e. Ns to score + time) are sperated by '|'

	int temp = 0;

	for (auto elem : NListsBreakDown1) {
		convertCSVListToVector(elem, NListsBreakDown2, ':'); // list of Ns is sperated from time with a ':'
		convertString(NListsBreakDown2[1], temp); // get the time for this list
		if (NListSwitchTimes.size() == 0) { // if this is the first list, then put this time on NListSwitchTimes
			NListSwitchTimes.push_back(temp);
		}
		else if (temp > 0) { // else, if it's not -1 (i.e. last list), put this time + sum of previous times
			NListSwitchTimes.push_back(NListSwitchTimes.back() + temp);
		}
		else { // else it's -1, this is the last list (note, if the first list has time -1, that is handled by the if)
			NListSwitchTimes.push_back(-1);
		}
		NListLists.push_back({}); // add a blank list so we have a container to fill
		convertCSVListToVector(NListsBreakDown2[0], NListLists.back(), ','); // fill the container we just added to NListLists
	}
	
	int nextOut = 0;
	
	std::cout << "testing Lists will change on updates:";
	for (auto elem : NListSwitchTimes) {
		std::cout << "  " << elem;
	}
	std::cout << std::endl;

	std::cout << "testing Lists:\n";
	for (auto elem : NListLists) {
		for (auto elem2 : elem) {
			std::cout << "  " << elem2;
			largestN = std::max(largestN, elem2);
			if (!N2OutMap.count(elem2)) {
				N2OutMap[elem2] = nextOut++;
			}
		}
		std::cout << std::endl;
	}

	std::cout << "  largest N found was : " << largestN << ". Brains will be run for this number of world steps before testing begins." << std::endl;

	// now get currentLargestN

	for (auto elem : NListLists[0]) {
		currentLargestN = std::max(currentLargestN, elem);
	}

	evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT); // each agent sees this number of inputs (+largest N) and is scored this number of times each evaluation
	testsPerEvaluation = testsPerEvaluationPL->get(PT); // each agent is reset and evaluated this number of times

	std::cout << "output map:\n";
	for (auto elem : N2OutMap) {
		std::cout << "  N: " << elem.first << " <- output: " << elem.second << std::endl;
	}
	std::cout << "brains will have 1 input and " << N2OutMap.size() << " outputs." << std::endl;

	groupName = groupNamePL->get(PT);
	brainName = brainNamePL->get(PT);

	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("score");
	popFileColumns.push_back("R");
	for (auto elem : N2OutMap) {
		popFileColumns.push_back("nBack_" + std::to_string(elem.first));
		std::cout << "adding: " << "nBack_" + std::to_string(elem.first) << std::endl;
	}
}

void NBackWorld::evaluate(std::map<std::string, std::shared_ptr<Group>> &groups, int analyze, int visualize, int debug) {
	// check to see if we need to advance to next NsList
	if (Global::update >= NListSwitchTimes[currentNList] && NListSwitchTimes[currentNList] != -1) {
		currentNList += 1;
		//std::cout << "advancing to next list... " << currentNList << std::endl;
		currentLargestN = 0;
		for (auto elem : NListLists[currentNList]) {
			currentLargestN = std::max(currentLargestN, elem);
		}
	}

	int popSize = groups[groupNamePL->get(PT)]->population.size();
	for (int i = 0; i < popSize; i++) {
		evaluateSolo(groups[groupNamePL->get(PT)]->population[i], analyze, visualize, debug);
	}
	if (analyze) {
		groups[groupNamePL->get(PT)]->archive();
	}
}

void NBackWorld::evaluateSolo(std::shared_ptr<Organism> org, int analyze, int visualize, int debug) {
		
	auto brain = org->brains[brainName];
	brain->setRecordActivity(true);

	double score = 0.0;
	std::vector<int> tallies(N2OutMap.size(), 0); // how many times did brain get each N in current list correct?
	std::vector<int> inputList(testsPerEvaluation + currentLargestN, 0);

	std::vector<std::vector<int>> worldStates;

	for (int r = 0; r < evaluationsPerGeneration; r++) {
		brain->resetBrain();

		int t = 0;


		for (int t = 0; t < inputList.size(); t++) {

			inputList[t] = Random::getInt(1);
			brain->setInput(0, inputList[t]);

			brain->update();

			// collect score and world data but only once we have reached currentLargestN
			if (t >= currentLargestN) {
				// add space in world data vector
				worldStates.push_back({});
				for (auto elem : NListLists[currentNList]) {
					worldStates.back().push_back(inputList[t - elem + 1]);
					if (Global::update >= delayOutputEvalPL->get(PT) && // if update is greater than delay time
						Bit(brain->readOutput(N2OutMap[elem])) == inputList[t - elem]) { // if output is correct 
						score += 1; // add 1 to score
						tallies[N2OutMap[elem]] += 1; // add 1 to correct outputs for this N
					}
				}
			}
		}
	}
	
	org->dataMap.append("score", (score*scoreMultPL->get(PT)) / (evaluationsPerGeneration*testsPerEvaluation*NListLists[currentNList].size()));
	// score is divided by number of evals * number of tests * number of N's in current list

	for (auto elem : N2OutMap) {
		org->dataMap.append("nBack_" + std::to_string(elem.first), (double)tallies[elem.second] / (double)(evaluationsPerGeneration*testsPerEvaluation));
		// since this is for one N at a time, it's just divided by number of evals * number of tests
	}

	if (visualize) {
		std::cout << "organism with ID " << org->ID << " scored " << org->dataMap.getAverage("score") << std::endl;
	}

	auto remapRule = TS::RemapRules::UNIQUE;
	auto lifeTimes = brain->getLifeTimes();
	//std::cout << "INPUT STATES" << std::endl;
	auto inputStates = TS::remapToIntTimeSeries(brain->getInputStates(), TS::RemapRules::BIT);
	//std::cout << "OUTPUT STATES" << std::endl;
	auto outputStates = TS::remapToIntTimeSeries(brain->getOutputStates(), TS::RemapRules::BIT);
	//std::cout << "BRAIN STATES" << std::endl;
	auto brainStates = TS::remapToIntTimeSeries(brain->getHiddenStates(), remapRule);

	//std::cout << "inputStates" << std::endl;
	//std::cout << TS::TimeSeriesToString(inputStates) << std::endl;
	//std::cout << "outputStates" << std::endl;
	//std::cout << TS::TimeSeriesToString(outputStates) << std::endl;
	//std::cout << "brainStates" << std::endl;
	//std::cout << TS::TimeSeriesToString(brainStates) << std::endl;
	//std::cout << "rawBrainStates" << std::endl;
	//for (auto r : brain->HiddenStates) {
	//	for (auto e : r) {
	//		std::cout << e << " ";
	//	}
	//	std::cout << std::endl;
	//}


	TS::intTimeSeries shortInputStates = TS::trimTimeSeries(inputStates,TS::Position::FIRST,lifeTimes, currentLargestN);

	TS::intTimeSeries shortOutputStatesBefore;// only needed if recurrent
	TS::intTimeSeries shortOutputStatesAfter; // always needed
	if (brain->recurrentOutput) {
		shortOutputStatesAfter = TS::trimTimeSeries(outputStates, TS::Position::FIRST, lifeTimes, currentLargestN + 1);
		shortOutputStatesBefore = TS::trimTimeSeries(outputStates, TS::Position::LAST, lifeTimes, currentLargestN + 1);
	}
	else {
		shortOutputStatesAfter = TS::trimTimeSeries(outputStates, TS::Position::FIRST, lifeTimes, currentLargestN);
	}

	// always recurrent
	TS::intTimeSeries shortBrainStatesBefore = TS::trimTimeSeries(brainStates, TS::Position::LAST, lifeTimes, currentLargestN+1);
	TS::intTimeSeries shortBrainStatesAfter = TS::trimTimeSeries(brainStates, TS::Position::FIRST, lifeTimes, currentLargestN+1);

	std::vector<int> shortLifeTimes = TS::updateLifeTimes(lifeTimes, -1 * currentLargestN);

	double R = ENT::ConditionalMutualEntropy(worldStates,shortBrainStatesAfter,shortInputStates);
	org->dataMap.append("R", R * RMultPL->get(PT));

	double rawR = ENT::MutualEntropy(worldStates, shortBrainStatesAfter);
	org->dataMap.append("rawR", rawR);

	



	double earlyRawR50 = ENT::MutualEntropy(TS::trimTimeSeries(worldStates, { 0,.5 }, shortLifeTimes), TS::trimTimeSeries(shortBrainStatesAfter, { 0,.5 }, shortLifeTimes));
	org->dataMap.append("earlyRawR50", earlyRawR50);

	double earlyRawR20 = ENT::MutualEntropy(TS::trimTimeSeries(worldStates, { 0,.2 }, shortLifeTimes), TS::trimTimeSeries(shortBrainStatesAfter, { 0,.2 }, shortLifeTimes));
	org->dataMap.append("earlyRawR20", earlyRawR20);

	double lateRawR50 = ENT::MutualEntropy(TS::trimTimeSeries(worldStates, { .5,1 }, shortLifeTimes), TS::trimTimeSeries(shortBrainStatesAfter, { .5,1 }, shortLifeTimes));
	org->dataMap.append("lateRawR50", lateRawR50);

	double lateRawR20 = ENT::MutualEntropy(TS::trimTimeSeries(worldStates, { .8,1 }, shortLifeTimes), TS::trimTimeSeries(shortBrainStatesAfter, { .8,1 }, shortLifeTimes));
	org->dataMap.append("lateRawR20", lateRawR20);



	if (analyze) {
		brain->saveConnectome();
		brain->saveStructure();

		FileManager::writeToFile("score.txt", std::to_string(org->dataMap.getAverage("score")));

		auto smearPair = SMR::getSmearednessConceptsNodesPair(shortInputStates, worldStates, shortBrainStatesAfter);
		FileManager::writeToFile("score.txt", std::to_string(smearPair.second));
		FileManager::writeToFile("score.txt", std::to_string(smearPair.first));

		std::cout << "rawR for worldStateSet { i,j }, brainStateSet, { i,j } " << std::endl;

		for (double i = 0; i <= 1; i += .1) {
			std::cout << i << " : ";
			for (double j = i + .1; j <= 1; j += .1) {
				std::cout << ENT::MutualEntropy(TS::trimTimeSeries(worldStates, { i,j }, shortLifeTimes), TS::trimTimeSeries(shortBrainStatesAfter, { i,j }, shortLifeTimes)) / ENT::Entropy(TS::trimTimeSeries(shortBrainStatesAfter, { i,j }, shortLifeTimes)) << " , ";
			}
			std::cout << std::endl;
		}

		//BRAINTOOLS::saveStateToState(brain, "StateToState.txt", TS::RemapRules::UNIQUE);
		std::string fileName = "StateToState.txt";
		S2S::saveStateToState({ brainStates, TS::extendTimeSeries(outputStates, lifeTimes, {0}, TS::Position::FIRST) }, { inputStates }, lifeTimes, "H_O__I_" + fileName);
		S2S::saveStateToState({ brainStates }, { outputStates, inputStates }, lifeTimes, "H__O_I_" + fileName);
		S2S::saveStateToState({ brainStates }, { inputStates }, lifeTimes, "H_I_" + fileName);




		std::cout << "worldEnt: " << ENT::Entropy(worldStates) << "  brainEnt: " << ENT::Entropy(shortBrainStatesAfter) << "  worldBrainEnt: " << ENT::Entropy(TS::Join(worldStates, shortBrainStatesAfter)) << "  rawR: " << rawR << std::endl;
		std::cout << "earlyRawR20: " << earlyRawR20 << "  earlyRawR50: " << earlyRawR50 << "  lateRawR50: " << lateRawR50 << "  lateRawR20: " << lateRawR20 << std::endl;

		std::cout << "organism with ID " << org->ID << " scored " << org->dataMap.getAverage("score") << std::endl;

		// save fragmentation matrix of brain(hidden) predictions of world features
		FRAG::saveFragMatrix(worldStates, shortBrainStatesAfter, "feature.py");

		// save data flow information - 
		std::vector<std::pair<double, double>> flowRanges = { {0,1},{0,.333},{.333,.666},{.666,1},{0,.5},{.5,1} };
		std::cout << shortBrainStatesAfter.size() << " " << shortOutputStatesBefore.size() << " " << shortBrainStatesBefore.size() << " " << shortInputStates.size() << std::endl;

		FRAG::saveFragMatrixSet(TS::Join(TS::trimTimeSeries(brainStates,TS::Position::FIRST,lifeTimes), outputStates), TS::Join(TS::trimTimeSeries(brainStates, TS::Position::LAST, lifeTimes), inputStates ), shortLifeTimes, flowRanges, "flowMap.py", "shared", -1);
	}
}

std::unordered_map<std::string, std::unordered_set<std::string>> NBackWorld::requiredGroups() {
	return { {groupName, {"B:" + brainName + ",1," + std::to_string(N2OutMap.size())}} };
}



