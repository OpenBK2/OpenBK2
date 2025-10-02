#pragma once
namespace NDb
{
namespace NCodeGenTool
{

struct SConfig
{
	vector<string> slns;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Solutions", &slns );
		return 0;
	}
};

}
}
