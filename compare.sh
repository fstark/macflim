#!/bin/bash

set -e

cd out

FLIMMAKER="../flimmaker"

FFMPEG_ARGS="-loglevel info -y"

FROM=${FROM:-"0"}
DURATION=${DURATION:-"1000"}

STEM=${STEM:-}

# STEM=Pulp_Fiction_Dance
# STEM=The_Wall
# STEM=1984
# STEM=Amiga_Ball
# STEM=The_Shining_Johnny
# STEM=2001
EXT=${EXT:-"mp4"}
VARIATION=${VARIATION:-""}
SOURCE="../sample/source/${STEM}.${EXT}"
STEM_OUT="${STEM}${VARIATION}"

# SOURCE="sample/source/${STEM}.mkv"
COMMON_ARGS=""
# "--from ${FROM} --duration ${DURATION}"
FILTER2=${FILTER2:-}
FILTER3=${FILTER3:-}
FILTER4=${FILTER4:-}

echo ""
echo " ================ ENCODING =============== "
echo ""
echo "FROM = ${FROM} DURATION= ${DURATION}"
echo "STEM = ${STEM} EXT= ${EXT}"
echo ""


# ./flimmaker sweet-dreams.mp4 --out sweet-dreams-plus-1.flim --mp4 output1.mp4 --bars none --filters Zzk10w20g1bbsc &
# ./flimmaker sweet-dreams.mp4 --profile perfect --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true --filters Zzk10w20g1bbsc --bars false --mp4 output2.mp4 --out sweet-dreams-plus.flim &
# ./flimmaker sweet-dreams.mp4 --profile plus --out sweet-dreams-plus-2.flim --mp4 output3.mp4 --bars none --filters Zzk10w20g1bbsc &
# ./flimmaker sweet-dreams.mp4 --profile plus --dither error --error-algorithm floyd --error-bleed 0.88 --error-bidi true --filters k10w10gbbsc --bars false --filters k1gbbsc --mp4 output4.mp4 --out sweet-dreams-plus.flim &

# ./flimmaker ${COMMON} --mp4 output1.mp4 --watermark 1 --profile perfect --dither ordered &
# ./flimmaker ${COMMON} --mp4 output2.mp4 --watermark 2 --profile perfect --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true &
# ./flimmaker ${COMMON} --mp4 output3.mp4 --watermark 3 --profile plus &
# ./flimmaker ${COMMON} --mp4 output4.mp4 --watermark 4 --profile plus --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true &

# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1bbsc --mp4 output1.mp4 &
# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1bbsc --mp4 output2.mp4 --dither error --error-algorithm floyd --error-bleed 1 --error-bidi true &
# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1.6bbsc --mp4 output3.mp4 &
# ./flimmaker sweet-dreams.mp4 --from 0 --duration 920 --profile plus --bars false --filters Z24k8w8g1.9bbsc --mp4 output4.mp4 &

ffmpeg ${FFMPEG_ARGS} -ss "${FROM}" -t "${DURATION}" -y -i ${SOURCE} -map_metadata -1 "${STEM_OUT}-original.mp4"

echo " VERSION 1 "

ffmpeg ${FFMPEG_ARGS} -y -i "${STEM_OUT}-original.mp4" -vf "scale=(iw*sar)*max(512.1/(iw*sar)\,342.1/ih):ih*max(512.1/(iw*sar)\,342.1/ih), crop=512:342" "${STEM_OUT}-small.mp4"

echo " VERSION 2 "

${FLIMMAKER} "${STEM_OUT}-original.mp4" ${COMMON_ARGS} --out ${STEM_OUT}-plus.flim --mp4 ${STEM_OUT}-plus.mp4 --watermark "" --profile plus ${FILTER2}

echo " VERSION 3 "

${FLIMMAKER} "${STEM_OUT}-original.mp4" ${COMMON_ARGS} --out ${STEM_OUT}-se.flim --mp4 ${STEM_OUT}-se.mp4 --watermark "" --profile se ${FILTER3}

echo " VERSION 4 "

${FLIMMAKER} "${STEM_OUT}-original.mp4" ${COMMON_ARGS} --out ${STEM_OUT}-se30.flim --mp4 ${STEM_OUT}-se30.mp4 --watermark "" --profile se30 ${FILTER4}

wait

echo " GRID "

ffmpeg ${FFMPEG_ARGS} -i "${STEM_OUT}-small.mp4" -i "${STEM_OUT}-plus.mp4" -i "${STEM_OUT}-se.mp4" -i "${STEM_OUT}-se30.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" ${STEM_OUT}-grid.mp4

# xdg-open ${STEM_OUT}-grid.mp4
vlc ${STEM_OUT}-grid.mp4 &



echo ""
echo " ========================================= "
echo ""

exit


# bleed = 1 for plus and se
# se30 => floyd bidi

./flimmaker sample/source/Amiga_Ball.mp4 --out Amiga_Ball-plus.flim --mp4 Amiga_Ball-plus.mp4 --profile plus --filters gq5 --from 30 --duration 10




#!/bin/bash




FFMPEG_ARGS="-loglevel warning -y -pattern_type sequence -start_number 1 -i "out-%06d.pgm" -s 512x342 -c:v libx264"

ffmpeg  -y -i sample/source/${STEM}.mp4 -vf "scale=(iw*sar)*max(512.1/(iw*sar)\,342.1/ih):ih*max(512.1/(iw*sar)\,342.1/ih), crop=512:342" "output1.mp4"

../../flimmaker ${FLIMAKER_ARGS} --stability 0.5 --out output2.flim --filters gscz --dither ordered
ffmpeg  -framerate 23.976 ${FFMPEG_ARGS} output2.mp4

../../flimmaker ${FLIMAKER_ARGS} --stability 0.5 --out output3.flim --filters gsqcz
ffmpeg -framerate 23.976 ${FFMPEG_ARGS} output3.mp4

../../flimmaker ${FLIMAKER_ARGS} --stability 0.5 --out output4.flim --filters gsqcz --dither ordered
ffmpeg -framerate 23.976 ${FFMPEG_ARGS} output4.mp4

ffmpeg -y -i output1.mp4 -i output2.mp4 -i output3.mp4 -i output4.mp4 -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]" -map "[v]" grid.mp4


#../../flimmaker ${FLIMAKER_ARGS} --stability 0.5 --out output1.flim --byterate 6000 --half-rate 
#ffmpeg -framerate 60 ${FFMPEG_ARGS} output1.mp4
#../../flimmaker ${FLIMAKER_ARGS} --stability 0.3 --out output4.flim --byterate 21888 --group
#ffmpeg -framerate 23.976 ${FFMPEG_ARGS} output4.mp4







./flimmaker ${COMMON_ARGS} --mp4 ${STEM}-plus.mp4 --watermark "PLUS" --profile plus
Encoding arguments :
--byterate 1500 --half-rate true --group false --bars true --dither ordered --filters gbbscz --codec null --codec z32 --codec lines --codec invert
Read 600 frames
**** fps                : 60
**** # of input  images : 600
**** # of output frames : 600
flimmaker: flimcompressor.hpp:205: void flimcompressor::compress(double, size_t, bool, const string&, const string&, const std::vector<flimcompressor::codec_spec>&, image::dithering, bool, std::string, float, bool): Assertion `ticks>0' failed.
./compare.sh: line 31: 50139 Aborted                 (core dumped) ./flimmaker ${COMMON_ARGS} --mp4 ${STEM}-plus.mp4 --watermark "PLUS" --profile plus
