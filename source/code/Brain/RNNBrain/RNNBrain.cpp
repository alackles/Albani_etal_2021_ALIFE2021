//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "../RNNBrain/RNNBrain.h"


std::shared_ptr<ParameterLink<std::string>> RNNBrain::genomeNamePL = Parameters::register_parameter(
    "BRAIN_RNN_NAMES-genomeName", (std::string)"root::", "namespace of genome used to encode this brain");

std::shared_ptr<ParameterLink<int>> RNNBrain::nrOfRecurrentNodesPL = Parameters::register_parameter("BRAIN_RNN-nrOfRecurringNodes", (int)8, "number of recurring nodes");

std::shared_ptr<ParameterLink<int>> RNNBrain::discretizeRecurrentPL = Parameters::register_parameter(
    "BRAIN_RNN-discretizeRecurrent", 0, "should recurrent nodes be discretized when being copied?\n"
    "if 0, no, leave them be.\n"
    "if 1 then map <= 0 to 0, and > 0 to 1\n"
    "if > then 1, values are mapped to new equally spaced values in range [-1..1] such that each bin has the same sized range\n"
    "    i.e. if 3 bin bounderies will be (-1.0,-.333-,.333-,1.0) and resulting values will be (-1.0,0.0,1.0)\n"
    "Note that this process ends up in a skewed mapping. mappings will always include -1.0 and 1.0. even values > 1 will result in remappings that do not have 0");

std::shared_ptr<ParameterLink<std::string>> RNNBrain::hiddenLayerSizesPL = Parameters::register_parameter("BRAIN_RNN-hiddenLayerSizes", (std::string)"0", "comma seperated list of hidden layer sizes (0 indicates no hidden layer)");

std::shared_ptr<ParameterLink<std::string>> RNNBrain::weightRangeMappingPL = Parameters::register_parameter(
    "BRAIN_RNN-weightRangeMapping", (std::string)"0,1,0,1,0",
    "comma seperated list of exactly five (double) values. weight values from genome will be extracted in the range [0..sum(list)].\n"
    "values between 0 and the first value will map to -1\n"
    "values between the first value and first+second will map to [-1..0]\n"
    "... i.e. each value in the list is the ratio of possible values that will map to each value/range in [-1,[-1..0],0,[0..1],1]\n"
    "the result is that this list sets the ratio of each type of weight (-1,variable(-1..0),0,variable(0..1),1)");

std::shared_ptr<ParameterLink<std::string>> RNNBrain::biasRangePL = Parameters::register_parameter("BRAIN_RNN-biasRange", (std::string)"-1.0,1.0", "bias will be generated per node in this range and used to initalize gate on update");

std::shared_ptr<ParameterLink<std::string>> RNNBrain::activationFunctionPL = Parameters::register_parameter(
    "BRAIN_RNN-activationFunction", (std::string)"tanh",
    "choose from linear(i.e. none),tanh,tanh(0-1),bit,triangle");



RNNBrain::RNNBrain(int _nrInNodes, int _nrOutNodes, std::shared_ptr<ParametersTable> _PT) : AbstractBrain(_nrInNodes, _nrOutNodes, _PT) {
    genomeName = genomeNamePL->get(_PT);
    nrRecurrentValues = nrOfRecurrentNodesPL->get(_PT);
    discretizeRecurrent = discretizeRecurrentPL->get(_PT);
    convertCSVListToVector(hiddenLayerSizesPL->get(), hiddenLayerSizes);
    convertCSVListToVector(biasRangePL->get(), biasRange);
    convertCSVListToVector(weightRangeMappingPL->get(), weightRangeMapping);

    // weightRangeMappingSums is a list of bounderies between the weight ranges [-1,[-1..0],0,[0..1],1]
    weightRangeMappingSums.push_back(weightRangeMapping[0]);
    weightRangeMappingSums.push_back(weightRangeMappingSums[0] + weightRangeMapping[1]);
    weightRangeMappingSums.push_back(weightRangeMappingSums[1] + weightRangeMapping[2]);
    weightRangeMappingSums.push_back(weightRangeMappingSums[2] + weightRangeMapping[3]);
    weightRangeMappingSums.push_back(weightRangeMappingSums[3] + weightRangeMapping[4]);

}

