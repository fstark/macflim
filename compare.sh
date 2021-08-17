#!/bin/bash

COMMON="sweet-dreams.mp4 --from 0 --duration 360 --filters k10w10gbbsc --bars false"

# ./flimmaker sweet-dreams.mp4 --out sweet-dreams-plus-1.flim --mp4 output1.mp4 --bars none --filters Zzk10w20g1bbsc &

# ./flimmaker sweet-dreams.mp4 --profile perfect --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true --filters Zzk10w20g1bbsc --bars false --mp4 output2.mp4 --out sweet-dreams-plus.flim &

# ./flimmaker sweet-dreams.mp4 --profile plus --out sweet-dreams-plus-2.flim --mp4 output3.mp4 --bars none --filters Zzk10w20g1bbsc &

# ./flimmaker sweet-dreams.mp4 --profile plus --dither error --error-algorithm floyd --error-bleed 0.88 --error-bidi true --filters k10w10gbbsc --bars false --filters k1gbbsc --mp4 output4.mp4 --out sweet-dreams-plus.flim &



# ./flimmaker ${COMMON} --mp4 output1.mp4 --watermark 1 --profile perfect --dither ordered &
# ./flimmaker ${COMMON} --mp4 output2.mp4 --watermark 2 --profile perfect --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true &
# ./flimmaker ${COMMON} --mp4 output3.mp4 --watermark 3 --profile plus &
# ./flimmaker ${COMMON} --mp4 output4.mp4 --watermark 4 --profile plus --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true &

# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1bbsc --mp4 output1.mp4 &
./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1bbsc --mp4 output2.mp4 --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true &
# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1.6bbsc --mp4 output3.mp4 &
# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1.9bbsc --mp4 output4.mp4 &


wait

ffmpeg -y -i output1.mp4 -i output2.mp4 -i output3.mp4 -i output4.mp4 -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]" -map "[v]" grid.mp4

open grid.mp4


# bleed = 1 for plus and se
# se30 => floyd bidi


