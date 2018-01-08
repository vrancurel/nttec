#!/bin/bash

bin=./benchmark
samples_nb=100
chunk_size=512
sce_type=enc_dec

for ec_type in gf2nrsv gf2nrsc gf2nfftrs gf2nfftaddrs gfpfftrs fntrs ngff4rs; do
  for k in 5; do
    for m in 2; do
      for word_size in 1 2 4 8; do
        echo ${bin}_e${ec_type}_w${word_size}_k${k}_m${m}_c${chunk_size}_s${sce_type}
        ${valgrind} ${bin} -e ${ec_type} -w ${word_size} -k ${k} -m ${m} -c ${chunk_size} -s ${sce_type}
      done
    done
  done
done

chunk_size=51200
sce_type=enc_only
for word_size in 1 2 4 8; do
  for ec_type in ngff4rs fntrs gfpfftrs; do
    for k in 8 16 32; do
      for m in 8 16 32 64; do
        echo ${bin}_e${ec_type}_w${word_size}_k${k}_m${m}_c${chunk_size}_s${sce_type}
        ${valgrind} ${bin} -e ${ec_type} -w ${word_size} -k ${k} -m ${m} -c ${chunk_size} -s ${sce_type}
      done
    done
  done
done
