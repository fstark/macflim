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
# $8 = poster
# $9 = filters

local SOURCE="$1"
local DEST="$2"
local PROFILE="$3"
local FROM="$4"
local DURATION="$5"
local GIFFROM="$6"
local GIFDURATION="$7"
local POSTER="$8"
local ARGS="$9"

${FLIMMAKER} "sample/source/${SOURCE}" --from ${GIFFROM} --duration ${GIFDURATION} --flim /dev/null --gif ${OUT_DIR}/${DEST}-${PROFILE}.gif --profile ${PROFILE} ${ARGS}
echo ${FLIMMAKER} "sample/source/${SOURCE}" --from ${FROM} --duration ${DURATION} --flim ${OUT_DIR}/${DEST}-${PROFILE}.flim --mp4 ${OUT_DIR}/${DEST}-${PROFILE}.mp4 --profile ${PROFILE} --poster ${POSTER} ${ARGS}
${FLIMMAKER} "sample/source/${SOURCE}" --from ${FROM} --duration ${DURATION} --flim ${OUT_DIR}/${DEST}-${PROFILE}.flim --mp4 ${OUT_DIR}/${DEST}-${PROFILE}.mp4 --profile ${PROFILE} --poster ${POSTER} ${ARGS}

# cd ${OUT_DIR}

# hmount '/home/fred/Development/macflim/MacFlim Source Code.dsk'
# hcopy -m ":MacFlim Sources:MacFlim" MacFlim
# humount

# local FULLNAME=${DEST}-${PROFILE}

# local filesize
# size ${FULLNAME}.flim
# local SIZE=filesize

# rm -f ${FULLNAME}.zip && zip -r ${FULLNAME}.zip ${FULLNAME}.flim
# dd if=/dev/zero of=${FULLNAME}.dsk bs=1k count=${filesize}
# hformat -l ${FULLNAME} ${FULLNAME}.dsk
# hmount ${FULLNAME}.dsk
# hcopy ${FULLNAME}.flim :
# hattrib -t FLIM -c FLPL :${FULLNAME}.flim
# hcopy -m MacFlim :"MacFlim"
# humount

# rm -f ${FULLNAME}.dsk.zip && zip -r ${FULLNAME}.dsk.zip ${FULLNAME}.dsk
# rm ${FULLNAME}.dsk

# rm MacFlim

# cd -
}

cd ${OUT_DIR}/../releases
# hmount '/home/fred/Development/macflim/MacFlim Source Code.dsk'
# hcopy ":MacFlim Sources:MacFlim.sit" "MacFlim.sit"
# humount
cd -

# encode SinCity.mp4 SinCity-Intro se30 02:09.5 75 02:10.3 4.85 00:00:02.253 "--filters k5w30g1.6sc"
# encode 300.mp4 300 se30 00:11:20.100 00:01:57 12:36 9.8 00:01:25.17
# encode 1984.mkv 1984 se30 0 360 0:20.15 2.2 00:00:39.98 "--filters k10g1.2sc"
# encode BlackOrWhite.mp4 BlackOrWhite se30 00:05:29 00:00:47 00:06:08 8 00:00:45.62 "--filters g1.6sc --error-bleed 0.8"
# encode Bad_Apple.mp4 BadApple se30 0 3:37 1:15 11 00:00:54.59 "--filters k20w20g1sc"
# encode iPodAd1.mp4 iPodIntroduction se30 0 1000 1 3 00:00:08.943 "--filters w10k10bsq5c --dither ordered"
# encode Matrix.mp4 Matrix se30 01:46:15 00:00:43.280 01:46:30.200 10 00:00:41
# encode Space_1999.mp4 Space-1999 se30 00:00:00 01:00:00 00:00:14 6.04 00:00:11.92 "--filters Zg1.3w25k10c --error-bleed 0.98"
# encode FadeToGrey.mp4 FadeToGrey se30 00:00:00 01:00:00 00:00:27.75 00:00:03.44 00:00:27.75 "--filters k10gsc"

