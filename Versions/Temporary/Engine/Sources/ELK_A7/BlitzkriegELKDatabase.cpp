#include "StdAfx.h"
#include "ELK.h"

#include <afxdb.h> 
#include <odbcinst.h> 
#include "BlitzkriegELKDatabase.h"

IMPLEMENT_DYNAMIC(CBlitzkriegELKRecordset, CRecordset)

CBlitzkriegELKRecordset::CBlitzkriegELKRecordset(CDatabase* pdb)
	: CRecordset(pdb)
{
	m_Path = _T("");
	m_Original = _T("");
	m_Translation = _T("");
	m_State = _T("");
	m_Description = _T("");
	m_nFields = 5;
	m_nDefaultType = snapshot;
}


CString CBlitzkriegELKRecordset::GetDefaultConnect()
{
	return _T("ODBC;DSN=Excel Files");
}

CString CBlitzkriegELKRecordset::GetDefaultSQL()
{
	return _T("[BlitzkriegELK]");
}

void CBlitzkriegELKRecordset::DoFieldExchange(CFieldExchange* pFX)
{
	pFX->SetFieldType(CFieldExchange::outputColumn);
	RFX_Text(pFX, _T("[Path]"), m_Path );
	RFX_Text(pFX, _T("[Original]"), m_Original, 0xFFFF, SQL_VARCHAR );
	RFX_Text(pFX, _T("[Translation]"), m_Translation, 0xFFFF, SQL_VARCHAR );
	RFX_Text(pFX, _T("[State]"), m_State );
	RFX_Text(pFX, _T("[Description]"), m_Description, 0xFFFF, SQL_VARCHAR );
}

#ifdef _DEBUG
void CBlitzkriegELKRecordset::AssertValid() const
{
	CRecordset::AssertValid();
}

void CBlitzkriegELKRecordset::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG

