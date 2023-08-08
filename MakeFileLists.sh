# Create the file list
PROD="P18ih"
LIB="SL18h"
YEAR="2014"
TRGSETUP="AuAu_200_production_2014"
GENERATOR="Pythia6"
PTHBIN="50_-1"
EMBEDDINGDATE="20192901"


FILETYPE=".MuDst.root"
MYJOBNAME="MuToTree"

FOLDERNAME="/star/data105/embedding/$TRGSETUP/${GENERATOR}_"
FILEDIR="${FOLDERNAME}pt${PTHBIN}"

FILELISTDIR="fileLists/${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_$MYJOBNAME"
mkdir -p $FILELISTDIR

nGroups=0
for folder in $FILEDIR*; do
    echo "Working on folder: $folder"
    FILELISTNAME=$FILELISTDIR/pt${PTHBIN}_${nGroups}.list
    echo "Creating file list: $FILELISTNAME"
    echo "Finding files of type: $FILETYPE"
    find "$folder/" -type f -name "*$FILETYPE" > $FILELISTNAME
    nGroups=$((nGroups+1))
done

