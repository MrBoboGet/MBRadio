#include <MBRadio.h>
#include <mutex>
#include <MrBoboSockets/MrBoboSockets.h>
#include <MBAudioEngine/MBAudioEngine.h>
#include <MBParsing/MBParsing.h>

#include <MrBoboSockets/MrBoboSockets.h>
namespace MBRadio
{
	void h_InferNameAndArtist(Song& SongToModify)
	{
		if (SongToModify.Artist != "" || SongToModify.SongName != "")
		{
			return;
		}
		MBPlay::URLStream URLParser(SongToModify.SongURI);
		if (URLParser.GetFilename() != "")
		{
			std::string NewArtist = "";
			std::string NewName = "";
			std::string FileName = URLParser.GetFilename();
			size_t FirstBind = FileName.find('-');
			if (FirstBind == FileName.npos)
			{
				return;
			}
			NewArtist = MBUtility::ReplaceAll(FileName.substr(0, FirstBind), "_", " ");
			NewName = MBUtility::ReplaceAll(FileName.substr(FirstBind + 1), "_", " ");
			if (NewName.find('.') != NewName.npos)
			{
				NewName = NewName.substr(0, NewName.find('.'));
			}
			SongToModify.Artist = std::move(NewArtist);
			SongToModify.SongName = std::move(NewName);
		}
	}
	
