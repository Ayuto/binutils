/**
* =============================================================================
* binutils
* Copyright(C) 2013 Ayuto. All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef _BINUTILS_TOOLS_H
#define _BINUTILS_TOOLS_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include <malloc.h>
#include "binutils_macros.h"
#include "dyncall.h"

#include "DynamicHooks.h"
using namespace DynamicHooks;

#include "boost/python.hpp"
using namespace boost::python;


// ============================================================================
// >> DEINITIONS
// ============================================================================
#define MAX_PARAMETER_STR 32


// ============================================================================
// >> Convention_t enum
// ============================================================================
inline int GetDynCallConvention(Convention_t eConv)
{
    switch (eConv)
    {
        case CONV_CDECL: return DC_CALL_C_DEFAULT;
        case CONV_THISCALL:
        #ifdef _WIN32
            return DC_CALL_C_X86_WIN32_THIS_MS;
        #else
            return DC_CALL_C_X86_WIN32_THIS_GNU;
        #endif
        case CONV_STDCALL: return DC_CALL_C_X86_WIN32_STD;
    }

    // TODO: Throw exception
    return 0;
}

// ============================================================================
// >> CLASSES
// ============================================================================
class CFunction;

// CPointer class
class CPointer
{
public:
    CPointer(unsigned long ulAddr = 0);
    
    operator unsigned long() const { return m_ulAddr; }
    
    // Implement some operators
    template<class T>
    const CPointer operator+(T const& rhs)
    { return CPointer(m_ulAddr + rhs); }
    
    template<class T>
    const CPointer operator-(T const& rhs)
    { return CPointer(m_ulAddr - rhs); }
    
    template<class T>
    const CPointer operator+=(T const& rhs)
    { m_ulAddr += rhs; return *this; }
    
    template<class T>
    const CPointer operator-=(T const& rhs)
    { m_ulAddr -= rhs; return *this; }
    
    bool operator!()
    { return !m_ulAddr; }
    
    template<class T>
    bool operator==(T const& rhs)
    { return m_ulAddr == rhs; }
    
    template<class T>
    bool operator!=(T const& rhs)
    { return m_ulAddr != rhs; }

    template<class T>
    T Get(int iOffset = 0)
    {
        if (!m_ulAddr)
            BOOST_RAISE_EXCEPTION(PyExc_ValueError, "Pointer is NULL.")

        return *(T *) (m_ulAddr + iOffset);
    }

    template<class T>
    void Set(T value, int iOffset = 0)
    {
        if (!m_ulAddr)
            BOOST_RAISE_EXCEPTION(PyExc_ValueError, "Pointer is NULL.")

        unsigned long newAddr = m_ulAddr + iOffset;
        *(T *) newAddr = value;
    }

    const char *        GetStringArray(int iOffset = 0);
    void                SetStringArray(char* szText, int iOffset = 0, int iSize = 0);
    
    CPointer*           GetPtr(int iOffset = 0);
    void                SetPtr(object oPtr, int iOffset = 0);

    unsigned long       GetSize();
    unsigned long       GetAddress() { return m_ulAddr; }

    bool                IsOverlapping(object oOther, unsigned long ulNumBytes);
    CPointer*           SearchBytes(object oBytes, unsigned long ulNumBytes);

    int                 Compare(object oOther, unsigned long ulNum);
    void                Copy(object oDest, unsigned long ulNumBytes);
    void                Move(object oDest, unsigned long ulNumBytes);

    CPointer*           GetVirtualFunc(int iIndex);

    void                Realloc(unsigned long ulSize);
    void                Dealloc();

    CFunction*          MakeFunction(Convention_t eConv, char* szParams, PyObject* pConverter = NULL);
    CFunction*          MakeVirtualFunction(int iIndex, Convention_t eConv, char* szParams, PyObject* pConverter = NULL);

public:
    unsigned long m_ulAddr;
};


// CFunction class
class CFunction: public CPointer
{
public:
    CFunction(unsigned long ulAddr, Convention_t eConv, char* szParams, PyObject* pConverter = NULL);

    object __call__(object args);
    object CallTrampoline(object args);

    void Hook(HookType_t eType, PyObject* pCallable);
    void Unhook(HookType_t eType, PyObject* pCallable);

    void AddPreHook(PyObject* pCallable);
    void AddPostHook(PyObject* pCallable);

    void RemovePreHook(PyObject* pCallable);
    void RemovePostHook(PyObject* pCallable);
    
    void SetParams(char* szPrams);
    const char* GetParams();

public:
    char         m_szParams[MAX_PARAMETER_STR];
    Convention_t m_eConv;
    object       m_oConverter;
};


// ============================================================================
// >> FUNCTIONS
// ============================================================================
int GetError();

inline unsigned long ExtractPyPtr(object obj)
{
    // Try to get a CPointer representation at first
    try
    {
        CPointer* pPtr = extract<CPointer *>(obj);
        return pPtr->GetAddress();
    }
    catch(...)
    {
        PyErr_Clear();
    }

    // If that fails, try to extract an unsigned long
    return extract<unsigned long>(obj);
}

inline CPointer* Alloc(unsigned long ulSize)
{
    return new CPointer((unsigned long) malloc(ulSize));
}

inline unsigned char* GetByteRepr(object obj)
{
    unsigned char* byterepr = NULL;
#if PYTHON_VERSION == 3
    // This is required because there's no straight way to get a string from a python
    // object from boost (without using the stl).
    PyArg_Parse(obj.ptr(), "y", &byterepr);
    if (!byterepr)
        return NULL;
#else
    char* tempstr = extract<char *>(obj);
    byterepr = (unsigned char *) tempstr;
#endif
    return byterepr;
}

#endif // _BINUTILS_TOOLS_H