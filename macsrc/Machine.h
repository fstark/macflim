#ifndef MACHINE_INCLUDED__
#define MACHINE_INCLUDED__

//	-------------------------------------------------------------------
//	Call once to perform checks about the kind of enviroment
//	we are running in
//	-------------------------------------------------------------------

void MachineInit( void );

//	-------------------------------------------------------------------
//	If TRUE, we are running on a very old machine, and should avoid 
//	*everything* fancy
//	It isn't just about speed or memory, but also System/ROM
//	-------------------------------------------------------------------

Boolean MachineIsMinimal( void );

//	-------------------------------------------------------------------
//	Returns the amount of memory that was available at startup
//	-------------------------------------------------------------------

Size MachineGetMemory( void );

//	-------------------------------------------------------------------
//	Some UI goodies eat CPU time, so we look at how quick the machine is
//	(can't rely on Environs)
//	-------------------------------------------------------------------

long MachineGetBogoMips( void );

//	-------------------------------------------------------------------
//	TRUE if we think that the screen is Black and White
//	-------------------------------------------------------------------

Boolean MachineIsBlackAndWhite( void );

#endif
