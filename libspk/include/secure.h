#ifndef __SECURE_H__
#define __SECURE_H__

#define SPRINTBUFFER 100

#ifdef CY_USESECURE
#define SPRINTF(a,b) sprintf_s ( a, SPRINTBUFFER, b
#else
#define SPRINTF(a,b) sprintf ( a, b 
#endif

#endif 
