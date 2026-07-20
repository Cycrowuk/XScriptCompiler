#pragma once

#include <string>
#include "../spkdll.h"
#include "../spkdef.h"

enum Compare {
	COMPARE_SAME		= 0,
	COMPARE_NEWER		= 1,
	COMPARE_OLDER		= -1,
};

#pragma warning( push )
#pragma warning( disable : 4251)

namespace Utils {
	std::wstring s2ws(const std::string& str);
	std::string ws2s(const std::wstring& wstr);

	class WString;
/**
 * String wrapper class
 */
	class SPKEXPORT SString : public std::string
{
public:
	// constructors
	SString(void);
	SString(const char *str);
	SString(const unsigned char *str);
	SString(const std::string &str);
	SString(const char c);
	SString(const unsigned char c);
	SString(const SString &str);
	SString(long l);
	SString(unsigned long l);
	SString(float f);
	SString(double f);
	SString(wchar_t* str, int len = -1)
	{
		if(len > 0)
			this->_assign(str, len);
		else
			this->_assign(str);
	}

	virtual ~SString(void);

	// conversion functions
	void fromLong(long l);
	const SString &fromFloat(float f, int dp = -1);
	void fromDouble(double f);
	const SString &fromDouble(double f, int dp);
	const SString &format(const char *sFormat, ...);
	const SString args(const SString *args, int max) const;
	const SString &arg(const SString &s1);
	const SString &arg(const SString &s1, const SString &s2);
	const SString &arg(const SString &s1, const SString &s2, const SString &s3);
	const SString &arg(const SString &s1, const SString &s2, const SString &s3, const SString &s4);
	const SString &arg(const SString &s1, const SString &s2, const SString &s3, const SString &s4, const SString &s5);
	
	static SString PadNumber(long iNum, int iAmount);
	static SString Number(long l) { return SString(l); }
	static SString Format(const char *sFormat, ...);
	static SString Null();
	static SString FromFloat(float f, int dp = -1);

	bool toBool() const;
	long toLong() const;
	long long toLong64() const;
	int toInt() const;
	double toDouble() const;
	float toFloat() const;

	const Utils::WString toWString() const;
	const long long lengthLL() const;

	// casting operators
	inline operator const char *() const			{ return (const char *)c_str(); }
	inline operator char *() const					{ return (char *)c_str(); }
	inline operator const unsigned char *() const	{ return (const unsigned char *)c_str(); }
	inline operator unsigned char *() const			{ return (unsigned char *)c_str(); }
	inline operator const std::string &() const		{ return (const std::string &)*this; }
	inline operator const int () const				{ return this->toLong(); }
	inline operator const long () const				{ return this->toLong(); }
	inline operator const double () const			{ return this->toDouble(); }
	inline operator const float () const			{ return this->toFloat(); }
	inline operator const bool () const				{ return (this->empty() ? false : ((this->toLong()) ? true : false)); }

	// assignment operators
	const SString &operator= ( const SString &str );
	const SString &operator= ( const std::string &str );
	const SString &operator= ( const char *str );
	const SString &operator= ( const unsigned char *str );
	const SString &operator= ( unsigned char c );
	const SString &operator= ( char c );
	const SString &operator= ( long l );
	const SString &operator= ( unsigned long l );
	const SString &operator= ( float l );
	const SString &operator= ( double l );

	// subscript operators
#ifdef _WIN32
	const unsigned char operator[] (int num) const;
	unsigned char& operator[] (int num);
#else
	const unsigned char operator[] (size_t num) const;
	unsigned char& operator[] (size_t num);
#endif

	// addition operators
	SString operator+ ( const char *str ) const;
	SString operator+ ( const unsigned char *str ) const;
	SString operator+ ( const std::string &str ) const;
	SString operator+ ( const SString &str ) const;
	SString operator+ ( const char c ) const;
	SString operator+ ( const unsigned char c ) const;
	SString operator+ ( const long l ) const;
	SString operator+ ( const unsigned long l ) const;
	SString operator+ ( const float f ) const;
	SString operator+ ( const double f ) const;

