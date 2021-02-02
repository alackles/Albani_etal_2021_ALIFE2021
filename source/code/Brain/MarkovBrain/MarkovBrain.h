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

#include <cmath>
#include <memory>
#include <iostream>
#include <set>
#include <vector>

#include "GateListBuilder/GateListBuilder.h"
#include "../../Genome/AbstractGenome.h"

#include "../../Utilities/Random.h"

#include "../AbstractBrain.h"

class MarkovBrain : public AbstractBrain {

    std::vector<std::map < std::vector<int>, int>> checkDet;

public:
    std::vector<std::shared_ptr<AbstractGate>> gates;

    //	static shared_ptr<ParameterLink<int>> bitsPerBrainAddressPL;  // how
    //many bits are evaluated to determine the brain addresses.
    //	static shared_ptr<ParameterLink<int>> bitsPerCodonPL;

    static std::shared_ptr<ParameterLink<bool>> useGateRegulationPL;

    static std::shared_ptr<ParameterLink<bool>> useOutputThresholdPL;
    static std::shared_ptr<ParameterLink<double>> outputThresholdPL;
    static std::shared_ptr<ParameterLink<bool>> useHiddenThresholdPL;
    static std::shared_ptr<ParameterLink<double>> hiddenThresholdPL;

    static std::shared_ptr<ParameterLink<bool>> recurrentOutputPL;
    static std::shared_ptr<ParameterLink<bool>> randomizeUnconnectedOutputsPL;
    static std::shared_ptr<ParameterLink<bool>> recordIOMapPL;
    static std::shared_ptr<ParameterLink<std::string>> IOMapFileNamePL;
    static std::shared_ptr<ParameterLink<int>> randomizeUnconnectedOutputsTypePL;
    static std::shared_ptr<ParameterLink<double>> randomizeUnconnectedOutputsMinPL;
    static std::shared_ptr<ParameterLink<double>> randomizeUnconnectedOutputsMaxPL;

    static std::shared_ptr<ParameterLink<int>> evaluationsPreUpdatePL;

    static std::shared_ptr<ParameterLink<int>> hiddenNodesPL;
    static std::shared_ptr<ParameterLink<std::string>> genomeNamePL;

    bool useGateRegulation;
    std::vector<int> gateRegulationAdresses; // values are -2 off, -1 on, 0 and up, on if (nodes[value] > 0)

    bool useOutputThreshold;
    double outputThreshold;
    bool useHiddenThreshold;
    double hiddenThreshold;

    bool randomizeUnconnectedOutputs;
    bool randomizeUnconnectedOutputsType;
    double randomizeUnconnectedOutputsMin;
    double randomizeUnconnectedOutputsMax;
    int evaluationsPreUpdate;

    int hiddenNodes;
    std::string genomeName;

    std::vector<double> nodes;
    std::vector<double> nextNodes;

    int nrNodes;

    std::shared_ptr<AbstractGateListBuilder> GLB;
    std::vector<int> nodesConnections, nextNodesConnections;

    //	static bool& cacheResults;
    //	static int& cacheResultsCount;

    // static void initializeParameters();
    std::vector<int> nodeMap;

    /*
     * Builds a look up table to convert genome site values into brain state
     * addresses - this is only used when there is a fixed number of brain states
     * if there is a variable number of brain states, then the node map must be
     * rebuilt.
     */
    static int makeNodeMap(std::vector<int>& nodeMap, int sizeInBits,
        int defaultNrOfBrainStates) {
        for (int i = 0; i < pow(2, (sizeInBits));
            i++) { // each site in the genome has 8 bits so we need to count though
                   // (  2 to the (8 * number of sites)  )
            nodeMap.push_back(i % defaultNrOfBrainStates);
        }

        return 1;
    }

    MarkovBrain() = delete;

    MarkovBrain(std::vector<std::shared_ptr<AbstractGate>> _gates, int _nrInNodes,
        int _nrOutNodes, std::shared_ptr<ParametersTable> PT_ = nullptr);
    MarkovBrain(std::shared_ptr<AbstractGateListBuilder> GLB_, int _nrInNodes,
        int _nrOutNodes, std::shared_ptr<ParametersTable> PT_ = nullptr);
    MarkovBrain(std::shared_ptr<AbstractGateListBuilder> GLB_,
        std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes,
        int _nrInNodes, int _nrOutNodes,
        std::shared_ptr<ParametersTable> PT_ = nullptr);

    virtual ~MarkovBrain() = default;

    virtual std::shared_ptr<AbstractBrain>
        makeCopy(std::shared_ptr<ParametersTable> PT_ = nullptr) override;

    void readParameters();

    virtual void update() override;

    void inOutReMap();

    // Make a brain like the brain that called this function, using genomes and
    // initalizing other elements.
    virtual std::shared_ptr<AbstractBrain> makeBrain(
        std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes) override;

    virtual std::string description() override;
    void fillInConnectionsLists();
    virtual DataMap getStats(std::string& prefix) override;
    virtual std::string getType() override { return "Markov"; }

    virtual void resetBrain() override;
    virtual void resetOutputs() override;
    virtual void resetInputs() override;

    virtual std::string gateList();
    virtual std::vector<std::vector<int>> getConnectivityMatrix();
    virtual int brainSize();
    int numGates();

    std::vector<int> getHiddenNodes();

    virtual void initializeGenomes(
        std::unordered_map<std::string, std::shared_ptr<AbstractGenome>>& _genomes) override;

    virtual std::unordered_set<std::string> requiredGenomes() override {
        return { genomeNamePL->get(PT) };
    }


    std::vector<std::shared_ptr<AbstractBrain>> getAllSingleGateKnockouts();

