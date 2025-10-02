#pragma once

struct IXmlSaver;

class CObjectFilter
{
	struct SEntry
	{
		enum EOperationType
		{
			OPERATION_UNION,
			OPERATION_INTERSECTION,
			OPERATION_DIFFERENCE
		};
		EOperationType eOpType;
		string szClassType;
		vector<string> matches;
		//
		bool Match( const string &szFullName, const string &szClassTypeName ) const;
		int operator&( IXmlSaver &saver );
	};
	//
	wstring wszName;
	vector<SEntry> entries;
	//
public:
	const wstring &GetName() const { return wszName; }
	void SetName( const wstring &_wszName ) { wszName = _wszName; }
	// does this object name matches filter?
	bool Match( const string &szFullName, const string &szClassTypeName ) const;
	//
	int operator&( IXmlSaver &saver );
};


