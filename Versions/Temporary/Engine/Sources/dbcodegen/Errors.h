#pragma once

class CCodeGenException
{
	std::string szDescription;
public:
	CCodeGenException( const std::string &_szDescription )
		: szDescription( _szDescription ) { }

	const std::string& GetDesc() const { return szDescription; }
};


