set terminal postscript eps enhanced color
set nokey
set title "Assym"
set xlabel "x"
set ylabel "y"
set xr [-5:5]
set yr [-3:1]
scale(x) = x * 0.686306;
a(x) = 1.0 + exp(sqrt(abs(scale(x))) * -0.75)
f(x) = (exp(scale(x)) - exp(-scale(x) * a(x))) / (exp(scale(x)) + exp(-scale(x)))
plot f(x)