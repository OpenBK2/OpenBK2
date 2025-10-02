#pragma once
namespace NDb
{

struct STypeObjectHeader
{
	string szClassTypeName;								// Object's class type name
	int nObjectID;												// legacy - ObjectID from database - remove it ASAP
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

