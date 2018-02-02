#    error Time to upgrade. GNU compilers < 2.7 not supported
/**************************************************************************
 * compiler.h -- C++ Compiler Portability Macros
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * $Id: compiler.h,v 1.1.1.1 2007/06/12 10:10:24 oliver Exp $
 *
 **************************************************************************/

/** \file compiler.h
 * A set of defines to encapsulate compiler and platform differences.
 * Please refer to the source code for full documentation on this file.
 *
 * Here is a summary of what this file does.
 *
 *  (1)  Defines macros for some STL includes which may be affected
 *       by file name length limitations.
 *
 *  (2)  Defines macros for some features not supported by all C++ compilers.
 *
 *  (3)  Defines 'explicit' as a null macro if the compiler doesn't support
 *       the explicit keyword.
 *
 *  (4)  Defines 'typename' as a null macro if the compiler doesn't support
 *       the typename keyword.
 *
 *  (5)  Defines bool, true and false if the compiler doesn't do so.
 *
 *  (6)  Defines SG_EXPLICIT_FUNCTION_TMPL_ARGS if the compiler
 *       supports calling a function template by providing its template
 *       arguments explicitly.
 *
 *  (7)  Defines SG_NEED_AUTO_PTR if STL doesn't provide auto_ptr<>.
 *
 *  (8)  Defines SG_NO_ARROW_OPERATOR if the compiler is unable
 *       to support operator->() for iterators.
 *
 *  (9)  Defines SG_USE_EXCEPTIONS if the compiler supports exceptions.
 *       Note: no FlightGear code uses exceptions.
 *
 *  (10) Define SG_NAMESPACES if the compiler supports namespaces.
 *
 *  (11) SG_MATH_FN_IN_NAMESPACE_STD -- not used??
 *
 *  (12) Define SG_HAVE_STD if std namespace is supported.
 *
 *  (13) Defines SG_CLASS_PARTIAL_SPECIALIZATION if the compiler 
 *       supports partial specialization of class templates.
 *
 *  (14) Defines SG_HAVE_STD_INCLUDES to use ISO C++ Standard headers.
 *
 *  (15) Defines SG_HAVE_STREAMBUF if <streambuf> of <streambuf.h> are present.
 *
 *  (16) Define SG_MATH_EXCEPTION_CLASH if math.h defines an exception class
 *       that clashes with the one defined in <stdexcept>.
 */

#ifndef _SG_COMPILER_H
#define _SG_COMPILER_H

/*
 * Helper macro SG_STRINGIZE:
 * Converts the parameter X to a string after macro replacement
 * on X has been performed.
 */
#define SG_STRINGIZE(X) SG_DO_STRINGIZE(X)
#define SG_DO_STRINGIZE(X) #X

#ifdef __GNUC__
#  if __GNUC__ == 2 
#    if __GNUC_MINOR__ < 8

       // g++-2.7.x
#      define STL_ALGORITHM  <algorithm>
#      define STL_FUNCTIONAL <functional>
#      define STL_IOMANIP    <iomanip.h>
#      define STL_IOSTREAM   <iostream.h>
#      define STL_ITERATOR   <iterator.h>
#      define STL_FSTREAM    <fstream.h>
#      define STL_STDEXCEPT  <stdexcept>
#      define STL_STRING     <string>
#      define STL_STRSTREAM  <strstream.h>

#      define SG_NEED_AUTO_PTR
#      define SG_NO_DEFAULT_TEMPLATE_ARGS
#      define SG_INCOMPLETE_FUNCTIONAL
#      define SG_NO_ARROW_OPERATOR

#    elif __GNUC_MINOR__ >= 8

       // g++-2.8.x and egcs-1.x
#      define SG_EXPLICIT_FUNCTION_TMPL_ARGS
#      define SG_NEED_AUTO_PTR
#      define SG_MEMBER_TEMPLATES
#      define SG_NAMESPACES
#      define SG_HAVE_STD
#      define SG_HAVE_STREAMBUF
#      define SG_CLASS_PARTIAL_SPECIALIZATION

