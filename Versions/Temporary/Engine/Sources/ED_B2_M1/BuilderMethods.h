#pragma once

interface IManipulator;

bool CheckStringValue( string *pszDescription, const string &szValueName, IManipulator *pBuilderMan );
bool CheckIntValue( string *pszDescription, const string &szValueName, int nMin, int nMax, IManipulator *pBuilderMan );