# SE

# encode iPodAd1.mp4 iPodIntroduction se 0 1000 1 3 00:00:08.943 "--filters w10k10bsq5c --dither ordered"
# encode SinCity.mp4 SinCity-Intro se 02:09.5 75 02:10.3 4.85 00:00:02.253 "--filters k5w30g1.6sc"
# encode 300.mp4 300 se 00:11:20.100 00:01:57 12:36 9.8 00:01:25.17
# encode 1984.mkv 1984 se 0 360 0:20.15 2.2 00:00:39.98 "--filters k10g1.2sc"
# encode BlackOrWhite.mp4 BlackOrWhite se 00:05:29 00:00:47 00:06:08 8 00:00:45.62 "--filters g1.6sc --error-bleed 0.8 --dither error"
# encode Matrix.mp4 Matrix se 01:46:15 00:00:43.280 01:46:30.200 10 00:00:41 "--dither error"
# encode FadeToGrey.mp4 FadeToGrey se 00:00:00 01:00:00 00:00:27.75 00:00:03.44 00:00:27.75 "--filters k10gsc --dither error"

# encode Gangnan_Style.mp4 Gangnan-Style se30 0 3600 1:54.46 8 00:04:06.67 ""
# encode Gangnan_Style.mp4 Gangnan-Style se 0 3600 1:54.46 8 00:04:06.67 ""
# ### encode Gangnan_Style.mp4 Gangnan-Style se30 0 3600 1:36.9 3.2 ""

# encode Sledgehammer.mp4 Sledgehammer plus 00:00:10 01:00:00 00:01:05.48 4 00:00:55.40 "--filters k15g1.6bbscz.6sc"
# encode Sledgehammer.mp4 Sledgehammer se 00:00:10 01:00:00 00:01:05.48 4 00:00:55.40 "--filters k10g1.6bsc"
# encode Sledgehammer.mp4 Sledgehammer se30 00:00:10 01:00:00 00:01:05.48 4 00:00:55.40 "--filters k10g1.6sc"

# encode The_Wall.mp4 The-Wall se30 0 3600 13 10 00:00:19.85 "--filters Z16k10w10g1.6sc" # SE = "--filters k10g1.6sc" [posters = 00:00:19.85, 00:01:23.19, 00:00:55.40, 00:03:19.07, 00:03:28:18]
# encode The_Wall.mp4 The-Wall se 0 3600 13 10 00:01:23.19 "--filters Z16k10w10g1.6sc --dither ordered --bars false" 

# encode 2001.mp4 2001 plus 00:00:53 00:01:38 2:15 7 00:01:25.05 "--filters k10g0.8bbscz --dither error"
# encode 2001.mp4 2001 se 00:00:53 00:01:38 2:15 7 00:01:25.05 "--filters k10g0.8bsc"
# encode 2001.mp4 2001 se30 00:00:53 00:01:38 2:25 7 00:01:25.05 "--filters k10g0.8sc"

# encode TakeOnMe.mp4 TakeOnMe plus 0 3600 1:36 7 00:00:58.33 "--filters w20k20g1bbsqczz --dither ordered"
# encode TakeOnMe.mp4 TakeOnMe se 0 3600 1:36 7 00:00:58.33 "--filters w20k20g1bsqcz --dither ordered"
# encode TakeOnMe.mp4 TakeOnMe se30 0 3600 1:36 7 00:00:58.33 "--filters w20k20g1sqc --dither ordered"

# encode Sweet_Dreams.mp4 SweetDreams plus 0 3600 2:21 7 00:01:56.08 "--filters w10k10g1bbsc --dither error --error-bleed 0.9" # Also poster 02:23.76
# encode Sweet_Dreams.mp4 SweetDreams se 0 3600 2:21 7 00:01:56.08 "--filters w10k10g1bbsc"
# encode Sweet_Dreams.mp4 SweetDreams se30 0 3600 2:21 7 00:01:56.08 "--filters w10k10g1bscZ"

