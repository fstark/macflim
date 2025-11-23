#!/bin/bash

make flimmaker


# ----------------- DONE

# STEM=SinCity FROM=2:9.5 DURATION=75 VARIATION=_Intro FILTER2="--filters k10w10g1bbsczzq9" FILTER3="--filters k5w30g1.6bsczz" FILTER4="--filters k5w30g1.6sc" ./compare.sh
# STEM=300 FROM=00:11:20.100 DURATION=00:01:57 FILTER3="--filters g1.6bsc --dither ordered --bars true" ./compare.sh

# # # MACPLUS IS UGLY BUT OK [PLUS/SE30]
# STEM=1984 EXT="mkv" FILTER2="--filters k15g1.4bbscz" FILTER3="--filters k10g1.4bscz" FILTER4="--filters k10g1.2sc" ./compare.sh

# STEM=BlackOrWhite FROM=00:05:29 DURATION=00:00:47 FILTER2="--filters k20g1.6bbsczz --bars true" FILTER3="--filters g1.6bsczz --dither error --bars false --error-bleed 0.8" FILTER4="--filters g1.6sc --error-bleed 0.8" ./compare.sh

# STEM=Matrix FROM=01:46:15 DURATION=00:00:43.280 ./compare.sh


# # GOOD SE30
# STEM=8Bits FILTER2="--filters g1.6bbsczz" FILTER3="--filters g1.6bscz --dither ordered --bars true" FILTER4="--error-bleed 0.98" ./compare.sh

# # SE/30 ONLY -- NEEDS REFINEMENT FOR VERTICAL LINE
# STEM=Bad_Apple DURATION=3:37 FILTER2="--dither ordered --filters k20w20g1bbbsczzq2" FILTER3="--dither ordered --filters k20w20g1bbbsczzq2" FILTER4="--filters k20w20g1sc" ./compare.sh

# # GOOD
# STEM=iPodAd1 FILTER2="--filters w10k10bsq5czz" FILTER3="--filters w10k10bsq5cz --dither ordered" FILTER4="--filters w10k10bsq5c --dither ordered" ./compare.sh

# STEM=Space_1999 FILTER4="--filters Zg1.3w25k10c --error-bleed 0.98" ./compare.sh

# STEM=FadeToGrey FILTER4="--filters k10gsc" ./compare.sh

# # Good PLUS, SE  & SE/30
# STEM=Sledgehammer FROM=10 DURATION=05:00 FILTER2="--filters k15g1.6bbscz" FILTER3="--filters k10g1.6bsc" FILTER4="--filters k10g1.6sc" ./compare.sh


# STEM=The_Wall FILTER3="--filters g1.6bsc --dither ordered --bars false" ./compare.sh


# STEM=2001 FROM=00:00:45 DURATION=00:01:46 FILTER2="--filters k10g0.8bbscz --dither error" FILTER3="--filters k10g0.8bsc" FILTER4="--filters k10g0.8sc" ./compare.sh

# # # Perfect for SE/30
# STEM=Gangnan_Style ./compare.sh

# STEM=TakeOnMe FILTER2="--filters w20k20g1bbsqczz --dither ordered" FILTER3="--filters w20k20g1bsqcz --dither ordered" FILTER4="--filters w20k20g1sqc --dither ordered" ./compare.sh

# STEM=Sweet_Dreams FILTER2="--filters w10k10g1bbsc --dither error --error-bleed 0.9" ./compare.sh

# # Find right FROM/DURATION
# STEM=Amiga_Ball ./compare.sh



# STEM=The_Shining_Johnny ./compare.sh




# VOLUME TOO LOW -- LOOK AT SOME AUTO CORRECTION
# STEM=StarWars VARIATION="_Attack" FROM=01:53:38 DURATION=00:03:59.950 ./compare.sh

# #
# STEM=EveryBreath FILTER2="--filters k10g1.6bbscz" FILTER3="--filters k10g1.6bsc" FILTER4="--filters k10g1.6sc" ./compare.sh

# # # LIMIT TO 5 MINS -- CHECK BETTER BOUNDS FOR SOUND
# STEM=StarWars VARIATION="_Intro" FROM=00:01:53 DURATION=00:05:00 ./compare.sh

# # ----------------- WORKING





# STEM=NeverLetMeDownAgain DURATION=300 ./compare.sh







# STEM=AxelF ./compare.sh

# STEM=Relax FILTER4="--filters k10gsc" ./compare.sh

# STEM=TrueFaith ./compare.sh

# # ERROR DURATION!
# STEM=Adrian1 FROM=00:00:00 DURATION=00:01:10 ./compare.sh





# ## ONLY SE/30
# STEM=GirlsJustWantToHaveFun ./compare.sh

# STEM=Shout ./compare.sh






# STEM=ItsAShame FILTER2="--filters k12w20g1.6bbscz" FILTER3="--filters k9w20g1.6bsc" FILTER4="--filters k9w20g1.6sc" ./compare.sh

# STEM=I_Need_A_Man ./compare.sh



# STEM=iPodAd2 ./compare.sh
# STEM=iPodAd3 ./compare.sh
# STEM=Marilyn ./compare.sh
# STEM=May_The_Force FILTER2="--dither error" ./compare.sh
# STEM=Me_Want_You_Play ./compare.sh
# STEM=Missionary_Man ./compare.sh
# STEM=Never_Gonna_Give_You_Up ./compare.sh
# STEM=OldAppleAd2 EXT=mov ./compare.sh
# STEM=Pulp_Fiction_Dance ./compare.sh

# STEM=Tron_Race FILTER4="--filters k9w20g1.6sc" ./compare.sh


# The_6_Million_Dollar_Man == BOF
# ./flimmaker sample/source/The_6_Million_Dollar_Man.mp4 --mp4 out.mp4 --filters g1.3k10w10c --from 23 --duration 157 && open out.mp4


# youtube-dl https://www.youtube.com/watch?v=PIb6AZdTr-A -f mp4 --output sample/source/GirlsJustWantToHaveFun.mp4
# youtube-dl https://www.youtube.com/watch?v=OJWJE0x7T4Q -f mp4 --output sample/source/Sledgehammer.mp4
# youtube-dl https://www.youtube.com/watch?v=Ye7FKc1JQe4 -f mp4 --output sample/source/Shout.mp4
# youtube-dl https://www.youtube.com/watch?v=JIrm0dHbCDU -f mp4 --output sample/source/Strangelove.mp4

# youtube-dl https://www.youtube.com/watch?v=pTFE8cirkdQ -f mp4 --output sample/source/BlackOrWhite.mp4
# youtube-dl https://www.youtube.com/watch?v=snILjFUkk_A -f mp4 --output sample/source/NeverLetMeDownAgain.mp4





# Axel F : https://www.youtube.com/watch?v=Qx2gvHjNhQ0
# Fade to Grey : https://www.youtube.com/watch?v=UMPC8QJF6sI
# True Faith : https://www.youtube.com/watch?v=mfI1S0PKJR8


# Relax: https://www.youtube.com/watch?v=Yem_iEHiyJ0
# Beds are Burning: https://www.youtube.com/watch?v=ejorQVy3m8E





# Space 1999 : https://www.youtube.com/watch?v=0CPJ-AbCsT8
