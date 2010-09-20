#!/usr/bin/env bash 

rm release/*.[ch] release/vodka*

for i in `ls *.[ch]`; do
    unifdef -DMETAOBJECT_PROTOCOL=0 -DBLOCK_MEMORY=0 -DWINDOW_INTERFACE=0 -DPROFILING_FUNCTIONS=0 -DDEBUGGING_FUNCTIONS=0 -DCONSTRUCT_COMPILER=0 -DEMACS_EDITOR=0 -DBLOAD_ONLY=0 -DBLOAD=0 -DBLOAD_AND_BSAVE=0 -DRUN_TIME=0 -DHELP_FUNCTIONS=0 -DTEXTPRO_FUNCTIONS=0 -DEXTENDED_MATH_FUNCTIONS=0 -DBSAVE_INSTANCES=0 -DBLOAD_INSTANCES=0 -DINSTANCE_SET_QUERIES=0 -DDEFINSTANCES_CONSTRUCT=0 -DDEFFACT_CONSTRUCT=0 -DFACT_SET_QUERIES=0 -DDEFTEMPLATE_CONSTRUCT=0 -DDEFMODULE_CONSTRUCT=0 -DDEFRULE_CONSTRUCT=0 -DOBJECT_SYSTEM=0 $i > ./release/$i;
done

cp vodka.vodka release/

rm ./release/classes*

cd release/

#for pre in `ls *.[ch]`; do
#	../../../etc/obf/opqcp $pre ./obf ;
#done