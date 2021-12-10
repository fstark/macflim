/*

	{
			;	Save registers
		movem.l D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screen address
		movea.l source,a3			;	a3 == source data

@loop:
		move.w	(a3)+,d7			;	header
		tst.w	d7					;	0x0000 => end of frame
		beq.s	@exit
									;	Low end of d7 is offset
;		move.w	d7,d6
;		and.w	#0x00ff,d6
;		add		d6,a4				;	new screen address
;		add		d6,a4				;	new screen address
;		movea.l	a4,a2				
						
		lsr.w	#8,d7
		tst.w d7
		bra.s @loop

@local:
		add #1,a3
		tst.w d7
		dbeq d7,@local

		beq @loop

@loop2:
;        move.w	(a3)+,(a2)
 		adda	#1,a3
        
;        add 	#64,a2		;	Take stride into account
@in:
		tst.w	d7
		dbeq.w	d7,@loop2
	
        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D5-D7/A2-A4
	}


long BlockRead( BlockPtr blk, short fRefNum, long read_size )

GetOtherBlock

	blockUnused = 0,		//	Nothing to be done
	blockReading = 1,		//	A PBRead is active on the block
	blockReady = 2,			//	The block have data in read from the disk
	blockPlaying = 3,		//	Content of the block is currently used by the sound driver
	blockPlayed = 4,		//	The content of the block have been played
	blockClosed = 5			//	We don't use this block anymore, buffer deallocated

BlockWaitPlayed

 */

#if 0

void UnpackZ8( char *dest, unsigned char *source )
{
	asm
	{
			;	Save registers
		movem.l D7/A3/A4,-(A7)

			;	Get parameters
		movea.l dest,a4
		movea.l source,a3

@loop:
			;	Get opcode
			;	0 => exit
			;	>0 => copy data
			;	<0 => skip data
        move.b 	(a3)+,d7
        tst.b	d7
		beq	 	@exit
        blt	  	@skip

@copy:
			;	Copy d7 bytes from a3 to a4
		move.b 	(a3)+,(a4)+
        subq.b 	#1,d7
        bne  	@copy

        bra  	@loop

@skip:
			;	Skip d7 bytes in a4
		ext		d7
		suba	d7,a4
        bra  	@loop

@exit:
			;	Done
        movem.l   (A7)+,D7/A3/A4
	}
}

/*
--top
  source		a6+c
  dest			a6+8
  return adrs	a6+4
  [a6]			a6
  n				a6-1
--bottom
*/

void UnpackZ8_orig( char *dest, unsigned char *source )
{
	char *d = dest;
	unsigned char *s = source;

	while (1)
	{
		register char n = *s++;
		if (n==0)
			break;
		if (n>0)
		{
			while (n--)
				*d++ = *s++;
		}
		else
			d += -n;
	}
}
#endif



#if 0

//	main display code

	while (!Button())
	{
		if (video_frame!=current_frame)
		{
			if (video_frame==0)
				ClearScreen();
			else
				UnpackXor( screenBits.baseAddr, (Ptr)fs[0].frame[video_frame].video_data );
	
			video_frame++;
			if (video_frame==20)
				video_frame = 0;
		}
	}





//	-------------------------------------------------------------------
//	VBL Task, called 60 times a second
//	-------------------------------------------------------------------

//	We insert that in the VBL task list
//	We have saved the value of A5 in it, so we can access our globals
//	from the VBL code
typedef struct MyTaskElem
{
    long myA5;
    VBLTask gTask;
};

//	The VBL task
struct MyTaskElem taskElem;

int video_frame = 0;

pascal void vbl_task()
{
		//	Recover the value of A5 for access to global
		//	from the 4 bytes before the VBL entry
    asm
    {
        move.l A5,-(SP)
        move.l -4(A0),A5
    }

		//	We want to be called next frame
		//	Unless we decide otherwise
	taskElem.gTask.vblCount = 1;

		//	If there is a player object
		//	we blast the movie

	if (video_frame!=current_frame)
	{
		if (video_frame==0)
			ClearScreen();
		else
			UnpackXor( screenBits.baseAddr, (Ptr)fs[0].frame[video_frame].video_data );

		video_frame++;
		if (video_frame==20)
			video_frame = 0;
	}

		//	Restore A5 to the value it had before entry
    asm
    {
       move.l (SP)+,A5
    }
}

//	-------------------------------------------------------------------
//	Installs VBL Handler
//	-------------------------------------------------------------------

static void InstallHandler()
{
    OSErr theError;
    taskElem.gTask.qType = vType;
    taskElem.gTask.vblAddr = vbl_task;
    taskElem.gTask.vblCount = 1;
    taskElem.gTask.vblPhase = 0;
    taskElem.myA5 = (long)CurrentA5;

    theError = VInstall( (QElemPtr)&taskElem.gTask );
//    assert( theError==noErr );
}

//	-------------------------------------------------------------------
//	Removes VBL Handler
//	-------------------------------------------------------------------

static void RemoveHandler()
{
    OSErr theError;
    theError = VRemove( (QElemPtr)&taskElem.gTask );
//    assert( theError==noErr );
}



#if 0

	current = 1-current;
	FSRead( fRefNum, &read_size, data[current] );

		//	We are back to block 0
	current = 1-current;


		//	Prepare async read
	pb.ioCompletion = NULL;
	pb.ioRefNum = fRefNum;
	pb.ioPosMode = fsAtMark;
	pb.ioPosOffset = 0;
	pb.ioResult = 0;

#endif

#endif




#if 0

//	-------------------------------------------------------------------
//	Unpack to screen buffer, suitable for non 512x342 screens
//	slower due to use of multiplication
//	(This is fixable, but non 512x342 have a lot of horsepower)
//	(and MacintoshXL deserves its own implementation)
//	-------------------------------------------------------------------

void UnpackZ32_ext( char *dest, char *source, int rowbytes )
{
	asm
	{
			;	Save registers
		movem.l D0/D5-D7/A2-A4,-(A7)

			;	Get parameters
		movea.l dest,a4				;	a4 == screenBase
		movea.l source,a3			;	a3 == source data
		move.w	rowbytes,d6			;	rowbytes

@loop:
		move.l	(a3)+,d7			;	header
		tst.l	d7					;	0x00000000 => end of frame
		beq.s	@exit

		movea.l	a4,a2				
									;	Low end of d7 is offset

		clr.l	d0
		sub.w	#4,d7
		move.w  d7,d0

		asr.w	#6,d0
		mulu.w	d6,d0	
		add.l	d0,a2

		and.w	#0x03f,d7
		add.w	d7,a2

		swap	d7

@loop2:
        move.l	(a3)+,(a2)
        add 	d6,a2				;	Take stride into account
		tst.w	d7
		dbeq.w	d7,@loop2
	
        bra.s     @loop

			;	Done
@exit:
        movem.l   (A7)+,D0/D5-D7/A2-A4
	}
}

#endif



