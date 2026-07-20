#ifndef __XBOD_H__
#define __XBOD_H__

#include <stdlib.h>

#define BOB_SECTION_NAME_CUT_BEGIN 0x43555431 // CUT1
#define BOB_SECTION_NAME_CUT_END 0x2f435554 // /CU1

class CXBod
{
public:
	static const int HeaderStart	= BOB_SECTION_NAME_CUT_BEGIN; 
	static const int HeaderEnd		= BOB_SECTION_NAME_CUT_END;

	CXBod() { m_sData = 0; m_lSize = 0; m_bUsedMalloc = false; }
	~CXBod()
	{
		if ( m_sData )
		{
			if ( m_bUsedMalloc )
				free(m_sData);
			else
				delete m_sData;
		}
	}

	int LoadData(const unsigned char *data, const size_t size);

private:
	bool			 m_bUsedMalloc;
	unsigned char	*m_sData;
	size_t			 m_lSize;
};

#endif //__XBOD_H__