	// compound operators
	const SString &operator+= ( const char *str );
	const SString &operator+= ( const unsigned char *str );
	const SString &operator+= ( const std::string &str );
	const SString &operator+= ( const SString &str );
	const SString &operator+= ( const char c );
	const SString &operator+= ( const unsigned char c );
	const SString &operator+= ( const long l );
	const SString &operator+= ( const unsigned long l );
	const SString &operator+= ( const float f );
	const SString &operator+= ( const double f );

	// comparison operators
	bool operator== ( const char *str ) const;
	bool operator== ( const unsigned char *str ) const;
	bool operator== ( const std::string &str ) const;
	bool operator== ( const SString &str ) const;

	bool operator!= ( const char *str ) const;
	bool operator!= ( const unsigned char *str ) const;
	bool operator!= ( const std::string &str ) const;
	bool operator!= ( const SString &str ) const;

	bool operator !() const;

	bool Compare(const SString &str, bool bCaseSensative = false) const;
	bool Compare(const unsigned char *str, bool bCaseSensative = false) const;
	bool Compare(const char *str, bool bCaseSensative = false) const;

	// file handling
	unsigned char *readToEndOfLine(unsigned char *data);
	char *readToEndOfLine(char *data);
	const SString &readToEndOfLine(FILE *id, int *line, bool upper);

	// tokens
	int countToken(const char *token) const;
	SString token(const char *token, int tok) const;
	SString tokens(const char *token, int from, int to = 0) const;
	SString *tokenise(const char *token, int *max) const;
	SString replaceToken(const char *token, int from, const SString &replace) const;
	SString remToken(const char* token, int from) const;
	SString remTokens(const char* token, int from, int to = -1) const;
	SString word(int word) const;
	SString words(int from, int to = 0) const;
	SString addToken(const char *token, const Utils::SString &str) const;
	
	// find/replacement
	long long findPos(const SString &find, size_t iStartPos = 0) const;
	SString findReplace(const SString &find, const SString &replace ) const;
	SString remove(char c) const;
	const SString &removeChar(char c);
	const SString &removeChar(const char *cs);
	SString findRemove(const SString &find) const;
	SString stripHtml() const;
	SString removeIf(int iChar, char c) const;

	bool containsAny(const SString &str, bool bCaseSensative = false) const;
	bool contains(const SString &str, bool bCaseSensative = false) const;
	bool contains(char c, bool bCaseSensative = false) const;
	bool isin(const SString &str, bool bCaseSensative = false) const;
	bool isin(char c, bool bCaseSensative = false) const;
	int compareVersion(const Utils::SString &v) const;
	bool match(const Utils::SString &pattern) const;

	// sub string
	SString left(long long num) const;
	SString right(long long num) const;
	SString mid(long long start, long long end) const;
	SString between(const SString &before, const SString &after) const;

	bool isNumber(bool integer = false) const;
	bool isCharNumber(int c) const;
	const SString &removeFirstSpace();
	const SString &removeEndSpace();
	const SString &truncate(int iNum);
	const SString &padNumber(int iNum);
	const SString &pad(int iAmoumt, char cWith);

	SString lower() const;
	SString upper() const;
	const SString &toLower();
	const SString &toUpper();

	const SString &toFilename();
	SString asFilename() const;

private:
	void _assign(const wchar_t* str);
	void _assign(const wchar_t* str, size_t len);
	bool _isCharNumber(char c) const;
	Utils::SString::size_type _token_nextPos(const char *token, Utils::SString::size_type curPos) const;
};

SPKEXPORT SString operator+(const char* str1, const SString& str2);
SPKEXPORT SString operator+(const unsigned char* str1, const SString& str2);

typedef SString String;
#pragma warning( pop )

}
