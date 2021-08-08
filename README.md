# MacFlim, the true Mac video player

Welcome to MacFlim, the video encoder and player for your previously obsolete Macintosh

![MacFlim](./assets/macflim.gif)

MacFlim aims at bringing movie playing abilities to vintage Macs, namely:

* Mac plus
* Mac SE
* Mac SE/30

on their beautiful internal back and white 512x342 display.

Some other macs may work in the future, but for now probably don't.

## What is in the repository?

* The source code of flimmaker, the command line encoder. It runs on Mac and Linux (and could probably compiled on windows with a C++20 compiler).

* The source code and binaries for "MacFlim Player", the new standalone video player for the vintage mac.

With this code, you should be able to encode and play a video sequence on your mac.

## I have no mac and I must stream!

If you have no access to a vintage mac, or if you want to look at results without having to transfer cumbesome files from your desktop to your mac, you can generate pixel exact movies that will let you know how the playback will look on the targetted hardware. See the ``--mp4`` option.

## What is new since MacFlim 1.0?

Well, the main change is that flims now have sound. This necessitated a complete rewrite of both the player and the encoder. All your old flims are now obsolete, sorry.

A negative change is that the new player app is currently vastly less powerful than MacFlim 1.0. It only lets you play a flim. But with sound.

Encoding input is greatly simplified: You don't have to resize the input to 512x342 any more. You don't need to have it in grayscale. You don't have to have it in pgm format. You can directly feed mp4 movies, or even youtube or vimeo urls to the encoder.

Output is different too: flims are encoded realitive to a target "profile", and the flim will only play correctly on hardare that would support this profile.

To ease with testing, you can also ask for an mp4 of the flim to be generated. This will let you iterate and tweak the encoding parameters without the need to transfer to your vintage hardware.

## Ok, how do I make this happen?

The pre-requisites are ffmpeg, youtube-dl (optional) and ImageMagick (optional)

* ``ffmpeg`` libraries are required for compilation. 
* ``youtube-dl`` is used if you want to directly encode movies from youtube or vimeo (or others).
* ``ImageMagick`` is used if you want to generate ``gif`` files.

On a Mac:

    brew install ffmpeg
    brew install youtube-dl
    brew install ImageMagick

On a linux:

    apt-get install libffmpeg-dev # ????
    apt-get install youtube-dl
    apt-get install ImageMagick

(or your regional equivalent)

Compiling is as simple as opening a terminal and typing ``make``. There are some warnings of obsolete functions use with ffmpeg, but it is already a miracle that it works. If anyone has a pull request to fix this, let me know.

After compilation, you can generate a sample flim using:

    ./flimmaker https://www.youtube.com/watch?v=dQw4w9WgXcQ --mp4 out.mp4

This will download the video and encode it for se30 playback, as 'out.flim'. You can immediatley play the ``out.mp4`` file, which is identical to the se30 playback. Enjoy!

## General flim creation options

There are quite a few options that control the flim generation.

The general format is:

    flimmaker [input-file-name] [--option-name value]

``input-file-name`` can be either:

* A local mp4 file. It will be opened using the installed ffmpeg library, and the "best" video and audio channels will be read and converted.

* An url supported by ``youtube-dl``. If the ``input-file-name`` starts with ``https://``, flimmaker will try to use ``youtube-dl`` to download the specified file and encode it.

