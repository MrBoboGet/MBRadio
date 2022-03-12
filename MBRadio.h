#include <MBCLI/MBCLI.h>
#include <MBPlay/MBPlay.h>
#include <MBAudioEngine/MBAudioDevices.h>
namespace MBRadio
{
	class MBRadio;
	class Song
	{
	public:
		std::string SongURI = "";
		std::string SongName = "";
		std::string Artist = "";
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
		size_t m_SongDisplayIndex = 0;
		size_t m_CurrentSongIndex = -1;
		std::vector<Song> m_PlaylistSongs = {};
	public:
		virtual bool Updated() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override; 
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;

		void AddSong(Song SongToAdd,size_t SongPosition = -1);
		void SetShuffle(bool IsShuffle);
		Song GetNextSong();
		void SetCurrentSong(size_t SongIndex);

	};
	
	class REPLWindow : public MBRadioWindow
	{
	private:
		bool m_IsActive = false;
		bool m_Updated = true;
		int m_Height = -1;
		int m_Width = -1;

		MBCLI::LineBuffer m_OutputBuffer;
		MBCLI::DefaultInputReciever m_InputLineReciever;

		MBRadio* m_AssociatedRadio = nullptr;
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
		
		std::mutex m_SongRequestMutex;
		std::atomic<bool> m_SongRequested{ false };
		Song m_RequestedSong;

		std::mutex m_SeekRequestMutex;
		std::atomic<bool> m_SeekRequested{ false };
		int64_t m_SeekPosition;

		std::mutex m_DataFetcherMutex;
		size_t m_AudioStreamIndex = -1;

		std::unique_ptr<MBPlay::OctetStreamDataFetcher> m_SongDataFetcher = nullptr;

		void p_AudioThread();
	public:
		SongPlaybacker();

		void SetInputSource(Song SongToPlay);

		bool InputLoaded();

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

		void AddSong(Song SongToAdd, size_t SongPosition = -1);
		void SetShuffle(bool IsShuffle);
		Song GetNextSong();
		void SetCurrentSong(size_t SongIndex);

		int Run();
	};
}