############################
# repMerge.py
#
# Author: Acacia Ackles
#
# Purpose:
# This script outputs a single long data format file of merged replicates and conditions.
#############################


######## Code #############

# import required modules

import pandas as pd
import csv
import glob
import argparse

# constants, filenames, and other things like that

# command-line inputs

parser = argparse.ArgumentParser(description='Command-line inputs for the data merger script.')
parser.add_argument('reps',metavar='-repRange', type=str, nargs=2, help='First and last rep to merge, space-separated.')
parser.add_argument('files', metavar='-files', type=str, nargs='?', help='Comma-separated files (e.g. max.csv,pop.csv,LOD_data.csv) to merge. If none specified will only merge LOD_data.csv)')
# conditions
# brains = ["Markov", "RNN"] # brains
worlds = ["BlockCatch", "PathFollow", "NBack"] # NBack

# parameters in MABE
# mapping onto the conditions we're interested in 

compstruct = {
    "BRN_Markov__MDA_1": "Markov", 
    "BRN_Markov__MDA_0__MAA_1__MBB_0": "Markov ANN", 
    "BRN_Markov__MDA_0__MAA_1__MBB_1": "Markov ANN bitted", 
    "BRN_RNN__RWR_01010__RDR_-1": "RNN", 
    "BRN_RNN__RWR_01010__RDR_1": "RNN discretized", 
    "BRN_RNN__RWR_01410__RDR_-1": "RNN sparse", 
    "BRN_RNN__RWR_01410__RDR_1": "RNN sparse discretized"
    }


reps = parser.parse_args().reps

lod_data_filename = "LOD_data.csv"
max_filename = "max.csv"
pop_filename = "pop.csv"

source_datapath = "../source/work/"
final_datapath = "../data/"

# list of the columns that we want to keep
df_columns = {'update','ID','score_AVE', 'score'}

# time to do the merging
def merge_my_file(filename):
    merged_file = pd.DataFrame(columns=df_columns)
    for world in worlds: 
        for kcomp, vcomp in compstruct.items():
            for rep in reps:
                globpath = source_datapath + "C*" + "WLD_" + world + "__" + kcomp + "/" + rep + "/"
                datapath = glob.glob(globpath + filename)
                if len(datapath) == 1:
                    datapath = "".join(datapath)
                    # if everything's fine, open up the files
                    with open(datapath) as f:
                        print(datapath)
                        filemerge = pd.read_csv(f, sep=",", usecols=lambda c: c in df_columns)
                elif not len(datapath):
                    print("No files matched.")
                    print("Path: ", globpath)
                    pass

                # let's merge them into one tidy file to output for later
                filemerge["rep"] = rep
                filemerge["compstruct"] = vcomp
                filemerge["world"] = world
                # rename the other columns to be sensible
                filemerge = filemerge.rename(
                    columns={"score_AVE": "score"})

                # add to our list of dataframes for each k
                merged_file = merged_file.append(filemerge, sort=False)
    filepath = final_datapath + "merged_" + filename
    merged_file.to_csv(filepath,index=False)

if parser.parse_args().files != "":
    files = parser.parse_args().files
else:
    files=[lod_data_filename]

files = [lod_data_filename]
for fname in files:
    merge_my_file(fname)
