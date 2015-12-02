#!/bin/sh
export LD_LIBRARY_PATH=/media/nfs/lib/lib:$LD_LIBRARY_PATH

./CRForest-Detector 2 example/config.txt
