#include "MapEditor/LuaKeywords.h"
#include <gtest/gtest.h>
#include <sstream>

TEST( LuaKeywords, ReadsWindowsLineEndingsBomAndWhitespace )
{
	std::istringstream file("\xef\xbb\xbfSleep\r\n  CameraMove \t\r\n\r\nSleep\r\nGetGameTime\n");
	EXPECT_EQ( NTextEditor::ReadLuaKeywordList(file),
		(std::vector<std::string>{"CameraMove", "GetGameTime", "Sleep"}) );
}

TEST( LuaKeywords, ReadsBeyondLegacyFixedLineBuffer )
{
	std::string name(600, 'x');
	std::istringstream file(name + "\nSleep\n");
	EXPECT_EQ( NTextEditor::ReadLuaKeywordList(file), (std::vector<std::string>{"Sleep", name}) );
}

TEST( LuaKeywords, FindsDeclarationsAndAssignedFunctions )
{
	const std::string text = "function Start() end\nlocal function Tick(x) end\n"
		"local handler = function() end\nfunction mission.Begin() end\nfunction mission:Stop() end\n"
		"mission.Update = function() end\nStart()\nfunction Start() end";
	EXPECT_EQ( NTextEditor::FindLuaFunctions(text),
		(std::vector<std::string>{"Start", "Tick", "handler", "mission.Begin", "mission.Update", "mission:Stop"}) );
}

TEST( LuaKeywords, IgnoresCommentsStringsAndAnonymousCallbacks )
{
	const std::string text = R"lua(
-- function Fake() end
--[=[ function Hidden() end ]=]
local text = "function Quoted() end"
local other = 'escaped \' function Wrong()'
local longText = [==[ function InsideLongString() end ]==]
Call(function() end)
function -- explanation
 Real() end
)lua";
	EXPECT_EQ( NTextEditor::FindLuaFunctions(text), (std::vector<std::string>{"Real"}) );
	EXPECT_TRUE( NTextEditor::FindLuaFunctions("--[[ function Unfinished(").empty() );
}
