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
parser.add_argument('reps',metavar='-repRange', type=int, nargs=2, help='First and last rep to merge, space-separated.')
# conditions
# brains = ["Markov", "RNN"] # brains
worlds = ["BlockCatch", "PathFollow", "NBack"] # NBack

# parameters in MABE
# mapping onto the conditions we're interested in 

compstruct = {
    "BRN_Markov__MDA_1": "Markov", 
    "BRN_Markov__MAA_1__MBB_0": "Markov RNN", 
    "BRN_Markov__MAA_1__MBB_1": "Markov RNN bitted", 
    "BRN_RNN__RWR_01010__RDR_-1": "RNN", 
    "BRN_RNN__RWR_01010__RDR_1": "RNN discretized", 
    "BRN_RNN__RWR_01410__RDR_-1": "RNN sparse", 
    "BRN_RNN__RWR_01410__RDR_1": "RNN sparse discretized"
    }


first_rep = parser.parse_args().reps[0]
last_rep = parser.parse_args().reps[1]
reps = [str(x) for x in range(first_rep,last_rep+1)]

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
    filepath = final_datapath + "beginmerged_" + filename
    merged_file.to_csv(filepath,index=False)


files = [lod_data_filename]
for fname in files:
    merge_my_file(fname)
