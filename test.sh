make clean
make
echo "" >> res2.txt
echo "" >> res2.txt
echo "" >> res2.txt
echo "int_1: 14.042, 10.922" >> res2.txt
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --custom >> res2.txt
echo "======" >> res2.txt
echo "int_2: 0.456, 0.320" >> res2.txt
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --custom >> res2.txt
echo "======"
echo "fp_1: 0.988, 0.979" >> res2.txt
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --custom >> res2.txt
echo "======"
echo "fp_2: 7.080, 1.502" >> res2.txt
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --custom>> res2.txt
echo "======"
echo "mm_1: 6.995, 2.011" >> res2.txt
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --custom >> res2.txt
echo "======"
echo "mm_2: 9.790, 6.722" >> res2.txt
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --custom >> res2.txt
echo "======"
