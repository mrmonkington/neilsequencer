rem     lowpass-filters the input-.wav-file
convert -raw %1.wav
filter %1.raw out.raw %1.h
snd2wav out.raw -16 -s
del out.raw
del %1.raw
del %1.wav
ren out.wav %1.wav
