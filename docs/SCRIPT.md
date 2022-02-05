*** SE30, Finder.
*** Mac Flim folder closed.
*** User double-click, opens the folder, and launch MacFlim
*** Selection of the first 5 flims, go to menu, play.

[F01] Hi guys, it's me again. Yeah, it's been a while since the last update, but as you can see things have been a bit hectic lately, and I had to get *inside* the computer to manage to deliver the new generation of Macflim.

And what an improvement it is over the previous version! A cool, downright futuristic user interface, which I call a flim library, that lets you collect your favourite flims in a single place. And sound, as you might have noticed. This time, it comes directly from the computer. Fancy!

Actually, I think I could show you a few cool flims, now that I've finally managed to get that working. Right, so, right now we're in... [doigt sur les levres puis levé comme pour voir la direction du vent] yup, that's a SE30. Top of the line computer right there, the best of the best currently available... well, let's see what we can do with it.

[F02] 1984
[F03] Ipod
[F04] Technologic

[F05] Right, sorry, sorry. That was a lot of ads, and god knows we get a lot of these nowadays, but I just had to, you know? They're iconic, really. And no wonder they didn't keep the Daft Punk imagery for that iPod introduction ad... But I feel it's quite fitting nonetheless.

Still, let's do something else. Why don't we look at some music -- we should have a few *short* extracts, to avoid all that nasty copyright business.

*** Selection of flim 6 to 12

[F06-11] RUS/SLEDGE/EVERY/BW/FADE/WALL

[F12] Wasn't that quite something? I think watching music is a cool thing, and I love having my Mac TV, or MTV, as I like to call it.

But we can do even better. Are you tired of going out with your friends and going to the movies? Wouldn't you rather stay home, and enjoy the cinematic experience from your chair? Now you can. The future is there, and it's glorious.

*** User selects the rest of the flims

[F13-Fxx] SW2/S2/SHINING/2001/Matrix???

[F17] Woah. And MacFlim is so advanced that it even allows to see music and flims from the future! Let's see some example, and I promise4 to come back afterward.

[F18-F22] Gangnam, Sin City, 300, Bad Apple?

*** Back to the app, finder, shutdown

*** Mac SE

*** Open flim directory, folders 'Flims SE/30, .... Flims XL'

*** Open 'Flim SE'. Select All. Open.

[F23] Hi guys, deja-vu. It's me ag- oh, this isn't the SE30 anymore, is it? Yeah, that happens sometimes, I've had to go to all sorts of computers to make sure macflim worked on as many apple products as possible. I'm sure it'll be fine. I did want to show you how well macflim worked on older machines anyway.

[F24] [bad apple AGAIN]

[F25] See the difference? Some sacrifices have to be made to be able to run it, sadly. Contrarily to the SE30, it doesn't handle quick changes all that well, and the lines aren't as crisp. But with the right dithering, you can get some aesthetically pleasing results.

[F26] [take on me]

**** loop starts, click to cancel, back to Finder. Shutdown

**** Mac Plus, launch MacFlim
**** Library is empty, use 'Add Folder'

[F27] Yeah, I don't-- oh, crap. That's a mac plus. I really need to stay still there, otherwise...

(Large arm movments).

Yeah. Otherwise, this happens.

(sigh)

Obviously, there's only so much I can do here. I need to select the flims very carefully, compress the image, apply all kind of filters... it's work. Still, in the end, it can work very nicely indeed.

[F28] [sweet dreams are made of these]

[F29] All the flims that you've see on the SE/30 are available on the plus. If you stay until the end of the video, you'll see a comparison of the quality between the various machine. Let's go on, we don't have all day.

**** Mac Plus shutdown

[F30] Hey, this... wait, can you hear me? Am I on mute? Hello? (show 'Sound?' -- facepalm)

[F31] Star-Wars intro

**** [Voix off] "Well, this is a Mac 512K, with the original HD20 hard drive. It is a floppy-interface based hard drive, is extremely slow while hogging most of the CPU time. It is impossible to access the flim data while playing sound.

**** Back to macflim, exit, finder, shutdown

**** Mac 128. So, there *is* something I want to show you. It is the anniversary of the original Mac 128 today. And frankly, while all the commercials at the time were nice, I think I personally would have gone for something else entirely.

**** Insert disk.

**** A feature of MacFlim I didn't talk about it the ability to transform flims into small self-playable application. It is great when you need the whole thing to fit on a 400KB disk...

[F32] Amigaball 128

**** Showcase du mode standalone. La boule d'amiga joue a l'ecran.


**** "Suck on *that*, AMIGA."

**** And you can play real flims too, no doubt the original mac was a multimedia machine.

[F33] Matrix 128

**** Exit desktop

**** So, we did all the black and white macintosh, right? Well, there is one missing, the Macintosh Portable. I wrote some specific assembly routines so MacFlim does work on it, but mine is currently broken. You have to believe me on this one.

**** And there is another one missing, one both older and younger than the original Mac

**** Shows Lisa XL

**** This is Mac Works XL 3, running on an unmodded Lisa 2/10. It runs a pretty old version of MacOs. It is a slower computer than the Mac, but has more RAM and the hard drive is better than the Mac floppy.

[F34] Launch Star Wars

So, that's all for today, hop you enjoyed. As a bonus, here is a visual comparion of the quality of the encodings in various cases. Please visit www.macflim.com and the github, to get MacFlim and some example flims. Have fun!

comparaison des 4 matrix.
comparaison des 4 bad apple.
comparaison des 4 rick roll.



ffmpeg -loglevel info -y -i "YT-99-MAT-se30.mp4" -i "YT-99-MAT-se.mp4" -i "YT-99-MAT-plus.mp4" -i "YT-99-MAT-xl.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" YT-99-MAT-grid.mp4 && open YT-99-MAT-grid.mp4 
ffmpeg -loglevel info -y -i "YT-99-BAD-se30.mp4" -i "YT-99-BAD-se.mp4" -i "YT-99-BAD-plus.mp4" -i "YT-99-BAD-xl.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" YT-99-BAD-grid.mp4 && open YT-99-BAD-grid.mp4 
ffmpeg -loglevel info -y -i "YT-99-RR-se30.mp4" -i "YT-99-RR-se.mp4" -i "YT-99-RR-plus.mp4" -i "YT-99-RR-xl.mp4" -filter_complex "[0:v][1:v]hstack=inputs=2[top]; [2:v][3:v]hstack=inputs=2[bottom]; [top][bottom]vstack=inputs=2[v]; [2:a]acopy[a]" -map_metadata -1 -map "[v]" -map "[a]" YT-99-RR-grid.mp4 && open YT-99-RR-grid.mp4 

