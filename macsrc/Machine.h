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
//	-------------------------------------------------------------------
Boolean MachineIsMinimal( void );

//	-------------------------------------------------------------------
//	Returns the amount of memory that was available at startup
//	-------------------------------------------------------------------
Size MachineGetMemory( void );

//	-------------------------------------------------------------------
//	Returns a default block size for the current machine
//	-------------------------------------------------------------------
//ßSize MachineGetDefaultBlock( void );

#endif