std::shared_ptr<AbstractBrain> RNNBrain::makeBrain(std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes) {
    // get new brain, set up genomeName, nrRecurrentValues, recurrentNoise, discretizeRecurrent, and hiddenLayerSizes

    std::shared_ptr<RNNBrain> newBrain = std::make_shared<RNNBrain>(nrInputValues, nrOutputValues, PT);

    auto genomeHandler = _genomes[newBrain->genomeName]->newHandler(_genomes[newBrain->genomeName], true);


    // resize nodes to input + hidden + output layers
    if (newBrain->hiddenLayerSizes[0] == 0) {
        newBrain->hiddenLayerSizes.clear();
    }
    newBrain->nodes.resize(2 + newBrain->hiddenLayerSizes.size());
    // resize input layer
    newBrain->nodes[0].resize((size_t)newBrain->nrInputValues + newBrain->nrRecurrentValues);
    // resize output layer
    newBrain->nodes.back().resize((size_t)newBrain->nrOutputValues + newBrain->nrRecurrentValues);
    // resize hiddenLayers
    for (size_t i = 0; i < newBrain->hiddenLayerSizes.size(); i++) {
        newBrain->nodes[i + 1].resize(newBrain->hiddenLayerSizes[i]);
    }

    // create the "layers" of weights (between each node layer
    newBrain->weights.resize((int)newBrain->nodes.size() - 1);
    // now for each weight layer
    for (size_t i = 0; i < (int)newBrain->weights.size(); i++) {
        // add space for a vector of weights for each node
        newBrain->weights[i].resize((int)newBrain->nodes[i].size());

        // for each node in this layer
        for (size_t j = 0; j < (int)newBrain->weights[i].size(); j++) {
            // add a weight for the wire from this node to each node in the next node layer
            newBrain->weights[i][j].resize((int)newBrain->nodes[i + 1].size());
        }
    }

    for (size_t i = 0; i < newBrain->weights.size(); i++) {
        for (size_t j = 0; j < newBrain->weights[i].size(); j++) {
            for (size_t k = 0; k < newBrain->weights[i][j].size(); k++) {
                double value = genomeHandler->readDouble(0, weightRangeMappingSums[4]);
                //std::cout << value << " ";
                if (value < weightRangeMappingSums[0]) { // first range, map to -1
                    //std::cout << "-1 -> ";
                    value = -1.0;
                }
                else if (value < weightRangeMappingSums[1]) { // second range, map to [-1..0]
                    //std::cout << " -1,0 -> ";
                    value = ((value - weightRangeMappingSums[0]) / weightRangeMapping[1]) - 1.0;
                }
                else if (value < weightRangeMappingSums[2]) { // third range, map to 0
                    //std::cout << " 0 -> ";
                    value = 0.0;
                }
                else if (value < weightRangeMappingSums[3]) { // fourth range, map to [0..1]
                    //std::cout << " 0,1 -> ";
                    value = (value - weightRangeMappingSums[2]) / weightRangeMapping[3];
                }
                else { // fifth range, map to 0
                    //std::cout << " 1 -> ";
                    value = 1.0;
                }
                //std::cout << value << std::endl;
                newBrain->weights[i][j][k] = value;
                //newBrain->weights[i][j][k] = value * value * value * 4.0;
            }
        }
    }

    newBrain->initialValues.push_back({}); // first row empty because it's inputs
    for (size_t i = 1; i < newBrain->nodes.size(); i++) {
        newBrain->initialValues.push_back(std::vector<double>(newBrain->nodes[i].size()));
        for (size_t j = 0; j < newBrain->initialValues[i].size(); j++) {
            newBrain->initialValues[i][j] = genomeHandler->readDouble(biasRange[0], biasRange[1]);
        }
    }

    //showBrain();
    //exit(0);

 	return newBrain;
}

