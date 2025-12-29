/*
  This command is used in the CICD to shutdown the Mac
  It will only work with System 7 (at least under minivmac)
  It is compiled & run after the generation of all the binaries
*/

#include <ShutDown.h>

int main()
{
	ShutDwnPower();
}
