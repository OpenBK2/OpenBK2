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

struct SHLSLShader
{
	int nVertexShaderNumber, nPixelShaderNumber;

	SHLSLShader() : nVertexShaderNumber(-1), nPixelShaderNumber(-1) {}
};

struct SHLSLShaderGroup
{
	string szName;
	vector<string> defines;
	vector<int> defineIDs;
	vector<SHLSLShader> shaders;
};

extern vector<SVShader> vertexShaders;
extern vector<SPShader> pixelShaders;
extern vector<SHLSLShaderGroup> hlslShaders;

