# The Comparative Hybrid Approach to Investigate Cognition across Substrates

This work is accepted as a full paper to the [2021 Conference on Artificial Life.](https://www.robot100.cz/alife2021/). You can watch our talk there between July 13-23, 2021!

The corresponding author is Sarah Albani; contact her at albanisa at msu dot edu.

## Overview

![Hybrid Brains](https://github.com/alackles/Albani_etal_2021_ALIFE2021/blob/main/figs/Markov_vs_RNN_H.png)

This work uses a new approach, the _comparative hybrid approach_, to investigate what components of computational structures allow for the performance of certain cognitive tasks. We look at **sparsity** and **discretization** as key elements of computational cognition. 

### Main Results

We found that: 

* Discretization alone helps performance on certain tasks, including a path-following task and a block-catching task, and
* Discretization alone hinders, but in combination with sparsity helps, performance on a simple memory task, and
* These results cannot be explained solely by the architecture of the brains in question

We hypothesize for future study that:

* The genetic encoding of computational substrates, not just their final virtual hardware, is important for fully understanding cognitive performance, and
* Information integration plays a key role in which genetic architectures are suitable for which cognitive tasks

## Source Code

Code in `source/` is from [MABE](https://github.com/Hintzelab/MABE); contact [Cliff Bohm](https://github.com/cliff-bohm) or other MABE project developers for assistance.

Code in `analysis/` is written by [Acacia Ackles](https://alackles.github.io/). Contact them for assistance.

## Reproducibility

All required code, including the source code for MABE, is contained in this repo; **clone the repository**, then continue to the following steps.

### Compile MABE

See the [MABE Quickstart Guide](https://github.com/Hintzelab/MABE/wiki/Installation-and-getting-started-with-MABE) for information on how to compile MABE. Brief instructions for command-line compilation of MABE are provided below.

First, make sure to `cd source` into the source folder. Then simply run:

```
sh tools/setup.cmd
./mbuild
```

This will generate the executable `MABE` inside the new work directory `work/`.

### Copy Settings Files

Settings files for this experiment are stored in `source/settings`. To run experiments with the proper conditions, **you must** copy all files from `settings/` to `work/`:

`cp settings/* work/`

### Run MABE

To ensure the experiment runs properly, you should move into the work folder with `cd work`. 

**Warning:** If you want to run the experiment exactly the way we ran it, you will be running 10 replicates of 21 different experimental conditions for 200,000 updates. **This will take a very long time**. You probably don't want to do this unless you are launching the experiments on a computing cluster. 

If you are at MSU, launching on a computing cluster can be achieved by doing the above on the HPCC and using the command

`python3 ../tools/mq.py -i`

If you are not at MSU, you can adjust down the number of runs or replicates by editing the `mq_conditions.txt` file. See again the [MABE wiki](https://github.com/Hintzelab/MABE/wiki/Installation-and-getting-started-with-MABE#generating-settings-files) for some ideas on how to do this, and in particular the [https://github.com/Hintzelab/MABE/blob/master/tools/mq_conditions.txt](default conditions file).

If you do wish to run these replicates locally, after editing the conditions file to something sane, you can run

`python3 ../tools/mq.py -l`

### Analysis Scripts

If you want to generate our data visualizations and statistical comparisons after generating your data, navigate your way to the `analysis/` folder. 

All scripts in this folder are set up to run sequentially. 

#### Data Tidying

`01_subind_repMerge.py` will compile the data from the files generated by running MABE above. 

You must specify the first and last replicate of the data, and these replicates should be sequential. For example, if you ran replicates 101 to 120, you would input:

`python3 01_subind_repMerge.py 101 120`

The result will be in the folder `data/`. 

#### Data Visualization

Data visualization is accomplished using R. In the script, you should change the variable `proj_path` to this repo's path on your local machine. Otherwise, the script can be run as-is. 

#### Statistical Comparison

Statistical comparison is also accomplished using R. As above, change your `proj_path` variable as needed. 

---

And that's it! You've successfully replicated our project!

For issues in this documentation, contact [Acacia Ackles](https://alackles.github.io/).

[//]: # see you, space cowboy...