# encode Amiga_Ball.mp4 AmigaBall plus 0:29 30 0:29 5 00:00:18.67 "--filters gq5"
# encode Amiga_Ball.mp4 AmigaBall se 0:29 30 0:29 5 00:00:18.67 "--filters gq5 --dither ordered --fps-ratio 1"
# encode Amiga_Ball.mp4 AmigaBall se30 0:29 30 0:29 5 00:00:18.67 "--filters gq5 --dither ordered"

# encode The_Shining_Johnny.mp4 TheShining se30 0:0 3600 1:07.5 3 00:01:08.64 "--filters w20g1.6sc"

# encode 8Bits.mp4 8Bits se30 0:0 3600 1:07.5 3 00:00:44.000 "--error-bleed 0.98"
# encode 8Bits.mp4 8Bits se 0:0 3600 1:07.5 3 00:00:44.000 "--filters g1.6bscz --dither ordered --bars true"
# encode 8Bits.mp4 8Bits plus 0:0 3600 1:07.5 3 00:00:44.000 "--filters g1.6bbsczz"

# # NOPE encode LouxorJAdore.mp4 LouxorJAdore-1 se30 39 10.95 32 4 "--filters k10w10g1.6sc"
# # NOPE encode LouxorJAdore.mp4 LouxorJAdore-2 se30 18 9.7 28.5 4 "--filters k10w10g1.6sc"

# encode StarWars.mp4  StarWars-Attack se30 1:53:38 3:59.950 1:54:59.450 1.750 00:03:37.259 "--filters k5g1sc" # posters = 1349, 1983, 2271, 4000, 5209 / 23.976
# encode StarWars.mp4  StarWars-Attack se 1:53:38 3:59.950 1:54:59.450 1.750 00:03:37.259 "--filters k5g1sc"

# # # STEM=StarWars VARIATION="_Attack" FROM=01:53:38 DURATION=00:03:59.950 ./compare.sh
# # # STEM=StarWars VARIATION="_Intro" FROM=00:01:53 DURATION=00:05:00 ./compare.sh

# encode RickRoll.mp4 RickRoll se30 0 3600 0 5 00:00:00.32 "--filters Z37k10w20g1.6sc"
# encode RickRoll.mp4 RickRoll se 0 3600 0 5 00:00:00.32 "--filters Z37k10w20g1.6sc"

# # # ONLY THE GIF NEED TO DO THE FLIM
# # # encode Russians.mp4 Russians se30 0 3600 1:35.2 4.9 "--filters k10w10g1.6bsc"
# # ## TODO
# # ## encode EveryBreath.mp4 EveryBreath
# # ## FILTER2="--filters k10g1.6bbscz" FILTER3="--filters k10g1.6bsc" FILTER4="--filters k10g1.6sc" ./compare.sh


# # 512, 128k, XL

# encode Matrix.mp4 Matrix xl 01:46:15 00:00:43.280 01:46:30.200 10 00:00:41
# encode StarWars.mp4             xx-SW xl 00:02:01 38 0 1 00:00:25.77 "--filters k10g1sc"


# # # YT extracts


### -----------------

# encode LaClasseAmericaine.mp4   00-SAMPLE1 xl 0:13 7 :15 2 2 "--filters k40w20sc --byterate 380 --silent false"
# encode LaClasseAmericaine.mp4   00-SAMPLE2 xl 0:13 7 :15 2 2 "--filters k40w20sc --byterate 380 --silent true"

# encode Zoolander.mp4            02-ZOO se30 6 2 0 1 10

# encode Zoolander.mp4            04-ZOO se30 17 3 0 1 10

