!/bin/sh
echo "###############################################################################################"
echo "running kinit: "
#eval kinit $USER
echo "running aklog: "
#eval aklog
echo "###############################################################################################"

SIMULATE="false"

ptHatBinEs=(11 15 20 20 25 30 40 50 -1);

for ipth in {0..$NBINS}; do
    echo "Submitting jobs for ptHatBin: ${ptHatBinEs[$ipth]} to ${ptHatBinEs[$ipth+1]}"
    #bash SubmitAllPtHatJobs.sh ${ptHatBinEs[$ipth]} ${ptHatBinEs[$ipth+1]} $SIMULATE
done