* A set of local 512x342 8 bits pgm files. If the ``input-file-name`` ends with ``.pgm``, it will be considered as a ``printf`` pattern and used to read local images (starting at index 1? #### CHECK ME). For instance, ``movie-%06d.pgm`` will read all files named ``movie-000001.pgm``, ``movie-000002.pgm``, etc... Yes, if one uses '%s', the app will crash. See the ``--fps`` and ``--audio`` option to specify the audio of pgm files.

All other arguments to ``flimmaker`` go in pairs.

### --out **flimname**

Specifies the name of the generated flim file. If there is no ``--out`` option specified, ``flimmaker`` uses *out.flim*.

### --mp4 **file**

Creates a 512x342 60fps mp4 file that renders exactly the flim, with its associated sound. This can be used to view the flim without having to load the ``.flim`` file to a vintage Macintosh, or uploaded to the web. While the sound channel is 44KHz 16 bits, it really contains the 22KHz 8 bits Macintosh sound. This is by far the easiest way to iterate with the encoder.

### --pgm **pattern**

Write every generated frame as a pgm file. This is useful to embed a specific frame ina web site, or to look at the detail of the generation of different set of parameters. The pattern should contain a single '%d', which will be replaced by the frame number. Existing files with this pattern will be removed. Again, if one uses '%s', the app will crash. Example: ``--pgm out-%06d.pgm``. Numbering starts a 0, which is inconsistent with input handling (#### check me).

### --gif **file**

Creates an animated gif file with the first 5 seconds of the flim. The animated gif is at 20 frame per second. using a gif makes it easier to embed in a web page.

### --profile **plus**|**se**|**se30**|**perfect**

Specifies the encoding/playback profile you want to use. There are 4 profiles:

* plus : The plus profile aims at playing the resulting file on a Macitosh Plus, limiting the decoding processing power as much as possible by keeping the data small. For this, it skips half of the frames, uses ordered dithering, blurs the image and adds a small border to the generated flim. It allows for a lot of "leakage" from a frame to the next. The compression parameters are also very lossy. The result will only be "good" if the input movie is very static.

* se : As the Macintosh SE has slightly more processing power than the Macintosh Plus, it can manage files will less compression. It still skips half of the frames, but uses the nicer floyd dithering.

* se30 : Targets the SE/30 the most powerful comnpact Macintosh. Encoding can use 4 times more space, doesn't skip frames, and limits leakage from a frame to the next. se30 movies are in general correct, in the sense that mostly anything can be faithfully encoded, with a few artifacts.

* perfect : this profile aims at a "perfect" playback. The resulting files can be played on an upgraded computer. For instance, playing from a se30 with a ram disk allows those "perfect" flims to be played.

Examples:

    # Sweet dreams is a good flim for a plus, as there is almost no camera movement, and very slow scenes changes
    ./flimmaker 'https://www.youtube.com/watch?v=qeMFqkcPYcg' --profile plus --out sweet-dreams-plus.flim --mp4 sweet-dreams-plus.mp4

    # Gangnam style has quite a lot scene changes and movements, but works correctly on the se30
    ./flimmaker 'https://www.youtube.com/watch?v=9bZkp7q19f0' --profile se30 --out gangnam-style-se30.flim --mp4 gangnam-style-se30.mp4

### --from **time**

Starts encoding at that specific time. Time format is ``[[<hours>:]<minutes>:]<seconds>``, so ``--from 30`` means *30 seconds* from start, ``--from 120`` means *120 seconds* from start, ``--from 1:`` means *1 minute* from start, and ``--from 1:13:12`` means *1 hour, 13 minutes and 12 seconds* from start.

### --duration **time**

Specify the duration of the flim. See ``--from`` for time format. The default duration is 5 minutes. Due to incompetent coding, encoding movies that last for longer than 10-15m is in general a bad idea.  

### --bars **boolean**

The Mac screen ratio is 3/2, but move movies out there are 4/3, 16/9 or something else. By default, flimmaker adds black borders around the border of the flim (because it keeps more of the original image and the black bars are less data to encode). Using ``--bars false`` instead crops the image. Note that, when there are already black bars in in the input video, using the 'Z' filter (Zoom) described later can help.

### --watermark **string**

Adds the parameter string to the top of every frame of the video. This is useful if you generate several similar videos with different parameters and want to keep track of those. Use ``auto`` as the string to have the encoding parameters placed in the video. Please do not use the watermark option when releasing your video to the world!

There are two additional seldom used options, for dealing with pgm input:

### --fps **frame-rate**

When specifying a set of pgm files as input, one can use the ``--fps`` to specify the timing of the resulting video. The daults is 24 fps.

### --audio **raw-audio-filename**

When specifiying a set of pgm files as input, the audio has to be provided using a raw ``wav`` file of 22200Hz, unsigned 8 bits samples. Such file can be created using audacity, or the ffmpeg and sox unix command lines (use ``apt-get install sox`` or ``brew install sox`` to install the sox tool):

    ffmpeg -i movie.mp4 audio.wav
    sox -V2 audio.wav -r 22200 -e unsigned-integer -b 8 audio.raw remix 1 norm


## Moar options!

Digging into the dirty details, here are the options that control the encoding itself (ie: the options driven by the profile).

You can specify ``--profile`` to set up basic options, the more specific ones will override the ones form the profile.

You need to specify those options *after* the ``--profile``

This is an advanced section, it's ok if you don't understand everything.

### --byterate byterate

The byterate is the number of bytes per ticks (a tick is 1/60th of a second) that are available to encode the video stream. 370 additional bytes are used for the sound, plus a handful of bytes overhead. You can play with this parameter if your mac has a faster/slower drive. If the byterate is too high, you will suffer sound and video skips at playback.

The Mac Plus is able to read and decode around 1500 bytes per tick, the Mac SE around 2500 and the Mac SE/30 6000.

### --half-rate **boolean**

Using ``--half-rate true`` will effectively halve the framerate of the input, resulting in a worse looking, but smaller flim. If Mac Flim II has troubles displaying your flim, using ``--half-rate true`` can vastly improve the visual result.

The Mac Plus and the Mac SE profiles are half-rate by default, while the SE/30 displays all the frames.

### --group **boolean**

Using ``--group false`` will have the player display partially constructed frames every 60th of a second. Due to limitations in the hardware/the way Mac Flim II works, the Mac Plus and the Mac SE cannot group the frames, and you can see the construction on screen. The SE/30 doesn't have to display partial results, which results more stable display. However, one can use ``--group true`` for the SE/30 to get some interesting low-fidelity effects.

The Mac Plus and the Mac SE profiles are not grouped, while the SE/30 is.

### --dither **ordered**|**floyd**

The conversion of the image to black and white can be done using either the ``ordered`` dithering or the ``floyd`` one. In general, the ``floyd`` ordering will give the typical *MacFlim* look. However, images from one frame to another have more differences, so it uses more bandwidth for encoding. Also, if the flim is composed of only large flat regular zones, ``ordered`` encoding may give nicer results than ``floyd``.

The Mac Plus uses ``ordered`` encoding by default while the Mac SE and SE/30 use the ``floyd`` encoding.

### --stability **double**

When using the ``floyd`` dithering, a small change in some part of consecutive frames can lead to very different dithering patterns. This is visually distracting and also consumes bandwidth.

The stability parameter makes the dithered pattern match the preceding frame more closely. A small stability will make images change a lot between frames, and a high stability will cause artifacts.

While the Mac Plus does not use ``floyd`` dithering, its default ``stability`` is 0.5. The Mac SE also uses a ``stability`` of 0.5. The mac SE/30 uses a ``stability`` of 0.3.

### --filters **giberrish**

After converting the input image to 512x342 grayscale, Mac Flim II applies a series of filters, before dithering the image to pure black and white.

Each filter is a single letter, with an optional numeric parameter. A filter can be specified twice, in which case it will be applied twice. There are no spaces. As an example, ``--filters g1.8b5scz`` means gamma 1.8, blur 5x5, sharpen, add corners to the frame and reduce it slightly.

Specifying a ``--filters`` argument completely replaces the default filters from the current profile.

Filter list:

* Blur 'b' (size) : blurs the image by averaging the neighbourd pixels. The argument is the size of the filter, a 3x3 grid (default) or a 5x5 grid. No other sizes are supported. As the spatial resolution of the image is used to encode the dithering, bluring the image often doesn't lead to a visible loss of quality and generally enables a better encoding by smoothing out details.

* Sharpen 's' : sharpens the image. It is a good idea to sharpen the image after blurring. This helps to have more defined zones, which, again, helps the encoding.

* Gamma 'g' (value) : applies a gamma transformation to the image. The higher the gamma, the darker the resulting image. Default gamma is 1.6.

* Round Corners 'c' : the round corners filter removes the corners, and produce a lovely period-accurate, rounded-cornered image.

* Zoom smaller 'z' : The image is zoomed out so that there are 32 black pixels on each side (64 pixels in total horizontally). As a result, the encoding is slightly more efficient. Use several 'z' to get an even smaller image.

* Zoom larger 'Z' : The 32 leftmosts and rightmost pixels of the images are dropped, and the resulting image is zoomed in. You can use several 'Z' to zoom even deeper in the image. One can use 'Zz' (or better, 'Zcz') to punch a 32 pixels wide black frame around the video.

* Invert 'i' : inverts the image; the black become white and the white becomes black. 'Zizi' adds a white border to the image, much to the hilarity of the most immature members of the French-speaking crowd.

* Flip 'f' : Horizontally flips the image. This can be useful when creating flims that can appear in the background of youtube videos, as it creates less spurious copyright strikes from the IA.

* Quantize 'q' (steps) : Quantize the colors so there are only 17 of them. This can help when encoding images of flat colors to avoid spurious gradient. It is particularly useful when using the ordered dithering, as there are only 17 different dithering patterns. Using a lower number can create interesting effects: for instance, q5 will generate images with only black, dark gray, pure gray, light gray and white colors. q2 will posterize the image into black and white, rendering dithering inoperand (useful for pure 2 colors black and white sources)

* Black 'k' (percent) : Remove the darkest part of the image. Often movies have black background that are not completely black. The dithering algorithm represents this by having a few white pixels in large black areas, which is visually distracting (and eats encoding bandwidth). The black filters collapses the darkest pixels into pure black. The rest of the image color is scaled to the remaining color range. By default the black filters removes the 6.25% darkest pixels.

* White 'w' (percent) : Same as the Black filter, but for white pixels. This is a slightly less frequent issue, as large pure white areas are rarer in movies.


### --codec **codec-definition**

The encoding is performed by a serie of codecs. Each frame is encoded by all the codecs with the existing byterate, and the one which produces the "best" image is choosen. As soon as the ``--codec`` command-line argument is specified, the codecs list is reset, and all codecs needs to be added manually. A codec is specified using its name, followed by an optional argument list, in the following format: ``--codec name:arg1=value1,arg2=value2``. The use of the ``--codec`` argument is mostly useful during development.

Each codec is defined by a name and an optional list of parameters. Here is the list of codecs and their parameters.

* null (0x00) : The ``null`` codec just doesn't encode anything. It is useful when an image doesn't change from a frame to the next.

* z16 (0x01) : The ``z16`` codec compresses the image in 16 pixels vertical bands. It is almost always inferior to the z32 codec.

* z32 (0x02) : The ``z32`` codec compresses the image in 32 pixels vertical bands. It is generally the most efficient codec.

* invert (0x03) : The ``invert`` codec (currently) bluntly inverts the whole image. This is useful in encoding movies that have sudden complete reversal of colors, which is a worst case for th ``z32`` codec, but can be trivialy encoded by ``invert``.

* lines (0x04) : The ``lines`` codec encodes a fixed amount of consecutive horizontal lines. It is usefull when there is a large change in the image, as it has less overhead than the z32 codec. The number of lines than can be encoded in a single frame is based on an average of 50 bytes per lines (which is wrong, but right)

The 0x00, 0x01, 0x02, 0x03 and 0x04 are the *signature* of the codecs, which can also be seen with the ``--watermark auto`` option.

One can see the exact behavior of each codec by specifying those one by one:

./flimmaker gangnam-style.mp4 --mp4 out.mp4 --duration 20 --codec z16


[to be continued]
