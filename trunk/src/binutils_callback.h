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

object CallCallback(CCallback* pCallback, unsigned long ulEBP, unsigned long ulECX);

template<class T>
T CallbackCaller(CCallback* pCallback, unsigned long ulEBP, unsigned long ulECX)
{
    return extract<T>(CallCallback(pCallback, ulEBP, ulECX));
}

template<>
inline void CallbackCaller(CCallback* pCallback, unsigned long ulEBP, unsigned long ulECX)
{
    CallCallback(pCallback, ulEBP, ulECX);
}

template<>
inline void* CallbackCaller(CCallback* pCallback, unsigned long ulEBP, unsigned long ulECX)
{
    return (void *) ExtractPyPtr(CallCallback(pCallback, ulEBP, ulECX));
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

public:
    // For variadic functions
    object        m_oCallback;
    Param_t*      m_pParams;
    Param_t*      m_pRetParam;
};

#endif // _BINUTILS_CALLBACK_H