# encode 1984.mkv                 06-1984 se30 15 10 0 1 1.43 "--filters k10g1.2sc"     # UGLY
# encode iPodAd1.mp4              07-IPOD se30 0 1000 0 1 8.943 "--filters w10k10bsq5c --dither ordered"
# encode Technologic.mp4          08-TECH se30 37.4 11.3 0 1 00:00:04 "--filters g1.6k10w10"

# encode Russians.mp4             10-RUS se30 0:49.95 9.10 0 1 7.44 "--filters k10w10g1.6bsc"
# encode Sledgehammer.mp4         11-SLEDGE se30 00:01:00 10 0 1 6.72 "--filters k10g1.6sc"
# encode EveryBreath.mp4          12-EVERY se30 00:00:36 13 0 1 8 "--filters k10g1.6sc"
# encode BlackOrWhite.mp4         13-BW se30 00:06:09 00:00:07 0 1 4.013 "--filters g1.6sc --error-bleed 0.8"
# encode FadeToGrey.mp4           14-FADE se30 00:00:19 12 0 1 12 "--filters k10gsc"
# encode The_Wall.mp4             15-WALL se30 3:55 14 0 1 00:00:13.32 "--filters Z16k10w10g1.6sc"

# encode StarWars.mp4             17-SW se30 00:02:01 25.8 0 1 00:00:25.77 "--filters k10g1sc"
# encode StarWars.mp4             18-SW2 se30 1:49:07.4 17.3 0 1 00:00:11.11 "--filters k10w10g1sc"
# encode The_Shining_Johnny.mp4   19-SHINING se30 0:45 26 0 1 00:00:23.64 "--filters w20g1.6sc"
# encode 2001.mp4                 20-2001 se30 00:02:18 14 0 1 00:00:00 "--filters k10g0.8ZZsc"

# encode Gangnan_Style.mp4        22-GANG se30 0:3:42 00:00:08 0 1 00:00:4 "--filters k6w10g1.6sc"
# encode SinCity.mp4              23-SIN se30 02:10 15 0 1 00:00:04.672 "--filters k5w30g1.6sc --dither error"
## 300 WITH THE MACFLIM SUBTITLES
# encode 300.mp4                  24-300 se30 00:12:35.500 14.3 0 1 00:00:09.802 "--srt sample/source/300-macflim.srt"
# encode Bad_Apple.mp4            25-BAD se30 1:11 21.5 0 1 00:00:04.171 "--filters k20w20g1sc"
# encode Matrix.mp4               26-MAT se30 1:46:27.5 13.60 0 1 00:00:09.427


# encode Bad_Apple.mp4            28-BAD se 1:11 21.5 0 1 00:00:04.171 "--filters k20w20g1sc"

# encode TakeOnMe.mp4             30-TAKE se 1:34.3 22 0 1 00:00:08.25 "--filters w20k20g1bsqcz --dither ordered"

# encode Sweet_Dreams.mp4         33-SWEET plus 2:14.6 19 0 1 00:00:09.200 "--filters w10k10g1bbsc --dither error --error-bleed 0.9"

# encode StarWars.mp4             36-SW 512k 00:02:01 25.8 0 1 00:00:25.77 "--filters k10g1sc"

# encode Amiga_Ball.mp4           37-AMIGA 128k 0:29 10 0 1 00:00:00.434 "--filters gzzq5 --byterate 380"
# encode Matrix.mp4               38-MAT xl   01:46:25.500 00:00:17 0 1 00:00:07.427 "--filters gbbszq15c --byterate 380"

# encode StarWars.mp4             39-SW xl 00:02:01 38 0 1 00:00:25.77 "--filters k10g1sc"

# encode ThatsAll.mp4             40-ALL se30 0 3600 0 1 00:00:03.867 "--dither error --filters k5w20g1.6sc"



