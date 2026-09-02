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
		std::string szClassType;
		std::vector<std::string> matches;
		//
		bool Match( const std::string &szFullName, const std::string &szClassTypeName ) const;
		int operator&( IXmlSaver &saver );
	};
	//
	std::wstring wszName;
	std::vector<SEntry> entries;
	//
public:
	const std::wstring &GetName() const { return wszName; }
	void SetName( const std::wstring &_wszName ) { wszName = _wszName; }
	// does this object name matches filter?
	bool Match( const std::string &szFullName, const std::string &szClassTypeName ) const;
	//
	int operator&( IXmlSaver &saver );
};


