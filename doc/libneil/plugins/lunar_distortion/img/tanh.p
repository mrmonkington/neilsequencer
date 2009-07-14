set terminal postscript eps enhanced color
set nokey
set title "HyTan"
set xlabel "x"
set ylabel "y"
set xr [-4:4]
set yr [-1:1]
plot (exp(x) - exp(-x)) / (exp(x) + exp(-x))
   
