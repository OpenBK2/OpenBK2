#pragma once

class CCodeGenException
{
	string szDescription;
public:
	CCodeGenException( const string &_szDescription )
		: szDescription( _szDescription ) { }

	const string& GetDesc() const { return szDescription; }
};


