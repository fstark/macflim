* [DONE] Add sound
* [DONE] Unknown profile crashes (--profile se)
* [DONE] check issue with default duration parameter
* [DONE] codec number in encoding log
* [DONE] Track random ffmpeg crash
* [DONE] No need for argument to specify mandatory input
* [DONE] Add automatic gif generation
* [DONE] rename --dump into --mp4
* better options for .pgm image generation

* Last sound frames
* --from doesn't work for sound
* Play last bits of flim on the mac
* add --pgm for dumping pgms
* command-line help

BUGS TO INVESTIGATE:

* rm 0.gif

---------------



note:

make && ./flimmaker --input badapple.mp4 --from 0 --duration 3600 --dump badapple-se30.mp4 --bars false --filters k20w20c && open badapple-se30.mp4




# SWEET DREAMS PLUS

# ordered
./flimmaker sweet-dreams.mp4 --profile plus --out sweet-dreams-plus-2.flim --mp4 sweet-dreams-plus-2.mp4 --bars none --filters Zzk10w20g1bbsc

./flimmaker sweet-dreams.mp4 --profile plus --bars false --filters Z24k8w8gbbsc --mp4 output3.mp4 &


# floyd
./flimmaker sweet-dreams.mp4 --profile plus --dither error --error-algorithm floyd --error-bleed 0.88 --error-bidi true --filters k10w10gbbsc --bars false --filters k1gbbsc --mp4 sweet-dreams-plus.mp4 --out sweet-dreams-plus.flim




./flimmaker sweet-dreams.mp4 --profile se --out sweet-dreams-se.flim --mp4 sweet-dreams-se.mp4 --bars none --filters Zzk10w20g1bbsc

./flimmaker gangnam-style.mp4 --profile se --out gangnam-style-se.flim --mp4 gangnam-style-se.mp4 --bars false --filters w10k10gsc

./flimmaker everybreath.mp4 --profile se --out everybreath-se.flim --mp4 everybreath-se.mp4 --bars false --filters w3k10gsc





./flimmaker sweet-dreams.mp4 --profile plus --from 55 --duration 5 --gif sweet-dreams-1.gif 
./flimmaker sweet-dreams.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --from 55 --duration 5 --gif sweet-dreams-2.gif 
./flimmaker sweet-dreams.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --dither floyd --from 55 --duration 5 --gif sweet-dreams-3.gif 
./flimmaker sweet-dreams.mp4 --profile se --bars none --filters Zk10w20g1bbsc --from 55 --duration 5 --gif sweet-dreams-4.gif 
./flimmaker sweet-dreams.mp4 --profile se30 --bars none --from 55 --duration 5 --gif sweet-dreams-5.gif 
./flimmaker sweet-dreams.mp4 --profile se30 --bars none --filters Zk10w20g1bbsc --from 55 --duration 5 --gif sweet-dreams-6.gif 

# Stability parameter demo

# .5 => We can see some leftover pixels around Annie face when she moves. However, there is no crawling pixels effect around
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 52 --duration 5 --gif stability-1a.gif 
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 52 --duration 5 --gif stability-1b.gif 
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 52 --duration 5 --gif stability-1c.gif 
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 52 --duration 5 --gif stability-1d.gif 

# When encoding for a plus, however, the stability helps using less bandwith, and the resulting image is closer to the source material, see Annie s hand. At low stability, the encoder gives up and use horizontal copies, but it doesn t help
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 52 --duration 5 --gif stability-2a.gif 
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 52 --duration 5 --gif stability-2b.gif 
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 52 --duration 5 --gif stability-2c.gif 
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 52 --duration 5 --gif stability-2d.gif 

# If we just look at only the stadard z32 encoding performance
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 52 --duration 5 --gif stability-3a.gif --codec z32
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 52 --duration 5 --gif stability-3b.gif --codec z32
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 52 --duration 5 --gif stability-3c.gif --codec z32
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 52 --duration 5 --gif stability-3d.gif --codec z32

# Compare the frame of the window, and the reflection of the drummer in the bottom 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.5 --from 2:2 --duration 5 --gif stability-4a.gif 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.3 --from 2:2 --duration 5 --gif stability-4b.gif 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.2 --from 2:2 --duration 5 --gif stability-4c.gif 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.0 --from 2:2 --duration 5 --gif stability-4d.gif 

./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.5 --from 2:2 --duration 5 --dither floyd --gif stability-5a.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.3 --from 2:2 --duration 5 --dither floyd --gif stability-5b.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.2 --from 2:2 --duration 5 --dither floyd --gif stability-5c.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.0 --from 2:2 --duration 5 --dither floyd --gif stability-5d.gif

./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 2:2 --duration 5 --dither floyd --gif stability-6a.gif
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 2:2 --duration 5 --dither floyd --gif stability-6b.gif
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 2:2 --duration 5 --dither floyd --gif stability-6c.gif
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 2:2 --duration 5 --dither floyd --gif stability-6d.gif

./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 2:2 --duration 5 --dither floyd --gif stability-7a.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 2:2 --duration 5 --dither floyd --gif stability-7b.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 2:2 --duration 5 --dither floyd --gif stability-7c.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 2:2 --duration 5 --dither floyd --gif stability-7d.gif


# Example with a lot of transitions

make && ./flimmaker sweet-dreams.mp4 --profile plus --bars false --from 1:27 --duration 5 --gif sweet-dreams.gif --filters Zk10w20g1bbsqc --dither floyd



make && ./flimmaker ipodad1.mp4 --out ipodad1.flim --dither ordered --filters w10k10bsq5c --mp4 out.mp4 --bars false 




# Generate asset

# macflim.gif
make && ./flimmaker ipodad1.mp4 --gif assets/macflim.gif --from 9.8 --duration 2 --profile perfect --filters g0.8q5c --dither ordered





make && ./flimmaker ipodad1.mp4 --profile plus --out ipodad1.flim --dither ordered --filters w10k10bsq5c --bars true



# Billie Jean plus
make && ./flimmaker billiejean.mp4 --profile plus --out billiejean-plus.flim --byterate 1500 --stability 0.5 --half-rate true --group false --bars true --dither floyd --filters w10k10g2bszzq --codec null --codec z32 --mp4 out.mp4