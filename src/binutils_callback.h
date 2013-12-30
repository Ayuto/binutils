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

#include "AsmJit.h"
using namespace AsmJit;


// ============================================================================
// >> CLASSES
// ============================================================================
class CCallback: public CPointer
{
public:
    CCallback(object oCallback, void* pFunc)
    {
        m_ulAddr    = (unsigned long) pFunc;
        m_oCallback = oCallback;
    }

    void Free()
    {
        // TODO: Figure out how to use std::free() on the generated code
        MemoryManager::getGlobal()->free((void *) m_ulAddr);
        m_ulAddr = 0;
    }

    CPointer GetThisPtr()
    {
        #ifdef _WIN32
            return m_ECX;
        #endif

        return *m_ESP.GetPtr(8);
    }

public:
    object   m_oCallback;
    CPointer m_ESP;
    CPointer m_ECX;
};


// ============================================================================
// >> FUNCTIONS
// ============================================================================
template<class T>
T CallCallback(CCallback* pCallback)
{
    return extract<T>(pCallback->m_oCallback(ptr(pCallback)));
}

template<>
void CallCallback(CCallback* pCallback)
{
    pCallback->m_oCallback(ptr(pCallback));
}

template<>
void* CallCallback(CCallback* pCallback)
{
    return (void *) ExtractPyPtr(pCallback->m_oCallback(ptr(pCallback)));
}

template<class T>
CCallback* CreateCallback(object oCallback, int iPopSize = 0)
{
    // Create a new callback object
    CCallback* pCallback = new CCallback(oCallback, NULL);

    // Create the C++ callback;
    Assembler a;

    // Epilog
    a.push(ebp);
    a.mov(ebp, esp);

    // Save esp and ecx, so the Python callback can access the parameters
    a.mov(dword_ptr_abs(&pCallback->m_ESP.m_ulAddr), esp);
    a.mov(dword_ptr_abs(&pCallback->m_ECX.m_ulAddr), ecx);

    // Call callback caller
    a.push(imm((sysint_t) pCallback));
    a.call((void *) &CallCallback<T>);
    a.add(esp, imm(4));

    // Prolog
    a.mov(esp, ebp);
    a.pop(ebp);

    // Return
    a.ret(imm(iPopSize));
    
    // Set the function's address
    pCallback->m_ulAddr = (unsigned long) a.make();

    return pCallback;
}

#endif // _BINUTILS_CALLBACK_H