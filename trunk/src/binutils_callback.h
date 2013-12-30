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

#ifndef _BINUTILS_CALLBACK_H
#define _BINUTILS_CALLBACK_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "binutils_tools.h"


// ============================================================================
// >> FUNCTIONS
// ============================================================================
// Forward declarations
class CCallback;

object CallCallback(CCallback* pCallback);

template<class T>
T CallbackCaller(CCallback* pCallback)
{
    return extract<T>(CallCallback(pCallback));
}

template<>
void CallbackCaller(CCallback* pCallback)
{
    CallCallback(pCallback);
}

template<>
void* CallbackCaller(CCallback* pCallback)
{
    return (void *) ExtractPyPtr(CallCallback(pCallback));
}


// ============================================================================
// >> CLASSES
// ============================================================================
class CCallback: public CFunction
{
public:
    CCallback(object oCallback, Convention_t eConv, char* szParams);
    ~CCallback();

    int      GetPopSize();
    int      GetArgumentCount();
    Param_t* GetArgument(int iIndex);
    void     Free();

    template<class T>
    T GetArgument(int iIndex)
    {
    #ifdef _WIN32
        if (m_eConv == CONV_THISCALL && iIndex == 0)
            return *(T *) &m_ulECX;
    #endif

        unsigned long reg = (m_ESP.m_ulAddr) + GetArgument(iIndex)->m_iOffset + 8;
        return *(T *) reg;
    }

public:
    // For variadic functions
    CPointer      m_ESP;
    unsigned long m_ulECX;
    object        m_oCallback;
    Param_t*      m_pParams;
    Param_t*      m_pRetParam;
};

#endif // _BINUTILS_CALLBACK_H