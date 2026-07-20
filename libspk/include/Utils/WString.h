#pragma once

#include <string>
#include <vector>
#include <cstdlib>
#include "../spkdll.h"
#include "../spkdef.h"

enum WStringCompare {
	Same		= 0,
	Newer		= 1,
	Older		= -1,
};

#pragma warning( push )
#pragma warning( disable : 4251)

namespace Utils {

	class SString;
	class WStringList;

/**
 * String wrapper class
 */
	class SPKEXPORT WString
{
public:
	static std::string ws2s(const std::wstring& wstr);
	static std::wstring s2ws(const std::string& str);
	static WString ConvertTime(time_t whattime, bool shortdesc = false);

private:
	std::wstring _data;

public:
	// constructors
	WString();
	WString(const char* str);
	WString(const wchar_t* str);
	WString(const std::string& str);
	WString(const std::wstring& str);
	WString(const char c);
	WString(const unsigned char c);
	WString(const wchar_t c);
	WString(const WString &str);
	WString(WString&&) noexcept = default;               // enable efficient move
	WString(long long l);
	WString(unsigned long long l);
	WString(long l);
	WString(unsigned long l);
	WString(float f);
	WString(double f);

	// explicit sized constructor - no default len to avoid ambiguity with single-arg ctor
	WString(const wchar_t* str, int len);

	virtual ~WString() noexcept;

	// conversion functions
	void fromLong(long l);
	void fromLong64(long long l);
	const WString& fromFloat(float f, int dp = -1);
	const WString& fromString(const char* str);
	const WString& fromString(const unsigned char* str);
	void fromDouble(double f);
	const WString &fromDouble(double f, int dp);
	const WString &format(const wchar_t *sFormat, ...);
	const WString args(const WString* args, int max) const;
	const WString args(const WStringList& args) const;
	const WString args(const std::vector<Utils::WString>& args) const;
	const WString &arg(const WString &s1);
	const WString &arg(const WString &s1, const WString &s2);
	const WString &arg(const WString &s1, const WString &s2, const WString &s3);
	const WString &arg(const WString &s1, const WString &s2, const WString &s3, const WString &s4);
	const WString &arg(const WString &s1, const WString &s2, const WString &s3, const WString &s4, const WString &s5);
	const WString& convertTime(time_t whattime, bool shortdesc = false);
	
	static WString PadNumber(long iNum, int iAmount);
	static WString Number(long l) { return WString(l); }
	static WString Format(const wchar_t *sFormat, ...);
	static WString Null();
	static WString FromFloat(float f, int dp = -1);
	static WString FromString(const char* data);
	static WString FromString(const unsigned char* data);

	bool toBool() const;
	long toLong() const;
	long long toLong64() const;
	int toInt() const;
	double toDouble() const;
	float toFloat() const;
	const wchar_t* toWChar() const;
	const wchar_t* c_str() const noexcept;
	std::string toUTF8() const;
	std::string toFileUTF8() const;
	std::wstring toStdWString() const { return _data; }

	const SString toString() const;
	const std::string toStdString() const;

	// casting operators
	inline operator const wchar_t *() const noexcept	{ return _data.c_str(); }
	inline operator const std::wstring &() const noexcept	{ return _data; }
	inline operator const int () const				{ return static_cast<int>(this->toLong()); }
	inline operator const long () const				{ return this->toLong(); }
	inline operator const long long () const		{ return this->toLong64(); }
	inline operator const double () const			{ return this->toDouble(); }
	inline operator const float () const			{ return this->toFloat(); }
	inline operator const bool () const				{ return (this->empty() ? false : ((this->toLong()) ? true : false)); }

	// assignment operators
	const WString &operator= ( const WString &str );
	WString& operator=(WString&&) noexcept = default;
	const WString &operator= ( const std::wstring &str );
	const WString &operator= ( const wchar_t *str );
	const WString &operator= ( wchar_t c );
	const WString &operator= ( long l );
	const WString &operator= ( unsigned long l );
	const WString &operator= ( float l );
	const WString &operator= ( double l );

	// subscript operators
#ifdef _WIN32
	const wchar_t operator[] (int num) const;
	wchar_t& operator[] (int num);
#else
	const wchar_t operator[] (size_t num) const;
	wchar_t& operator[] (size_t num);
#endif
	const wchar_t& at(size_t num) const { return _data.at(num); }

