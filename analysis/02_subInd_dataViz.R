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
library(gridExtra)

# Files to process
#datafiles <- c("lod_merge.csv", "max_merge.csv", "pop_merge.csv")
datafiles <- c("merged_max.csv")

#############
# Visualization Function
#############

# file must be ALREADY MERGED using 01_assoclearning_repMerge.py
data_visualize <- function(data_filename) {

  # grab the file prefix (before .csv)
  # credit: https://stackoverflow.com/questions/30836747/regex-to-remove-csv-in-r
  data_prefix <- str_extract(data_filename, '.*(?=\\.csv$)')
  
  # read in the datafile
  df <- read.csv(paste(data_path, data_filename, sep=""))
  
  # identify factors
  facs = c("rep", "brain", "gatetype", "threshhold", "weighting", "discretize")
  df[facs] <- lapply(df[facs], as.factor)
  
  # filter the data down to what we actually want
  df_summary <- df %>% 
    group_by(brain, update, gatetype, threshhold, weighting, discretize) %>% # group_by preserves the columns we want to have as variables
    summarise(mean.score=mean(score), 
              sd.score=sd(score), 
              n=n()) %>%
              #goal=mean(sum(reachGoal_AVE != 0)/n)) %>% # how many solved the problem?
    mutate(se.score = sd.score/sqrt(n), # get confidence intervals
           lower.ci.score = mean.score - qt(1-(0.05/2), n-1)*se.score,
           upper.ci.score = mean.score + qt(1-(0.05/2), n-1)*se.score)
  
  # create visualizations
  print(head(df_summary))

  # scores of Markov Brains
  df_score_markov <- ggplot(data=subset(df_summary, brain=="Markov"), aes(x=update,y=mean.score, color=threshhold)) +
    geom_line() +
    geom_ribbon(aes(ymin=lower.ci.score,ymax=upper.ci.score, fill=threshhold), linetype=0, alpha=0.3) +
    facet_wrap(~gatetype) +
    theme_minimal() +
    theme(legend.position = "bottom") +
    xlab("Generations") +
    ylab("Score") +
    #geom_hline(yintercept = 0, linetype = "dashed", color="black") +
    theme(axis.title=element_text(size=14)) +
    NULL
  # scores of RNNs
  df_score_rnn <- ggplot(data=subset(df_summary, brain=="RNN"), aes(x=update,y=mean.score, color=discretize)) +
    geom_line() +
    geom_ribbon(aes(ymin=lower.ci.score,ymax=upper.ci.score, fill=discretize), linetype=0, alpha=0.3) +
    facet_wrap(~weighting) +
    theme_minimal() +
    theme(legend.position = "bottom") +
    xlab("Generations") +
    ylab("Score") +
    #geom_hline(yintercept = 0, linetype = "dashed", color="black") +
    theme(axis.title=element_text(size=14)) +
    NULL
  
  combo_plot <- grid.arrange(df_score_markov, df_score_rnn, ncol = 1)
  score_filename <- paste(data_prefix, "_score.pdf")
 #  
 #  # proportion that reached the goal
 # df_solve <- ggplot(data=df_summary, aes(x=update,y=goal, color=brain)) + 
 #    geom_line() + 
 #    facet_wrap(~turn) +
 #    theme_minimal() +
 #    theme(legend.position = "bottom") +
 #    xlab("Generations") +
 #    ylab("Proportion Reached Goal") +
 #    theme_dark() + 
 #    theme(axis.title=element_text(size=14))
 #  solve_filename <- paste(data_prefix, "_solve.pdf")
 #  
  ggsave(filename=paste(fig_path,score_filename,sep=""),plot=combo_plot, width=9, height=9, units="in")
 #  ggsave(filename=paste(fig_path,solve_filename,sep=""),plot=df_solve, width=10, height=5, units="in")
 #  
  return(paste(data_prefix, "done"))
}

###########
# Call the function
##########

lapply(datafiles, data_visualize)
