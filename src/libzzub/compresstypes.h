#pragma once

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;

#define FALSE 0
#define TRUE 1
typedef signed char INT8, *PINT8;
typedef signed short INT16, *PINT16;
typedef signed int INT32, *PINT32;
typedef unsigned char UINT8, *PUINT8;
typedef unsigned short UINT16, *PUINT16;
typedef unsigned int UINT32, *PUINT32;
typedef BOOL *LPBOOL;
typedef BYTE *LPBYTE;
typedef int *LPINT;
typedef WORD *LPWORD;
typedef long *LPLONG;
typedef DWORD *LPDWORD;
typedef void *LPVOID;

typedef char* LPSTR;
typedef const char* LPCSTR;

#define LocalAlloc(uFlags, uBytes) calloc(1, uBytes)
#define LocalFree(hMem) free(hMem)
