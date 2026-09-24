#pragma once

#include <algorithm>
#include <cctype>
#include <istream>
#include <set>
#include <string>
#include <vector>

namespace NTextEditor
{
inline std::vector<std::string> ReadLuaKeywordList( std::istream &input )
{
	// Treat CRLF, indentation and a UTF-8 BOM as file formatting, not part of
	// the identifiers supplied to Lexilla and the completion list.
	std::vector<std::string> words;
	std::string line;
	bool first = true;
	while ( std::getline( input, line ) )
	{
		if ( first && line.compare( 0, 3, "\xef\xbb\xbf" ) == 0 ) line.erase( 0, 3 );
		first = false;
		size_t begin = 0;
		while ( begin < line.size() )
		{
			while ( begin < line.size() && std::isspace( static_cast<unsigned char>(line[begin]) ) ) ++begin;
			size_t end = begin;
			while ( end < line.size() && !std::isspace( static_cast<unsigned char>(line[end]) ) ) ++end;
			if ( end > begin ) words.push_back( line.substr( begin, end - begin ) );
			begin = end;
		}
	}
	std::sort( words.begin(), words.end() );
	words.erase( std::unique(words.begin(), words.end()), words.end() );
	return words;
}

inline std::string JoinLuaKeywords( const std::vector<std::string> &words )
{
	std::string result;
	for ( const auto &word : words )
	{
		if ( !result.empty() ) result += ' ';
		result += word;
	}
	return result;
}

inline std::vector<std::string> FindLuaFunctions( const std::string &source )
{
	// A small declaration scanner, not a Lua parser. Skipping strings and
	// comments first prevents examples in comments from becoming keywords.
	auto startsName = []( char c ) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
	auto inName = [startsName]( char c ) { return startsName(c) || (c >= '0' && c <= '9'); };
	auto skipLongString = [&source]( size_t &pos ) {
		if ( pos >= source.size() || source[pos] != '[' ) return false;
		size_t end = pos + 1;
		while ( end < source.size() && source[end] == '=' ) ++end;
		if ( end >= source.size() || source[end] != '[' ) return false;
		const std::string close = "]" + source.substr( pos + 1, end - pos - 1 ) + "]";
		const size_t found = source.find( close, end + 1 );
		pos = found == std::string::npos ? source.size() : found + close.size();
		return true;
	};
	std::vector<std::string> tokens;
	for ( size_t pos = 0; pos < source.size(); )
	{
		if ( std::isspace( static_cast<unsigned char>(source[pos]) ) ) { ++pos; continue; }
		if ( source.compare( pos, 2, "--" ) == 0 )
		{
			pos += 2;
			if ( !skipLongString(pos) )
			{
				const size_t end = source.find_first_of( "\r\n", pos );
				pos = end == std::string::npos ? source.size() : end;
			}
			continue;
		}
		if ( source[pos] == '\'' || source[pos] == '"' )
		{
			const char quote = source[pos++];
			while ( pos < source.size() )
			{
				const char c = source[pos++];
				if ( c == '\\' && pos < source.size() ) ++pos;
				else if ( c == quote ) break;
			}
			tokens.push_back( "<string>" );
			continue;
		}
		if ( skipLongString(pos) ) { tokens.push_back( "<string>" ); continue; }
		const size_t begin = pos++;
		if ( startsName(source[begin]) )
			while ( pos < source.size() && inName(source[pos]) ) ++pos;
		tokens.push_back( source.substr( begin, pos - begin ) );
	}
	std::set<std::string> names;
	for ( size_t i = 0; i < tokens.size(); ++i )
	{
		if ( tokens[i] != "function" ) continue;
		size_t begin = i + 1, end = begin;
		if ( begin < tokens.size() && tokens[begin] == "(" && i >= 2 && tokens[i - 1] == "=" )
		{
			// Also recognize: local name = function(...) and table.name = function(...).
			begin = end = i - 2;
			while ( begin >= 2 && tokens[begin - 1] == "." && startsName(tokens[begin - 2][0]) ) begin -= 2;
			++end;
		}
		else
		{
			if ( begin >= tokens.size() || !startsName(tokens[begin][0]) ) continue;
			end = begin + 1;
			while ( end + 1 < tokens.size() && (tokens[end] == "." || tokens[end] == ":") && startsName(tokens[end + 1][0]) ) end += 2;
			if ( end >= tokens.size() || tokens[end] != "(" ) continue;
		}
		if ( !startsName(tokens[begin][0]) ) continue;
		std::string name;
		for ( size_t j = begin; j < end; ++j ) name += tokens[j];
		names.insert( name );
	}
	return { names.begin(), names.end() };
}
}
