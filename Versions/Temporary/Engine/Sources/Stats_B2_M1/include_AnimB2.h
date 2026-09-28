// Pasted into the generated SAnimB2 (DBAnimB2.h) by the #include in its
// .cll: the SAnimBase accessors the GLB animation binding reads its source
// through, which the generator has no way to write itself.
const NFile::CFilePath &GetModelFileRef() const override { return szModelFileRef; }
const std::string &GetClipName() const override { return szClipName; }
int GetFirstFrame() const override { return nFirstFrame; }
int GetLastFrame() const override { return nLastFrame; }
