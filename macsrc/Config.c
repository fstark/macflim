#include "Config.h"
#include "Log.h"

#include <Memory.h>

#include <stdio.h>

struct Machine
{
	short rom;
	short model;

	Boolean supportsSounds;	//	If true, we can play sound (machine is sufficiently powerful)

	long unimplemented;		//	The unimplemented trap address
	
	Boolean minimal;		//	The machine needs to use the minimal implemenation of MacFlim
};

static struct Machine gMachine;

void CheckMachine()
{
	Environs( &gMachine.rom, &gMachine.model );

	gMachine.unimplemented = GetTrapAddress( 0xA89F );

	gMachine.minimal = GetTrapAddress( 0xA9AA )==gMachine.unimplemented;

	if (gMachine.minimal)
		ExitToShell();

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
#ifdef FORCE_MINIMAL
	return TRUE;
#endif

	if (gMachine.model==0x00)	//	MacXL
		return TRUE;

	return gMachine.minimal;
}

//	Return TRUE is we are running on a computer where HFS is unsupported
//Boolean NoHFS( void );
