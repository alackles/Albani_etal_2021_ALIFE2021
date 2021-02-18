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

# constants, filenames, and other things like that

# conditions
brains = ["Markov", "RNN"] # brains
worlds = ["BlockCatch", "PathFollow", "NBack"] # NBack
mgates = {"ann": "MDA_0__MAA_1", "det": "MDA_1__MAA_0"} #markov: gates
mht = ["0", "1"] # markov: discretizing
rwr = ["01010", "11111", "10201"] # rnn: weight biasing towards/away from 0
rdr = ["1", "-1", "5"] # rnn: discretization

first_rep = 101
last_rep = 109
reps = [str(x) for x in range(first_rep, last_rep+1)]

lod_data_filename = "LOD_data.csv"
lod_org_filename = "LOD_organisms.csv"
max_filename = "max.csv"
pop_filename = "pop.csv"

#ss_org_filename = "snapshot_organisms_10.csv"
#ss_data_filename = "snapshot_data_10.csv"
source_datapath = "../source/work/"
final_datapath = "../data/"
# list of the columns that we want to keep
df_columns = {'update','ID','score_AVE', 'score'}
#genome_col = "GENOME_root::_sites" # this is the nasty default column name for the genome :(

# time to do the merging

def merge_my_file(filename):
    merged_file = pd.DataFrame(columns=df_columns)
    for world in worlds: 
        for brain in brains:
            for rep in reps:
                # find the files and make sure everything's fine
                gatetype = "NA"
                threshhold = "NA"
                weighting = "NA"
                discretize = "NA"
                # TODO: MAKE THIS LESS HORRIBLE
                if brain == "Markov":
                    for gate, code in mgates.items():
                        for ht in mht: 
                            gatetype = gate
                            threshhold = ht
                            globpath = source_datapath + "C*" + "WLD_" + world + "__" + "BRN_" + brain + "__" + code + "__" + "MHT_" + ht + "/" + rep + "/"
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
                                break

                            # let's merge them into one tidy file to output for later
                            filemerge["rep"] = rep
                            filemerge["brain"] = brain
                            filemerge["gatetype"] = gatetype
                            filemerge["threshhold"] = threshhold
                            filemerge["weighting"] = weighting
                            filemerge["discretize"] = discretize
                            # rename the other columns to be sensible
                            filemerge = filemerge.rename(
                                columns={"score_AVE": "score"})

                            # add to our list of dataframes for each k
                            merged_file = merged_file.append(filemerge)
                elif brain == "RNN":
                    for wr in rwr:
                        for dr in rdr: 
                            weighting = wr
                            discretize = dr
                            globpath = source_datapath + "C*" + "WLD_" + world + "__" + "BRN_" + brain + "__" + "RWR_" + wr + "__" + "RDR_" + dr + "/" + rep + "/"
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
                                break

                            # let's merge them into one tidy file to output for later
                            filemerge["rep"] = rep
                            filemerge["brain"] = brain
                            filemerge["gatetype"] = gatetype
                            filemerge["threshhold"] = threshhold
                            filemerge["weighting"] = weighting
                            filemerge["discretize"] = discretize
                            # rename the other columns to be sensible
                            filemerge = filemerge.rename(
                                columns={"score_AVE": "score"})

                            # add to our list of dataframes for each k
                            merged_file = merged_file.append(filemerge)
    filepath = final_datapath + "merged_" + filename
    merged_file.to_csv(filepath,index=False)

files = [lod_data_filename, max_filename]
for fname in files:
    merge_my_file(fname)