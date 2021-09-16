#!/bin/bash

set -e

# STEM=SinCity FROM=2:9.5 DURATION=75 VARIATION=_Intro FILTER2="--filters k10w10g1bbsczzq9" FILTER3="--filters k5w30g1.6bsczz" FILTER4="--filters k5w30g1.6sc" ./compare.sh

FLIMMAKER="./flimmaker"

OUT_DIR=/mnt/data0/WebSites/www.macflim.com/macflim2/samples

mkdir -p ${OUT_DIR}


function size {
    filesize=`du -k $1 | awk '{print $1+1024}'`
}

function encode {

# $1 = Source
# $2 = Dest
# $3 = profile
# $4 = from
# $5 = duration
# $6 = from-gif
# $7 = duration-gif
# $8 = filters

local SOURCE="$1"
local DEST="$2"
local PROFILE="$3"
local FROM="$4"
local DURATION="$5"
local GIFFROM="$6"
local GIFDURATION="$7"
local ARGS="$8"

${FLIMMAKER} "sample/source/${SOURCE}" --from ${GIFFROM} --duration ${GIFDURATION} --flim /dev/null --gif ${OUT_DIR}/${DEST}-${PROFILE}.gif --profile se30 ${ARGS}
${FLIMMAKER} "sample/source/${SOURCE}" --from ${FROM} --duration ${DURATION} --flim ${OUT_DIR}/${DEST}-${PROFILE}.flim --mp4 ${OUT_DIR}/${DEST}-${PROFILE}.mp4 --profile ${PROFILE} ${ARGS}

cd ${OUT_DIR}

hmount '/home/fred/Development/macflim/MacFlim Source Code.dsk'
hcopy -m ":player:MacFlim Player" MacFlimPlayer
humount

local FULLNAME=${DEST}-${PROFILE}

local filesize
size ${FULLNAME}.flim
local SIZE=filesize

rm -f ${FULLNAME}.zip && zip -r ${FULLNAME}.zip ${FULLNAME}.flim
dd if=/dev/zero of=${FULLNAME}.dsk bs=1k count=${filesize}
hformat -l ${FULLNAME} ${FULLNAME}.dsk
hmount ${FULLNAME}.dsk
hcopy ${FULLNAME}.flim :
hattrib -t FLIM -c FLPL :${FULLNAME}.flim
hcopy -m MacFlimPlayer :"MacFlim Player"
humount

rm -f ${FULLNAME}.dsk.zip && zip -r ${FULLNAME}.dsk.zip ${FULLNAME}.dsk
rm ${FULLNAME}.dsk

rm MacFlimPlayer

cd -
}

# encode SinCity.mp4 SinCity-Intro se30 02:09.5 75 02:10.3 4.85 "--filters k5w30g1.6sc"
# encode 300.mp4 300 se30 00:11:20.100 00:01:57 12:36 9.8
# encode 1984.mkv 1984 se30 0 360 0:20.15 2.2 "--filters k10g1.2sc"
# encode BlackOrWhite.mp4 BlackOrWhite se30 00:05:29 00:00:47 00:06:08 8 "--filters g1.6sc --error-bleed 0.8"
# encode Bad_Apple.mp4 BadApple se30 0 3:37 1:15 11 "--filters k20w20g1sc"
# encode iPodAd1.mp4 iPodIntroduction se30 0 1000 1 3 "--filters w10k10bsq5c --dither ordered"

encode Matrix.mp4 Matrix se30 01:46:15 00:00:43.280 01:46:30.200 10
