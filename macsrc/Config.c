#include "Config.h"

#include <Memory.h>

#include <stdio.h>

struct Machine
{
	short rom;
	short model;

	Boolean supportsSounds;	//	If true, we can play sound (machine is sufficiently powerful)
};

static struct Machine gMachine;

void CheckMachine()
{
	Environs( &gMachine.rom, &gMachine.model );

//	machine->hasMaxApplZone = GetTrapAddress( 0xA063 )!=0;

//	printf( "%x %x\n", gMachine.rom, gMachine.model );

/*
	if (machine->hasMaxApplZone)
		printf( "With MaxApplZone\n" );
	else
		printf( "Without MaxApplZone\n" );
*/
}

Boolean MinimalVersion( void )
{
#ifndef FORCE_MINIMAL
	if (gMachine.model==0x00)	//	MacXL
		return TRUE;
	return FALSE;
#else
	return TRUE;
#endif
}

//	Return TRUE is we are running on a computer where HFS is unsupported
//Boolean NoHFS( void );
	