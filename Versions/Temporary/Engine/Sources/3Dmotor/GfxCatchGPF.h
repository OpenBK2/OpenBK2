#pragma once

void InitCatchGPF();
void AddIgnoreAccessViolationRegion( const void *pStart, int nSize );
void RemoveIgnoreAccessViolationRegion( const void *pStart, int nSize );
