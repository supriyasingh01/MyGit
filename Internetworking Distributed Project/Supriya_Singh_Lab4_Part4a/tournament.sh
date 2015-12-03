#1/bin/bash
START=$(date +%s)
./clientUDP nodea 3456
END=$(date +%s)
DIFF=$(( $END - $START ))
echo "It took $DIFF seconds"
Throughput=`expr 1024*8/DIFF`
echo "Throughput: in seconds $Throughput"
md5sum data.bin
