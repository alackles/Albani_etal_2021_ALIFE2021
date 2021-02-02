//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once

#include <math.h>
#include <memory>
#include <iostream>
#include <set>
#include <vector>
#include <string>

#include "../../Genome/AbstractGenome.h"

#include "../../Utilities/Random.h"

#include "../AbstractBrain.h"


class RNNBrain: public AbstractBrain {
public:

    static std::shared_ptr<ParameterLink<std::string>> genomeNamePL;
    static std::shared_ptr<ParameterLink<int>> nrOfRecurrentNodesPL;
    static std::shared_ptr<ParameterLink<int>> discretizeRecurrentPL;
    static std::shared_ptr<ParameterLink<std::string>> hiddenLayerSizesPL;
    static std::shared_ptr<ParameterLink<std::string>> weightRangeMappingPL;
    static std::shared_ptr<ParameterLink<std::string>> IO_RangesPL;

    static std::shared_ptr<ParameterLink<std::string>> biasRangePL;
    static std::shared_ptr<ParameterLink<std::string>> activationFunctionPL;


    int nrRecurrentValues;
    std::vector<int> hiddenLayerSizes;
	double recurrentNoise;
    int discretizeRecurrent;
    std::string genomeName;
    std::vector<double> weightRangeMapping;
    std::vector<double> weightRangeMappingSums;

    std::vector<double> biasRange = { 0,0 };
    std::vector<std::vector<double>> initialValues;
    std::string activationFunction;

    std::vector<std::vector<double>> nodes;
    std::vector<std::vector<std::vector<double>>> weights;
	RNNBrain() = delete;

	RNNBrain(int _nrInNodes, int _nrOutNodes, std::shared_ptr<ParametersTable> _PT = nullptr);

	virtual ~RNNBrain() = default;

	virtual void update() override;

	virtual std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes) override;

	virtual std::string description() override;
	virtual DataMap getStats(std::string& prefix) override;
	virtual std::string getType() override {
		return "RNN";
	}

	virtual void resetBrain() override;
	virtual void resetOutputs() override;

    void applyActivationToLayer(std::vector<double> &V);

    virtual std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> _PT = nullptr) override;

	virtual std::unordered_set<std::string> requiredGenomes() override {
		return { genomeName };
	}

    inline void setInput(const int& inputAddress, const double& value) override;
    inline double readInput(const int& inputAddress) override;
    inline void setOutput(const int& outputAddress, const double& value) override;
    inline double readOutput(const int& outputAddress) override;

	void initializeGenomes(std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes) override;

    std::vector<int> getHiddenNodes();
    std::vector<double> getRawHiddenNodes();

    virtual std::string brainState() {
        std::string S="[";
        for(int i=0;i<nrInputValues;i++)
            S+= std::to_string(nodes[0][i])+",";
        for(int i=0;i<(int)nodes[(int)nodes.size()-1].size();i++){
            if(i!=0)
                S+=",";
            S+= std::to_string(nodes[(int)nodes.size()-1][i]);
        }
        S+="]";
        return S;
    }
    void showBrain();

    // brainConnectome - a square map of input,output,hidden x input,output,hidden where each cell is the count of actual wires from T -> T+1
    std::vector<std::vector<int>> getConnectome() override {

        int brainInCount = nrInputValues;
        int brainOutCount = nrOutputValues;
        int brainHiddenCount = nrRecurrentValues;
        int nodesCount = brainInCount + brainOutCount + brainHiddenCount;

        std::vector<std::vector<int>> connectome(nodesCount, std::vector<int>(nodesCount, 1)); // assume all connected
        for (int r = 0; r < connectome.size(); r++) {
            for (int c = 0; c < connectome[0].size(); c++) {
                if (c < brainInCount) {
                    connectome[r][c] = -1;
                }
                else if (!recurrentOutput && r >= brainInCount && r < brainInCount + brainOutCount) {
                    connectome[r][c] = -1;
                }
            }
        }
        return(connectome);
    }

    void saveConnectome(std::string fileName = "brainConnectome.py") override {
        int brainInCount = nrInputValues;
        int brainOutCount = nrOutputValues;
        int brainHiddenCount = nrRecurrentValues;
        int nodesCount = brainInCount + brainOutCount + brainHiddenCount;

        std::string outString = "numIn = " + std::to_string(brainInCount) + "\n";
        outString += "numOut = " + std::to_string(brainOutCount) + "\n";
        outString += "numHidden = " + std::to_string(brainHiddenCount) + "\n\n";

        auto brainConnectome = getConnectome();

        outString += "brainConnectome = [\n";
        for (auto row : brainConnectome) {
            outString += "[";
            for (auto val : row) {
                outString += std::to_string(val) + ",";
            }
            outString += "],\n";
        }
        outString += "]\n";
        if (fileName.substr(fileName.size() - 3, 3) != ".py") {
            fileName += ".py";
        }
        FileManager::writeToFile(fileName, outString);
    }

    // saveBrainStructure
    // at the moment, just assumes all input + hidden (t) connect to all output + hidden (t+1)
    void saveStructure(std::string fileName = "brainStructure.txt") override {
        std::string outString = "";

        int brainInCount = nrInputValues;
        int brainOutCount = nrOutputValues;
        int brainHiddenCount = nrRecurrentValues;

        for (int i = 0; i < brainInCount; i++) {
            outString += "i" + std::to_string(i) + " [style=filled fillcolor = green]\n";
        }
        for (int i = 0; i < brainHiddenCount; i++) {
            outString += "h" + std::to_string(i) + " [style=filled fillcolor = white]\n";
        }
        for (int i = 0; i < brainOutCount; i++) {
            outString += "o" + std::to_string(i) + " [style=filled fillcolor = pink]\n";
        }

        FileManager::writeToFile(fileName, outString + "\n");
    }
};

inline std::shared_ptr<AbstractBrain> RNNBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
	return std::make_shared<RNNBrain>(ins, outs, PT);
}



