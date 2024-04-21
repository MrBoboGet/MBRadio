#pragma once
#include <string>
#include <vector>
#include <MBParsing/JSON.h>

namespace MBRadio
{
    struct MBRServer
    {
        std::string Address;
        std::string HostName;
        friend MBParsing::JSONObject ToJSON(MBRServer const& Server)
        {
            MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
            ReturnValue["Address"] = Server.Address;
            ReturnValue["HostName"] = Server.HostName;
            return ReturnValue;
        }
        friend void FromJSON(MBRServer& Server,MBParsing::JSONObject const& ObjecToParse)
        {
            if(ObjecToParse.HasAttribute("Address"))
            {
                Server.Address = ObjecToParse["Address"].GetStringData();
            }
            if(ObjecToParse.HasAttribute("HostName"))
            {
                Server.HostName = ObjecToParse["HostName"].GetStringData();
            }
        }
    };
    struct Config
    {
        MBRServer Server;
        friend MBParsing::JSONObject ToJSON(Config const& Config)
        {
            MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
            ReturnValue["Server"] = ToJSON(Config.Server);
            return ReturnValue;
        }
        friend void FromJSON(Config& Server,MBParsing::JSONObject const& ObjecToParse)
        {
            FromJSON(Server.Server,ObjecToParse["Server"]);
        }
    };
}
