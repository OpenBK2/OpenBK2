#pragma once
virtual float GetSpeedFactor() const { return 1.f; }
// Empty keeps all legacy animation resources on the Granny3D path.
virtual const NFile::CFilePath &GetModelFileRef() const
{
	static const NFile::CFilePath emptyPath;
	return emptyPath;
}
// Optional GLB clip/range metadata. Granny resources never consult these values.
virtual const std::string &GetClipName() const
{
	static const std::string emptyName;
	return emptyName;
}
virtual int GetFirstFrame() const { return 0; }
virtual int GetLastFrame() const { return 0; }
virtual int GetLengthMilliseconds() const { return 0; }

