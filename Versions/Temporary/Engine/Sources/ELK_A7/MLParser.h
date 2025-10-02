#if !defined(__ELK_MLPARSER__)
#define __ELK_MLPARSER__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
namespace NML
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SMLUnicodeTextPart
	{
		wstring wszTag;
		wstring wszText;
	};
	typedef vector<SMLUnicodeTextPart> CMLUnicodeText;

	int Parse( CMLUnicodeText *pMLUnicodeText, const wstring &rwszText, bool bJoinTags );

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SMLMBCSTextPart
	{
		CString strTag;
		CString strText;
	};
	typedef vector<SMLMBCSTextPart> CMLMBCSText;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int Parse( CMLUnicodeText *pMLUnicodeText, const wstring &rwszText, bool bJoinTags );
	int Parse( CMLMBCSText *pMLMBCSText, const CString &rstrText, bool bJoinTags, int nCodePage );
};
#endif // !defined(__ELK_MLPARSER__)

