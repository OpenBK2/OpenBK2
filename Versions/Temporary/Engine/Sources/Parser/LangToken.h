#pragma once

namespace NLang
{
	class CLangNode;
}

struct SToken
{
	string psz;
	CPtr<NLang::CLangNode> pNode;

	SToken() { }
};

#define YYSTYPE SToken


