#ifndef __SPKDEF_H__
#define __SPKDEF_H__

#define CLEANSPLIT(s, n)	if ( n == 1 ) delete s; else if ( n > 1 ) delete [] s;

#endif //__SPKDEF_H__