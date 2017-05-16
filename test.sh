make clean
make
echo "" >> res2.txt
echo "" >> res2.txt
echo "" >> res2.txt
echo "int_1: 16.691, 9.690" >> res2.txt
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --custom >> res2.txt
echo "======" >> res2.txt
echo "int_2: 0.496, 0.349" >> res2.txt
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --custom >> res2.txt
echo "======"
echo "fp_1: 0.999, 0.981" >> res2.txt
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --custom >> res2.txt
echo "======"
echo "fp_2: 3.363, 1.997" >> res2.txt
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --custom>> res2.txt
echo "======"
echo "mm_1: 7.884, 1.526" >> res2.txt
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --custom >> res2.txt
echo "======"
echo "mm_2: 10.936, 6.885" >> res2.txt
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --custom >> res2.txt
echo "======"
