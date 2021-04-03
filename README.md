# MacFlim Video player source code

[DOES NOT CONTAIN THE MacFlim Macintosh application SOURCE CODE YET]

Please do not barf on code quality. It was not in releasable state, but people wanted to use it. You may even be one of those people. Hi!

## Content

``flimmaker`` : A C++ binary that can generate flim files for Macintosh playback

``flimutil`` : A C utility that can manipulate flim files

# How to compile on linux?

Just use ``make``

```
$ make
c++ -O3 flimmaker.cpp -o flimmaker
cc -O3 -Wno-unused-result  flimutil.c -o flimutil
$ 
```

# And on other platforms?

Use ``make``, or make adjustment and do a pull request. I don't want dependencies, so be as simple as possible.

# What is ``flimmaker``? How do I use it?

``flimmaker`` is the C++ tool that generates flim files. It performs the dithering on grayscale images and generate the "proprietary" files for playback. ``flimmaker`` runs on a set of 512x342, 8 bits grayscale pgm files. See further on how to generate such files using ``ffmpeg``.

Syntax is:

``flimmaker`` [-g] --in \<%d.pgm> [--from \<index>] [--to \<index>] [--cover \<index>] --out \<file>

-g : enable debug information (mostly info on progress)

--in \<pattern> : filename pattern for the pgm files that contains a * '%d' that will be replaced by an incrementing index. Yes, if you put '%s', you'll crash.

--from \<index> : the index we take the first image from. Default to 1

--to \<index> : the last index of the frame to be included (the number of frames is to-from+1). flimmaker will stop if it cannot read a file, so you can safely pass a large number here. Default to basically infinity.

--cover \<index> : the frame index of the cover. If specified, flimmaker will generate a set of 24 pgm images, named cover-000000.pgm to cover-000023.pgm in the current directory. Those images can be used to generate an animated gif of the movie. Default to 1/3rd of "to" and "from". If outside of the movie, there will be no cover generated.

-- out \<file> : the name of the flim file to generate

The input is supposed to be 24 frames per second (ie: every frame represent 1/24th of a second)

The generated flim will always contain the 4 standard streams:

* 256x171x12 fps (only half of the input images are used)
* 256x171x24 fps
* 512x342x12 fps (only half of the input images are used)
* 512x342x24 fps

# What is ``flimutil``? How do I use it?

``flimutil`` is a C tool that reads a flim and check/display the content. It is a separate tool for now, as I am still evaluating if I want to bloat ``flimmaker`` or not with additional features (like cutting flims, concatenating flims, extracting streams, upgrading to new formats, etc).

# Can you walk me in the process of creating my own flim?

Sure. To follow this tutorial, you will need the following packages:

* youtube-dl => (optional) retreive youtube sample movie

* mediainfo => (optional) find source material frame rate

* ffmpeg => generate pgm images for ``flimmaker``

* ImageMagick => (optional) generate gifs of you flim

## Create a directory to store all the temporary files
``mkdir sample && cd sample``

## Get the source media (youtube example)

Remember that you can use ``youtube-dl -F`` to list formats. ``-f 22`` is not always the right one.

```youtube-dl -f 22 https://youtu.be/MiRtNavqfpg -o sample.mp4```

## Check the framerate of the media (useful later)

```mediainfo sample.mp4```

```
...
Video
...
Frame rate mode                          : Constant
Frame rate                               : 30.000 FPS
...


Audio
...
Sampling rate                            : 44.1 kHz
Frame rate                               : 43.066 FPS (1024 SPF)
...
```

In this example, the source contains 30 image per second. You will need to decide if you want to play it slower, but with all the images, or if you prefer skipping images.

## Extract the part you want to encode

As files are just huge for now, you probably want to select a minute or so that you are intersted in.

``ffmpeg -v warning -stats -y -ss 00:00:02.100 -t 00:00:30 -i sample.mp4 extract.mp4``

## Rescale it to 512x342

The command is a testament to the exterme user friendliness of ``ffmpeg``.

``ffmpeg -v warning -stats -y -i extract.mp4 -vf "scale=(iw*sar)*max(512.1/(iw*sar)\,342.1/ih):ih*max(512.1/(iw*sar)\,342.1/ih), crop=512:342" 512x342.mp4``

## Extract all the grayscale images

The 30 below is the original framerate. Why do we have to pass it is something I cannot understand while sober. We then ask ffmpeg to extract 24 images every second. Note: the first image is 1, not 0.

``ffmpeg -v warning -stats -y -r 30 -i 512x342.mp4 -r 24 source-%06d.pgm``

## Execute flimmaker on the images, to generate the flim

The output of ``flimmaker`` is pretty terse for now, and the -g option doesn't do much more yet.

