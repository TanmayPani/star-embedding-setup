#!/bin/sh
echo "###############################################################################################"
echo "running kinit: "
eval kinit $USER
echo "running aklog: "
eval aklog
echo "###############################################################################################"

DATE=$(date +%Y%m%d)
MYJOBNAME="MuToPico${DATE}"
NFILESPERJOB=10
SIMULATE="false"

PROD="P18ih"
LIB="SL18h"
YEAR="2014"
TRGSETUP="AuAu_200_production_2014"
GENERATOR="Pythia6"
EMBEDDINGDATE="20192901"
FILETYPE="MuDst.root"

FOLDERNAME="${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_$MYJOBNAME"
OUTDIR="/gpfs01/star/pwg/tpani/output/$FOLDERNAME"
XMLDIR="XMLScripts/$FOLDERNAME"
FILELISTSDIR="/gpfs01/star/pwg/tpani/MuDstToPicoDst/fileLists/$FOLDERNAME"

#rm -rf $OUTDIR
#rm -rf $XMLDIR
#rm -rf $FILELISTSDIR

mkdir -p $OUTDIR
mkdir -p $XMLDIR
mkdir -p $FILELISTSDIR

EMBEDDINGPATHEADER="/star/data105/embedding/$TRGSETUP/${GENERATOR}"

ptHats=("11_15" "15_20" "20_25" "25_30" "30_40" "40_50")
#ptHats=("11_15" "15_20" "20_25" "25_30" "30_40" "40_50" "50_-1")
#ptHats=("50_-1")

for pth in "${ptHats[@]}"; do
    echo "##################### Working on ptHat: $pth #####################"
    nGroups=0
    EMBEDDINGPATH="${EMBEDDINGPATHEADER}_pt${pth}_"
    for folder in $EMBEDDINGPATH*; do
        echo "###############################################################################################"
        fileListName="$FILELISTSDIR/pt${pth}_$nGroups.list"
        echo "Working on folder: $folder"
        echo "Creating file list: $fileListName"
        find "$folder" -type f -name "*.$FILETYPE" > $fileListName

        finalOutDir="${OUTDIR}/pt${pth}_${nGroups}"
        mkdir -p $finalOutDir
        mkdir -p $finalOutDir/out 
        mkdir -p $finalOutDir/log
        mkdir -p $finalOutDir/gen
        echo "Output folder: $finalOutDir"
        echo "Creating XML job file for list: $fileListName"

        XMLFILE="$XMLDIR/${pth}_${nGroups}.xml"

cat> "$XMLFILE" <<EOL
<?xml version="1.0" encoding="utf-8" ?>
<job  name="${MYJOBNAME}_${pth}_${nGroups}" maxFilesPerProcess="$NFILESPERJOB" fileListSyntax="paths" simulateSubmission ="$SIMULATE" >

    <input URL="filelist:$fileListName" nFiles= "all" />
    <stdout URL="file:$finalOutDir/log/\$JOBID.log" />
    <stderr URL="file:$finalOutDir/log/\$JOBID.err" />
    <Generator> 
        <Location>$finalOutDir/gen/</Location>
    </Generator> 

    <SandBox>
        <Package>
            <File>file:./genDst.C</File>
            <File>file:./StRoot/</File>
            <File>file:./.sl73_gcc485/</File>
        </Package>
    </SandBox>

    <command>
        starver new
        @ nFile=0
        while ( \$nFile &lt; \$INPUTFILECOUNT )
            eval set fileName='\$INPUTFILE'\${nFile}
            echo "Processing file: \$fileName"
            root4star -l -b -q 'genDst.C(-1, "'"picoDst,PicoVtxMode:PicoVtxVpdOrDefault,PicoCovMtxMode:PicoCovMtxWrite"'", "'"\$fileName"'")'
            echo "Done processing file: \$fileName"
            cp ./INPUTFILES/*.picoDst.root $finalOutDir/out/.
            rm -f ./INPUTFILES/*.picoDst.root
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
done



