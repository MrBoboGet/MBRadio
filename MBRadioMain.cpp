#include "MBRadio.h"
#include <iostream>
#include <MBCLI/MBCLI.h>
#include <ctime>
#include <MrBoboSockets/MrBoboSockets.h>
#include <MBMedia/MBMedia.h>

//#include <DiscordSDK/cpp/discord.h>

int main(int argc, const char** argv)
{
	//std::cout << std::string("asd", 1000) << std::endl;
	//exit(0);
	//discord::Core* core{};
	//auto result = discord::Core::Create(958468397505576971, DiscordCreateFlags_Default, &core);
	//discord::Activity activity{};
	//activity.SetType(discord::ActivityType::Listening);
	//
	//uint64_t CurrentTime = std::time(nullptr);
	//activity.GetTimestamps().SetStart(CurrentTime);
	//activity.SetState("Testing");
	//activity.SetDetails("https://mrboboget.se/");
	//activity.GetAssets().SetLargeImage("gnutophat");
	//
	//core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
	//
	//});
	//while (true)
	//{
	//	core->RunCallbacks();
	//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//}
	//exit(0);
	MBMedia::SetLogLevel(MBMedia::LogLevel::None);
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
