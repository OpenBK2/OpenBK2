#if !defined(__BUILDER_METHODS__)
#define __BUILDER_METHODS__
#pragma once

interface IManipulator;

bool CheckStringValue( string *pszDescription, const string &szValueName, IManipulator *pBuilderMan );
bool CheckIntValue( string *pszDescription, const string &szValueName, int nMin, int nMax, IManipulator *pBuilderMan );

#endif // !defined(__BUILDER_METHODS__)

