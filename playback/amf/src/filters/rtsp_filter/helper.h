#ifndef __HELPER_H__
#define __HELPER_H__

class CAutoString
{
	struct Srep;
	Srep *rep;
public:
	CAutoString();
	CAutoString(int strSize);
	CAutoString(const char*);
	CAutoString(const CAutoString&);
	CAutoString& operator=(const CAutoString&);
	CAutoString& operator+=(const CAutoString&);
	~CAutoString();
	char* GetStr() const;
	int StrLen() const;
};

CAutoString operator+(const CAutoString&, const CAutoString&);

#endif
