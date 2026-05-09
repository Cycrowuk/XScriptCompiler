#pragma once

#include <string>
#include <vector>

enum class StringCompare {
	Same		= 0,
	Newer		= 1,
	Older		= -1,
};

#pragma warning( push )
#pragma warning( disable : 4251)

/**
 * String wrapper class
 */
namespace XLib {
#define SPKEXPORT 

class SPKEXPORT String : public std::wstring
{
private:
	std::string _narrowString;

public:
	// constructors
	String(void);
	String(const char *str);
	String(const unsigned char *str);
	String(const std::string &str);
	String(const char c);
	String(const unsigned char c);
	String(const String& str);
	String(const wchar_t* str);
	String(const wchar_t c);
	String(const std::wstring& str);
	String(long l);
	String(unsigned long l);
	String(float f);
	String(double f);

	virtual ~String(void);

	// conversion functions
	void fromString(const std::string& str);
	void fromLong(long l);
	const String &fromFloat(float f, int dp = -1);
	void fromDouble(double f);
	const String &fromDouble(double f, int dp);
	const String& format(const char* sFormat, ...);
	const String& format(const wchar_t* sFormat, ...);
	const String args(const String *args, int max) const;
	const String &arg(const String &s1);
	const String &arg(const String &s1, const String &s2);
	const String &arg(const String &s1, const String &s2, const String &s3);
	const String &arg(const String &s1, const String &s2, const String &s3, const String &s4);
	const String &arg(const String &s1, const String &s2, const String &s3, const String &s4, const String &s5);
	
	static String PadNumber(long iNum, int iAmount);
	static String Number(long l) { return String(l); }
	static String Format(const char* sFormat, ...);
	static String Format(const wchar_t* sFormat, ...);
	static String Null();
	static String FromFloat(float f, int dp = -1);

	bool toBool() const;
	long toLong() const;
	int toInt() const;
	double toDouble() const;
	float toFloat() const;
	const std::string& toString() const;

	// casting operators
	inline operator const wchar_t *() const			{ return (const wchar_t *)this->c_str(); }
	inline operator wchar_t *() const				{ return (wchar_t *)this->c_str(); }
	inline operator const char *() const			{ return (const char *)this->toString().c_str(); }
	inline operator char *() const					{ return (char *)this->toString().c_str(); }
	inline operator const unsigned char *() const	{ return (const unsigned char *)this->toString().c_str(); }
	inline operator unsigned char *() const			{ return (unsigned char *)this->toString().c_str(); }
	inline operator const std::string &() const		{ return this->toString(); }
	inline operator const std::wstring &() const	{ return (*this); }
	inline operator const int () const				{ return this->toLong(); }
	inline operator const long () const				{ return this->toLong(); }
	inline operator const double () const			{ return this->toDouble(); }
	inline operator const float () const			{ return this->toFloat(); }
	inline operator const bool () const				{ return (this->empty() ? false : ((this->toLong()) ? true : false)); }

	const char* n_str() const { return (const char *)this->toString().c_str(); }

	// assignment operators
	const String &operator= ( const String &str );
	const String& operator= (const std::wstring& str);
	const String& operator= (const wchar_t* str);
	const String& operator= (const std::string& str);
	const String& operator= (const char* str);
	const String &operator= ( const unsigned char *str );
	const String &operator= ( unsigned char c );
	const String &operator= ( char c );
	const String &operator= ( long l );
	const String &operator= ( unsigned long l );
	const String &operator= ( float l );
	const String &operator= ( double l );

	// subscript operators
	const wchar_t operator[](size_t num)  const;
	wchar_t &operator[](size_t num);

	// addition operators
	String operator+ ( const char *str ) const;
	String operator+ ( const wchar_t *str ) const;
	String operator+ ( const unsigned char *str ) const;
	String operator+ ( const std::wstring &str ) const;
	String operator+ (const std::string& str) const;
	String operator+ ( const String &str ) const;
	String operator+ ( const char c ) const;
	String operator+ ( const unsigned char c ) const;
	String operator+ ( const long l ) const;
	String operator+ ( const unsigned long l ) const;
	String operator+ ( const float f ) const;
	String operator+ ( const double f ) const;

