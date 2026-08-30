#pragma once
virtual float GetSpeedFactor() const { return 1.f; }
// Empty keeps all legacy animation resources on the Granny3D path.
virtual const NFile::CFilePath &GetModelFileRef() const
{
	static const NFile::CFilePath emptyPath;
	return emptyPath;
}

