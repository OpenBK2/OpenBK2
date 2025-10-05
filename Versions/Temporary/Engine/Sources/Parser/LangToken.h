#pragma once

namespace NLang
{
	class CLangNode;
}

struct SToken
{
	std::string psz;
	CPtr<NLang::CLangNode> pNode;

	SToken() { }
};

#define YYSTYPE SToken