	// compound operators
	const String& operator+= (const char* str);
	const String &operator+= ( const wchar_t *str );
	const String &operator+= ( const unsigned char *str );
	const String &operator+= ( const std::string &str );
	const String &operator+= ( const std::wstring &str );
	const String &operator+= ( const String &str );
	const String &operator+= ( const char c );
	const String &operator+= ( const unsigned char c );
	const String &operator+= ( const long l );
	const String &operator+= ( const unsigned long l );
	const String &operator+= ( const float f );
	const String &operator+= ( const double f );

	// comparison operators
	bool operator== ( const char *str ) const;
	bool operator== (const wchar_t* str) const;
	bool operator== ( const unsigned char *str ) const;
	bool operator== ( const std::string &str ) const;
	bool operator== (const std::wstring& str) const;
	bool operator== ( const String &str ) const;

	bool operator!= ( const char *str ) const;
	bool operator!= ( const wchar_t *str ) const;
	bool operator!= ( const unsigned char *str ) const;
	bool operator!= ( const std::string &str ) const;
	bool operator!= ( const std::wstring &str ) const;
	bool operator!= ( const String &str ) const;

	bool operator !() const;

	bool Compare(const String &str, bool bCaseSensative = false) const;
	bool Compare(const wchar_t* str, bool bCaseSensative = false) const;
	bool Compare(const char* str, bool bCaseSensative = false) const;
	bool Compare(const unsigned char* str, bool bCaseSensative = false) const;
	bool Compare(const std::string &str, bool bCaseSensative = false) const;

	// file handling
	unsigned char *readToEndOfLine(unsigned char *data);
	char *readToEndOfLine(char *data);
	const String &readToEndOfLine(FILE *id, int *line, bool upper);

	// tokens
	unsigned int countToken(const XLib::String& token) const;
	unsigned int countToken(const char *token) const;
	String token(const String &token, int tok) const;
	String tokens(const String &token, int from, int to = 0) const;
	std::vector<XLib::String> tokenise(const XLib::String& token) const;
	String replaceToken(const String &token, int from, const String &replace) const;
	String remToken(const String &token, int t) const;
	String addToken(const String& token, const XLib::String& str) const;
	String word(int word) const;
	String words(int from, int to = 0) const;
	String addWord(const XLib::String& str) const;
	String remWord(int t) const;
	String replaceWord(int t, const XLib::String& str) const;

	// find/replacement
	size_t findPos(const String &find, size_t iStartPos = 0) const;
	String findReplace(const String &find, const String &replace ) const;
	String remove(char c) const;
	const String &removeChar(char c);
	const String &removeChar(const char *cs);
	String findRemove(const String &find) const;
	String stripHtml() const;
	String removeIf(int iChar, wchar_t c) const;

	bool containsAny(const String &str, bool bCaseSensative = false) const;
	bool contains(const String &str, bool bCaseSensative = false) const;
	bool contains(char c, bool bCaseSensative = false) const;
	bool isin(const String &str, bool bCaseSensative = false) const;
	bool isin(char c, bool bCaseSensative = false) const;
	StringCompare compareVersion(const XLib::String &v) const;
	bool match(const XLib::String &pattern) const;

	// sub string
	String left(long num) const;
	String right(int num) const;
	String mid(int start, int end) const;
	String between(const String &before, const String &after) const;

	bool isInteger() const;
	bool isNumber(bool integer = false) const;
	bool isCharNumber(size_t c) const;
	const String &removeFirstSpace();
	const String &removeEndSpace();
	const String &truncate(int iNum);
	const String &padNumber(int iNum);
	const String &pad(int iAmoumt, wchar_t cWith);

	String lower() const;
	String upper() const;
	const String &toLower();
	const String &toUpper();

	const String &toFilename();
	String asFilename() const;

private:
	bool _isCharNumber(wchar_t c) const;
	XLib::String::size_type _token_nextPos(const wchar_t* token, XLib::String::size_type curPos) const;
	XLib::String::size_type _token_nextPos(const char* token, XLib::String::size_type curPos) const;
};

SPKEXPORT String operator+(const char *str1, const String &str2);

typedef std::vector<XLib::String> StringList;

#pragma warning( pop )

}
