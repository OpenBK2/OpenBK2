#ifndef __COMMANDER__
#define __COMMANDER__

class CCommonUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "GeneralInternalInterfaces.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ****
//  generic manager, tracks for more than 1 task at a time
// ****
class CCommander : public ICommander
{
	public: int operator&( IBinSaver &saver ); private:;
	//
	struct STaskSortPresicate 
	{
		bool operator()( const IGeneralTask *pT1, const IGeneralTask *pT2 ) const 
		{	return pT1->GetSeverity() > pT2->GetSeverity(); }
	};
	//
	struct STaskBadSituationPredicate
	{
		bool operator()( const IGeneralTask *pT ) const { return pT->GetSeverity() < 0; }
	};
	//
	struct SFinishedPredicate
	{
		bool operator()( const IGeneralTask *pT ) const { return pT->IsFinished(); }
	};
	//
	struct SSegmentPredicate
	{
		void operator()( IGeneralTask *pT ) { pT->Segment(); }
	};
	struct SCalcRatingPredicate
	{
		IWorkerEnumerator *pEn;
		EForceType eType;
		SCalcRatingPredicate( IWorkerEnumerator *_pEn, EForceType _eType ) : pEn( _pEn ), eType( _eType ) {  }
		void operator()( pair<float, CPtr<CCommonUnit> > &value )
			{ value.first = pEn->EvaluateWorkerRating( value.second, eType ); }
		bool operator()( const pair<float, CPtr<CCommonUnit> > &v1,
										 const pair<float, CPtr<CCommonUnit> > &v2 )
		{
			return v1.first < v2.first;
		}
	};
	//
	struct STaskCalcSeverityPredicate
	{
		int nNumberPositive, nNumberNegative;
		float fSeverityPositive, fSeverityNegative;
		void operator()( const IGeneralTask *pT ) 
		{ 
			const float fSeverity = pT->GetSeverity(); 
			if ( fSeverity >= 0 )
			{
				++nNumberPositive; 
				fSeverityPositive += fSeverity;
			}
			else
			{
				++nNumberNegative;
				fSeverityNegative += fSeverity;
			}
		}
		STaskCalcSeverityPredicate();
		const float GetSeverity() const;
	};
	// 
	struct STakeWorkersPredicate 
	{
		CPtr<ICommander> pManager;
		void operator()( IGeneralTask *pT ) { pT->ReleaseWorker( pManager, 0 ); }
		STakeWorkersPredicate( interface ICommander *pManager ) : pManager( pManager ) {  }
	};
protected:

	typedef vector< CObj<IGeneralTask> > Tasks;
	Tasks tasks;			// all tasks of this colonel. 
	float fMeanSeverity;

	void EnumWorkersInternal( const enum EForceType eType, IWorkerEnumerator *pEn, CommonUnits *pUnits );
public:
	CCommander();
	virtual float GetMeanSeverity() const ;
	virtual void Segment();
	
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __GENERAL_INTERNAL__

