#pragma once
//------------------------------------------------------------------------------
/**
    @file core/debug.h

    Nebula debug macros.
  
    n_assert()  - the vanilla assert() Macro
    n_verify()  - like assert() except that the statement in parens is simply evaluated, unchecked, if __NEBULA_NO_ASSERT is set
    n_assert2() - an assert() plus a message from the programmer
    
    @copyright
    (C) 2002 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"

void n_printf(const char *, ...) __attribute__((format(printf,1,2)));
void n_error(const char*, ...) __attribute__((format(printf,1,2)));
void n_dbgout(const char*, ...) __attribute__((format(printf,1,2)));
void n_warning(const char*, ...) __attribute__((format(printf,1,2)));
void n_confirm(const char*, ...) __attribute__((format(printf,1,2)));
void n_sleep(double);
void n_barf(const char *, const char *, int);
void n_barf2(const char*, const char*, const char*, int);
void n_barf_fmt(const char *, const char *, const char *, int, ...);
void n_cough(const char*, const char*, int);
void n_cough2(const char*, const char*, const char*, int);
void n_cough_fmt(const char*, const char*, const char*, int, ...);
void n_break();

// backward compatibility
#define n_message n_confirm

#if __NEBULA_NO_ASSERT__
#define n_assert(exp) if(!(exp)){}
#define n_assert2(exp, msg) if(!(exp)){}
#define n_assert_fmt(exp, msg, ...) if(!(exp)){}
#define n_verify(exp) (exp)
#define n_verify2(exp,imsg) (exp)
#define n_warn(exp) if (!(exp)) {}
#define n_warn2(exp, msg) if (!(exp)) {}
#define n_warn_fmt(exp,  msg, ...) if (!(exp)) {}
#if __WIN32__
#define n_dxtrace(hr, msg)
#endif
#define n_static_assert(exp)
#else
#define n_assert(exp) { if (!(exp)) n_barf(#exp, __FILE__, __LINE__); }
#define n_assert2(exp, msg) { if (!(exp)) n_barf2(#exp, msg, __FILE__, __LINE__); }
#define n_assert_fmt(exp, msg, ...) { if (!(exp)) n_barf_fmt(#exp, msg, __FILE__, __LINE__, __VA_ARGS__); }
#define n_warn(exp) { if (!(exp)) n_cough(#exp, __FILE__, __LINE__); }
#define n_warn2(exp, msg) { if (!(exp)) n_cough2(#exp, msg, __FILE__, __LINE__); }
#define n_warn_fmt(exp,  msg, ...) { if (!(exp)) n_cough_fmt(#exp, msg, __FILE__, __LINE__, __VA_ARGS__); }
#define n_crash(msg) { n_error("*** NEBULA CRASH ***\n%s(%d)\nmessage: %s\n", __FILE__, __LINE__, msg); }
#define n_static_assert(exp) { int _x[ 2*((exp) != 0)-1 ]; (void)(_x[0]=0);}
#if __WIN32__
// dx9 specific: check HRESULT and display DX9 specific message box
#define n_dxtrace(hr, msg) { if (FAILED(hr)) DXTrace(__FILE__,__LINE__,hr,msg,true); }
#endif
#endif

//------------------------------------------------------------------------------
