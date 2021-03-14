# Acacia Ackles
# Created for: Data visualization with associative learning#

##########
# Setup
##########

# Set paths

proj_path <- "/home/acacia/Documents/research/substrate_independence/"
data_path <- paste(proj_path, "data/", sep="")
fig_path <- paste(proj_path, "figs/", sep="")

# Load libraries
library(tidyverse)
library(emmeans)

# Files to process
datafile <- "merged_LOD_data.csv"


# file must be ALREADY MERGED using 01_subInd_repMerge.py
data_load <- function(data_filename) {

  
  # read in the datafile
  df <- read.csv(paste(data_path, data_filename, sep=""))
  
  # identify factors
  facs = c("rep", "compstruct", "world")
  df[facs] <- lapply(df[facs], as.factor)
  
  # reorder the compstruct factors
  df$compstruct <- factor(df$compstruct, 
                          levels = c("Markov",
                                     "Markov RNN bitted",
                                     "Markov RNN",
                                     "RNN sparse discretized",
                                     "RNN discretized",
                                     "RNN sparse",
                                     "RNN"))
  
  # reorder the worlds
  df$world <- factor(df$world,
                     levels=c("NBack",
                              "PathFollow",
                              "BlockCatch"))
  
  return(df)

}

df <- data_load(datafile)

df <- df %>%
  filter(update==200000)

# EMm gives us which values are significant
model_fit <- glm(data=df,score ~ compstruct*world)
emm <- emmeans(model_fit, specs=pairwise~compstruct|world)
summary(emm)

# specifically test whether MB and MB RNN bitted in BlockCatch are different distributions
# they are not
blockcatch.mb <- df %>%
  filter(update==200000, compstruct=="Markov", world=="BlockCatch") %>%
  select(score) %>%
  unlist %>%
  as.numeric()

blockcatch.mbrnn <- df %>%
  filter(update==200000, compstruct=="Markov RNN bitted", world=="BlockCatch") %>%
  select(score) %>%
  unlist %>%
  as.numeric()

ks <- ks.test(blockcatch.mb, blockcatch.mbrnn)
ks