void RNNBrain::resetBrain() {
    for (auto& N : nodes) {
        for (size_t i = 0; i < (int)N.size(); i++) {
            N[i] = 0.0;
        }
    }
    if (recordActivity) {
        if (lifeTimes.back() != 0) {
            lifeTimes.push_back(0);
        }
    }

}

void RNNBrain::setInput(const int& inputAddress, const double& value){
    nodes[0][inputAddress]=value;
}

double RNNBrain::readInput(const int& inputAddress){
    return nodes[0][inputAddress];
}

void RNNBrain::setOutput(const int& outputAddress, const double& value){
    nodes[(int)nodes.size()-1][outputAddress]=value;
}

double RNNBrain::readOutput(const int& outputAddress){
    return nodes[(int)nodes.size()-1][outputAddress];
}

void RNNBrain::initializeGenomes(std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes) {
	_genomes[genomeName]->fillRandom();
}

std::vector<int> RNNBrain::getHiddenNodes() {
    std::vector<int> temp = {};
	for (size_t i = nrOutputValues; i<(int)nodes[(int)nodes.size() - 1].size(); i++) {
		temp.push_back(Bit(nodes[(int)nodes.size() - 1][i]));
	}
	return temp;
}

std::vector<double> RNNBrain::getRawHiddenNodes() {
    std::vector<double> temp = {};
	for (size_t i = nrOutputValues; i<(int)nodes[(int)nodes.size() - 1].size(); i++) {
		temp.push_back(nodes[(int)nodes.size() - 1][i]);
	}
	return temp;
}

void RNNBrain::update() {
    // input and hidden have been set so it's time to record state...
    if (recordActivity) {
        InputStates.push_back(std::vector<double>(nrInputValues));
        for (size_t i = 0; i < nrInputValues; i++) {
            InputStates.back()[i] = nodes[0][i];
        }
        if (lifeTimes.back() == 0) {
            HiddenStates.push_back(std::vector<double>(nrRecurrentValues));
            for (size_t i = 0; i < nrRecurrentValues; i++) {
                HiddenStates.back()[i] = nodes[0][(size_t)(nrInputValues) + i];
            }
        }
    }
    // for every layer, update the nodes in that layer
    // skip first layer, because it's input and recurrent
    for (size_t layer = 1; layer < nodes.size(); layer++) {
        for (size_t i = 0; i < nodes[layer].size(); i++) {
            nodes[layer][i] = initialValues[layer][i];
        }
        // for each node in the prior layer
        for (size_t i = 0; i < nodes[layer - 1].size(); i++) {
            // for each node in this layer
            for (size_t j = 0; j < nodes[layer].size(); j++) {
                // add the node value from the prior layer * that nodes weight for this node to this node
                nodes[layer][j] += weights[(size_t)(layer) - 1][i][j] * nodes[(size_t)(layer) - 1][i];
            }
        }
        // apply apply Activation Function to this layer
        applyActivationToLayer(nodes[layer]);
    }
    int lastLayer = nodes.size() - 1;
    for (size_t i = 0; i < nrRecurrentValues; i++) {
        if (discretizeRecurrent < 1) {
            nodes[0][(size_t)(nrInputValues) + i] = nodes[lastLayer][(size_t)(nrOutputValues) + i];
        }
        else if (discretizeRecurrent == 1) {
            nodes[0][(size_t)(nrInputValues) + i] = Bit(nodes[lastLayer][(size_t)(nrOutputValues) + i]);
        }
        else {
            // move value is in to range 0 to discretizeRecurrent
            nodes[0][(size_t)(nrInputValues)+i] = std::max(-1.0, std::min(1.0, nodes[lastLayer][(size_t)(nrOutputValues)+i]));
            nodes[0][(size_t)(nrInputValues)+i] = (((nodes[0][(size_t)(nrInputValues)+i]) + 1.0)/2.0)* discretizeRecurrent;
            // use int to discretize
            nodes[0][(size_t)(nrInputValues)+i] = (double)(int)(nodes[0][(size_t)(nrInputValues)+i]);
            
            if (nodes[0][(size_t)(nrInputValues)+i] == discretizeRecurrent) { // if node value was exactly 1
                nodes[0][(size_t)(nrInputValues)+i]--;
            }
            // move back to [0..1] (with / discretizeRecurrent-1) and then to [-1...1] (with * 2 - 1)
            nodes[0][(size_t)(nrInputValues)+i]  = (nodes[0][(size_t)(nrInputValues)+i] / (double)(discretizeRecurrent-1) * 2.0) - 1.0;
        }
    }
    // output and hidden+1 have been set so it's time to record state...
    if (recordActivity) {
        OutputStates.push_back(std::vector<double>(nrOutputValues));
        for (size_t i = 0; i < nrOutputValues; i++) {
            OutputStates.back()[i] = nodes[lastLayer][i];
        }
        HiddenStates.push_back(std::vector<double>(nrRecurrentValues));
        for (size_t i = 0; i < nrRecurrentValues; i++) {
            HiddenStates.back()[i] = nodes[0][(size_t)(nrInputValues)+i];
        }
        lifeTimes.back()++;
    }

    /*
    for(int l=0;l<nodes.size();l++){
        printf("layer: %i : ",l);
        for(int i=0;i<nodes[l].size();i++)
            printf("%0.2f ",nodes[l][i]);
        printf("\n");
    }
     //*/
}