#      define STL_ALGORITHM  <algorithm>
#      define STL_FUNCTIONAL <functional>
#      define STL_IOMANIP    <iomanip>
#      define STL_IOSTREAM   <iostream>
#      define STL_ITERATOR   <iterator>
#      define STL_FSTREAM    <fstream>
#      define STL_STDEXCEPT  <stdexcept>
#      define STL_STRING     <string>
#      define STL_STRSTREAM  <strstream>

#    endif
#  elif __GNUC__ >= 3
       // g++-3.0.x
#      define SG_EXPLICIT_FUNCTION_TMPL_ARGS
#      define SG_NEED_AUTO_PTR
#      define SG_MEMBER_TEMPLATES
#      define SG_NAMESPACES
#      define SG_HAVE_STD
#      define SG_HAVE_STREAMBUF
#      define SG_CLASS_PARTIAL_SPECIALIZATION
#      define SG_HAVE_STD_INCLUDES

#      define STL_ALGORITHM  <algorithm>
#      define STL_FUNCTIONAL <functional>
#      define STL_IOMANIP    <iomanip>
#      define STL_IOSTREAM   <iostream>
#      define STL_ITERATOR   <iterator>
#      define STL_FSTREAM    <fstream>
#      define STL_STDEXCEPT  <stdexcept>
#      define STL_STRING     <string>
#      define STL_STRSTREAM  <strstream>
#  else
#    error Time to upgrade. GNU compilers < 2.7 not supported
#  endif

#  define SG_COMPILER_STR "GNU C++ version " SG_STRINGIZE(__GNUC__) "." SG_STRINGIZE(__GNUC_MINOR__)

#endif // __GNUC__

/* KAI C++ */
#if defined(__KCC)

#  define SG_NAMESPACES
#  define SG_HAVE_STD
#  define SG_HAVE_STREAMBUF
#  define SG_HAVE_TRAITS
#  define SG_HAVE_STD_INCLUDES

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_IOSTREAM   <iostream>
#  define STL_ITERATOR   <iterator>
#  define STL_FSTREAM    <fstream>
#  define STL_STDEXCEPT  <stdexcept>
#  define STL_STRING     <string>
#  define STL_STRSTREAM  <strstream>

#  define SG_COMPILER_STR "Kai C++ version " SG_STRINGIZE(__KCC_VERSION)

#endif // __KCC

//
// Metrowerks 
//
#if defined(__MWERKS__)
/*
  CodeWarrior compiler from Metrowerks, Inc.
*/
#  define SG_HAVE_TRAITS
#  define SG_HAVE_STD_INCLUDES
#  define SG_HAVE_STD
#  define SG_NAMESPACES

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_IOSTREAM   <iostream>
#  define STL_ITERATOR   <iterator>
#  define STL_FSTREAM    <fstream>
#  define STL_STDEXCEPT  <stdexcept>
#  define STL_STRING     <string>

// Temp:
#  define bcopy(from, to, n) memcpy(to, from, n)

// -rp- please use FG_MEM_COPY everywhere !
// #define FG_MEM_COPY(to,from,n) memcpy(to, from, n)

// -dw- currently used glut has no game mode stuff
#  define GLUT_WRONG_VERSION

#  define SG_COMPILER_STR "Metrowerks CodeWarrior C++ version " SG_STRINGIZE(__MWERKS__)

#endif // __MWERKS__

//
// Microsoft compilers.
//
#ifdef _MSC_VER
#  define bcopy(from, to, n) memcpy(to, from, n)
#  define FG_MEM_COPY(to,from,n) memcpy(to, from, n)

#  if _MSC_VER >= 1200  // msvc++ 6.0 or greater
#    define SG_NAMESPACES
#    define SG_HAVE_STD
#    define SG_HAVE_STD_INCLUDES
#    define SG_HAVE_STREAMBUF

