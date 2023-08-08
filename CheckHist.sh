#!/bin/bash

HISTDIR="./mergedHistograms"
for histFile in $HISTDIR/pt*.hist.root; do
    echo $histFile
    root4star -l -b -q checkHist.C\(\"$histFile\"\)
done