	// addition operators
	WString operator+ (const wchar_t* str) const;
	WString operator+ (const char* str) const;
	WString operator+ ( const std::wstring &str ) const;
	WString operator+ ( const WString &str ) const;
	WString operator+ ( const wchar_t c ) const;
	WString operator+ ( const long l ) const;
	WString operator+ ( const unsigned long l ) const;
	WString operator+ ( const float f ) const;
	WString operator+ ( const double f ) const;

	// compound operators
	const WString &operator+= ( const wchar_t *str );
	const WString &operator+= ( const std::wstring &str );
	const WString &operator+= ( const WString &str );
	const WString &operator+= ( const wchar_t c );
	const WString &operator+= ( const long l );
	const WString &operator+= ( const unsigned long l );
	const WString &operator+= ( const float f );
	const WString &operator+= ( const double f );

	// comparison operators
	bool operator== ( const wchar_t *str ) const;
	bool operator== ( const std::wstring &str ) const;
	bool operator== ( const WString &str ) const;

	bool operator!= ( const wchar_t *str ) const;
	bool operator!= ( const std::wstring &str ) const;
	bool operator!= ( const WString &str ) const;

	bool operator !() const;

	bool Compare(const WString &str, bool bCaseSensative = false) const;
	bool Compare(const wchar_t *str, bool bCaseSensative = false) const;

	//std::wstring functions
	bool empty() const noexcept;
	void clear() noexcept;
	WString &erase(size_t offset, size_t count);
	size_t length() const noexcept;
	WString substr(size_t offset, size_t count) const;
	WString substr(size_t offset) const;
	wchar_t back() const;

	// file handling
	wchar_t *readToEndOfLine(wchar_t *data);
	const WString &readToEndOfLine(FILE *id, int *line, bool upper);

	// tokens
	int countToken(const wchar_t *token) const;
	WString token(const wchar_t *token, int tok) const;
	WString tokens(const wchar_t *token, int from, int to = 0) const;
	WString *tokenise(const wchar_t *token, int *max) const;
	size_t tokenise(const wchar_t* token, std::vector<WString>& str) const;
	std::vector<std::wstring> tokenise(const wchar_t* token) const;
	WString replaceToken(const wchar_t *token, int from, const WString &replace) const;
	WString remToken(const wchar_t* token, int from) const;
	WString remTokens(const wchar_t* token, int from, int to = -1) const;
	WString word(int word) const;
	WString words(int from, int to = 0) const;
	WString addToken(const wchar_t *token, const Utils::WString &str) const;
	
	// find/replacement
	long findPos(const WString &find, int iStartPos = 0) const;
	WString findReplace(const WString &find, const WString &replace ) const;
	WString remove(wchar_t c) const;
	const WString &removeChar(wchar_t c);
	const WString &removeChar(const wchar_t *cs);
	WString findRemove(const WString &find) const;
	WString stripHtml() const;
	WString removeIf(int iChar, wchar_t c) const;

	bool containsAny(const WString &str, bool bCaseSensative = false) const;
	bool contains(const WString &str, bool bCaseSensative = false) const;
	bool contains(wchar_t c, bool bCaseSensative = false) const;
	bool isin(const WString &str, bool bCaseSensative = false) const;
	bool isin(wchar_t c, bool bCaseSensative = false) const;
	int compareVersion(const Utils::WString &v) const;
	bool match(const Utils::WString &pattern) const;
	bool startsWith(const WString& prefix) const;
	bool endsWith(const WString& prefix) const;

	WString& prepend(const WString &str);

	// sub string
	WString left(long num) const;
	WString right(int num) const;
	WString mid(int start, int end) const;
	WString between(const WString &before, const WString &after) const;

	bool isNumber(bool integer = false) const;
	bool isCharNumber(int c) const;
	const WString &removeFirstSpace();
	const WString &removeEndSpace();
	const WString &truncate(int iNum);
	const WString &padNumber(int iNum);
	const WString &pad(int iAmoumt, wchar_t cWith);

	WString lower() const;
	WString upper() const;
	const WString &toLower();
	const WString &toUpper();

	const WString &toFilename();
	WString asFilename() const;

private:
	void _assign(const wchar_t* str) { _data.assign(str); }
	void _assign(const wchar_t* str, size_t len) { _data.assign(str, len); }
	bool _isCharNumber(wchar_t c) const;
	std::wstring::size_type _token_nextPos(const wchar_t *token, std::wstring::size_type curPos) const;
};

SPKEXPORT WString operator+(const wchar_t* str1, const WString& str2);
SPKEXPORT WString operator+(const char* str1, const WString& str2);

#pragma warning( pop )

}
