# Create the file list
PROD="P18ih"
LIB="SL18h"
YEAR="2014"
TRGSETUP="AuAu_200_production_2014"
GENERATOR="Pythia6"
EMBEDDINGDATE="20192901"
FILETYPE="picoDst.root"
MYJOBNAME="MuToPico20230718"

FOLDERNAME="${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_$MYJOBNAME"
FILEDIR="/gpfs01/star/pwg/tpani/output/$FOLDERNAME"

FILELISTDIR="fileLists/${GENERATOR}Embedding_${TRGSETUP}_${PROD}_${LIB}_${EMBEDDINGDATE}_$MYJOBNAME"
mkdir -p $FILELISTDIR

for folder in $FILEDIR/*; do
    FILELISTNAME=$FILELISTDIR/$(basename $folder).list
    find "$folder/" -type f -name "*.picoDst.root" > $FILELISTNAME
done

