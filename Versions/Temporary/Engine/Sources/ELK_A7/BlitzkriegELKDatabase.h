
#pragma once
class CBlitzkriegELKRecordset : public CRecordset
{
public:
	CBlitzkriegELKRecordset(CDatabase* pDatabase = NULL);
	DECLARE_DYNAMIC(CBlitzkriegELKRecordset)

	CString	m_Path;
	CString	m_Original;
	CString	m_Translation;
	CString	m_State;
	CString	m_Description;


	public:
	virtual CString GetDefaultConnect();    // Default connection string
	virtual CString GetDefaultSQL();    // Default SQL for Recordset
	virtual void DoFieldExchange(CFieldExchange* pFX);  // RFX support

// Implementation
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

