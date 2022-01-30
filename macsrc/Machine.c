#include "Machine.h"

//	-------------------------------------------------------------------
//	INCLUDES
//	-------------------------------------------------------------------

#include <stdio.h>

#include "Config.h"
#include "Log.h"

//	-------------------------------------------------------------------

struct Machine
{
	short rom;
	short model;

	Boolean originalROM;	//	If true, we can't use 128K ROM traps
	Boolean supportsSounds;	//	If true, we can play sound (machine is sufficiently powerful)
	Boolean supportsColor;	//	If we risk finding color

	long unimplemented;		//	The unimplemented trap address
	
	Boolean minimal;		//	The machine needs to use the minimal implementation of MacFlim

	Size memory;

	long bogoMips;			//	Perf of this mac
							//	SE30 = 18450
							//	Plus = 2438
};

static struct Machine gMachine;

//	-------------------------------------------------------------------
//	Assembly version to ensure consistence accross compiler options
//	-------------------------------------------------------------------

static long BogoMips()
{
/*	long t = Ticks;
	long i = 0;
	
	while (t==Ticks);
	t = Ticks;
	while (t==Ticks) i++;
	return i;
*/
	asm {
			movem.l   D7,-(A7)
	        move.l    Ticks,D7
	        moveq     #0,D0
loop0:
	        cmp.l     Ticks,D7
	        beq	      @loop0
	        move.l    Ticks,D7
loop1:
	        addq.l    #1,D0
	        cmp.l     Ticks,D7
	        beq       @loop1
			movem.l   (A7)+,D7
	}
}

void MachineInit()
{
	Size growBytes;

	Environs( &gMachine.rom, &gMachine.model );

	gMachine.originalROM = gMachine.rom<117;

	gMachine.memory = MaxMem( &growBytes );
	gMachine.memory += growBytes;

	gMachine.supportsColor = FALSE;

	gMachine.unimplemented = GetTrapAddress( 0xA89F );

//	printf( "%p ", gMachine.unimplemented );
//	printf( "%p\n", GetTrapAddress( 0xA063 ) );	//	MaxApplZone -- doesn't work

	gMachine.minimal = FALSE;

	if (gMachine.originalROM)
		gMachine.minimal = TRUE;

		//	MacXL: for whatever reason MacWorks XL 3.0 don't catch 'originalROM'
		//	This seems illogical, as 128K ROM was MacWorks Plus...
	if (gMachine.model==0x00)
		gMachine.minimal = TRUE;

#ifdef FORCE_MINIMAL
	gMachine.minimal = TRUE;
#endif

//	printf( "ROM:%x MODEL:%x\n", gMachine.rom, gMachine.model );

	gMachine.bogoMips = BogoMips();

	if (!gMachine.originalROM)
	{
		SysEnvRec theEnvRec;
		OSErr err;

		err = SysEnvirons( 1, &theEnvRec );

		if (err!=noErr)
		{
	/*
			printf( "version = %d\n", theEnvRec.environsVersion );
			printf( "machine = %d\n", theEnvRec.machineType );
			printf( "system  = %x\n", theEnvRec.systemVersion );
			printf( "color   = %s\n", theEnvRec.hasColorQD?"Y":"N" );
	*/
			gMachine.supportsColor = theEnvRec.hasColorQD;
		}
	}
}

//	-------------------------------------------------------------------

Boolean MachineIsMinimal( void )
{
	return gMachine.minimal;
}

//	-------------------------------------------------------------------

Size MachineGetMemory( void )
{
	return gMachine.memory;
}

//	-------------------------------------------------------------------

long MachineGetBogoMips()
{
	return gMachine.bogoMips;
}

//	-------------------------------------------------------------------

Boolean MachineIsBlackAndWhite()
{
	GDHandle d = TheGDevice;
	PixMapHandle p;
	
	if (!gMachine.supportsColor)
		return TRUE;			//	If no colorQD TheGDevice will point to garbage

	if (!d)
		return TRUE;
	p = (*d)->gdPMap;
	if (!p)
		return TRUE;
	if ((*p)->pixelSize==1)
		return TRUE;

	return FALSE;
}
