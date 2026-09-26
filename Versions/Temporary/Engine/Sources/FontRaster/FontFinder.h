#pragma once

// Finding a font file by its family name, weight and slant, the way GDI's
// CreateFont found a face from a name, but portably: it reads the fonts in a
// set of directories with FreeType itself, so it needs neither the Windows
// registry nor fontconfig.
//
// The game's font records, and the editor's font export, name faces the GDI
// way ("Tahoma", weight 400), while FreeType wants a file. This bridges the two.

#include <cstdint>
#include <string>
#include <vector>

namespace NFontRaster
{

struct SFontMatch
{
	std::string szFile;
	int nFaceIndex = 0;					// as SOptions::nFaceIndex takes it, named instance included
	std::string szFamily;				// what the file calls itself
	std::string szStyle;
	int nWeight = 400;
	bool bItalic = false;
	int nWidthClass = 5;				// OS/2 usWidthClass: 5 is normal, 3 condensed, 7 expanded
};

// Where fonts are installed on this system: %WINDIR%\Fonts and the per-user
// fonts directory on Windows; /usr/share/fonts, /usr/local/share/fonts,
// ~/.local/share/fonts and ~/.fonts elsewhere. Only those that exist.
std::vector<std::string> GetSystemFontDirectories();

// Every face in every TrueType and OpenType file under a set of directories,
// recursively, read once. Opening each file is what makes a search slow (half a
// second over Windows' own fonts), so a program that searches more than once,
// as the game's glyph fallback does, keeps a catalog and searches that.
class CFontCatalog
{
	std::vector<SFontMatch> faces;
public:
	explicit CFontCatalog( const std::vector<std::string> &directories );
	// as FindFont below
	bool Find( const std::string &szFamily, int nWeight, bool bItalic, SFontMatch *pMatch ) const;
	// every face, variable fonts' named instances included, files in sorted order
	const std::vector<SFontMatch> &GetFaces() const { return faces; }
};

// Searches every TrueType and OpenType file under directories, recursively,
// for a face whose family name matches szFamily ignoring case, and picks the
// closest to nWeight and bItalic at normal width. A slant mismatch counts for
// more than a width one, and that for more than any weight difference: Arial
// Narrow calls its family "Arial" too, and GDI would not have picked it for
// "Arial". Variable fonts are searched by their named instances.
// Returns false when no face has that family name. The choice is deterministic:
// files are visited in sorted order and ties go to the first.
bool FindFont( const std::string &szFamily, int nWeight, bool bItalic, const std::vector<std::string> &directories,
	SFontMatch *pMatch );

// The face index, named instance included, in a font already in memory that
// is closest to nWeight and bItalic, scored as FindFont scores: how a game font
// record's Thickness and Italic pick Bold out of a variable font in the paks.
// 0 when the data is not a font.
int SelectFace( const std::vector<uint8_t> &data, int nWeight, bool bItalic );

}
