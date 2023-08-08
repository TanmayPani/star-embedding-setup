#!/bin/bash

OUTDIR="/gpfs01/star/pwg/tpani/output/Pythia6Embedding_AuAu_200_production_2014_P18ih_SL18h_20192901_MuToTree20230806"

TYPE="hist"

for folders in $OUTDIR/*; do
    echo $folders
    MERGEDFILENAME="$(basename $folders).$TYPE.root"
    hadd -f -n 50 "$MERGEDFILENAME" $folders/$TYPE/*.$TYPE.root
done