#    define STL_ALGORITHM  <algorithm>
#    define STL_FUNCTIONAL <functional>
#    define STL_IOMANIP    <iomanip>
#    define STL_IOSTREAM   <iostream>
#    define STL_ITERATOR   <iterator>
#    define STL_FSTREAM    <fstream>
#    define STL_STDEXCEPT  <stdexcept>
#    define STL_STRING     <string>
#    define STL_STRSTREAM  <strstream>

#    define isnan _isnan
#  if _MSC_VER < 1900  // msvc140 has fixed this
#    define snprintf _snprintf
#  endif

#    pragma warning(disable: 4786) // identifier was truncated to '255' characters
#    pragma warning(disable: 4244) // conversion from double to float
#    pragma warning(disable: 4305) //

#  else
#    error What version of MSVC++ is this?
#  endif

#    define SG_COMPILER_STR "Microsoft Visual C++ version " SG_STRINGIZE(_MSC_VER)

#endif // _MSC_VER

#ifdef __BORLANDC__
# if defined(HAVE_SGI_STL_PORT)

// Use quotes around long file names to get around Borland's include hackery

#  define STL_ALGORITHM  "algorithm"
#  define STL_FUNCTIONAL "functional"

#  define SG_MATH_EXCEPTION_CLASH

# else

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_STDEXCEPT  <stdexcept>
#  define STL_STRSTREAM  <strstream>

#  define SG_INCOMPLETE_FUNCTIONAL

# endif // HAVE_SGI_STL_PORT

#  define STL_IOSTREAM   <iostream>
#  define STL_ITERATOR   <iterator>
#  define STL_FSTREAM    <fstream>
#  define STL_STRING     <string>
#  define SG_NO_DEFAULT_TEMPLATE_ARGS
#  define SG_NAMESPACES
// #  define SG_HAVE_STD

#  define SG_COMPILER_STR "Borland C++ version " SG_STRINGIZE(__BORLANDC__)

#endif // __BORLANDC__

//
// Native SGI compilers
//

#if defined ( sgi ) && !defined( __GNUC__ )
#  define SG_HAVE_NATIVE_SGI_COMPILERS

#  define SG_EXPLICIT_FUNCTION_TMPL_ARGS
#  define SG_CLASS_PARTIAL_SPECIALIZATION
#  define SG_NEED_AUTO_PTR
#  define SG_MEMBER_TEMPLATES
#  define SG_NAMESPACES
#  define SG_HAVE_STD
#  define SG_HAVE_STREAMBUF
#  define SG_HAVE_TRAITS
#  define SG_HAVE_STD_INCLUDES

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_IOSTREAM   <iostream>
#  define STL_ITERATOR   <iterator>
#  define STL_FSTREAM    <fstream>
#  define STL_STDEXCEPT  <stdexcept>
#if (_COMPILER_VERSION < 740)
#  define STL_STRING     <irix_string>
#else
#  define STL_STRING     <string>
#endif
#  define STL_STRSTREAM  <strstream>

#pragma set woff 1001,1012,1014,1116,1155,1172,1174
#pragma set woff 1401,1460,1551,1552,1681

#ifdef __cplusplus
#pragma set woff 1682,3303
#if (_COMPILER_VERSION >= 740)
#  pragma set woff 3624
#endif
#endif

#  define SG_COMPILER_STR "SGI MipsPro compiler version " SG_STRINGIZE(_COMPILER_VERSION)

#endif // Native SGI compilers


#if defined (__sun)
#  include <strings.h>
#  include <memory.h>
#  if defined ( __cplusplus )
     // typedef unsigned int size_t;
     extern "C" {
       extern void *memmove(void *, const void *, size_t);
     }
#  else
     extern void *memmove(void *, const void *, size_t);
#  endif // __cplusplus

#  if  !defined( __GNUC__ )
#   define SG_COMPILER_STR "Sun compiler version " SG_STRINGIZE(__SUNPRO_CC)
#  endif

#endif // sun