# encode Matrix.mp4               YT-99-MAT se30 01:46:27.500 00:00:30.780 0 1 00:00:09.427 "--filters gc --watermark SE/30"
# encode Matrix.mp4               YT-99-MAT se   01:46:27.500 00:00:30.780 0 1 00:00:09.427 "--filters gbsqc --dither error  --watermark SE"
# encode Matrix.mp4               YT-99-MAT plus 01:46:27.500 00:00:30.780 0 1 00:00:09.427 "--filters gbbszq15c  --watermark PLUS"
encode Matrix.mp4               YT-99-MAT 128k   01:46:27.500 00:00:30.780 0 1 00:00:09.427 "--filters gbbszq15c --byterate 380 --watermark 128K"
( cd /mnt/data0/WebSites/www.macflim.com/macflim2/samples && ffmpeg -loglevel info -y -i "YT-99-MAT-se30.mp4" -i "YT-99-MAT-se.mp4" -i "YT-99-MAT-plus.mp4" -i "YT-99-MAT-128k.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" YT-99-MAT-grid.mp4 && open YT-99-MAT-grid.mp4 )

# encode Bad_Apple.mp4            YT-99-BAD se30 1:11 21.5 0 1 00:00:04.171 "--filters k20w20g1sc --watermark SE/30"
# encode Bad_Apple.mp4            YT-99-BAD se 1:11 21.5 0 1 00:00:04.171 "--filters k20w20g1sc --watermark SE"
# encode Bad_Apple.mp4            YT-99-BAD plus 1:11 21.5 0 1 00:00:04.171 "--filters k20w20g1sc --watermark PLUS"
encode Bad_Apple.mp4            YT-99-BAD 128k 1:11 21.5 0 1 00:00:04.171 "--filters k20w20g1sc --byterate 380 --watermark 128K"
( cd /mnt/data0/WebSites/www.macflim.com/macflim2/samples && ffmpeg -loglevel info -y -i "YT-99-BAD-se30.mp4" -i "YT-99-BAD-se.mp4" -i "YT-99-BAD-plus.mp4" -i "YT-99-BAD-128k.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" YT-99-BAD-grid.mp4 && open YT-99-BAD-grid.mp4 )

encode RickRoll.mp4 YT-99-RR se30 0 30 0 5 2 "--filters k10w20g1.6sc --watermark SE/30"
encode RickRoll.mp4 YT-99-RR se 0 30 0 5 2 "--filters k10w20g1.6sc --watermark SE"
encode RickRoll.mp4 YT-99-RR plus 0 30 0 5 2 "--filters k10w20g1.6sc --watermark PLUS"
encode RickRoll.mp4 YT-99-RR 128k 0 30 0 5 2 "--filters Z37k10w20g1.6sczz --byterate 380 --watermark 128K"
( cd /mnt/data0/WebSites/www.macflim.com/macflim2/samples && ffmpeg -loglevel info -y -i "YT-99-RR-se30.mp4" -i "YT-99-RR-se.mp4" -i "YT-99-RR-plus.mp4" -i "YT-99-RR-128k.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" YT-99-RR-grid.mp4 && open YT-99-RR-grid.mp4 )


### -----------------


# encode Matrix.mp4               33-MAT xl 01:46:25 00:00:32 0 1 00:00:03.337 "--filters k10g1sc"

# encode Amiga_Ball.mp4           YT-51-AMIGA se 0:29 10 0 1 00:00:00.434 "--filters gq5 --dither ordered --fps-ratio 1"

# # # PLUS

# encode Amiga_Ball.mp4           YT-61-AMIGA plus 0:29 10 0 1 00:00:00.434 "--filters gq5"


# # # CONCLUSION

# encode LouxorJAdore.mp4         YT-70-ADORE1 se30 20 6.2 0 1 00:00:02.960 "--filters k10w10g1.6sc"
# encode LouxorJAdore.mp4         YT-71-ADORE2 se30 40 8.5 0 1 00:00:01.560 "--filters k10w10g1.6sc"

# # # END

# encode ThatsAll.mp4             YT-80-ALL plus 0 3600 0 1 00:00:03.867 "--dither error --filters k5w20g1.6Z80sc"

