#include <MBCLI/MBCLI.h>
#include <MBPlay/MBPlay.h>
#include <MBAudioEngine/MBAudioDevices.h>
#include <MBParsing/MBParsing.h>
#include "MBRadio_Defines.h"
namespace MBRadio
{
	class MBRadio;
	class Song
	{
	public:
		std::string SongURI = "";
		std::string SongName = "";
		std::string Artist = "";

		bool operator==(Song const& rhs)
		{
			bool ReturnValue = true;
			if (SongURI != rhs.SongURI || SongName != rhs.SongURI || Artist != rhs.Artist)
			{
				ReturnValue = false;
			}

			return(ReturnValue);
		}
		bool operator!=(Song const& rhs)
		{
			return(!(*this == rhs));
		}
	};
	class Playlist
	{
	private:
		std::vector<Song> m_Songs = {};
	public:
		Song GetNextSong();
		void Shuffle();
		std::vector<Song> const& GetSongs();
	};
	struct CursorInfo
	{
		bool Hidden = true;
		MBCLI::CursorPosition Position = { 0,0 };
	};
	class MBRadioWindow
	{
		//allt antas vara thread safe
	public:
		virtual bool Updated() = 0;
		virtual void SetActiveWindow(bool ActiveStatus) = 0;
		virtual CursorInfo GetCursorInfo()
		{
			return(CursorInfo());
		}
		virtual void SetDimension(int Width, int Height) = 0;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() = 0;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) = 0;
	};

	class PlayListWindow : public MBRadioWindow
	{
	private:
		std::mutex m_InternalsMutex;
		bool m_ActiveWindow = false;
		int m_Width = -1;
		int m_Height = -1;
		bool m_Updated = true;
		bool m_Shuffle = false;
		int m_SongDisplayIndex = 0;
		int m_CurrentSongIndex = -1;
		std::vector<Song> m_PlaylistSongs = {};
		std::vector<size_t> m_ShuffleIndexes = {};
	public:
		virtual bool Updated() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override; 
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;

		void RemoveSong(size_t SongIndex);
		void ClearSongs();
		void Scroll(int ScrollIndex);
		int GetCurrentDisplayIndex() const;

		void AddSong(Song SongToAdd,size_t SongPosition = -1);
		void SetShuffle(bool IsShuffle);
		Song GetNextSong();
		void SetNextSong(size_t SongIndex);

	};
	//REPLWindow window handlers

	class REPLWindow_QueryDisplayer : public MBRadioWindow
	{
	private:
		std::mutex m_InternalsMutex;
		std::atomic<bool> m_Updated{ true };
		MBCLI::TableCreator m_ResultTable;
		int m_CurrentRowOffset = 0;
		int m_CurrentCharacterOffset = 0;

		int m_Width = 0;
		int m_Height = 0;

	public:
		//REPLWindow_QueryDisplayer(MBParsing::JSONObject const& QueryDirectiveResponse,int InitialWidth,int InitialHeight);
		REPLWindow_QueryDisplayer(std::vector<std::vector<std::string>> QueryDirectiveResponse,int InitialWidth,int InitialHeight);
		


		virtual bool Updated() override;
		virtual CursorInfo GetCursorInfo() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;
	};


	class REPLWindow : public MBRadioWindow
	{
	private:
		bool m_IsActive = false;
		std::atomic<bool> m_Updated{true};
		int m_Height = -1;
		int m_Width = -1;

		std::unique_ptr<MBRadioWindow> m_ResultWindow = nullptr;

		MBCLI::LineBuffer m_OutputBuffer;
		MBCLI::DefaultInputReciever m_InputLineReciever;

		std::vector<std::string> m_PreviousCommands;
		size_t m_CurrentCommandIndex = -1;

		MBRadio* m_AssociatedRadio = nullptr;

		void p_HandleQuerryCommand(std::string const& QuerryCommand);
		void p_HandleAddCommand(std::string const& AddCommand);

		std::vector<std::vector<std::string>> p_GetQuerryResponse(std::string const& Host, std::string const& Querry,MBParsing::JSONObject VerificationData,MBError* OutError);

		MBParsing::JSONObject p_GetSavedMBSiteUserAuthentication();
		std::string p_GetMBSiteURL();
	public:
		REPLWindow(MBRadio* AssociatedRadio);

		virtual bool Updated() override;
		virtual CursorInfo GetCursorInfo() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;
	};
	class SongPlaybacker
	{
	private:
		std::atomic<bool> m_Stopping{ false };

		std::atomic<bool> m_Play{ true };
		
		std::thread m_AudioThread;
		std::unique_ptr<MBAE::AudioOutputDevice> m_OutputDevice = nullptr;
		
		std::mutex m_RequestMutex;
		std::condition_variable m_RequestConditional;

		std::atomic<bool> m_SongRequested{ false };
		Song m_RequestedSong;
		std::atomic<bool> m_SeekRequested{ false };
		double m_SeekPosition;

		//size_t m_AudioStreamIndex = -1;

		std::atomic<bool> m_InputLoaded{ false };
		std::atomic<bool> m_ErrorLoadingInput{ false };
		std::atomic<double> m_SongDuration{ -1 };
		std::atomic<double> m_SongPosition{ -1 };

		Song m_CurrentSong;
		
		void p_AudioThread();
	public:
		SongPlaybacker();

		void SetInputSource(Song SongToPlay);
	
		Song GetCurrentSong();

		bool InputLoaded();
		bool InputAvailable();

		void Pause();
		void Play();
		bool Paused();
		void Seek(double SecondOffset);
		double GetSongDuration();
		double GetSongPosition();

		~SongPlaybacker();
	};
	class SongWindow : public MBRadioWindow
	{
	private:
		int m_Width = -1;
		int m_Height = -1;
		std::atomic<bool> m_Updated{ false };
		std::atomic<float> m_CurrentSongTimeInSecs{ 150 };
		std::atomic<float> m_CurrentSongTotalTimeInSecs{-1};
		MBCLI::TerminalWindowBuffer p_GetProgressBarString(float ElapsedTime, float TotalTime);
		MBCLI::TerminalWindowBuffer p_GetTimeBuffer();
		static std::string p_TimeToString(float TimeToConvert);
		MBCLI::TerminalWindowBuffer p_GetSongProgressBar();

		Song m_CurrentSong;
		std::unique_ptr<SongPlaybacker> m_Playbacker = nullptr;
		MBRadio* m_AssociatedRadio = nullptr;

		std::thread m_UpdateThread;
		std::atomic<bool> m_Stopping{ false };
		void p_UpdateHandler();
#ifdef MBRADIO_DISCORDINTEGRATION
		static void p_FreeDiscordImpl(void*);
		std::unique_ptr<void, void (*)(void*)> m_DiscordImpl = std::unique_ptr<void, void(*)(void*)>(nullptr,&SongWindow::p_FreeDiscordImpl);
#endif // 

		//std::unique_ptr<MBMedia::AudioStream> m_InputStream = nullptr;
		//std::unique_ptr<MBAE::AudioOutputDevice> m_OutputDevice = nullptr;
	public:
		virtual bool Updated() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;

		SongWindow(MBRadio* AssociatedRadio);
		void Pause();
		void Play();
		bool Paused();
		void PlaySong(Song SongToPlay);
		void Seek(double SecondOffset);
		double GetSongDuration();
		double GetSongPosition();
	};
	enum class MBRadioWindowType
	{
		REPL,
		Song,
		Playlist,
		Null,
	};
	class MBRadio
	{
	private:
		//får enbart kommas åt av p_UpdateWindow
		MBCLI::MBTerminal* m_AssociatedTerminal = nullptr;
		MBRadioWindowType m_ActiveWindow = MBRadioWindowType::REPL;

		void p_UpdateWindow();

		std::mutex m_InternalsMutex;
		int m_TerminalWidth = 0;
		int m_TerminalHeight = 0;
		MBCLI::TerminalWindowBuffer m_WindowBuffer;

		SongWindow m_SongWindow;
		PlayListWindow m_PlaylistWindow;
		REPLWindow m_REPLWindow;

		static void p_WindowResizeCallback(void* Terminal,int NewWidth,int NewHeight);
	public:
		MBRadio(MBCLI::MBTerminal* TerminalToUse);
		void PlaySong(Song SongToplay);
		void Update();
		
		void SetPause(bool PauseStatus);
		bool GetPause();
		int GetSongDisplayIndex();
		void Scroll(int SongIndex);
		void ClearSongs();
		void RemoveSong(size_t SongIndex);

		void AddSong(Song SongToAdd, size_t SongPosition = -1);
		void SetShuffle(bool IsShuffle);
		Song GetNextSong();
		void SetCurrentSong(size_t SongIndex);

		int Run();
	};
}