void inline RNNBrain::resetOutputs() {
    for (int o = 0; o < nrOutputValues; o++) {
        nodes[(int)nodes.size() - 1][o] = 0.0;
    }
}

std::string RNNBrain::description() {
    std::string S = "RNN Brain";
	return S;
}

DataMap RNNBrain::getStats(std::string& prefix) {
	DataMap dataMap;
    

	return (dataMap);
}

void RNNBrain::applyActivationToLayer(std::vector<double> &V){
    for (auto&& result : V) {
        // select activation function (if none/linear, do nothing)
        if (activationFunction == "none" || activationFunction == "linear") {
            // if none / linear, do nothing
        }
        else if (activationFunction == "tanh") {
            result = tanh(result);
        }
        else if (activationFunction == "tanh(0-1)") {
            result = tanh(result) * .5 + .5;
        }
        else if (activationFunction == "bit") {
            result = Bit(result);
        }
        else if (activationFunction == "triangle") {
            result = std::max(1.0 - std::abs(result * 2.0), -1.0);
        }
    }
}

std::shared_ptr<AbstractBrain> RNNBrain::makeCopy(std::shared_ptr<ParametersTable> _PT){
    if (_PT == nullptr) {
        _PT = PT;
    }
    auto newBrain = std::make_shared<RNNBrain>(nrInputValues, nrOutputValues, _PT);
    newBrain->nodes = nodes;
    newBrain->weights = weights;
    newBrain->initialValues = initialValues;

    //for(int i=0;i<(int)weights.size();i++)
    //    for(int j=0;j<(int)weights[i].size();j++)
    //        for(int k=0;k<(int)weights[i][j].size();k++)
    //            newBrain->weights[i][j][k]=weights[i][j][k];
    
    return newBrain;
}


void RNNBrain::showBrain(){
    printf("I: %i O:%i \n",nrInputValues, nrOutputValues);
    for(int l=0;l<(int)nodes.size();l++){
        printf("layer %i size %i\n",l,(int)nodes[l].size());
    }
}