    std::vector<std::vector<double>> getTPMforTimepoint() {
        bool fbState = DecomposableFeedbackGate::feedbackON;
        int maxReps = 1000;//100;
        std::cout << maxReps << std::endl;
        std::vector<double> recoverState = nodes;
        std::vector<double> recoverNextNodeStates = nextNodes;
        std::vector<double> recoverI = inputValues;
        std::vector<double> recoverO = outputValues;
        DecomposableFeedbackGate::feedbackON = false;
        std::vector<std::vector<double>> TPM;
        for (int state = 0; state < (1 << nodes.size()); state++) {
            std::vector<double> nextState = std::vector<double>(nodes.size(), 0.0);
            for (int reps = 0; reps < maxReps; reps++) {
                for (int i = 0; i < nodes.size(); i++) {
                    if (i < nrInputValues)
                        inputValues[i] = (double)((state >> i) & 1);
                    else
                        nodes[i] = (double)((state >> i) & 1);
                }
                update();
                for (int i = 0; i < nodes.size(); i++) {
                    if (nodes[i] > 0.0)
                        nextState[i] += 1.0;
                }
            }
            for (int i = 0; i < nextState.size(); i++)
                nextState[i] /= maxReps;
            TPM.push_back(nextState);
        }

        //recover current state of everything
        nodes = recoverState;
        nextNodes = recoverNextNodeStates;
        inputValues = recoverI;
        outputValues = recoverO;
        DecomposableFeedbackGate::feedbackON = fbState;
        return TPM;
    }

    std::string getsampledTPM() {
        std::vector<std::vector<double>> TPM = getTPMforTimepoint();
        std::string S = to_string("[");
        for (int i = 0; i < (int)TPM.size(); i++) {
            if (i != 0)
                S += ",";
            S += "[";
            for (int o = 0; o < (int)TPM[i].size(); o++) {
                if (o != 0)
                    S += ",";
                S += std::to_string(TPM[i][o]);
            }
            S += "]";
        }
        return S + "]";
    }

    std::string getTPMdescription() {
        std::string S = to_string("[");
        for (int g = 0; g < gates.size(); g++) {
            if (g != 0)
                S += ",";
            S += gates[g]->getTPMdescription();
        }
        return S + "]";
    }


    // brainConnectome - a square map of input,output,hidden x input,output,hidden where each cell is the count of actual wires from T -> T+1
    std::vector<std::vector<int>> getConnectome() override{

        int brainInCount = nrInputValues;
        int brainOutCount = nrOutputValues;
        int brainHiddenCount = nrNodes - (nrInputValues + nrOutputValues);

        std::vector<std::vector<int>> connectome(nodes.size(), std::vector<int>(nodes.size()));
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

        for (auto gate : gates) {
            auto gateConnectome = gate->getConnectionsLists();
            for (auto input : gateConnectome.first) {
                for (auto output : gateConnectome.second) {
                    if (connectome[input][output] == -1) {
                        std::cout << "  in MarkovBrain::getConnectome, there appears to be a bad connection " << input << " -> " << output << ". ignoring..." << std::endl;
                        //exit(1);
                    }
                    else {
                        connectome[input][output]++;
                    }
                }
            }
        }
        return(connectome);
    }

    void saveConnectome(std::string fileName = "brainConnectome.py") override {
        int brainInCount = nrInputValues;
        int brainOutCount = nrOutputValues;
        int brainHiddenCount = nrNodes - (nrInputValues + nrOutputValues);

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
    void saveStructure(std::string fileName = "brainStructure.txt") override {
        std::string outString = "";

        int brainHiddenCount = nrNodes - (nrInputValues + nrOutputValues);
        int brainInCount = nrInputValues;
        int brainOutCount = nrOutputValues;

        for (int i = 0; i < brainInCount; i++) {
            outString += "i" + std::to_string(i) + " [style=filled fillcolor = green]\n";
        }
        for (int i = 0; i < brainHiddenCount; i++) {
            outString += "h" + std::to_string(i) + " [style=filled fillcolor = white]\n";
        }
        for (int i = 0; i < brainOutCount; i++) {
            outString += "o" + std::to_string(i) + " [style=filled fillcolor = pink]\n";
        }

        int gateID = 0;
        for (auto gate : gates) {
            outString += "g" + std::to_string(gateID) + " [style=filled fillcolor = lightblue shape = box]\n";
            auto gateConnectome = gate->getConnectionsLists();
            for (auto input : gateConnectome.first) { // for this gates inputs
                if (input < brainInCount) { // if in is an input
                    outString += "i" + std::to_string(input) + " -> g" + std::to_string(gateID) + "\n";
                }
                else if (input >= brainInCount + brainOutCount) { // if in is an hidden
                    outString += "h" + std::to_string(input - (brainInCount + brainOutCount)) + " -> g" + std::to_string(gateID) + "\n";
                }
            }
            for (auto output : gateConnectome.second) { // for this gates outputs
                if (output < brainInCount + brainOutCount && output >= brainInCount) { // if out is an output
                    outString += "g" + std::to_string(gateID) + " -> o" + std::to_string(output - brainInCount) + "\n";
                }
                else if (output >= brainInCount + brainOutCount) { // if out is hidden
                    outString += "g" + std::to_string(gateID) + " -> h" + std::to_string(output - (brainInCount + brainOutCount)) + "\n";
                }
            }
            gateID++;
        }
        FileManager::writeToFile(fileName, outString + "\n");
    }

};

inline std::shared_ptr<AbstractBrain>
MarkovBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
    return std::make_shared<MarkovBrain>(std::make_shared<ClassicGateListBuilder>(PT), ins,
        outs, PT);
}


