#!/bin/bash

set -e

# STEM=SinCity FROM=2:9.5 DURATION=75 VARIATION=_Intro FILTER2="--filters k10w10g1bbsczzq9" FILTER3="--filters k5w30g1.6bsczz" FILTER4="--filters k5w30g1.6sc" ./compare.sh

FLIMMAKER="./flimmaker"

OUT_DIR=/mnt/data0/WebSites/www.macflim.com/macflim2/samples

mkdir -p ${OUT_DIR}


function size {
    filesize=`du -k $1 | awk '{print $1+2048}'`
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

${FLIMMAKER} "sample/source/${SOURCE}" --from ${GIFFROM} --duration ${GIFDURATION} --flim /dev/null --gif ${OUT_DIR}/${DEST}-${PROFILE}.gif --profile ${PROFILE} ${ARGS}
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

cd ${OUT_DIR}/../releases
hmount '/home/fred/Development/macflim/MacFlim Source Code.dsk'
hcopy ":player:MacFlim Player.sit" "MacFlim Player.sit"
humount
cd -

# encode SinCity.mp4 SinCity-Intro se30 02:09.5 75 02:10.3 4.85 "--filters k5w30g1.6sc"
# encode 300.mp4 300 se30 00:11:20.100 00:01:57 12:36 9.8
# encode 1984.mkv 1984 se30 0 360 0:20.15 2.2 "--filters k10g1.2sc"
# encode BlackOrWhite.mp4 BlackOrWhite se30 00:05:29 00:00:47 00:06:08 8 "--filters g1.6sc --error-bleed 0.8"
# encode Bad_Apple.mp4 BadApple se30 0 3:37 1:15 11 "--filters k20w20g1sc"
# encode iPodAd1.mp4 iPodIntroduction se30 0 1000 1 3 "--filters w10k10bsq5c --dither ordered"
# encode Matrix.mp4 Matrix se30 01:46:15 00:00:43.280 01:46:30.200 10
# encode Space_1999.mp4 Space-1999 se30 00:00:00 01:00:00 00:00:14 6.04 "--filters Zg1.3w25k10c --error-bleed 0.98"
# encode FadeToGrey.mp4 FadeToGrey se30 00:00:00 01:00:00 00:00:27.75 3.44 "--filters k10gsc"

# encode iPodAd1.mp4 iPodIntroduction se 0 1000 1 3 "--filters w10k10bsq5c --dither ordered"
# encode SinCity.mp4 SinCity-Intro se 02:09.5 75 02:10.3 4.85 "--filters k5w30g1.6sc"
# encode 300.mp4 300 se 00:11:20.100 00:01:57 12:36 9.8
# encode 1984.mkv 1984 se 0 360 0:20.15 2.2 "--filters k10g1.2sc"
# encode BlackOrWhite.mp4 BlackOrWhite se 00:05:29 00:00:47 00:06:08 8 "--filters g1.6sc --error-bleed 0.8 --dither error"
encode Matrix.mp4 Matrix se 01:46:15 00:00:43.280 01:46:30.200 10 "--dither error"
encode FadeToGrey.mp4 FadeToGrey se 00:00:00 01:00:00 00:00:27.75 3.44 "--filters k10gsc --dither error"

# encode Sledgehammer.mp4 Sledgehammer plus 00:00:10 01:00:00 00:01:05.48 4 "--filters k15g1.6bbscz.6sc"
# encode Sledgehammer.mp4 Sledgehammer se 00:00:00 01:00:00 00:01:05.48 4 "--filters k10g1.6bsc"
# encode Sledgehammer.mp4 Sledgehammer se30 00:00:10 01:00:00 00:01:05.48 4 "--filters k10g1.6sc"

# encode The_Wall.mp4 The-Wall se30 0 3600 13 10 "--filters Z16k10w10g1.6sc" # SE = "--filters k10g1.6sc"
# encode The_Wall.mp4 The-Wall se 0 3600 13 10 "--filters Z16k10w10g1.6sc --dither ordered --bars false" 

# encode 2001.mp4 2001 plus 00:00:53 00:01:46 2:15 7 "--filters k10g0.8bbscz --dither error"
# encode 2001.mp4 2001 se 00:00:53 00:01:46 2:15 7 "--filters k10g0.8bsc"
# encode 2001.mp4 2001 se30 00:00:53 00:01:46 2:25 7 "--filters k10g0.8sc"
# encode Gangnan_Style.mp4 Gangnan-Style se30 0 3600 1:54.46 8 ""
## encode Gangnan_Style.mp4 Gangnan-Style se30 0 3600 1:36.9 3.2 ""

# encode TakeOnMe.mp4 TakeOnMe plus 0 3600 1:36 7 "--filters w20k20g1bbsqczz --dither ordered"
# encode TakeOnMe.mp4 TakeOnMe se 0 3600 1:36 7 "--filters w20k20g1bsqcz --dither ordered"
# encode TakeOnMe.mp4 TakeOnMe se30 0 3600 1:36 7 "--filters w20k20g1sqc --dither ordered"

# encode Sweet_Dreams.mp4 SweetDreams plus 0 3600 2:21 7 "--filters w10k10g1bbsc --dither error --error-bleed 0.9"
# encode Sweet_Dreams.mp4 SweetDreams se 0 3600 2:21 7 "--filters w10k10g1bbsc"
# encode Sweet_Dreams.mp4 SweetDreams se30 0 3600 2:21 7 "--filters w10k10g1bscZ"

# encode Amiga_Ball.mp4 AmigaBall plus 0:29 30 0:29 5 "--filters gq5"
# encode Amiga_Ball.mp4 AmigaBall se 0:29 30 0:29 5 "--filters gq5 --dither ordered"
# encode Amiga_Ball.mp4 AmigaBall se30 0:29 30 0:29 5 "--filters gq5 --dither ordered"

# encode The_Shining_Johnny.mp4 TheShining se30 0:0 3600 1:07.5 3 "--filters w20g1.6sc"

# encode 8Bits.mp4 8Bits se30 0:0 3600 1:07.5 3 "--error-bleed 0.98"
# encode 8Bits.mp4 8Bits se 0:0 3600 1:07.5 3 "--filters g1.6bscz --dither ordered --bars true"
# encode 8Bits.mp4 8Bits plus 0:0 3600 1:07.5 3 "--filters g1.6bbsczz"

#encode LouxorJAdore.mp4 LouxorJAdore-1 se30 39 10.95 32 4 "--filters k10w10g1.6sc"
#encode LouxorJAdore.mp4 LouxorJAdore-2 se30 18 9.7 28.5 4 "--filters k10w10g1.6sc"

# encode StarWars.mp4  StarWars-Attack se30 1:53:38 3:59.950 1:54:59.450 1.750 "--filters k5g1sc"
encode StarWars.mp4  StarWars-Attack se 1:53:38 3:59.950 1:54:59.450 1.750 "--filters k5g1sc"

## STEM=StarWars VARIATION="_Attack" FROM=01:53:38 DURATION=00:03:59.950 ./compare.sh
## STEM=StarWars VARIATION="_Intro" FROM=00:01:53 DURATION=00:05:00 ./compare.sh

# encode RickRoll.mp4 RickRoll se30 0 3600 0 5 "--filters Z37k10w20g1.6sc"
encode RickRoll.mp4 RickRoll se 0 3600 0 5 "--filters Z37k10w20g1.6sc"
