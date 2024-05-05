#include "MBRadio.h"
#include "MBCLI/MBCLI.h"
#include <mutex>
#include <MrBoboSockets/MrBoboSockets.h>
#include <MBAudioEngine/MBAudioEngine.h>
#include <MBParsing/MBParsing.h>

#include <MrBoboSockets/MrBoboSockets.h>

#include <algorithm>
#include <random>
//#include <DiscordSDK/cpp/discord.h>

#include <MrPostOGet/MrPostOGet.h>
#include <MBSystem/MBSystem.h>
namespace MBRadio
{
    void h_InferNameAndArtist(Song& SongToModify)
    {
        if (SongToModify.Artist != "" || SongToModify.SongName != "")
        {
            return;
        }
        MBStreaming::URLStream URLParser(SongToModify.SongURI);
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
    REPLWindow_QueryDisplayer::REPLWindow_QueryDisplayer(std::vector<std::vector<std::string>> QueryDirectiveResponse,int Width,int Height)
    {
        m_Height = Height;
        m_Width = Width;
        try
        {
            m_ResultTable = MBCLI::TableCreator(QueryDirectiveResponse, Width);
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
    MBCLI::CursorInfo REPLWindow_QueryDisplayer::GetCursorInfo()
    {
        MBCLI::CursorInfo ReturnValue;
        ReturnValue.Hidden = true;
        ReturnValue.Position = { 0,0 };
        return(ReturnValue);
    }
    void REPLWindow_QueryDisplayer::SetFocus(bool ActiveStatus)
    {
        //
    }
    void REPLWindow_QueryDisplayer::SetDimensions(MBCLI::Dimensions Dims)
    {
        m_Updated.store(true);
        std::lock_guard<std::mutex> Lock(m_InternalsMutex);
        m_Width = Dims.Width;
        m_Height = Dims.Height;
        m_ResultTable.SetWidth(m_Width);
    }
    MBCLI::TerminalWindowBuffer REPLWindow_QueryDisplayer::GetBuffer()
    {
        std::lock_guard<std::mutex> Lock(m_InternalsMutex);
        return(m_ResultTable.GetWindowBuffer(m_CurrentRowOffset, m_CurrentCharacterOffset, m_Height));
    }
    void REPLWindow_QueryDisplayer::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
    {
        std::lock_guard<std::mutex> Lock(m_InternalsMutex);
        //bool CTRLHeld = InputToHandle.KeyModifiers & uint64_t(MBCLI::KeyModifier::SHIFT);
        if ( InputToHandle.CharacterInput == "k")
        {
            m_CurrentRowOffset -= 1;
            m_CurrentCharacterOffset = 0;
            if (m_CurrentRowOffset < 0)
            {
                m_CurrentRowOffset = 0;
            }
            m_Updated.store(true);
        }
        if (InputToHandle.CharacterInput == "K")
        {

            m_CurrentCharacterOffset -= 1;
            m_CurrentCharacterOffset = m_CurrentCharacterOffset < 0 ? 0 : m_CurrentCharacterOffset;
            m_Updated.store(true);
        }
        if (InputToHandle.CharacterInput == "j")
        {
            m_CurrentRowOffset += 1;
            m_CurrentCharacterOffset = 0;
            if (m_CurrentRowOffset >= m_ResultTable.GetRowCount())
            {
                m_CurrentRowOffset = m_ResultTable.GetRowCount();
            }
        }
        if (InputToHandle.CharacterInput == "J")
        {
            m_CurrentCharacterOffset += 1;
        }
    }
    //END REPLWindow_QueryDisplayer



    //BEGIN  REPLWindow
    REPLWindow::REPLWindow(MBRadio* AssociatedRadio)
    {
        m_AssociatedRadio = AssociatedRadio;
        m_Repl.SetOnEnterFunc([&](std::string const& String){p_HandleCommand(String);});
        m_Repl.AddCompletionFunc(p_Completions);
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
    void REPLWindow::SetFocus(bool ActiveStatus)
    {
        if (m_IsActive != ActiveStatus)
        {
            m_Updated = true;
        }
        m_IsActive = ActiveStatus;
    }
    MBCLI::CursorInfo REPLWindow::GetCursorInfo() 
    {
        MBCLI::CursorInfo ReturnValue;
        if (m_ResultWindow == nullptr)
        {
            ReturnValue.Hidden = false;
            int InputPosition = m_Repl.GetCursorInfo().Position.ColumnIndex;
            int CommandHeight = (m_Height) / 3;

            int NumberOfLines = InputPosition / (m_Width - 2);
            ReturnValue.Position.ColumnIndex = 1 + InputPosition % (m_Width - 2);
            ReturnValue.Position.RowIndex = CommandHeight - 2 - NumberOfLines;
        }
        else
        {
            return(m_ResultWindow->GetCursorInfo());
        }
        return(ReturnValue);
    }
    void REPLWindow::SetDimensions(MBCLI::Dimensions Dims)
    {
        m_Width = Dims.Width;
        m_Height = Dims.Height;
        if (m_ResultWindow != nullptr)
        {
            MBCLI::Dimensions NewDims;
            NewDims.Width = m_Width;
            NewDims.Height = m_Height-(m_Height / 3);
            m_ResultWindow->SetDimensions(NewDims);
        }
    }
    MBCLI::TerminalWindowBuffer REPLWindow::GetBuffer()
    {
        MBCLI::TerminalWindowBuffer ReturnValue(m_Width,m_Height);
        
        int CommandHeight = (m_Height ) / 3;
        MBCLI::TerminalWindowBuffer CommandBuffer(m_Width, CommandHeight);

        if (m_IsActive && m_ResultWindow == nullptr)
        {
            CommandBuffer.WriteBorder(CommandBuffer.GetWidth(), CommandBuffer.GetHeight(), 0, 0, MBCLI::ANSITerminalColor::BrightGreen);
        }
        else 
        {
            CommandBuffer.WriteBorder(CommandBuffer.GetWidth(), CommandBuffer.GetHeight(), 0, 0, MBCLI::ANSITerminalColor::White);
        }
        std::vector<MBUnicode::GraphemeCluster> const& CommandCharacters = m_Repl.GetLineBuffer();
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
            //CommandBuffer.BufferCharacters[CurrentRowIndex][CurrentColumnIndex].Character = CommandCharacters[i];
            MBCLI::TerminalCharacter CharacterToWrite;
            CharacterToWrite.Character = CommandCharacters[i];
            CommandBuffer.WriteCharacters(CurrentRowIndex,CurrentColumnIndex,&CharacterToWrite,&CharacterToWrite+1);
            CurrentColumnIndex++;
        }

        if (m_ResultWindow == nullptr)
        {
            MBCLI::TerminalWindowBuffer OutputBuffer = m_OutputBuffer.GetWindowBuffer(0, m_Width, m_Height - CommandHeight);
            ReturnValue.WriteBuffer(OutputBuffer, CommandHeight, 0);
        }
        else
        {
            ReturnValue.WriteBuffer(m_ResultWindow->GetBuffer(), CommandHeight, 0);
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
        //return("https://mrboboget.se");
        return m_AssociatedRadio->GetConfig().Server.Address;
        //return("https://192.168.0.123");
    }
    std::vector<std::vector<std::string>> REPLWindow::p_GetQuerryResponse(std::string const& Host, std::string const& Querry, MBParsing::JSONObject VerificationData,MBError* OutError)
    {
        std::vector<std::vector<std::string>> ReturnValue;
        MBParsing::JSONObject DirectiveToSend = MBParsing::JSONObject(MBParsing::JSONObjectType::Aggregate);
        DirectiveToSend["UserVerification"] = std::move(VerificationData);
        DirectiveToSend["Directive"] = "QueryDatabase";
        DirectiveToSend["DirectiveArguments"] = std::map<std::string, MBParsing::JSONObject>({ {"Query",Querry} });


        MrPostOGet::HTTPClient ClientToUse;
        //TODO hardcoded af, madafacking xd
        ClientToUse.ConnectToHost(Host);
        MrPostOGet::HTTPRequestBody BodyToSend;
        BodyToSend.DocumentType = MBMIME::MIMEType::json;
        BodyToSend.DocumentData = DirectiveToSend.ToString();
        MrPostOGet::HTTPRequestResponse Response = ClientToUse.SendRequest(MrPostOGet::HTTPRequestType::GET, "/DBGeneralAPI", BodyToSend);
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
            if (OutError != nullptr)
            {
                *OutError = std::move(ParseError);
            }
            return(ReturnValue);
        }
        try
        {
            if (DirectiveResponse.GetAttribute("MBDBAPI_Status").GetStringData() != "ok")
            {
                if (OutError != nullptr)
                {
                    *OutError = false;
                    OutError->ErrorMessage = "Error on executing querry: " + DirectiveResponse.GetAttribute("MBDBAPI_Status").GetStringData();
                    return(ReturnValue);
                }
            }
            std::vector<MBParsing::JSONObject> const& Results = DirectiveResponse.GetAttribute("DirectiveResponse").GetAttribute("QueryResult").GetArrayData();
            for (size_t i = 0; i < Results.size(); i++)
            {
                std::vector<MBParsing::JSONObject> const& CurrentRow = Results[i].GetArrayData();
                std::vector<std::string> NewRowData;
                for (size_t j = 0; j < CurrentRow.size(); j++)
                {
                    std::string NewString = CurrentRow[j].ToString();
                    if (CurrentRow[j].GetType() == MBParsing::JSONObjectType::String)
                    {
                        NewString = NewString.substr(1, NewString.size() - 2);
                    }
                    NewRowData.push_back(NewString);
                }
                ReturnValue.push_back(std::move(NewRowData));
            }
        }
        catch (std::exception const& e)
        {
            if (OutError != nullptr)
            {
                *OutError = false;
                OutError->ErrorMessage = "Error parsing querry directive response: " + std::string(e.what());
            }
            ReturnValue.clear();
            return(ReturnValue);
        }
        //m_ResultTable = MBCLI::TableCreator(TableContent, m_Width);
        return(ReturnValue);
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
            MBParsing::JSONObject VerificationData = p_GetSavedMBSiteUserAuthentication();
            std::string Host = p_GetMBSiteURL();
            MBError QuerryError = true;
            std::vector<std::vector<std::string>> QuerryData = p_GetQuerryResponse(Host, QueryString,std::move(VerificationData), &QuerryError);
            if (!QuerryError)
            {
                m_OutputBuffer.AddLine("> " + QuerryError.ErrorMessage);
            }
            else
            {
                m_ResultWindow = std::unique_ptr<MBCLI::Window>(new REPLWindow_QueryDisplayer(std::move(QuerryData), m_Width, m_Height - (m_Height / 3)));
            }
        }

    }
    void REPLWindow::p_HandleAddCommand(std::string const& AddCommand)
    {
        std::vector<std::string> CommandTokens = {};
        size_t ParseOffset = 4;//play
        MBError ParseError = true;
        MBParsing::SkipWhitespace(AddCommand, ParseOffset, &ParseOffset);
        while (ParseOffset < AddCommand.size() && ParseError)
        {
            if (AddCommand[ParseOffset] == '"')
            {
                CommandTokens.push_back(MBParsing::ParseQuotedString(AddCommand, ParseOffset, &ParseOffset, &ParseError));
            }
            else
            {
                size_t EndPos = std::min(AddCommand.find(' ', ParseOffset), AddCommand.size());
                CommandTokens.push_back(AddCommand.substr(ParseOffset, EndPos-ParseOffset));
                ParseOffset = EndPos;
            }
            MBParsing::SkipWhitespace(AddCommand, ParseOffset, &ParseOffset);
        }
        if (CommandTokens.size() == 1)
        {
            Song NewSong;
            NewSong.Artist = "";
            NewSong.SongName = "";
            NewSong.SongURI = std::move(CommandTokens[0]);
            m_AssociatedRadio->AddSong(std::move(NewSong));
        }
        else if (CommandTokens[0] == "query")
        {
            if (CommandTokens.size() != 3)
            {
                m_OutputBuffer.AddLine("> Error parsing add command: query needs exactly 2 arguments");
                return;
            }
            else
            {
                try
                {
                    std::string Host = p_GetMBSiteURL();
                    MBParsing::JSONObject UserAuthentication = p_GetSavedMBSiteUserAuthentication();
                    std::string QuerryString = CommandTokens[1];
                    int ColumnOffset = std::stoi(CommandTokens[2]);
                    if (ColumnOffset < 0)
                    {
                        m_OutputBuffer.AddLine("> Invalid column offset: Columnoffset has to be 0 or greater");
                        return;
                    }
                    MBError QuerryError = true;
                    std::vector<std::vector<std::string>> QueryResult = p_GetQuerryResponse(Host, QuerryString,std::move(UserAuthentication),&QuerryError);
                    if (!QuerryError)
                    {
                        m_OutputBuffer.AddLine("> Error executing querry: " + QuerryError.ErrorMessage);
                        return;
                    }
                    if (QueryResult.size() > 0)
                    {
                        if (ColumnOffset >= QueryResult[0].size())
                        {
                            m_OutputBuffer.AddLine("> Error execurting query: Column offset to large");
                            return;
                        }
                        for (size_t i = 0; i < QueryResult.size(); i++)
                        {
                            std::string TableURI = QueryResult[i][ColumnOffset];
                            std::string TotalURI = Host + "/DB/" + TableURI;
                            Song NewSong;
                            NewSong.Artist = "";
                            NewSong.SongName = "";
                            NewSong.SongURI = std::move(TotalURI);
                            m_AssociatedRadio->AddSong(std::move(NewSong));
                        }
                    }
                }
                catch (std::exception const& e)
                {
                    m_OutputBuffer.AddLine("> Error parsing column index: "+std::string(e.what()));
                    return;
                }
                
            }
        }
    }
    void REPLWindow::HandleInput(MBCLI::ConsoleInput const& InputToHandle)
    {
        if (m_ResultWindow != nullptr)
        {
            if (InputToHandle.CharacterInput == '\x1b')
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
        m_Updated = true;
        m_Repl.HandleInput(InputToHandle);
    }
    void REPLWindow::p_HandleCommand(std::string const& CurrentCommand)
    {
        //evaluate command egentligen, men nu bara läser vi av karaktärerna
        m_OutputBuffer.AddLine(CurrentCommand);
        if(CurrentCommand == "play")
        {
            m_AssociatedRadio->SetPause(false);
        }
        else if (CurrentCommand.substr(0, 5) == "play ")
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
                m_AssociatedRadio->SetCurrentSong(Index);
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
            p_HandleAddCommand(CurrentCommand);
        }
        else if (CurrentCommand.substr(0, 5) == "query")
        {
            //hardcodat
            p_HandleQuerryCommand(CurrentCommand);
        }
        else if (CurrentCommand.substr(0, 7) == "shuffle")
        {
            m_AssociatedRadio->SetShuffle(true);
        }
        else if (CurrentCommand.substr(0, 9) == "noshuffle")
        {
            m_AssociatedRadio->SetShuffle(false);
        }
        else if (CurrentCommand.substr(0, 5) == "clear")
        {
            m_AssociatedRadio->ClearSongs();
        }
        else if (CurrentCommand.substr(0, 5) == "pause")
        {
            m_AssociatedRadio->SetPause(!m_AssociatedRadio->GetPause());
        }
        else if (CurrentCommand.substr(0, 6) == "scroll")
        {
            int ScrollIndex = 0;
            try
            {
                size_t FirstSpace = CurrentCommand.find(' ');
                if (FirstSpace == CurrentCommand.npos)
                {
                    m_OutputBuffer.AddLine("> Error in scroll command: scroll requires atleast 1 argumnet");
                    return;
                }
                ScrollIndex = std::stoi(CurrentCommand.substr(FirstSpace+1));
            }
            catch(std::exception const e)
            {
                m_OutputBuffer.AddLine("> Error in parsing scroll index: "+std::string(e.what()));
                return;
            }
            m_AssociatedRadio->Scroll(ScrollIndex);
        }
        else if (CurrentCommand.substr(0, 6) == "remove" || CurrentCommand[0] == 'r')
        {
            std::vector<std::string> Arguments = MBUtility::Split(CurrentCommand, " ");
            if (Arguments.size() < 2)
            {
                m_OutputBuffer.AddLine("> Error in remove command: remove requires atleast 1 argument");
                return;
            }
            int SongIndex = 0;
            try
            {
                SongIndex = std::stoi(Arguments[1]);
            }
            catch (std::exception const& e)
            {
                m_OutputBuffer.AddLine("> Error in parsing song index");
                return;
            }
            if (SongIndex < 0)
            {
                m_OutputBuffer.AddLine("> Error in remove: song index can't be negative");
            }
            m_AssociatedRadio->RemoveSong(SongIndex);

        }
        else if (CurrentCommand == "n")
        {
            Song NewSong = m_AssociatedRadio->GetNextSong();
            m_AssociatedRadio->PlaySong(NewSong);
        }
    }
    std::vector<std::string> REPLWindow::p_Completions(MBTUI::REPL_Line const& Line)
    {
        std::vector<std::string> ReturnValue;
        if(Line.TokenIndex == 0)
        {
            ReturnValue = {"play","add","shuffle","noshuffle","clear","pause","scroll","remove"};
        }
        else if(Line.TokenIndex == 1)
        {
            if(Line.Tokens[0] == "add")
            {
                ReturnValue = {"query"};
            }
        }
        return ReturnValue;
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
    void PlayListWindow::SetFocus(bool ActiveStatus)
    {
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        m_ActiveWindow = ActiveStatus;
    }
    void PlayListWindow::SetDimensions(MBCLI::Dimensions Dims)
    {
        //
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        m_Width = Dims.Width;
        m_Height = Dims.Height;
        m_Updated = true;
    }
    MBCLI::TerminalWindowBuffer PlayListWindow::GetBuffer()
    {
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        MBCLI::TerminalWindowBuffer ReturnValue(m_Width,m_Height);
        ReturnValue.WriteBorder(ReturnValue.GetWidth(), ReturnValue.GetHeight(), 0, 0, MBCLI::ANSITerminalColor::BrightWhite);        
        int MaxSongs = m_Height - 2;
        int FirstSongIndex = m_SongDisplayIndex >= 0 ? m_SongDisplayIndex : 0;
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
    void PlayListWindow::RemoveSong(size_t SongIndex)
    {
        if (SongIndex != -1)
        {
            if (SongIndex >= m_PlaylistSongs.size())
            {
                return;
            }
            m_PlaylistSongs.erase(m_PlaylistSongs.begin() + SongIndex);
            if (m_Shuffle)
            {
                auto const& IteratorPosition = std::find(m_ShuffleIndexes.begin(), m_ShuffleIndexes.end(), SongIndex);
                m_ShuffleIndexes.erase(IteratorPosition);
                for (size_t i = 0; i < m_ShuffleIndexes.size(); i++)
                {
                    if (m_ShuffleIndexes[i] > SongIndex)
                    {
                        m_ShuffleIndexes[i] -= 1;
                    }
                }
            }
        }
        m_Updated = true;
    }
    void PlayListWindow::ClearSongs()
    {
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        m_Updated = true;
        m_CurrentSongIndex = -1;
        m_SongDisplayIndex = -1;
        m_PlaylistSongs.clear();
        m_ShuffleIndexes.clear();
    }

    void PlayListWindow::Scroll(int ScrollIndex)
    {
        if (ScrollIndex < 0)
        {
            ScrollIndex = 0;
        }
        if(m_SongDisplayIndex != ScrollIndex)
        {
            m_Updated = true;   
        }
        m_SongDisplayIndex = ScrollIndex;
    }
    int PlayListWindow::GetCurrentDisplayIndex() const
    {
        return(m_SongDisplayIndex);
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
        if (m_Shuffle)
        {
            size_t SongIndex = SongPosition != -1 ? SongPosition : m_PlaylistSongs.size()-1;
            m_ShuffleIndexes.push_back(SongIndex);
        }
        m_Updated = true;
    }
    void PlayListWindow::SetShuffle(bool IsShuffle)
    {
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        std::vector<size_t> NewShuffleIndexes;
        if (IsShuffle)
        {
            NewShuffleIndexes.reserve(m_PlaylistSongs.size());
            for (size_t i = 0; i < m_PlaylistSongs.size(); i++)
            {
                NewShuffleIndexes.push_back(i);
            }
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(NewShuffleIndexes.begin(), NewShuffleIndexes.end(), g);
            m_ShuffleIndexes = std::move(NewShuffleIndexes);
        }
        m_Shuffle = IsShuffle;
    }
    Song PlayListWindow::GetNextSong()
    {
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        Song ReturnValue;
        m_Updated = true;
        if (!m_Shuffle)
        {
            m_CurrentSongIndex += 1;
            if (m_CurrentSongIndex >= m_PlaylistSongs.size())
            {
                ReturnValue = Song();
            }
            else
            {
                ReturnValue = m_PlaylistSongs[m_CurrentSongIndex];
            }
        }
        else
        {
            if (m_ShuffleIndexes.size() > 0)
            {
                m_CurrentSongIndex = m_ShuffleIndexes.back();
                ReturnValue = m_PlaylistSongs[m_ShuffleIndexes.back()];
                m_ShuffleIndexes.pop_back();
            }
        }
        int MaxSongs = m_Height - 2;
        if (std::abs(m_CurrentSongIndex - m_SongDisplayIndex) > MaxSongs || (m_CurrentSongIndex >= 0 && m_CurrentSongIndex < m_SongDisplayIndex))
        {
            m_SongDisplayIndex = m_CurrentSongIndex;
        }
        return(ReturnValue);
    }
    void PlayListWindow::SetNextSong(size_t SongIndex)
    {
        std::lock_guard<std::mutex> InternalsLock(m_InternalsMutex);
        //Semantiken för shuffle?
        if (m_Shuffle)
        {
            m_ShuffleIndexes.push_back(SongIndex);
        }
        else
        {
            m_CurrentSongIndex = SongIndex - 1;
        }
    }
    //END PlayListWindow

    //BEGIN SongWindow
#ifdef MBRADIO_DISCORDINTEGRATION
    struct DiscordImpl
    {
        Song CurrentSong;
        discord::Activity Activity;
        discord::Core* Core;
    };
    void SongWindow::p_FreeDiscordImpl(void*)
    {
        //lowkey inget man kan göra...

    }
#endif // 


    SongWindow::SongWindow(MBRadio* AssociatedRadio)
    {
        m_AssociatedRadio = AssociatedRadio;
        m_Playbacker = std::unique_ptr<SongPlaybacker>(new SongPlaybacker());
#ifdef MBRADIO_DISCORDINTEGRATION
        m_DiscordImpl = std::unique_ptr<void, void(*)(void*)>(new DiscordImpl(), p_FreeDiscordImpl);
        DiscordImpl* Impl = (DiscordImpl*)m_DiscordImpl.get();
        auto result = discord::Core::Create(958468397505576971, DiscordCreateFlags_NoRequireDiscord, &Impl->Core);
        Impl->Activity.SetType(discord::ActivityType::Listening);

        uint64_t CurrentTime = std::time(nullptr);
        Impl->Activity.GetTimestamps().SetStart(CurrentTime);
        Impl->Activity.SetState("Idling");
        Impl->Activity.SetDetails("");
        Impl->Activity.GetAssets().SetLargeImage("gnutophat");

        Impl->Core->ActivityManager().UpdateActivity(Impl->Activity, [](discord::Result result) {

        });
#endif // 
        m_UpdateThread = std::thread(&SongWindow::p_UpdateHandler, this);

    }
    bool SongWindow::Updated()
    {
        return(m_Updated.load());
    }
    void SongWindow::SetFocus(bool ActiveStatus)
    {
        //
    }
    void SongWindow::SetDimensions(MBCLI::Dimensions Dims)
    {
        m_Width = Dims.Width;
        m_Height = Dims.Height;
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
        if(RemainingCharacters < 0)
        {
            RemainingCharacters = 0;   
        }
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
        int TotalSize = TimePart.GetWidth() + 1 + ProgressPart.GetWidth();
        int ColumnOffset = ((m_Width - TotalSize) / 2)+1;
        ReturnValue.WriteBuffer(TimePart, 0, ColumnOffset);
        ReturnValue.WriteBuffer(ProgressPart, 0, ColumnOffset + 1 + TimePart.GetWidth());
        return(ReturnValue);
    }
    MBCLI::TerminalWindowBuffer SongWindow::GetBuffer()
    {
        MBCLI::TerminalWindowBuffer ReturnValue(m_Width, m_Height);
        ReturnValue.WriteBorder(ReturnValue.GetWidth(), ReturnValue.GetHeight(), 0, 0, MBCLI::ANSITerminalColor::BrightWhite);
        //ReturnValue.WriteCharacters((m_Height - 2), MBCLI::TextJustification::Middle, "No song playing");
        std::string SongInfoText = "No song playing";
        if (m_Playbacker->InputLoaded() && m_Playbacker->InputAvailable())
        {
            SongInfoText = m_CurrentSong.Artist + " - " + m_CurrentSong.SongName;
            if (m_Playbacker->Paused())
            {
                SongInfoText = "(paused) " + SongInfoText;
            }
        }
        if (m_Playbacker->InputLoaded() && !m_Playbacker->InputAvailable())
        {
            SongInfoText = "Error loading song";
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
                if (m_Playbacker->SongIsFinished())
                {
                    m_AssociatedRadio->PlaySong(m_AssociatedRadio->GetNextSong());
                }
#ifdef MBRADIO_DISCORDINTEGRATION
                Song CurrentSong = m_Playbacker->GetCurrentSong();
                DiscordImpl* Impl = (DiscordImpl*)m_DiscordImpl.get();
                if (CurrentSong != Impl->CurrentSong)
                {
                    Impl->CurrentSong = CurrentSong;
                    Impl->Activity.SetState(CurrentSong.Artist.c_str());
                    Impl->Activity.SetDetails(CurrentSong.SongName.c_str());
                    Impl->Activity.GetTimestamps().SetStart(std::time(nullptr) - uint64_t(NewElapsed));
                    Impl->Core->ActivityManager().UpdateActivity(Impl->Activity, [](discord::Result result) {

                    });
                }
#endif // MBRADIO_DISCORDINTEGRATION
            }
#ifdef MBRADIO_DISCORDINTEGRATION
            DiscordImpl* Impl = (DiscordImpl*)m_DiscordImpl.get();
            Impl->Core->RunCallbacks();
#endif // MBRADIO_DISCORDINTEGRATION

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
        m_Playbacker->Play();
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
        MBMedia::AudioParameters CurrentParameters;
        std::unique_ptr<MBStreaming::OctetStreamDataFetcher> SongDataFetcher;
        size_t AudioStreamIndex = -1;
        size_t OutputStreamSampleOffset = 0;
        while (!m_Stopping.load())
        {
            if (m_SongRequested.load())
            {
                m_InputLoaded.store(false);
                m_ErrorLoadingInput.store(false);
                m_SongDuration.store(-1);
                m_SongPosition.store(-1);


                CurrentInputStream = nullptr;
                std::lock_guard<std::mutex> Lock(m_RequestMutex);
                m_SongRequested.store(false);
                m_OutputDevice->Clear();
                AudioStreamIndex = -1;
                if (m_RequestedSong.SongURI == "")
                {
                    CurrentInputStream = nullptr;
                    SongDataFetcher = nullptr;
                    continue;
                }
                SongDataFetcher = nullptr;
                MBStreaming::URLStream URLParser(m_RequestedSong.SongURI);
                SongDataFetcher = std::unique_ptr<MBStreaming::OctetStreamDataFetcher>(new MBStreaming::OctetStreamDataFetcher(URLParser.GetSearchableInputStream(), 5));
                SongDataFetcher->WaitForStreamInfo();
                for (size_t i = 0; i < SongDataFetcher->AvailableStreams(); i++)
                {
                    if (SongDataFetcher->GetStreamInfo(i).GetMediaType() == MBMedia::MediaType::Audio)
                    {
                        AudioStreamIndex = i;
                    }
                    else
                    {
                        SongDataFetcher->DiscardStream(i);
                    }
                }
                if(AudioStreamIndex != -1)
                {
                    CurrentInputStream = std::unique_ptr<MBMedia::AudioStream>(new MBMedia::AudioInputConverter(SongDataFetcher->GetAudioStream(AudioStreamIndex), m_OutputDevice->GetOutputParameters()));
                    CurrentParameters = SongDataFetcher->GetStreamInfo(AudioStreamIndex).GetAudioInfo().AudioInfo;
                    
                    MBMedia::TimeBase StreamTimebase = SongDataFetcher->GetStreamInfo(AudioStreamIndex).StreamTimebase;
                    int64_t TimebaseDuration = SongDataFetcher->GetStreamInfo(AudioStreamIndex).StreamDuration;
                    double SongDuration = (TimebaseDuration*StreamTimebase.num)/double(StreamTimebase.den);
                    if (SongDuration < 0)
                    {
                        SongDuration = -1;
                    }
                    m_SongDuration.store(SongDuration);
                }
                else
                {
                    m_ErrorLoadingInput.store(true);
                }
                m_InputLoaded.store(true);
                OutputStreamSampleOffset = 0;
            }
            if (m_SeekRequested.load())
            {
                //bara denna tråd jkan ändra AudioStreamIndex
                std::lock_guard<std::mutex> Lock(m_RequestMutex);
                m_OutputDevice->Clear();
                m_SeekRequested.store(false);
                if (SongDataFetcher != nullptr)
                {
                    if (AudioStreamIndex != -1)
                    {
                        MBMedia::TimeBase StreamTimebase = SongDataFetcher->GetStreamInfo(AudioStreamIndex).StreamTimebase;
                        int64_t SongTimebasePosition = (m_SeekPosition * StreamTimebase.den) / StreamTimebase.num;
                        SongDataFetcher->SeekPosition(AudioStreamIndex, SongTimebasePosition);

                        m_SongPosition.store(m_SeekPosition);
                        CurrentInputStream = std::unique_ptr<MBMedia::AudioStream>(new MBMedia::AudioInputConverter(SongDataFetcher->GetAudioStream(AudioStreamIndex), m_OutputDevice->GetOutputParameters()));
                        OutputStreamSampleOffset = m_SeekPosition*CurrentInputStream->GetAudioParameters().SampleRate;
                    }
                }
            }
            if (SongDataFetcher == nullptr)
            {
                continue;
            }

            if (m_OutputDevice->GetCurrentOutputBufferSize() < 4096 && m_Play.load() && AudioStreamIndex != -1)
            {
                //eftersom det bara är den här tråden som kan ändra på DataFetcher, och alla andra trådar bara läser från tråden behöver vi inte oroa oss för race conditions
                if (CurrentInputStream != nullptr)
                {
                    MBMedia::AudioBuffer TempBuffer = MBMedia::AudioBuffer(m_OutputDevice->GetOutputParameters(), 4096);
                    size_t RecievedSamples = CurrentInputStream->GetNextSamples(TempBuffer.GetData(), 4096, 0);
                    OutputStreamSampleOffset += RecievedSamples;
                    m_OutputDevice->InsertAudioData(TempBuffer.GetData(), RecievedSamples);
                    if(CurrentInputStream->IsFinished())
                    {
                        m_Finished.store(true); 
                    }
                }
                else
                {
                    m_Finished.store(true);   
                }
                //milli sekunder
                m_OutputDevice->UpdateBuffer(4096);
                uint64_t Milli = (((4096 * 1000)*3)/4) / CurrentParameters.SampleRate;
                
                std::unique_lock<std::mutex> WaitLock(m_RequestMutex);
                m_RequestConditional.wait_for(WaitLock, std::chrono::milliseconds(Milli));
                
                //MBMedia::TimeBase StreamTimebase = SongDataFetcher->GetStreamInfo(AudioStreamIndex).StreamTimebase;
                //int64_t TimebaseDuration = SongDataFetcher->GetStreamTimestamp(AudioStreamIndex);
                double SongPosition = double(OutputStreamSampleOffset) / CurrentInputStream->GetAudioParameters().SampleRate;
                m_SongPosition.store(SongPosition);
            }
            if(AudioStreamIndex == -1)
            {
                std::unique_lock<std::mutex> WaitLock(m_RequestMutex);
                m_RequestConditional.wait(WaitLock);
            }
            std::unique_lock<std::mutex> WaitLock(m_RequestMutex);
            if (m_Play.load() == false)
            {
                uint64_t Milli = (((4096 * 1000) * 3) / 4) / CurrentParameters.SampleRate;
                m_RequestConditional.wait_for(WaitLock, std::chrono::milliseconds(Milli));
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
        std::lock_guard<std::mutex> Lock(m_RequestMutex);
        m_Play.store(false);
    }
    void SongPlaybacker::Play()
    {
        std::lock_guard<std::mutex> Lock(m_RequestMutex);
        m_Play.store(true);
        m_RequestConditional.notify_one();
    }
    bool SongPlaybacker::Paused()
    {
        return(!m_Play.load());
    }
    void SongPlaybacker::SetInputSource(Song SongToPlay)
    {
        std::lock_guard<std::mutex> SongRequestMutex(m_RequestMutex);
        m_SongRequested.store(true);
        m_RequestConditional.notify_one();
        m_RequestedSong = std::move(SongToPlay);
        m_CurrentSong = m_RequestedSong;
        m_Finished.store(false);
    }
    Song SongPlaybacker::GetCurrentSong()
    {
        std::lock_guard<std::mutex> SongRequestMutex(m_RequestMutex);
        return(m_CurrentSong);
    }
    bool SongPlaybacker::InputAvailable()
    {
        return(!m_ErrorLoadingInput.load());
    }
    bool SongPlaybacker::InputLoaded()
    {
        return(m_InputLoaded.load());
    }
    void SongPlaybacker::Seek(double SecondOffset)
    {
        std::lock_guard<std::mutex> SongRequestMutex(m_RequestMutex);
        m_SeekRequested.store(true);
        m_SeekPosition = m_SongPosition.load() + SecondOffset;
        m_RequestConditional.notify_one();
        m_Finished.store(false);
        //m_SongDataFetcher->SeekPosition(m_AudioStreamIndex, StreamPosition);
    }
    double SongPlaybacker::GetSongDuration()
    {
        return(m_SongDuration.load());
    }
    double SongPlaybacker::GetSongPosition()
    {
        return(m_SongPosition.load());
    }
    bool SongPlaybacker::SongIsFinished() const
    {
        return(m_Finished.load()); 
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
        //TODO hardcoded af xd xd xd
        m_Config.Server.Address = "https://mrboboget.se";
        std::filesystem::path ConfigPath = MBSystem::GetUserHomeDirectory()/".mbradio/config.json";
        if(std::filesystem::exists(ConfigPath))
        {
            Config NewConfig;
            try
            {
                MBError OutError = true;
                auto StoredJSON = MBParsing::ParseJSONObject(ConfigPath,&OutError);
                FromJSON(NewConfig,StoredJSON);
                std::swap(NewConfig,m_Config);
            }
            catch(...)
            {
                   
            }
        }


        MBCLI::TerminalInfo Info = TerminalToUse->GetTerminalInfo();
        TerminalToUse->InitializeWindowMode();
        TerminalToUse->SetBufferRenderMode(true);
        TerminalToUse->SetResizeCallback([this](int Width,int Height){p_WindowResizeCallback(Width,Height);});



        m_TerminalWidth = Info.Width;
        m_TerminalHeight = Info.Height;
        m_REPLWindow.SetDimensions(MBCLI::Dimensions(m_TerminalWidth, m_TerminalHeight));
        m_PlaylistWindow.SetDimensions(MBCLI::Dimensions(m_TerminalWidth, m_TerminalHeight));
        m_SongWindow.SetDimensions(MBCLI::Dimensions(m_TerminalWidth, m_TerminalHeight));
        m_WindowBuffer = MBCLI::TerminalWindowBuffer(m_TerminalWidth, m_TerminalHeight);
        m_REPLWindow.SetFocus(true);

        int SongHeight = 4;
        int SongWidth = m_TerminalWidth;
        int REPLHeight = m_TerminalHeight - SongHeight;
        int REPLWidth = (m_TerminalWidth * 2) / 3;
        int PlaylistHeight = REPLHeight;
        int PlaylistWidth = m_TerminalWidth - REPLWidth;

        std::unique_ptr<MBCLI::WindowManager> TopWindow = std::make_unique<MBCLI::WindowManager>();
        TopWindow->SetVertical(false);
        TopWindow->AddWindow(MBUtility::SmartPtr<MBCLI::Window>(&m_REPLWindow));
        MBCLI::WindowStretchInfo ListStretch;
        ListStretch.PercentSize = 1/float(3);
        TopWindow->AddWindow(MBUtility::SmartPtr<MBCLI::Window>(&m_PlaylistWindow),ListStretch);
        m_WindowManager.SetVertical(true);
        m_WindowManager.AddWindow(std::move(TopWindow));
        MBCLI::WindowStretchInfo BarStretch;
        BarStretch.MinSize = 4;
        m_WindowManager.AddWindow(MBUtility::SmartPtr<MBCLI::Window>(&m_SongWindow),BarStretch);
        m_WindowManager.SetDimensions(MBCLI::Dimensions(m_TerminalWidth,m_TerminalHeight));
    }
    void MBRadio::p_WindowResizeCallback(int NewWidth, int NewHeight)
    {
        {
            m_AssociatedTerminal->Clear();
            std::lock_guard<std::mutex> Lock(m_InternalsMutex);
            m_TerminalWidth = NewWidth;
            m_TerminalHeight = NewHeight;
            m_WindowBuffer = MBCLI::TerminalWindowBuffer(NewWidth, NewHeight);
        }
        p_UpdateWindow();
    }
    int MBRadio::Run()
    {
        //MBCLI::ConsoleInput TestTest = m_AssociatedTerminal->ReadNextInput();
        while (true)
        {
            MBCLI::ConsoleInput NewInput = m_AssociatedTerminal->ReadNextInput();

            //DEBUG
            //if (NewInput.SpecialInput != MBCLI::SpecialKey::Null)
            //{
            //    std::cout << "Special: " << std::dec << int(NewInput.SpecialInput) << std::endl;
            //    continue;
            //}
            //std::string OutputString = NewInput.CharacterInput.ToString();
            //for (char Character : OutputString)
            //{
            //    std::cout << std::hex << int(Character) << std::endl;
            //}
            //std::cout << "KeyModifier" << int(NewInput.KeyModifiers) << std::endl;
            //continue;
            //DEBUG
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
                else if (NewInput.SpecialInput == MBCLI::SpecialKey::Up)
                {
                    m_PlaylistWindow.Scroll(m_PlaylistWindow.GetCurrentDisplayIndex() - 1);
                }
                else if (NewInput.SpecialInput == MBCLI::SpecialKey::Down)
                {
                    m_PlaylistWindow.Scroll(m_PlaylistWindow.GetCurrentDisplayIndex() + 1);
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
            //bool WindowUpdated = false;
            //if (m_PlaylistWindow.Updated() || m_REPLWindow.Updated() || m_SongWindow.Updated())
            //{
            //    WindowUpdated = true;
            //}
            if(m_WindowManager.Updated())
            {
                p_UpdateWindow();
            }
        }
    }
    void MBRadio::Update()
    {
        p_UpdateWindow();
    }

    void MBRadio::SetPause(bool PauseStatus)
    {
        if (PauseStatus)
        {
            m_SongWindow.Pause();
        }
        else
        {
            m_SongWindow.Play();
        }
    }
    bool MBRadio::GetPause()
    {
        return(m_SongWindow.Paused());
    }
    int MBRadio::GetSongDisplayIndex()
    {
        return(m_PlaylistWindow.GetCurrentDisplayIndex());
    }
    void MBRadio::Scroll(int SongIndex)
    {
        m_PlaylistWindow.Scroll(SongIndex);
    }
    void MBRadio::ClearSongs()
    {
        m_PlaylistWindow.ClearSongs();
    }
    void MBRadio::RemoveSong(size_t SongIndex)
    {
        m_PlaylistWindow.RemoveSong(SongIndex);
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
        m_PlaylistWindow.SetNextSong(SongIndex);
    }
    void MBRadio::p_UpdateWindow()
    {
        std::lock_guard<std::mutex> Lock(m_InternalsMutex);
        //int SongHeight = 4;
        //int SongWidth = m_TerminalWidth;
        //int REPLHeight = m_TerminalHeight - SongHeight;
        //int REPLWidth = (m_TerminalWidth * 2) / 3;
        //int PlaylistHeight = REPLHeight;
        //int PlaylistWidth = m_TerminalWidth - REPLWidth;



        //m_SongWindow.SetDimensions( MBCLI::Dimensions(SongWidth, SongHeight));
        //m_PlaylistWindow.SetDimensions(MBCLI::Dimensions(PlaylistWidth, PlaylistHeight));
        //m_REPLWindow.SetDimensions(MBCLI::Dimensions(REPLWidth, REPLHeight));

        //m_WindowBuffer.WriteBuffer(m_SongWindow.GetBuffer(), 0, 0);
        //m_WindowBuffer.WriteBuffer(m_REPLWindow.GetBuffer(), SongHeight, 0);
        //m_WindowBuffer.WriteBuffer(m_PlaylistWindow.GetBuffer(), SongHeight, REPLWidth);
        m_WindowManager.SetDimensions(MBCLI::Dimensions(m_TerminalWidth,m_TerminalHeight));
        m_WindowBuffer = m_WindowManager.GetBuffer();
        MBCLI::CursorInfo NewCursorInfo = m_WindowManager.GetCursorInfo();
        m_AssociatedTerminal->PrintWindowBuffer(m_WindowBuffer, 0, 0);
        m_AssociatedTerminal->SetCursorPosition(NewCursorInfo.Position);
        m_AssociatedTerminal->Refresh();
        if (NewCursorInfo.Hidden)
        {
            m_AssociatedTerminal->HideCursor();
        }
        else
        {
            m_AssociatedTerminal->ShowCursor();
        }

    }
    //END MBRadio

}