# encode EveryBreath.mp4          YT-90-EVERY xl 00:00:36 13 0 1 8 "--filters k10g1.6sc --byterate 380"
# encode Sweet_Dreams.mp4         YT-92-SWEET xl 2:14.6 19 0 1 00:00:09.200 "--filters w10k10g1bbsc --error-bleed 0.9 --byterate 380"
# encode SinCity.mp4              YT-93-SIN xl 02:10 15 0 1 00:00:04.672 "--filters k5w30g1.6sc --byterate 380"


# OLD encode StarWars.mp4  TEST1 se30 1:54:53 10 0 1 "--filters k5g1sc"

# OLD encode 1984.mkv                 XL xl 15 10 0 1 "--filters k10g1.2sc"     # UGLY
# OLD encode 1984.mkv                 1984 xl 0 1000 0 1 "--filters k10g1.2sc --byterate 480 --group true"     # UGLY
# OLD encode RickRoll.mp4 XL xl 0 30 0 5 "--filters Z37k10w20g1.6sczz --byterate 380 --group false"

# encode Technologic.mp4 Technologic se30 0 3600 0:33.6 3.5 00:00:14.08 "--filters g1.6k10w10"
# encode Amiga_Ball.mp4           YT-61-AMIGA xl 0:29 10 0 1 00:00:00.434 "--filters gq5"

# encode FredTest.mp4            00-SAMPLE plus 8 3 8 3 8 "--filters g1.2w60k5ZZZc --error-bleed 0.95 --dither error"


# TESTS FOR XL VERSIONS
# encode Matrix.mp4               Matrix xl   01:46:15 00:00:43.280 01:46:30.200 10 00:00:41 "--filters gbbsq15c --srt sample/source/Matrix.eng.srt"
# encode Matrix.mp4               Matrix 512k   01:46:15 00:00:43.280 01:46:30.200 10 00:00:41 "--filters gbbszq15c --byterate 480 --srt sample/source/Matrix.eng.srt"
# encode Matrix.mp4               Matrix 128k   01:46:25.500 00:00:17 01:46:30.200 10 00:00:07.427 "--filters gbbszq15c --byterate 380 --srt sample/source/Matrix.eng.srt"
# encode StarWars.mp4             StarWars-Intro 512k 00:02:01 38 00:02:11 5 00:00:25.77 "--filters k10g1sc --byterate 480"
# encode StarWars.mp4             StarWars-Intro 128k 00:02:01 19.5 00:02:11 5 00:00:25.77 "--filters k10g1sc --byterate 380"
# encode StarWars.mp4             StarWars-Intro se30 00:02:01 38 00:02:11 5 00:00:25.77 "--filters k10g1sc"
# encode StarWars.mp4             StarWars-Intro portable 00:02:01 38 00:02:11 5 00:00:25.77 "--filters k10g1sc"
# encode StarWars.mp4             StarWars-Intro se 00:02:01 38 00:02:11 5 00:00:25.77 "--filters k10g1sc"
# encode StarWars.mp4             StarWars-Intro plus 00:02:01 38 00:02:11 5 00:00:25.77 "--filters k10g1sc"

# encode Matrix.mp4               Matrix portable   01:46:15 00:00:43.280 01:46:30.200 10 00:00:41 "--filters gbbsq15c --srt sample/source/Matrix.eng.srt"


# encode StarWars.mp4             Model200 xl 00:02:01 38 00:02:11 5 00:00:25.77 "--width 224 --height 128 --filters k10g1s --byterate 580 --bars none --group false"
# encode StarWars.mp4             GameBoy xl 00:02:01 38 00:02:11 5 00:00:25.77 "--width 160 --height 144 --filters k10g1s --byterate 580 --bars none --group false"
# encode StarWars.mp4             PlayDate xl 00:02:01 38 00:02:11 5 00:00:25.77 "--width 384 --height 240 --filters k10g1s --byterate 1200 --bars none --group false"

