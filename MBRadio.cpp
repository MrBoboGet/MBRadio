#include <MBRadio.h>
#include <mutex>
namespace MBRadio
{

	//BEGIN  REPLWindow
	bool REPLWindow::Updated()
	{
		return(m_Updated);
	}
	void REPLWindow::SetActiveWindow(bool ActiveStatus)
	{
		m_IsActive = ActiveStatus;
	}
	void REPLWindow::SetDimension(int Width, int Height)
	{
		m_Width = Width;
		m_Height = Height;
	}
	MBCLI::TerminalWindowBuffer REPLWindow::GetDisplay()
	{
		MBCLI::TerminalWindowBuffer ReturnValue(m_Width,m_Height);
		
		int CommandHeight = (m_Height ) / 3;
		MBCLI::TerminalWindowBuffer CommandBuffer(m_Width, CommandHeight);

		if (m_IsActive)
		{
			for (size_t i = 0; i < m_Width; i++)
			{
				CommandBuffer.BufferCharacters[0][i].Character = "\xe2\x96\x88";
				CommandBuffer.BufferCharacters[CommandHeight - 1][i].Character = "\xe2\x96\x88";
			}
			for (size_t i = 0; i < CommandHeight; i++)
			{
				CommandBuffer.BufferCharacters[i][0].Character = "\xe2\x96\x88";
				CommandBuffer.BufferCharacters[i][m_Width-1].Character = "\xe2\x96\x88	";
			}
		}
		std::vector<MBUnicode::GraphemeCluster> const& CommandCharacters = m_InputLineReciever.GetLineBuffer();
		size_t CurrentRowIndex = CommandHeight - 2;
		size_t CurrentColumnIndex = 1;
		for (size_t i = 0; i < CommandCharacters.size(); i++)
		{
			if ((CurrentColumnIndex+2) % m_Width == 0)
			{
				CurrentRowIndex -=1;
				CurrentColumnIndex = 1;
				if (CurrentRowIndex == -1)
				{
					break;
				}
			}
			CommandBuffer.BufferCharacters[CurrentRowIndex][CurrentColumnIndex].Character = CommandCharacters[i];
			CurrentColumnIndex++;
		}

		MBCLI::TerminalWindowBuffer OutputBuffer = m_OutputBuffer.GetWindowBuffer(0, m_Width, m_Height - CommandHeight);

		ReturnValue.WriteBuffer(CommandBuffer, 0, 0);
		ReturnValue.WriteBuffer(OutputBuffer, CommandHeight, 0);

		m_Updated = false;
		return(ReturnValue);
	}
	void REPLWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		if (InputToHandle.CharacterInput != '\n' && InputToHandle.CharacterInput != "\r\n")
		{
			m_InputLineReciever.InsertInput(InputToHandle);
		}
		else
		{
			//evaluate command egentligen, men nu bara läser vi av karaktärerna
			m_OutputBuffer.AddLine(m_InputLineReciever.GetLineString());
			m_InputLineReciever.Reset();
		}
		m_Updated = true;

	}
	//END REPLWindow

	//BEGIN PlayListWindow
	bool PlayListWindow::Updated()
	{
		return(false);
	}
	void PlayListWindow::SetActiveWindow(bool ActiveStatus)
	{
		//
	}
	void PlayListWindow::SetDimension(int Width, int Height)
	{
		//
		m_Width = Width;
		m_Height = Height;
	}
	MBCLI::TerminalWindowBuffer PlayListWindow::GetDisplay()
	{
		MBCLI::TerminalWindowBuffer ReturnValue(m_Width,m_Height);


		return(ReturnValue);
	}
	void PlayListWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		//
	}
	//END PlayListWindow

	//BEGIN SongWindow
	bool SongWindow::Updated()
	{
		return(false);
	}
	void SongWindow::SetActiveWindow(bool ActiveStatus)
	{
		//
	}
	void SongWindow::SetDimension(int Width, int Height)
	{
		m_Width = Width;
		m_Height = Height;
	}
	MBCLI::TerminalWindowBuffer SongWindow::GetDisplay()
	{
		MBCLI::TerminalWindowBuffer ReturnValue(m_Width, m_Height);


		return(ReturnValue);
	}
	void SongWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		//
	}
	//END SongWindow


	//BEGIN MBRadio
	MBRadio::MBRadio(MBCLI::MBTerminal* TerminalToUse)
	{
		m_AssociatedTerminal = TerminalToUse;
		MBCLI::TerminalInfo Info = TerminalToUse->GetTerminalInfo();
		TerminalToUse->InitializeWindowMode();
		TerminalToUse->SetBufferRenderMode(true);
		TerminalToUse->SetResizeCallback(MBRadio::p_WindowResizeCallback, this);

		m_TerminalWidth = Info.Width;
		m_TerminalHeight = Info.Height;
		m_REPLWindow.SetDimension(m_TerminalWidth, m_TerminalHeight);
		m_PlaylistWindow.SetDimension(m_TerminalWidth, m_TerminalHeight);
		m_SongWindow.SetDimension(m_TerminalWidth, m_TerminalHeight);
		m_WindowBuffer = MBCLI::TerminalWindowBuffer(m_TerminalWidth, m_TerminalHeight);
		m_REPLWindow.SetActiveWindow(true);

	}
	void MBRadio::p_WindowResizeCallback(void* Terminal, int NewWidth, int NewHeight)
	{
		MBRadio* RadioObject = (MBRadio*)Terminal;
		{
			RadioObject->m_AssociatedTerminal->Clear();
			std::lock_guard<std::mutex> Lock(RadioObject->m_InternalsMutex);
			RadioObject->m_TerminalWidth = NewWidth;
			RadioObject->m_TerminalHeight = NewHeight;
			RadioObject->m_WindowBuffer = MBCLI::TerminalWindowBuffer(NewWidth, NewHeight);
		}
		RadioObject->p_UpdateWindow();
	}
	int MBRadio::Run()
	{
		while (true)
		{
			//busy waiting, aningen inneffektivt med jaja walla
			if (m_AssociatedTerminal->InputAvailable())
			{
				MBCLI::ConsoleInput NewInput = m_AssociatedTerminal->ReadNextInput();
				if (m_ActiveWindow == MBRadioWindowType::REPL)
				{
					m_REPLWindow.HandleInput(NewInput);
				}
				else if (m_ActiveWindow == MBRadioWindowType::Song)
				{
					m_SongWindow.HandleInput(NewInput);
				}
				else if(m_ActiveWindow == MBRadioWindowType::Playlist)
				{
					m_PlaylistWindow.HandleInput(NewInput);
				}
			}
			bool WindowUpdated = false;
			if (m_PlaylistWindow.Updated() || m_REPLWindow.Updated() || m_SongWindow.Updated())
			{
				WindowUpdated = true;
			}
			if (WindowUpdated)
			{
				p_UpdateWindow();
			}
		}
	}
	void MBRadio::p_UpdateWindow()
	{
		std::lock_guard<std::mutex> Lock(m_InternalsMutex);
		int SongHeight = 4;
		int SongWidth = m_TerminalWidth;
		int REPLHeight = m_TerminalHeight - SongHeight;
		int REPLWidth = (m_TerminalWidth * 2) / 3;
		int PlaylistHeight = REPLHeight;
		int PlaylistWidth = m_TerminalWidth - REPLWidth;

		m_SongWindow.SetDimension(SongWidth, SongHeight);
		m_PlaylistWindow.SetDimension(PlaylistWidth, PlaylistHeight);
		m_REPLWindow.SetDimension(REPLWidth, REPLHeight);

		m_WindowBuffer.WriteBuffer(m_SongWindow.GetDisplay(), 0, 0);
		m_WindowBuffer.WriteBuffer(m_REPLWindow.GetDisplay(), SongHeight, 0);
		m_WindowBuffer.WriteBuffer(m_PlaylistWindow.GetDisplay(), SongHeight, REPLWidth);


		m_AssociatedTerminal->PrintWindowBuffer(m_WindowBuffer, 0, 0);
		m_AssociatedTerminal->Refresh();


	}
	//END MBRadio
}