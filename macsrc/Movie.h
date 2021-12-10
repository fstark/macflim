#ifndef MOVIE_INCLUDED__
#define MOVIE_INCLUDED__

/*
	File format:
		File header: 'FLIM' + 0x0a + 1013 characters of comments +
		
		2 character of checksum.

		Checksum is the fletcher16 of bytes 1024 to the end of file.

		First video block starts at 0x400 (1024)

		2 bytes version
		4 bytes toc offset
		4 bytes frame count

		frame_count times:
			2 bytes : ticks
			2 bytes : sound_size
					  including 2 bytes sound_size and header,
					  eg: 378 if only a single tick of data)
			6 bytes : sound header
			n bytes : sound_size-8 bytes of sound data
					  8 bits unsigned mono 22200Hz
			          size multiple of 370
			2 bytes : video_size (big endian)
			          size of encoded video for this frame
			          including 2 bytes of video_size
			n bytes : video_size-2 bytes of video data

		toc for each entry:
			2 bytes : size of frame
*/

struct MovieRec;
typedef struct MovieRec* MoviePtr;

MoviePtr MovieOpen( short fRefNum, long maxBlockSize );
void MovieDispos( MoviePtr movie );
void MovieSeekStart( MoviePtr movie );
int MovieGetBlockCount( MoviePtr movie );
int MovieGetMaxBlockSize( MoviePtr movie );
int MovieGetFrameCountOfBlock( MoviePtr movie, int index );
int MovieGetBlockFrameCount( MoviePtr movie, int index );
int MovieGetBlockSize( MoviePtr movie, int index );
int MovieGetFileRefNum( MoviePtr movie );

#endif
