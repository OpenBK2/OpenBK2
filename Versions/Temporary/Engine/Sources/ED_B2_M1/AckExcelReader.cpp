#include "StdAfx.h"
#include <afxdb.h> 
#include <odbcinst.h> 
#include "AckExcelReader.h"

namespace NAcks
{

// Ack RECORDSET 
class CAckRecordset : public CRecordset
{
public:
	CAckRecordset(CDatabase* pDatabase = NULL): CRecordset(pDatabase)
	{
		m_szSituationCode = _T("");
		m_szRecordCode = _T("");
		m_szFileName = _T("");
		m_szProbability = _T("");
		m_szSubsetCode = _T("");
		m_nFields = 5;

		m_nDefaultType = dynamic;
	}

	DECLARE_DYNAMIC(CAckRecordset)

	CString m_szSituationCode;
	CString m_szRecordCode;
	CString m_szFileName;
	CString m_szProbability;
	CString m_szSubsetCode;
public:
	virtual CString GetDefaultConnect() { return _T("ODBC;DSN=Excel Files"); }
	virtual CString GetDefaultSQL() { return _T("[ACKS$]"); }

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType( CFieldExchange::outputColumn );
		RFX_Text(pFX, _T("[Situation code name]"), m_szSituationCode, 0xFFFF, SQL_VARCHAR );
		RFX_Text(pFX, _T("[RecordCode]"), m_szRecordCode, 0xFFFF, SQL_VARCHAR );
		RFX_Text(pFX, _T("[FileName]"), m_szFileName, 0xFFFF, SQL_VARCHAR );
		RFX_Text(pFX, _T("[Probability]"), m_szProbability );
		RFX_Text(pFX, _T("[SUBSET Code]"), m_szSubsetCode );
	}
};

IMPLEMENT_DYNAMIC(CAckRecordset, CRecordset)

static bool GetExcelODBCDriverName( CString *pRes )
{
	ASSERT( pRes );

	pRes->Empty();
	TCHAR szBuf[2001];
	const WORD cbBufMax = 2000;
	WORD cbBufOut;
	LPTSTR pszBuf = szBuf;

	if ( SQLGetInstalledDrivers( szBuf, cbBufMax, &cbBufOut ) )
	{
		do
		{
			if( _tcsstr( pszBuf, _T( "Excel" ) ) != 0 )
			{
				( *pRes ) = pszBuf;
				return true;
			}
			pszBuf = _tcschr( pszBuf, '\0' ) + 1;
		}
		while( pszBuf[1] != '\0' );
	}
	return false;
}

bool LoadAcksTable( vector<SAckEntry> *pRes, const string &szFileName )
{
	CDatabase database;
	CString strDriver;
	GetExcelODBCDriverName( &strDriver );
	CString strDsn;

	TRY
	{
		strDsn.Format( _T( "ODBC;DRIVER={%s};DSN='';DBQ=%s;MAXSCANROWS=0" ), LPCTSTR( strDriver ), szFileName.c_str() );
		database.Open( NULL, false, false, strDsn );
		//
		CAckRecordset rs( &database );
		rs.Open( CRecordset::forwardOnly, 0, CRecordset::readOnly );
		string szLastNormalSituationCode;
		while( !rs.IsEOF() )
		{
			vector<SAckEntry>::iterator pos = pRes->insert( pRes->end(), SAckEntry() );
			//
			pos->szSituationCode = (LPCSTR)rs.m_szSituationCode;
			if ( pos->szSituationCode.empty() )
				pos->szSituationCode = szLastNormalSituationCode;
			else
				szLastNormalSituationCode = pos->szSituationCode;
			pos->szRecordCode = (LPCSTR)rs.m_szRecordCode;
			pos->szFileName = (LPCSTR)rs.m_szFileName;
			pos->fProbability = atof( (LPCSTR)rs.m_szProbability );
			pos->nSubsetCode = rs.m_szSubsetCode.IsEmpty() ? 0 : atoi( (LPCSTR)rs.m_szSubsetCode );
			//
			rs.MoveNext();
		}
	}
	CATCH(CDBException , pEx)
	{
		pEx->ReportError();
	}
	AND_CATCH(CMemoryException, pEx)
	{
		pEx->ReportError();
	}
	END_CATCH
	//
	database.Close();

	return true;
}

}
