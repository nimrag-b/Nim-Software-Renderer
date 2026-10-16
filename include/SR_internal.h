#ifndef _SR_INTERNAL_H_
#define _SR_INTERNAL_H_

#ifdef _WIN32
#define SR_DLLFUNC __declspec(dllexport)
#else
#define SR_DLLFUNC
#endif

#endif //_SR_INTERNAL_H_