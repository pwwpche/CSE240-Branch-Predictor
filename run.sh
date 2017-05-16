make clean
make
echo "int_1:" >> res.txt
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --gshare:12 >> res.txt
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --tournament:14:12:14 >> res.txt
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --custom >> res.txt
echo "======" >> res.txt
echo "int_2:" >> res.txt
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --gshare:12 >> res.txt
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --tournament:14:12:14 >> res.txt
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --custom >> res.txt
echo "======"
echo "fp_1:" >> res.txt
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --gshare:12 >> res.txt
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --tournament:14:12:14 >> res.txt
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --custom >> res.txt
echo "======"
echo "fp_2:" >> res.txt
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --gshare:12 >> res.txt
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --tournament:14:12:14 >> res.txt
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --custom>> res.txt
echo "======"
echo "mm_1:" >> res.txt
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --gshare:12 >> res.txt
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --tournament:14:12:14 >> res.txt
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --custom >> res.txt
echo "======"
echo "mm_2:" >> res.txt
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --gshare:12 >> res.txt
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --tournament:14:12:14 >> res.txt
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --custom >> res.txt
echo "======"