//
// Intel C++ Compiler
//
#if defined(__ICC) || defined (__ECC)
#  define SG_NAMESPACES
#  define SG_HAVE_STD
#  define SG_HAVE_STREAMBUF
#  define SG_HAVE_TRAITS
#  define SG_HAVE_STD_INCLUDES

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_IOSTREAM   <iostream>
#  define STL_ITERATOR   <iterator>
#  define STL_FSTREAM    <fstream>
#  define STL_STDEXCEPT  <stdexcept>
#  define STL_STRING     <string>
#  define STL_STRSTREAM  <strstream>

#  define SG_COMPILER_STR "Intel C++ version " SG_STRINGIZE(__ICC)

#endif // __ICC

//
// Platform dependent gl.h and glut.h definitions
//

#ifdef __APPLE__
#  define SG_GL_H <OpenGL/gl.h>
#  define SG_GLX_H <AGL/agl.h>
#  define SG_GLU_H <OpenGL/glu.h>
#  define SG_GLEXT_H <OpenGL/glext.h>
#  define SG_GLUT_H <GLUT/glut.h>

inline int (isnan)(double r) { return !(r <= 0 || r >= 0); }
#else
#  define SG_GL_H <GL/gl.h>
#  define SG_GLX_H <GL/glx.h>
#  define SG_GLU_H <GL/glu.h>
#  define SG_GLEXT_H <GL/glext.h>
#  define SG_GLUT_H <GL/glut.h>
#endif


//
// No user modifiable definitions beyond here.
//

#ifdef SG_NEED_EXPLICIT
#  define explicit
#endif

#ifdef SG_NEED_TYPENAME
#  define typename
#endif

#ifdef SG_NEED_MUTABLE
#  define mutable
#endif

#ifdef SG_NEED_BOOL
   typedef int bool;
#  define true 1
#  define false 0
#endif

#ifdef SG_EXPLICIT_FUNCTION_TMPL_ARGS
#  define SG_NULL_TMPL_ARGS <>
#else
#  define SG_NULL_TMPL_ARGS
#endif

#ifdef SG_CLASS_PARTIAL_SPECIALIZATION
# define SG_TEMPLATE_NULL template<>
#else
# define SG_TEMPLATE_NULL
#endif

// SG_NO_NAMESPACES is a hook so that users can disable namespaces
// without having to edit library headers.
#if defined(SG_NAMESPACES) && !defined(SG_NO_NAMESPACES)
#   define SG_NAMESPACE(X) namespace X {
#   define SG_NAMESPACE_END }
#   define SG_USING_NAMESPACE(X) using namespace X
# else
#   define SG_NAMESPACE(X)
#   define SG_NAMESPACE_END
#   define SG_USING_NAMESPACE(X)
#endif

/** \def SG_USING_STD(x)
 *  Expands to using std::x if SG_HAVE_STD is defined 
 */
# ifdef SG_HAVE_STD
#  define SG_USING_STD(X) using std::X
#  define STD std
# else
#  define SG_USING_STD(X) 
#  define STD
# endif

// Additional <functional> implementation from SGI STL 3.11
// Adapter function objects: pointers to member functions
#ifdef SG_INCOMPLETE_FUNCTIONAL

template <class _Ret, class _Tp>
class const_mem_fun_ref_t
#ifndef __BORLANDC__
    : public unary_function<_Tp,_Ret>
#endif // __BORLANDC__
{
public:
  explicit const_mem_fun_ref_t(_Ret (_Tp::*__pf)() const) : _M_f(__pf) {}
  _Ret operator()(const _Tp& __r) const { return (__r.*_M_f)(); }
private:
  _Ret (_Tp::*_M_f)() const;
};

template <class _Ret, class _Tp>
inline const_mem_fun_ref_t<_Ret,_Tp> mem_fun_ref(_Ret (_Tp::*__f)() const)
  { return const_mem_fun_ref_t<_Ret,_Tp>(__f); }

#endif // SG_INCOMPLETE_FUNCTIONAL

#endif // _SG_COMPILER_H

