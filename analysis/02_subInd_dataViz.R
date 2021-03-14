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

# Files to process
datafiles <- c("merged_LOD_data.csv")

#############
# Visualization Function
#############

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
  
data_viz_time <- function(df, data_filename) {
  ## DATA PROCESSING & PLOTTING FOR SCORE ACROSS TIME
  
  # grab the file prefix (before .csv)
  # credit: https://stackoverflow.com/questions/30836747/regex-to-remove-csv-in-r
  data_prefix <- str_extract(data_filename, '.*(?=\\.csv$)')

  #filter the data down to what we actually want
  df_meanscore <- df %>%
    group_by(compstruct, update, world) %>% # group_by preserves the columns we want to have as variables
    summarise(mean.score=mean(score),
              sd.score=sd(score),
              n=n()) %>%
    filter(n != 1) %>%
    mutate(se.score = sd.score/sqrt(n), # get confidence intervals
           lower.ci.score = mean.score - qt(1-(0.05/2), n-1)*se.score,
           upper.ci.score = mean.score + qt(1-(0.05/2), n-1)*se.score) %>%
    {.}
  # create visualizations
  plot_meanscore <- ggplot(data=df_meanscore, aes(x=update,y=mean.score, color=compstruct)) +
    geom_line() +
    geom_ribbon(aes(ymin=lower.ci.score,ymax=upper.ci.score, fill=compstruct), linetype=0, alpha=0.3) +
    facet_wrap(~world, ncol=3) +
    theme_minimal() +
    theme(legend.position = "bottom") +
    xlab("Generations") +
    ylab("Score") +
    theme(axis.title=element_text(size=14)) +
    NULL
  
  # ouput file
  score_filename <- paste(data_prefix, "_time_score.png",sep="")
  ggsave(filename=paste(fig_path,score_filename,sep=""),plot=plot_meanscore, width=12, height=8, units="in")

}

data_viz_end <- function(df, data_filename) {

  ## DATA PROCESSING & VISUALIZATION FOR FINAL SCORE
  
  # grab the file prefix (before .csv)
  # credit: https://stackoverflow.com/questions/30836747/regex-to-remove-csv-in-r
  data_prefix <- str_extract(data_filename, '.*(?=\\.csv$)')
  
  df_endscore <- df %>%
    filter(update == 200000) %>%
    {.}
  
  # add color scale
  color_map <- c("Markov" = "#CC3300", 
                 "Markov RNN bitted" = "#FF6633", 
                 "Markov RNN" = "#FF9966", 
                 "RNN sparse discretized"  = "#CC99FF", 
                 "RNN discretized" = "#9966CC", 
                 "RNN sparse" = "#9966CC", 
                 "RNN" = "#663399")
  
  plot_endscore <- ggplot(data=df_endscore,
                     aes(x=compstruct, y=score, fill=compstruct, alpha=0.5, color=compstruct)) +
    geom_violin(scale="width", aes(color=NA)) +
    geom_boxplot(aes(fill=NA),width=0.1)+
    stat_summary(fun=mean, geom="point", size=2, color="black") +
    scale_fill_manual(values=color_map) + 
    scale_color_manual(values=color_map)+
    facet_wrap(~world, ncol=1, scales="free_y") +
    theme_bw() +
    theme(legend.position = "none") +
    xlab("\nComputational Structure") +
    ylab("Final Score\n") +
    theme(axis.title=element_text(size=14)) +
    theme(axis.text.x = element_text(angle=45, hjust=1))+
    theme(strip.text = element_text(size=14)) +
    theme(strip.background=element_rect(fill="white"))
    NULL
  
  endscore_filename <- paste(data_prefix, "_end_score.pdf",sep="")
  ggsave(filename=paste(fig_path,endscore_filename,sep=""),plot=plot_endscore, width=4.5, height=12, units="in")
  paste("endscore done")
}

###########
# Call the function
##########


for (file in datafiles) {
  data_viz_end(data_load(file), file)
  #data_viz_time(data_load(file),file)
}
