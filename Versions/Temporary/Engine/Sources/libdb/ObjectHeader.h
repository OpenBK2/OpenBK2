#pragma once
namespace NDb
{

struct STypeObjectHeader
{
	std::string szClassTypeName;								// Object's class type name
	// The constructor retains the legacy -1 sentinel.
	int nObjectID = 0;										// legacy - ObjectID from database - remove it ASAP
	//
	STypeObjectHeader(): nObjectID(-1) {}
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &szClassTypeName );
		saver.Add( 3, &nObjectID );
		return 0;
	}
};

}

