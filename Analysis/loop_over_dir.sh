# This script can loop over the root files in root data/
# The characteristic of TCahin will merge the Tree under all files
# So this is a bypass way for dealing with different data sheet without
# merge them.
# please do "make" before run it
FILES="root_data/CERN_BD/tar_root/*.root"
for f in $FILES
do
    echo "Processing file $f ... "
    ls $f > input.txt
    ./makePlots
done
