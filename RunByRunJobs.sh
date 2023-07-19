#!/bin/sh

echo "###############################################################################################"
echo "running kinit: "
eval kinit $USER
echo "running aklog: "
eval aklog
echo "###############################################################################################"

OUTDIR="/gpfs01/star/pwg/tpani/output/EmbeddingPicosToTree"
mkdir -p $OUTDIR

MYJOBNAME=$(date +%Y%m%d)
NMAXPROCESSFILES="10"
SIMULATE="false"

PROD="P18ih"
LIB="SL18h"
YEAR="2014"
TRGSETUP="AuAu_200_production_2014"
GENERATOR="Pythia6"
EMBEDDINGDATE="20192901"

FILELISTHEAD="fileLists/${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_MuToPico20230718"

if [[ $SIMULATE == "true" ]]; then
	NFILES="20"
elif [[ $SIMULATE == "false" ]]; then
	NFILES="all"
else
	NFILES="all"
fi

JOBUSERNAME="${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${MYJOBNAME}"
mkdir -p $OUTDIR/$JOBUSERNAME

mkdir -p JOBXML_FILES/${JOBUSERNAME}

PTHATS=("11_15" "15_20" "20_25" "25_30" "30_40" "40_50" "50_-1")

ITERATION=0

for PTHAT in "${PTHATS[@]}"; do #Set to some really big number for a real submission

	#mkdir -p $OUTDIR/$JOBUSERNAME/$RUNNUMBER
	mkdir -p $OUTDIR/$JOBUSERNAME/gen
	mkdir -p $OUTDIR/$JOBUSERNAME/log
	mkdir -p $OUTDIR/$JOBUSERNAME/out 
	mkdir -p $OUTDIR/$JOBUSERNAME/out/EventTrees
	mkdir -p $OUTDIR/$JOBUSERNAME/out/Histograms
	#mkdir -p $OUTDIR/$JOBUSERNAME/out/JetTrees 

	XMLSCRIPT="JOBXML_FILES/$JOBUSERNAME/SubmitJobs_$RUNNUMBER.xml"

cat> "$XMLSCRIPT" <<EOL
<?xml version="1.0" encoding="utf-8" ?> 
<job  name="D${MYJOBNAME}_R${RUNNUMBER}_" maxFilesPerProcess="$NMAXPROCESSFILES" fileListSyntax="xrootd" simulateSubmission="$SIMULATE" >

	<input URL="" nFiles="$NFILES" />

	<stdout URL="file:$OUTDIR/$JOBUSERNAME/log/${RUNNUMBER}_\$JOBINDEX.log" />
	<stderr URL="file:$OUTDIR/$JOBUSERNAME/log/${RUNNUMBER}_\$JOBINDEX.err" />
	<output fromScratch="EventTree_${RUNNUMBER}_\$JOBINDEX.root" toURL="file:$OUTDIR/$JOBUSERNAME/out/EventTrees/" /> 
	<output fromScratch="Histograms_${RUNNUMBER}_\$JOBINDEX.root" toURL="file:$OUTDIR/$JOBUSERNAME/out/Histograms/" /> 
	<Generator> 
		<Location>$OUTDIR/$JOBUSERNAME/gen/</Location> 
	</Generator>

    <SandBox>
        <Package>
            <File>file:./readPicoDstAnalysisMaker.C</File>
            <File>file:./StRoot/</File>
            <File>file:./.sl73_gcc485/</File>
        </Package>
    </SandBox>

	<command>
		ln -s $FASTJET/include/fastjet
		ln -s $FASTJET/include/siscone

		starver new
		setenv FASTJET $FASTJET

	    root -b -q -l readPicoDstAnalysisMaker.C\(\"\$FILELIST\",\"${RUNNUMBER}_\$JOBINDEX.root\",1000000000\)
	   
	    unlink fastjet
	    unlink siscone   
	</command>

</job>
EOL
	star-submit $XMLSCRIPT
done < $RUNLIST

rm -r *.dataset *.session.xml

#bash concheck.sh $myDate $BNLusername
