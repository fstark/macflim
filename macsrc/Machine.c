#include "Machine.h"

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
	
	Boolean minimal;		//	The machine needs to use the minimal implementation of MacFlim

	Size memory;
};

static struct Machine gMachine;

void MachineInit()
{
	Environs( &gMachine.rom, &gMachine.model );

	MaxMem( &gMachine.memory );

	gMachine.unimplemented = GetTrapAddress( 0xA89F );

//	printf( "%p ", gMachine.unimplemented );
//	printf( "%p\n", GetTrapAddress( 0xA063 ) );	//	MaxApplZone -- doesn't work

	gMachine.minimal = FALSE;

	if (gMachine.model==0x00)	//	MacXL (but I don't think we really need to)
		gMachine.minimal = TRUE;

	if (gMachine.memory<128000L)
		gMachine.minimal = TRUE;

#ifdef FORCE_MINIMAL
	gMachine.minimal = TRUE;
#endif

//	machine->hasMaxApplZone = GetTrapAddress( 0xA063 )!=0;

//	printf( "ROM:%x MODEL:%x\n", gMachine.rom, gMachine.model );

/*
	if (machine->hasMaxApplZone)
		printf( "With MaxApplZone\n" );
	else
		printf( "Without MaxApplZone\n" );
*/
}

Boolean MachineIsMinimal( void )
{
	return gMachine.minimal;
}

//	Return TRUE is we are running on a computer where HFS is unsupported
//Boolean NoHFS( void );

Size MachineGetMemory( void )
{
	return gMachine.memory;
}

/*
Size MachineGetDefaultBlock( void )
{
	if (gMachine.minimal)
		return 10000L;
	
	return 300000L;
}
*/