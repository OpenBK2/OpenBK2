#pragma once

#include <cstdint>

struct SVShader
{
	string szName;
	vector<uint32_t> vsShader11, vsShader20;
	//string szShader;
};

struct SRS
{
	string sz1, sz2;
};

struct STSS
{
	string sz1, sz2, sz3;
};

struct SStates
{
	vector<SRS> rs;
	vector<STSS> tss;

	void Clear() { rs.clear(); tss.clear(); }
};

struct SPShader
{
	string szName;
	//string szShader, szShader14;
	SStates states, shader;
	vector<uint32_t> psShader11, psShader14, psShader20, psShader20a;
};

extern vector<SVShader> vertexShaders;
extern vector<SPShader> pixelShaders;

