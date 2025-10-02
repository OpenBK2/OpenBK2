#pragma once

#include "StructMetaInfo.h"
#include "OwnValue.h"
#include "Variant.h"
#include "ObjMan.h"
#include "BindProcessor.h"
#include "ObjManIterator.h"

namespace NLXML
{
	class CXMLNode;
}
namespace NXxml
{
	class CXmlNode;
}

namespace NDb
{

namespace NBind
{

typedef vector< pair<string, wstring> > CAttributesList;
class CBindStruct : public IObjMan, public ILoadableObjMan
{
	OBJECT_NOCOPY_METHODS( CBindStruct );
	//
	CObj<CResource> pStruct;
	vector<UValue> ownValues;
	CAttributesList attributes;	// additional object header attributes
	CObj<NMetaInfo::SStructMetaInfo> pMetaInfo;
	SBindProcessor bindProcessor;
	CDBID dbidMain;
	bool bLoaded;			// indicates, does object was already loaded
	bool bChanged;		// indicates changes in object (object requires save)
	bool bNewObject;	// object was create as new, but newer saved to disk
public:
	CBindStruct(): bLoaded( false ), bChanged( false ), bNewObject( false ) {}
	CBindStruct( CResource *_pStruct, NMetaInfo::SStructMetaInfo *_pMetaInfo );
	~CBindStruct();
	//
	NMetaInfo::SStructMetaInfo *GetMetaInfo() const { return pMetaInfo; }
	const string &GetTypeName() const { return pMetaInfo->pStructTypeDef->szTypeName; }
	const CAttributesList &GetAttributes() const { return attributes; }
	//
	bool IsLoaded() const { return bLoaded; }
	void SetLoaded() { bLoaded = true; }
	void ResetLoaded() { bLoaded = false; }
	//
	bool IsNew() const { return bNewObject; }
	void SetNew( bool bNew ) { bNewObject = bNew; }
	//
	void SetDBID( const CDBID &_dbid );
	//
	void SetChanged();
	void ResetChanged() { bChanged = false; }
	bool IsChanged() const { return bLoaded && bChanged; }
	//
	IObjMan *CreateManipulator( const string &szName ) { return bindProcessor.CreateManipulator( szName, this ); }
	IObjManIterator *CreateIterator( bool bShowHidden )	{ return bindProcessor.CreateIterator( "", pMetaInfo->pStructTypeDef, this, bShowHidden ); }
	string GetFullName() const { return ""; }
	//
	bool SetValue( const string &szName, const CVariant &value ) { SetChanged(); return bindProcessor.SetValue( szName, value ); }
	bool GetValue( const string &szName, CVariant *pValue ) { return bindProcessor.GetValue( szName, pValue ); }
	//
	bool Insert( const string &szName, const int nPos, const int nAmount = 1, bool bSetDefault = false ) { SetChanged(); return bindProcessor.Insert( szName, nPos, nAmount, bSetDefault ); }
	bool Remove( const string &szName, const int nPos, const int nAmount = 1 ) { SetChanged(); return bindProcessor.Remove( szName, nPos, nAmount ); }
	// get property field descriptor by name
	const NTypeDef::STypeStructBase::SField *GetDesc( const string &szFullFieldName ) const { return FindField( szFullFieldName, 0, pMetaInfo->pStructTypeDef ); }
	//
	CResource *GetObject() { return const_cast<CResource*>( pStruct.GetPtr() ); }
	const CDBID &GetDBID() const { return dbidMain; }
	// additional custom attributes
	wstring GetAttribute( const string &szName ) const;
	void SetAttribute( const string &szName, const wstring &szValue );
	//
	CBindArray *GetBindArray( const string &szName ) { return bindProcessor.GetBindArray( szName ); }
	//
	bool LoadXML( const string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pNode ) { bool bRes = bindProcessor.LoadXML( szAddName, pType, pNode, this ); SetLoaded(); return bRes; }
	bool SaveXML( const string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pNode ) { ResetChanged(); return bindProcessor.SaveXML( szAddName, pType, pNode, this ); }
	bool SetDefault( const string &szAddName, NTypeDef::STypeStructBase *pType ) { SetChanged(); return bindProcessor.SetDefault( szAddName, pType ); }
};

class CStructIterator : public IObjManIterator
{
	OBJECT_NOCOPY_METHODS( CStructIterator );
	//
	struct SLevel
	{
		string szAddName;
		CPtr<NTypeDef::STypeStructBase> pTypeStruct;
		int nCurrField;
		CObj<IObjManIterator> pAggregatedIterator;
		//
		bool IsEnd() const { return nCurrField >= pTypeStruct->fields.size() && pAggregatedIterator == 0; }
		string GetName() const { return pAggregatedIterator != 0 ? pAggregatedIterator->GetName() : szAddName + pTypeStruct->fields[nCurrField].szName; }
		const NTypeDef::STypeStructBase::SField *GetDesc() const { return pAggregatedIterator != 0 ? pAggregatedIterator->GetDesc() : &( pTypeStruct->fields[nCurrField] ); }
	};
	list<SLevel> levels;
	CPtr<IObjMan> pObjMan;
	bool bShowHidden;
	//
	bool GotoNextFieldInLevels();
	bool AddLevel( const string &_szAddName, NTypeDef::STypeStructBase *_pTypeStruct );
	//
	CStructIterator(): bShowHidden(false) {}
public:
	CStructIterator( const string &_szAddName, NTypeDef::STypeStructBase *_pTypeStruct, IObjMan *_pObjMan, bool _bShowHidden )
		: pObjMan( _pObjMan ), bShowHidden( _bShowHidden )
	{
		AddLevel( _szAddName, _pTypeStruct );
	}
	//
	string GetBaseName() const { return pObjMan->GetFullName(); }
	// goto next field
	bool Next();
	// have we reached end?
	bool IsEnd() const { return levels.empty(); }
	// get current field's (full) name
	string GetName() const { return levels.empty() ? "" : levels.back().GetName(); }
	// get current field descriptor
	const NTypeDef::STypeStructBase::SField *GetDesc() const { return levels.empty() ? 0 : levels.back().GetDesc(); }
};

};
};
