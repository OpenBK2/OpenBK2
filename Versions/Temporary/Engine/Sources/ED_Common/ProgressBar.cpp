#include "stdafx.h"
#include "ProgressBarWindow.h"

namespace 
{
	CProgressBarWindow progressBar;
};

namespace NProgressBar {


void CreateProgressBar()
{
	ASSERT( !IsWindow( progressBar.GetSafeHwnd() ) );
	progressBar.Create( 0 );
}


void DestroyProgressBar()
{
	if ( IsWindow( progressBar.GetSafeHwnd() ) )
	{
		progressBar.DestroyWindow();
	}
}


void Start( int nRange, const string & szCaption )
{
	progressBar.Start( nRange, szCaption );
}


void Finish()
{
	progressBar.Finish();
}


void StepIt()
{
	progressBar.StepIt();
}


void SetCaption( const string & szCaption )
{
	progressBar.SetCaption( szCaption );
}


};


