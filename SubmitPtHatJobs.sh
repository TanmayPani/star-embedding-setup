#!/bin/sh
echo "###############################################################################################"
echo "running kinit: "
eval kinit $USER
echo "running aklog: "
eval aklog
echo "###############################################################################################"

PTHATMIN=$1
PTHATMAX=$2
SIMULATE=$3

PTHATBIN="${PTHATMIN}_${PTHATMAX}"

echo "Submitting jobs for ptHatBin: $PTHATBIN"

CURRENTDIR=$(pwd)

DATE=$(date +%Y%m%d)
#DATE="20230806"

MYJOBNAME="MuToTree"
NFILESPERJOB=20

PROD="P18ih"
LIB="SL18h"
YEAR="2014"
TRGSETUP="AuAu_200_production_2014"
GENERATOR="Pythia6"
EMBEDDINGDATE="20192901"
FILETYPE="MuDst.root"

FOLDERNAME="${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_${MYJOBNAME}${DATE}"
OUTDIR="/gpfs01/star/pwg/$USER/output/$FOLDERNAME"
XMLDIR="JOBXML_FILES/$FOLDERNAME"
FILELISTSDIR="$CURRENTDIR/fileLists/${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_${MYJOBNAME}"

mkdir -p $OUTDIR
mkdir -p $XMLDIR

echo "##################### Working on ptHat bin: $PTHATBIN #####################"
nGroups=0
FILELISTHEAD="${FILELISTSDIR}/pt${PTHATBIN}_"
for fileListName in ${FILELISTHEAD}*; do
echo "###############################################################################################"
finalOutDir="${OUTDIR}/pt${PTHATBIN}_${nGroups}"
mkdir -p $finalOutDir
mkdir -p $finalOutDir/tree
mkdir -p $finalOutDir/hist 
mkdir -p $finalOutDir/log
mkdir -p $finalOutDir/gen
echo "Output folder: $finalOutDir"
echo "Creating XML job file for list: $fileListName"

XMLFILE="$XMLDIR/${PTHATBIN}_${nGroups}.xml"

#add these lines to the XML file to save the output files
#<output fromScratch="*.tree.root" toURL="file:$finalOutDir/tree/" /> 
#<output fromScratch="*.hist.root" toURL="file:$finalOutDir/hist/" /> 

cat> "$XMLFILE" <<EOL
<?xml version="1.0" encoding="utf-8" ?>
<job  name="${MYJOBNAME}_${PTHATBIN}_${nGroups}" maxFilesPerProcess="$NFILESPERJOB" fileListSyntax="paths" simulateSubmission ="$SIMULATE" >

    <input URL="filelist:$fileListName" nFiles= "all" />
    <stdout URL="file:$finalOutDir/log/\$JOBID.log" />
    <stderr URL="file:$finalOutDir/log/\$JOBID.err" />
    <Generator> 
        <Location>$finalOutDir/gen/</Location>
    </Generator> 

    <SandBox>
        <Package>
            <File>file:./genDst.C</File>
            <File>file:./readPicoDstAnalysisMaker.C</File>
            <File>file:./StRoot/</File>
            <File>file:./.sl73_gcc485/</File>
        </Package>
    </SandBox>

    <command>
        starver new
        ln -s $FASTJET/include/fastjet
        ln -s $FASTJET/include/siscone
        setenv FASTJET $FASTJET

        @ nFile=0
        while ( \$nFile &lt; \$INPUTFILECOUNT )
            eval set fileName='\$INPUTFILE'\${nFile}
            echo "Processing file: \$fileName"
            root4star -l -b -q 'genDst.C(-1, "'"picoDst,PicoVtxMode:PicoVtxVpdOrDefault,PicoCovMtxMode:PicoCovMtxWrite"'", "'"\$fileName"'")'
            echo "converted file: \$fileName into picoDst.root"
            rm -f \$fileName

            root -b -q -l readPicoDstAnalysisMaker.C\(\"\$fileName\",\"dummyFileName.root\",1000000000\)
            rm -f ./INPUTFILES/*.picoDst.root

            cp *.hist.root $finalOutDir/hist/
            cp *.tree.root $finalOutDir/tree/

            rm -f *.hist.root
            rm -f *.tree.root

            @ nFile++
        end
    </command>
        
</job>
EOL
echo "Submitting job for file list: $fileListName"
star-submit $XMLFILE
nGroups=$(($nGroups+1))
done
rm -rf *.session.xml
rm -rf *.dataset
echo "###############################################################################################"



