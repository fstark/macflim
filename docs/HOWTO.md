Release process:

1 - Update Notes.c
2 - Generates Binaries for MacFlim and Mini MacFlim
3 - Put correct version number in Finder info
4 - Copy Mac Flim 2.x.x.dsk to new version
5 - Open in Minivmac and copy Apps and sample to this new disk & rename disk
6 - Quit minivmac and Execute doit.sh in macsrc
7 -
git tag -a v2.0.8 -m "Release 2.0.8"
git push origin v2.0.8
8 - Create release in github

# Create a release
git tag -a v2.0.5 -m "Release 2.0.5"
git push origin v2.0.5

Create the release in github


Pre-release of final MacFlim 2.0 version

Fixes since 2.0.6:

* Added option for single frame read-ahead, useful for network play

Format: 800K dsk file


