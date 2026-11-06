#pragma once

class CCommonUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "GeneralInternalInterfaces.h"

#include "AILogic_export.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ****
//  generic manager, tracks for more than 1 task at a time
// ****
class CCommander : public ICommander
{
public:
	AILOGIC_EXPORT int operator&( IBinSaver &saver ) override;

private:
	struct STaskCalcSeverityPredicate
	{
		int nNumberPositive = {}, nNumberNegative = {};
		float fSeverityPositive = {}, fSeverityNegative = {};

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

protected:
	typedef std::vector<CObj<IGeneralTask>> Tasks;
	Tasks tasks; // all tasks of this colonel.
	float fMeanSeverity = {};

	AILOGIC_EXPORT void EnumWorkersInternal( const enum EForceType eType, IWorkerEnumerator *pEn, CommonUnits *pUnits );

public:
	AILOGIC_EXPORT CCommander();
	AILOGIC_EXPORT float GetMeanSeverity() const override;
	AILOGIC_EXPORT void Segment() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