``../flimmaker --in "source-%06d.pgm" --out sample.flim --cover 116``

```
Stopped read at [source-000735.pgm]
Added 13 frames
Stopped read at [source-000735.pgm]
Added 6 frames
Stopped read at [source-000735.pgm]
Added 13 frames
Stopped read at [source-000735.pgm]
Added 6 frames
```

The "stopped read" message indicates that ``flimmaker`` didn't find the image 735 and assumed (correctly) that the flim should ends at 734. You can also use ``--from`` and ``--to`` to control which part you want the flim to be created from (in which case you would extract a larger part of the source media and manually search for the first and last images you want you flim to include)

``flimmaker`` generates everything in batches of 20, for internal reasons that are linked to playback. For 24fps streams, if the source material is not a multiple of 20 frames, it will duplicate the last frames to generate an integral number of 20 frames blocks. For 12fps streams, it will duplicate the last frames if the total source if not a multiple of 40.
 
The "added nn frames" message indicates that it added frames at the end. You can tweak the extract duration if you want to be "perfect" (the '-t 00:00:30' of the extract part)

## Check that the flim is correct

``../flimutil sample.flim``

should display:

```
STREAM COUNT: 4
  #0 (0/0) 256x171 12.000 fps
           380 frames, starting at 108, length 2079360
  #1 (0/0) 256x171 24.000 fps
           740 frames, starting at 2079468, length 4049280
  #2 (0/0) 512x342 12.000 fps
           380 frames, starting at 6128748, length 8317440
  #3 (0/0) 512x342 24.000 fps
           740 frames, starting at 14446188, length 16197120
```

## Generating a static cover gif

This uses ImageMagick to create a simple gif, suitable for insertion in web pages.

``convert cover-000000.pgm sample-poster.gif``

Note: if you plan to get this image on your vintage mac, I suggest you generate a ``tga`` file and open it with Photoshop 1.3. For web display, you can also choose to generate ``png`` images. Note that you absolutely want to avoid lossy compression, as black and white dithered images are a worst case for lossy compression.

## Generating the animated cover gif (the one I use on macflim.com)

``convert -delay 5 -loop 0 "cover-*.pgm" sample.gif``

## Generating an mp4 identical to the flim

All the 512x342 images have been generated by ``flimmaker`` in the current directory, so you can use ``ffmpeg`` to create a mac-like movie.

``ffmpeg -y -framerate 24 -pattern_type sequence -start_number 1 -framerate 24 -i "out-%06d.pgm" -s 512x342 sample.flim.mp4``

## Generating an mp4 flim suitable for youtube upload

If you want to update a flim to youtube with maximal quality (without filming the flim), you can use this command to generate something high quality that youtube will not distort, and get a result similar to https://youtu.be/xUCUK2k_hjk (choose HD in youtube, or the compression kills everything)

``ffmpeg -y -framerate 24 -pattern_type sequence -start_number 1 -framerate 24 -i "out-%06d.pgm" -filter:v pad="in_w:in_h+90:0:-45" -s 1280x1080 sample-hq.flim.mp4``

## Cleanup

```
$ rm *.pgm extract.mp4 512x342.mp4
$ ls -l
total 240860
-rw-rw-r-- 1 fred fred  30643308 avril  3 14:00 sample.flim
-rw-rw-r-- 1 fred fred  18182043 avril  3 13:39 sample.flim.mp4
-rw-rw-r-- 1 fred fred    516493 avril  3 13:36 sample.gif
-rw-rw-r-- 1 fred fred  52392940 avril  3 13:47 sample-hq.flim.mp4
-rw-rw-r-- 1 fred fred 144868275 avril  1 16:19 sample.mp4
-rw-rw-r-- 1 fred fred     21573 avril  3 13:36 sample-poster.gif
$
```

sample.flim : the flim you can play on the real hardware

sample.flim.mp4 : exactly the same thing, but that you can play locally to check. No suitable for video platform, because of the Mac aspect-ratio and the low quality settings.

sample.gif : a short animated poster of you flim

sample-hq.flim.mp4 : the flim in high quality and good aspect ratio for sharing

sample.mp4 : the original source material

sample-poster.gif : a simple black and white poster

# Where are the code comments?

Release or comments. Had to choose.

# Why templates?

It sounded like a good idea in the beginning, and I was tired.

# Why so many hard-coded behaviors?

See above.

# Any other things?

If you do something, or publish stuff, I'd love to be aware, so send me a pointer either to fstark on m68kmla (https://68kmla.org/forums/profile/20641-fstark/), or on my youtube channel (https://www.youtube.com/channel/UCotU6vnCI9H4YUEopzwDjRQ) or my reddit account (https://www.reddit.com/user/frederic_stark/).

Or better, @fredericstark on twitter (https://twitter.com/fredericstark).
