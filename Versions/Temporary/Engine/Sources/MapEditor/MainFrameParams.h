#pragma once


struct SMainFrameParams
{
	//save maximized form
	bool bMaximized;
	CTRect<int> rect;

	SMainFrameParams();

	static std::string GetSection();

	void Load();
	void Save();
};


