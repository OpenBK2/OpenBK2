#pragma once

struct IManipulator;

bool CheckStringValue( std::string *pszDescription, const std::string &szValueName, IManipulator *pBuilderMan );
bool CheckIntValue( std::string *pszDescription, const std::string &szValueName, int nMin, int nMax, IManipulator *pBuilderMan );



