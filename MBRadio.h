#include <MBCLI/MBCLI.h>
namespace MBRadio
{
	
	class Song
	{
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

	class MBRadioWindow
	{
		//allt antas vara thread safe
	public:
		virtual bool Updated() = 0;
		virtual void SetActiveWindow(bool ActiveStatus) = 0;
		virtual void SetDimension(int Width, int Height) = 0;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() = 0;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) = 0;
	};

	class PlayListWindow : public MBRadioWindow
	{
	private:
		int m_Width = -1;
		int m_Height = -1;
	public:
		virtual bool Updated() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override; 
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;
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
	public:
		virtual bool Updated() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;
	};

	class SongWindow : public MBRadioWindow
	{
	private:
		int m_Width = -1;
		int m_Height = -1;
	public:
		virtual bool Updated() override;
		virtual void SetActiveWindow(bool ActiveStatus) override;
		virtual void SetDimension(int Width, int Height) override;
		virtual MBCLI::TerminalWindowBuffer GetDisplay() override;
		virtual void HandleInput(MBCLI::ConsoleInput const& InputToHandle) override;
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
		int Run();
	};
}