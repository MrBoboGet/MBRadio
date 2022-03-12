#include "MBRadio.h"
#include <iostream>
#include <MBCLI/MBCLI.h>
#include <ctime>
#include <MrBoboSockets/MrBoboSockets.h>
int main(int argc, const char** argv)
{
	//std::cout << std::string("asd", 1000) << std::endl;
	//exit(0);
	MBSockets::Init();
	//MBSockets::HTTPFileStream FileStreamTest;
	//FileStreamTest.SetInputURL("https://127.0.0.1/DB/Playlists/TestPlaylist.mbdbo");
	//const size_t BufferSize = 3000;
	//char Buffer[BufferSize];
	//size_t Result = FileStreamTest.Read(Buffer, 3000);
	//if (Result == -1)
	//{
	//	std::cout << ":(" << std::endl;
	//}
	//std::cout << std::string(Buffer, BufferSize) << std::endl;
	//std::exit(0);
	MBCLI::MBTerminal TerminalToUse;
	MBRadio::MBRadio RadioObject(&TerminalToUse);
	return(RadioObject.Run());
}