	//BEGIN REPLWindow_QueryDisplayer
	REPLWindow_QueryDisplayer::REPLWindow_QueryDisplayer(MBParsing::JSONObject const& QueryDirectiveResponse,int Width,int Height)
	{
		m_Height = Height;
		m_Width = Width;
		try
		{
			std::vector<std::vector<std::string>> TableContent;
			std::vector<MBParsing::JSONObject> const& Results = QueryDirectiveResponse.GetAttribute("QueryResult").GetArrayData();
			for (size_t i = 0; i < Results.size(); i++)
			{
				std::vector<MBParsing::JSONObject> const& CurrentRow = Results[i].GetArrayData();
				std::vector<std::string> NewRowData;
				for (size_t j = 0; j < CurrentRow.size(); j++)
				{
					NewRowData.push_back(CurrentRow[j].ToString());
				}
				TableContent.push_back(std::move(NewRowData));
			}
			m_ResultTable = MBCLI::TableCreator(TableContent, m_Width);
		}
		catch (std::exception const& e)
		{

		}
	}
	bool REPLWindow_QueryDisplayer::Updated()
	{
		bool ReturnValue = m_Updated.load();
		return(ReturnValue);
	}
	CursorInfo REPLWindow_QueryDisplayer::GetCursorInfo()
	{
		CursorInfo ReturnValue;
		ReturnValue.Hidden = true;
		ReturnValue.Position = { 0,0 };
		return(ReturnValue);
	}
	void REPLWindow_QueryDisplayer::SetActiveWindow(bool ActiveStatus)
	{
		//
	}
	void REPLWindow_QueryDisplayer::SetDimension(int Width, int Height)
	{
		m_Updated.store(true);
		std::lock_guard<std::mutex> Lock(m_InternalsMutex);
		m_Width = Width;
		m_Height = Height;
		m_ResultTable.SetWidth(m_Width);
	}
	MBCLI::TerminalWindowBuffer REPLWindow_QueryDisplayer::GetDisplay()
	{
		std::lock_guard<std::mutex> Lock(m_InternalsMutex);
		return(m_ResultTable.GetWindowBuffer(m_CurrentRowOffset, m_CurrentCharacterOffset, m_Height));
	}
	void REPLWindow_QueryDisplayer::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		std::lock_guard<std::mutex> Lock(m_InternalsMutex);
		bool CTRLHeld = InputToHandle.KeyModifiers & uint64_t(MBCLI::KeyModifier::SHIFT);
		if (InputToHandle.SpecialInput == MBCLI::SpecialKey::Down || InputToHandle.CharacterInput == "w" || InputToHandle.CharacterInput == MBCLI::CTRL('w'))
		{
			if (CTRLHeld)
			{
				m_CurrentCharacterOffset += 1;
			}
			else
			{
				m_CurrentRowOffset += 1;
			}
			m_Updated.store(true);
		}
		else if (InputToHandle.SpecialInput == MBCLI::SpecialKey::Up || InputToHandle.CharacterInput == "s" || InputToHandle.CharacterInput == MBCLI::CTRL('s'))
		{
			if (CTRLHeld)
			{
				m_CurrentCharacterOffset -= 1;
				m_CurrentCharacterOffset = m_CurrentCharacterOffset < 0 ? 0 : m_CurrentCharacterOffset;
			}
			else
			{
				m_CurrentRowOffset -= 1;
				m_CurrentRowOffset = m_CurrentRowOffset < 0 ? 0 : m_CurrentRowOffset;
			}
			m_Updated.store(true);
		}
	}
	//END REPLWindow_QueryDisplayer



	//BEGIN  REPLWindow
	REPLWindow::REPLWindow(MBRadio* AssociatedRadio)
	{
		m_AssociatedRadio = AssociatedRadio;
	}
	bool REPLWindow::Updated()
	{
		bool ReturnValue = m_Updated.load();
		if (m_ResultWindow != nullptr && m_ResultWindow->Updated())
		{
			ReturnValue = true;
		}
		return(ReturnValue);
	}
	void REPLWindow::SetActiveWindow(bool ActiveStatus)
	{
		if (m_IsActive != ActiveStatus)
		{
			m_Updated = true;
		}
		m_IsActive = ActiveStatus;
	}
	CursorInfo REPLWindow::GetCursorInfo() 
	{
		CursorInfo ReturnValue;
		ReturnValue.Hidden = false;
		int InputPosition = m_InputLineReciever.GetCursorPosition();
		int CommandHeight = (m_Height) / 3;
		
		int NumberOfLines = InputPosition / (m_Width - 2);
		ReturnValue.Position.ColumnIndex = 1+InputPosition % (m_Width - 2);
		ReturnValue.Position.RowIndex = CommandHeight - 2 - NumberOfLines;
		return(ReturnValue);
	}
	void REPLWindow::SetDimension(int Width, int Height)
	{
		m_Width = Width;
		m_Height = Height;
		if (m_ResultWindow != nullptr)
		{
			m_ResultWindow->SetDimension(Width,m_Height-(m_Height / 3));
		}
	}
	MBCLI::TerminalWindowBuffer REPLWindow::GetDisplay()
	{
		MBCLI::TerminalWindowBuffer ReturnValue(m_Width,m_Height);
		
		int CommandHeight = (m_Height ) / 3;
		MBCLI::TerminalWindowBuffer CommandBuffer(m_Width, CommandHeight);

		if (m_IsActive)
		{
			CommandBuffer.WriteBorder(CommandBuffer.Width, CommandBuffer.Height, 0, 0, MBCLI::TerminalColor::BrightGreen);
		}
		else 
		{
			CommandBuffer.WriteBorder(CommandBuffer.Width, CommandBuffer.Height, 0, 0, MBCLI::TerminalColor::White);
		}
		std::vector<MBUnicode::GraphemeCluster> const& CommandCharacters = m_InputLineReciever.GetLineBuffer();
		size_t CurrentRowIndex = CommandHeight - 2;
		size_t CurrentColumnIndex = 1;
		for (size_t i = 0; i < CommandCharacters.size(); i++)
		{
			if ((CurrentColumnIndex) % (m_Width-1) == 0)
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

		if (m_ResultWindow == nullptr)
		{
			MBCLI::TerminalWindowBuffer OutputBuffer = m_OutputBuffer.GetWindowBuffer(0, m_Width, m_Height - CommandHeight);
			ReturnValue.WriteBuffer(OutputBuffer, CommandHeight, 0);
		}
		else
		{
			ReturnValue.WriteBuffer(m_ResultWindow->GetDisplay(), CommandHeight, 0);
		}
		ReturnValue.WriteBuffer(CommandBuffer, 0, 0);

		m_Updated = false;
		return(ReturnValue);
	}

	MBParsing::JSONObject REPLWindow::p_GetSavedMBSiteUserAuthentication()
	{
		MBParsing::JSONObject ReturnValue = MBParsing::JSONObject(MBParsing::JSONObjectType::Aggregate);
		ReturnValue["VerificationType"] = "PasswordHash";
		ReturnValue["User"] = "";
		ReturnValue["Password"] = "";
		return(ReturnValue);
	}
	std::string REPLWindow::p_GetMBSiteURL()
	{
		return("https://127.0.0.1");
	}
	void REPLWindow::p_HandleQuerryCommand(std::string const& QuerryCommand)
	{
		size_t ParseOffest = 6;
		MBError ParseError = "";
		std::string QueryString = MBParsing::ParseQuotedString(QuerryCommand, ParseOffest, &ParseOffest, &ParseError);
		m_OutputBuffer.AddLine(QuerryCommand);
		if (!ParseError)
		{
			m_OutputBuffer.AddLine("> Error executing querry: " + ParseError.ErrorMessage);
			return;
		}
		else
		{
			MBParsing::JSONObject DirectiveToSend = MBParsing::JSONObject(MBParsing::JSONObjectType::Aggregate);
			DirectiveToSend["Directive"] = "QueryDatabase";
			DirectiveToSend["UserVerification"] = p_GetSavedMBSiteUserAuthentication();
			DirectiveToSend["DirectiveArguments"] = std::map<std::string, MBParsing::JSONObject>({ {"Query",QueryString} });

			std::string HostURL = p_GetMBSiteURL();
			
			MBSockets::HTTPClient ClientToUse;
			ClientToUse.ConnectToHost(HostURL);
			MBSockets::HTTPRequestBody BodyToSend;
			BodyToSend.DocumentType = MBMIME::MIMEType::json;
			BodyToSend.DocumentData = DirectiveToSend.ToString();
			MBSockets::HTTPRequestResponse Response = ClientToUse.SendRequest(MBSockets::HTTPRequestType::GET, "/DBGeneralAPI",BodyToSend);
			std::string ResponseBody;
			while (ClientToUse.IsConnected() && ClientToUse.DataIsAvailable())
			{
				const size_t ReadSize = 4096;
				std::string Buffer = std::string(4096, 0);
				size_t ReadBytes = ClientToUse.Read(Buffer.data(), ReadSize);
				Buffer.resize(ReadBytes);
				ResponseBody += Buffer;
				if (ReadBytes < ReadSize)
				{
					break;
				}
			}
			MBError ParseError;
			MBParsing::JSONObject DirectiveResponse = MBParsing::ParseJSONObject(ResponseBody, 0, nullptr, &ParseError);
			if (!ParseError)
			{
				m_OutputBuffer.AddLine("> Error executing querry: " + ParseError.ErrorMessage);
				return;
			}
			if (DirectiveResponse.GetAttribute("MBDBAPI_Status").GetStringData() != "ok")
			{
				m_OutputBuffer.AddLine("> Error executing querry: " + DirectiveResponse.GetAttribute("MBDBAPI_Status").GetStringData());
				return;
			}
			m_ResultWindow = std::unique_ptr<MBRadioWindow>(new REPLWindow_QueryDisplayer(DirectiveResponse.GetAttribute("DirectiveResponse"), m_Width, m_Height-(m_Height / 3)));
		}

	}
	void REPLWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		if (m_ResultWindow != nullptr)
		{
			if (InputToHandle.SpecialInput == MBCLI::SpecialKey::Esc)
			{
				m_ResultWindow = nullptr;
				m_Updated = true;
			}
			else
			{
				m_ResultWindow->HandleInput(InputToHandle);
			}
			return;
		}

		if (InputToHandle.CharacterInput != '\n' && InputToHandle.CharacterInput != "\r\n")
		{
			m_InputLineReciever.InsertInput(InputToHandle);
		}	 
		else
		{
			//evaluate command egentligen, men nu bara läser vi av karaktärerna
			std::string CurrentCommand = m_InputLineReciever.GetLineString();
			m_OutputBuffer.AddLine(m_InputLineReciever.GetLineString());
			m_InputLineReciever.Reset();
			if (CurrentCommand.substr(0, 5) == "play ")
			{
				std::string SongURL = CurrentCommand.substr(CurrentCommand.find(' ',0) + 1);
				if (CurrentCommand[5] >= 48 && CurrentCommand[5] <= 57)
				{
					int Index = -1;
					try
					{
						Index = std::stoi(SongURL);
					}
					catch(std::exception const& e)
					{
						return;
					}
					m_AssociatedRadio->SetCurrentSong(Index - 1);
					m_AssociatedRadio->PlaySong(m_AssociatedRadio->GetNextSong());
				}
				else
				{
					Song NewSong;
					NewSong.Artist = "";
					NewSong.SongName = "";
					NewSong.SongURI = std::move(SongURL);
					m_AssociatedRadio->PlaySong(std::move(NewSong));
				}
			}
			else if (CurrentCommand.substr(0, 4) == "add ")
			{
				std::string SongURL = CurrentCommand.substr(CurrentCommand.find(' ', 0) + 1);
				Song NewSong;
				NewSong.Artist = "";
				NewSong.SongName = "";
				NewSong.SongURI = std::move(SongURL);
				m_AssociatedRadio->AddSong(std::move(NewSong));
			}
			else if (CurrentCommand.substr(0, 5) == "query")
			{
				//hardcodat
				p_HandleQuerryCommand(CurrentCommand);
			}
			else if (CurrentCommand == "n")
			{
				Song NewSong = m_AssociatedRadio->GetNextSong();
				m_AssociatedRadio->PlaySong(NewSong);
			}
		}
		m_Updated = true;

	}
	//END REPLWindow

	//BEGIN PlayListWindow
	bool PlayListWindow::Updated()
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		bool ReturnValue = m_Updated;
		m_Updated = false;
		return(ReturnValue);
	}
	void PlayListWindow::SetActiveWindow(bool ActiveStatus)
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		m_ActiveWindow = ActiveStatus;
	}
	void PlayListWindow::SetDimension(int Width, int Height)
	{
		//
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		m_Width = Width;
		m_Height = Height;
	}
	MBCLI::TerminalWindowBuffer PlayListWindow::GetDisplay()
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		MBCLI::TerminalWindowBuffer ReturnValue(m_Width,m_Height);
		ReturnValue.WriteBorder(ReturnValue.Width, ReturnValue.Height, 0, 0, MBCLI::TerminalColor::BrightWhite);
		size_t FirstSongIndex = m_SongDisplayIndex;
		size_t MaxSongs = m_Height - 2;

		for (size_t i = FirstSongIndex; i < m_PlaylistSongs.size() && i-FirstSongIndex < MaxSongs; i++)
		{
			std::string StringToWrite = "";
			if (i == m_CurrentSongIndex)
			{
				StringToWrite += "> ";
			}
			StringToWrite += std::to_string(i) + " ";
			if (m_PlaylistSongs[i].SongName != "")
			{
				if (m_PlaylistSongs[i].Artist != "")
				{
					StringToWrite += m_PlaylistSongs[i].Artist + " - ";
				}
				StringToWrite += m_PlaylistSongs[i].SongName;
			}
			else
			{
				StringToWrite += m_PlaylistSongs[i].SongURI;
			}
			ReturnValue.WriteCharacters((m_Height-1)-(i  - FirstSongIndex + 1),1, StringToWrite);
		}
		return(ReturnValue);
	}
	void PlayListWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		//
	}
	void PlayListWindow::AddSong(Song SongToAdd,size_t SongPosition)
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		//p_u
		h_InferNameAndArtist(SongToAdd);
		auto IteratorPosition = m_PlaylistSongs.begin();
		if (SongPosition >= m_PlaylistSongs.size())
		{
			IteratorPosition = m_PlaylistSongs.end();
		}
		m_PlaylistSongs.insert(IteratorPosition, std::move(SongToAdd));
	}
	void PlayListWindow::SetShuffle(bool IsShuffle)
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		
	}
	Song PlayListWindow::GetNextSong()
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		if (!m_Shuffle)
		{
			m_CurrentSongIndex += 1;
			if (m_CurrentSongIndex >= m_PlaylistSongs.size())
			{
				return(Song());
			}
			return(m_PlaylistSongs[m_CurrentSongIndex]);
		}
		else
		{
			return(Song());
		}
	}
	void PlayListWindow::SetCurrentSong(size_t SongIndex)
	{
		std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
		//Semantiken för shuffle?
		m_CurrentSongIndex = SongIndex;
		
	}
	//END PlayListWindow

	//BEGIN SongWindow
	SongWindow::SongWindow(MBRadio* AssociatedRadio)
	{
		m_AssociatedRadio = AssociatedRadio;
		m_Playbacker = std::unique_ptr<SongPlaybacker>(new SongPlaybacker());
		m_UpdateThread = std::thread(&SongWindow::p_UpdateHandler, this);
	}
	bool SongWindow::Updated()
	{
		return(m_Updated.load());
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
	std::string SongWindow::p_TimeToString(float TimeToConvert)
	{
		std::string ReturnValue = "";
		if (TimeToConvert >= 0)
		{
			int Minutes = TimeToConvert / 60;
			int Seconds = std::fmod(TimeToConvert, 60);
			ReturnValue += std::to_string(Minutes);
			std::string SecondsString = std::to_string(Seconds);
			if (SecondsString.size() < 2)
			{
				SecondsString = '0' + SecondsString;
			}
			ReturnValue += ":" + SecondsString;
		}
		else if (TimeToConvert < 0)
		{
			if (TimeToConvert == -1)
			{
				return("--");
			}
			else if(TimeToConvert == -2)
			{
				return("?");
			}
		}
		return(ReturnValue);
	}
	MBCLI::TerminalWindowBuffer SongWindow::p_GetTimeBuffer()
	{
		std::string ElapsedString = p_TimeToString(m_CurrentSongTimeInSecs.load());
		std::string TotalTime = p_TimeToString(m_CurrentSongTotalTimeInSecs.load());
		std::string TotalString = ElapsedString + "/" + TotalTime;
		std::vector<MBUnicode::GraphemeCluster> Clusters;
		MBUnicode::GraphemeCluster::ParseGraphemeClusters(Clusters, TotalString.data(), TotalString.size(), 0);
		MBCLI::TerminalWindowBuffer ReturnValue = MBCLI::TerminalWindowBuffer(Clusters.size(), 1);
		ReturnValue.WriteCharacters(0, 0, Clusters);
		return(ReturnValue);
	}
	std::string h_MultiplyString(std::string const& StringToMultiply, size_t MultiplyCount)
	{
		std::string ReturnValue = StringToMultiply;
		for (size_t i = 1; i < MultiplyCount; i++)
		{
			ReturnValue += StringToMultiply;
		}
		return(ReturnValue);
	}
	MBCLI::TerminalWindowBuffer SongWindow::p_GetProgressBarString(float ElapsedTime, float TotalTime)
	{
		const size_t MaxTimeStringSize = 15;//inte helt självklar
		const float MarginPercent = 0.8;
		int TotalProgressBarSize = (m_Width - MaxTimeStringSize) * MarginPercent;
		if (TotalProgressBarSize <= 0)
		{
			return(MBCLI::TerminalWindowBuffer(0,0));
		}
		int CirclePosition = 0;
		MBCLI::TerminalWindowBuffer ReturnValue = MBCLI::TerminalWindowBuffer(TotalProgressBarSize,1);
		if (ElapsedTime < 0 || (TotalTime < 0 || TotalTime == 0))
		{
			CirclePosition = 0;
		}
		else
		{
			float PercentElapsedTime = ElapsedTime / TotalTime;
			int ElapsedCharacters = TotalProgressBarSize * (PercentElapsedTime);
			CirclePosition = std::max(ElapsedCharacters - 1,0);
		}
		int ProgressedCharacters = CirclePosition;
		int RemainingCharacters = TotalProgressBarSize - ProgressedCharacters-1;
		if (CirclePosition > 0)
		{
			ReturnValue.SetWriteColor(MBCLI::ANSITerminalColor::Green);
			ReturnValue.WriteCharacters(0,0,h_MultiplyString("\xE2\x94\x80", CirclePosition));
		}
		ReturnValue.SetWriteColor(MBCLI::ANSITerminalColor::White);
		ReturnValue.WriteCharacters(0, CirclePosition, "O");
		if (RemainingCharacters == 0 || RemainingCharacters == -1)
		{
			return(ReturnValue);
		}
		ReturnValue.WriteCharacters(0, CirclePosition + 1, h_MultiplyString("-",RemainingCharacters));
		return(ReturnValue);
	}
	MBCLI::TerminalWindowBuffer SongWindow::p_GetSongProgressBar()
	{
		MBCLI::TerminalWindowBuffer ReturnValue = MBCLI::TerminalWindowBuffer(m_Width-2, 1);
		//std::string ReturnValue = p_TimeToString(m_CurrentSongTimeInSecs) + "/" + p_TimeToString(m_CurrentSongTotalTimeInSecs)+" ";
		MBCLI::TerminalWindowBuffer TimePart = p_GetTimeBuffer();
		MBCLI::TerminalWindowBuffer ProgressPart = p_GetProgressBarString(m_CurrentSongTimeInSecs.load(), m_CurrentSongTotalTimeInSecs.load());
		int TotalSize = TimePart.Width + 1 + ProgressPart.Width;
		int ColumnOffset = ((m_Width - TotalSize) / 2)+1;
		ReturnValue.WriteBuffer(TimePart, 0, ColumnOffset);
		ReturnValue.WriteBuffer(ProgressPart, 0, ColumnOffset + 1 + TimePart.Width);
		return(ReturnValue);
	}
	MBCLI::TerminalWindowBuffer SongWindow::GetDisplay()
	{
		MBCLI::TerminalWindowBuffer ReturnValue(m_Width, m_Height);
		ReturnValue.WriteBorder(ReturnValue.Width, ReturnValue.Height, 0, 0, MBCLI::TerminalColor::BrightWhite);
		//ReturnValue.WriteCharacters((m_Height - 2), MBCLI::TextJustification::Middle, "No song playing");
		std::string SongInfoText = "No song playing";
		if (m_Playbacker->InputLoaded())
		{
			SongInfoText = m_CurrentSong.Artist + " - " + m_CurrentSong.SongName;
		}
		ReturnValue.WriteCharacters((m_Height - 2), MBCLI::TextJustification::Middle, SongInfoText);
		ReturnValue.WriteBuffer(p_GetSongProgressBar(),m_Height-3,1);
		m_Updated.store(false);
		return(ReturnValue);
	}
	void SongWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
	{
		//
	}
	void SongWindow::p_UpdateHandler()
	{
		while (m_Stopping.load() == false)
		{
			if (m_Playbacker->InputLoaded())
			{
				double NewElapsed = m_Playbacker->GetSongPosition();
				double NewTotalTime = m_Playbacker->GetSongDuration();
				if (NewElapsed != m_CurrentSongTimeInSecs.load() || NewTotalTime != m_CurrentSongTotalTimeInSecs.load())
				{
					m_Updated.store(true);
					m_CurrentSongTimeInSecs.store(m_Playbacker->GetSongPosition());
					m_CurrentSongTotalTimeInSecs.store(m_Playbacker->GetSongDuration());
					m_AssociatedRadio->Update();
				}

			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
	void SongWindow::Pause()
	{
		m_Playbacker->Pause();
	}
	void SongWindow::Play()
	{
		m_Playbacker->Play();
	}
	bool SongWindow::Paused()
	{
		return(m_Playbacker->Paused());
	}
	void SongWindow::PlaySong(Song SongToPlay)
	{
		m_CurrentSong = SongToPlay;
		//lägger till namn
		h_InferNameAndArtist(m_CurrentSong);
		m_Playbacker->SetInputSource(std::move(SongToPlay));
	}
	void SongWindow::Seek(double SecondOffset)
	{
		m_Playbacker->Seek(SecondOffset);
	}
	double SongWindow::GetSongDuration()
	{
		return(m_Playbacker->GetSongDuration());
	}
	double SongWindow::GetSongPosition()
	{
		return(m_Playbacker->GetSongPosition());
	}
	//END SongWindow

	//BEGIN AudioPlaybacker
	void SongPlaybacker::p_AudioThread()
	{
		//inneffektivt för mycket polling, men men
		std::unique_ptr<MBMedia::AudioStream> CurrentInputStream = nullptr;
		m_OutputDevice->Start();
		while (!m_Stopping.load())
		{
			if (m_SongRequested.load())
			{

				CurrentInputStream = nullptr;
				std::lock_guard<std::mutex> Lock(m_SongRequestMutex);
				m_SongRequested.store(false);
				std::lock_guard<std::mutex> FetcherLock(m_DataFetcherMutex);
				m_OutputDevice->Clear();
				m_AudioStreamIndex = -1;
				if (m_RequestedSong.SongURI == "")
				{
					CurrentInputStream = nullptr;
					m_SongDataFetcher = nullptr;
					continue;
				}
				m_SongDataFetcher = nullptr;
				MBPlay::URLStream URLParser(m_RequestedSong.SongURI);
				m_SongDataFetcher = std::unique_ptr<MBPlay::OctetStreamDataFetcher>(new MBPlay::OctetStreamDataFetcher(URLParser.GetSearchableInputStream(), 5));
				m_SongDataFetcher->WaitForStreamInfo();
				for (size_t i = 0; i < m_SongDataFetcher->AvailableStreams(); i++)
				{
					if (m_SongDataFetcher->GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
					{
						m_AudioStreamIndex = i;
					}
					else
					{
						m_SongDataFetcher->DiscardStream(i);
					}
				}
				if(m_AudioStreamIndex != -1)
				{
					CurrentInputStream = std::unique_ptr<MBMedia::AudioStream>(new MBMedia::AudioInputConverter(m_SongDataFetcher->GetAudioStream(m_AudioStreamIndex), m_OutputDevice->GetOutputParameters()));
				}
			}

			if (m_SeekRequested.load())
			{
				//bara denna tråd jkan ändra AudioStreamIndex
				std::lock_guard<std::mutex> Lock(m_SeekRequestMutex);
				m_OutputDevice->Clear();
				m_SeekRequested.store(false);
				if (m_SongDataFetcher != nullptr)
				{
					m_SongDataFetcher->SeekPosition(m_AudioStreamIndex, m_SeekPosition);
				}
			}
			if (m_SongDataFetcher == nullptr)
			{
				continue;
			}
			if (m_OutputDevice->GetCurrentOutputBufferSize() < 4096 && m_Play.load())
			{
				//eftersom det bara är den här tråden som kan ändra på DataFetcher, och alla andra trådar bara läser från tråden behöver vi inte oroa oss för race conditions
				if (CurrentInputStream != nullptr)
				{
					MBMedia::AudioBuffer TempBuffer = MBMedia::AudioBuffer(m_OutputDevice->GetOutputParameters(), 4096);
					size_t RecievedSamples = CurrentInputStream->GetNextSamples(TempBuffer.GetData(), 4096, 0);
					m_OutputDevice->InsertAudioData(TempBuffer.GetData(), RecievedSamples);
				}
				m_OutputDevice->UpdateBuffer(4096);
			}	
		}
	}
	SongPlaybacker::SongPlaybacker()
	{
		MBAE::Init();
		MBAE::AudioDeviceManager DeviceManager;
		DeviceManager.Initialize();
		m_OutputDevice = DeviceManager.GetDefaultDevice();


		m_AudioThread = std::thread(&SongPlaybacker::p_AudioThread, this);
	}
	void SongPlaybacker::Pause()
	{
		m_Play.store(false);
	}
	void SongPlaybacker::Play()
	{
		m_Play.store(true);
	}
	bool SongPlaybacker::Paused()
	{
		return(!m_Play.load());
	}
	void SongPlaybacker::SetInputSource(Song SongToPlay)
	{
		std::lock_guard<std::mutex> SongRequestMutex(m_SongRequestMutex);
		m_SongRequested.store(true);
		m_RequestedSong = std::move(SongToPlay);


		//m_SongDataFetcher = std::unique_ptr<MBPlay::OctetStreamDataFetcher>(new MBPlay::OctetStreamDataFetcher(std::unique_ptr<MBUtility::MBSearchableInputStream>(new MBSockets::HTTPFileStream(SongToPlay.SongURI)), 5));
		//m_SongDataFetcher->WaitForStreamInfo();
		//m_AudioStreamIndex = -1;
		//for (size_t i = 0; i < m_SongDataFetcher->AvailableStreams(); i++)
		//{
		//	if (m_SongDataFetcher->GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
		//	{
		//		m_AudioStreamIndex = i;
		//		break;
		//	}
		//}
		//m_CurrentSong = SongToPlay;
	}
	bool SongPlaybacker::InputLoaded()
	{
		std::lock_guard<std::mutex> DataFetcherLock(m_DataFetcherMutex);
		if (m_SongDataFetcher == nullptr)
		{
			return false;
		}
		return(m_SongDataFetcher->AvailableStreams() != -1);
	}
	void SongPlaybacker::Seek(double SecondOffset)
	{
		std::lock_guard<std::mutex> SongRequestMutex(m_SongRequestMutex);
		std::lock_guard<std::mutex> DataFetcherLock(m_DataFetcherMutex);
		if (m_SongDataFetcher == nullptr)
		{
			return;
		}
		MBMedia::TimeBase StreamTimebase = m_SongDataFetcher->GetStreamInfo(m_AudioStreamIndex).StreamTimebase;
		
		int64_t CurrentPosition = m_SongDataFetcher->GetStreamTimestamp(m_AudioStreamIndex);
		int64_t StreamPositionOffset = (SecondOffset * StreamTimebase.den) / StreamTimebase.num;
		CurrentPosition += StreamPositionOffset;
		CurrentPosition = std::max(CurrentPosition, int64_t(0));
		m_SeekRequested.store(true);
		m_SeekPosition = CurrentPosition;
		//m_SongDataFetcher->SeekPosition(m_AudioStreamIndex, StreamPosition);
	}
	double SongPlaybacker::GetSongDuration()
	{
		std::lock_guard<std::mutex> Lock(m_DataFetcherMutex);
		if (m_SongDataFetcher == nullptr)
		{
			return -1;
		}
		if (m_SongDataFetcher->AvailableStreams() == -1 || m_SongDataFetcher->AvailableStreams() == 0)
		{
			return(-1);
		}
		MBMedia::TimeBase StreamTimebase = m_SongDataFetcher->GetStreamInfo(m_AudioStreamIndex).StreamTimebase;
		double ReturnValue = (m_SongDataFetcher->GetStreamInfo(m_AudioStreamIndex).StreamDuration * StreamTimebase.num) / double(StreamTimebase.den);
		return(ReturnValue);
	}
	double SongPlaybacker::GetSongPosition()
	{
		std::lock_guard<std::mutex> Lock(m_DataFetcherMutex);
		if (m_SongDataFetcher == nullptr)
		{
			return -1;
		}
		if (m_SongDataFetcher->AvailableStreams() == -1 || m_SongDataFetcher->AvailableStreams() == 0)
		{
			return(-1);
		}
		MBMedia::TimeBase StreamTimebase = m_SongDataFetcher->GetStreamInfo(m_AudioStreamIndex).StreamTimebase;
		double ReturnValue = (m_SongDataFetcher->GetStreamTimestamp(m_AudioStreamIndex) * StreamTimebase.num) / double(StreamTimebase.den);
		return(ReturnValue);
	}
	SongPlaybacker::~SongPlaybacker()
	{
		m_Stopping.store(true);
		m_AudioThread.join();
	}
	//END AudioPlaybacker


	//BEGIN MBRadio
	void MBRadio::PlaySong(Song SongToplay)
	{
		m_SongWindow.PlaySong(SongToplay);
	}
	MBRadio::MBRadio(MBCLI::MBTerminal* TerminalToUse)
		: m_REPLWindow(this),m_SongWindow(this)
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
		//MBCLI::ConsoleInput TestTest = m_AssociatedTerminal->ReadNextInput();
		while (true)
		{
			MBCLI::ConsoleInput NewInput = m_AssociatedTerminal->ReadNextInput();
			if (NewInput.KeyModifiers & uint64_t(MBCLI::KeyModifier::CTRL))
			{
				if (NewInput.CharacterInput == MBCLI::CTRL('p'))
				{
					if (m_SongWindow.Paused())
					{
						m_SongWindow.Play();
					}
					else
					{
						m_SongWindow.Pause();
					}
				}
				else if (NewInput.SpecialInput == MBCLI::SpecialKey::Left)
				{
					m_SongWindow.Seek(-5);
				}
				else if (NewInput.SpecialInput == MBCLI::SpecialKey::Right)
				{
					m_SongWindow.Seek(5);
				}
				else if (NewInput.CharacterInput == MBCLI::CTRL('r'))
				{
					m_SongWindow.Seek(-100000);
				}
			}
			else if (m_ActiveWindow == MBRadioWindowType::REPL)
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
	void MBRadio::Update()
	{
		p_UpdateWindow();
	}
	void MBRadio::AddSong(Song SongToAdd, size_t SongPosition)
	{
		return(m_PlaylistWindow.AddSong(SongToAdd, SongPosition));
	}
	void MBRadio::SetShuffle(bool IsShuffle)
	{
		m_PlaylistWindow.SetShuffle(IsShuffle);
	}
	Song MBRadio::GetNextSong()
	{
		return(m_PlaylistWindow.GetNextSong());
	}
	void MBRadio::SetCurrentSong(size_t SongIndex)
	{
		m_PlaylistWindow.SetCurrentSong(SongIndex);
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

		CursorInfo NewCursorInfo;
		if (m_ActiveWindow == MBRadioWindowType::REPL)
		{
			NewCursorInfo = m_REPLWindow.GetCursorInfo();
			NewCursorInfo.Position.RowIndex += SongHeight;
		}
		else if (m_ActiveWindow == MBRadioWindowType::Song)
		{
			NewCursorInfo = m_SongWindow.GetCursorInfo();

		}
		else if (m_ActiveWindow == MBRadioWindowType::Playlist)
		{
			NewCursorInfo = m_PlaylistWindow.GetCursorInfo();
			NewCursorInfo.Position.RowIndex += SongHeight;
			NewCursorInfo.Position.ColumnIndex += REPLWidth;
		}
		if (NewCursorInfo.Hidden)
		{
			m_AssociatedTerminal->HideCursor();
		}
		else
		{
			m_AssociatedTerminal->ShowCursor();
		}
		m_AssociatedTerminal->PrintWindowBuffer(m_WindowBuffer, 0, 0);
		m_AssociatedTerminal->SetCursorPosition(NewCursorInfo.Position);
		m_AssociatedTerminal->Refresh();


	}
	//END MBRadio

}