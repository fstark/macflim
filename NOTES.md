hmount "../MacFlim Source Code.dsk"
hcopy Flim.c ":MacFlim Sources:Sources:"


hmount MacFlim-Release-build.dsk
hcopy :MacFlim /tmp
humount
hmount '/media/fred/ZULU IISI/HD40_512 - MacFlim2 Demo v0.3 - 1.6GB.hda' 
hcopy -m /tmp/MacFlim.bin :MacFlim
humount

