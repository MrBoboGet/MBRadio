#include "MBRadio.h"
#include <iostream>
#include <MBCLI/MBCLI.h>
#include <ctime>
int main(int argc, const char** argv)
{
	MBCLI::MBTerminal TerminalToUse;
	MBRadio::MBRadio RadioObject(&TerminalToUse);
	return(RadioObject.Run());
}