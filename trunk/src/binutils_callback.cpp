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

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "binutils_callback.h"

#include "AsmJit.h"
using namespace AsmJit;

#include "utilities.h"
#include "DynamicHooks.h"
using namespace DynamicHooks;


// ============================================================================
// >> CLASSES
// ============================================================================
CCallback::CCallback(object oCallback, Convention_t eConv, char* szParams)
    : CFunction(0, eConv, szParams)
{
    m_eConv = eConv;
    m_oCallback = oCallback;

    // Parse the parameter string
    m_pParams = new Param_t;
    m_pRetParam = new Param_t;
    ParseParams(eConv, szParams, m_pParams, m_pRetParam);

    // Find the proper callback caller function
    void* pCallCallbackFunc = NULL;
    switch(m_pRetParam->m_cParam)
    {
        case SIGCHAR_VOID:      pCallCallbackFunc = (void *) &CallbackCaller<void>; break;
        case SIGCHAR_BOOL:      pCallCallbackFunc = (void *) &CallbackCaller<bool>; break;
        case SIGCHAR_CHAR:      pCallCallbackFunc = (void *) &CallbackCaller<char>; break;
        case SIGCHAR_UCHAR:     pCallCallbackFunc = (void *) &CallbackCaller<unsigned char>; break;
        case SIGCHAR_SHORT:     pCallCallbackFunc = (void *) &CallbackCaller<short>; break;
        case SIGCHAR_USHORT:    pCallCallbackFunc = (void *) &CallbackCaller<unsigned short>; break;
        case SIGCHAR_INT:       pCallCallbackFunc = (void *) &CallbackCaller<int>; break;
        case SIGCHAR_UINT:      pCallCallbackFunc = (void *) &CallbackCaller<unsigned int>; break;
        case SIGCHAR_LONG:      pCallCallbackFunc = (void *) &CallbackCaller<long>; break;
        case SIGCHAR_ULONG:     pCallCallbackFunc = (void *) &CallbackCaller<unsigned long>; break;
        case SIGCHAR_LONGLONG:  pCallCallbackFunc = (void *) &CallbackCaller<long long>; break;
        case SIGCHAR_ULONGLONG: pCallCallbackFunc = (void *) &CallbackCaller<unsigned long long>; break;
        case SIGCHAR_FLOAT:     pCallCallbackFunc = (void *) &CallbackCaller<float>; break;
        case SIGCHAR_DOUBLE:    pCallCallbackFunc = (void *) &CallbackCaller<double>; break;
        case SIGCHAR_POINTER:   pCallCallbackFunc = (void *) &CallbackCaller<void *>; break;
        case SIGCHAR_STRING:    pCallCallbackFunc = (void *) &CallbackCaller<char *>; break;
        default: BOOST_RAISE_EXCEPTION(PyExc_ValueError, "Unknown return type.");
    }

    // Generate the function
    Assembler a;

    // Epilog
    a.push(ebp);
    a.mov(ebp, esp);

    // Save esp and ecx, so the Python callback can access the parameters
    a.mov(dword_ptr_abs(&m_ESP.m_ulAddr), esp);
    a.mov(dword_ptr_abs(&m_ulECX), ecx);

    // Call callback caller
    a.push(imm((sysint_t) this));
    a.call(pCallCallbackFunc);
    a.add(esp, imm(4));

    // Prolog
    a.mov(esp, ebp);
    a.pop(ebp);

    // Return
    a.ret(imm(GetPopSize()));

    m_ulAddr = (unsigned long) a.make();
}

CCallback::~CCallback()
{
    delete m_pParams;
    delete m_pRetParam;
}

int CCallback::GetPopSize()
{
#ifdef _WIN32
    if (m_eConv == CONV_THISCALL || m_eConv == CONV_STDCALL)
    {
        Param_t* pParam = GetArgument(GetArgumentCount() - 1);
        return pParam->m_iOffset + pParam->m_iSize;
    }
#endif
    return 0;
}

int CCallback::GetArgumentCount()
{
    int count = 0;
    Param_t* temp = m_pParams;
    while(temp)
    {
        count++;
        temp = temp->m_pNext;
    }
        return count - 1;
}

Param_t* CCallback::GetArgument(int iIndex)
{
    Param_t* temp = m_pParams;
    while(temp && iIndex > 0)
    {
        iIndex--;
        temp = temp->m_pNext;
    }
    return temp;
}

void CCallback::Free()
{
    // TODO: Figure out how to use std::free() on the generated code
    MemoryManager::getGlobal()->free((void *) m_ulAddr);
    m_ulAddr = 0;
}


// ============================================================================
// >> FUNCTIONS
// ============================================================================
object CallCallback(CCallback* pCallback)
{
    BEGIN_BOOST_PY()

        list arg_list;
        for(int i=0; i < pCallback->GetArgumentCount(); i++)
        {
            Param_t* arg = pCallback->GetArgument(i);
            object val;
            switch(arg->m_cParam)
            {
                case SIGCHAR_BOOL:      val = object(pCallback->GetArgument<bool>(i)); break;
                case SIGCHAR_CHAR:      val = object(pCallback->GetArgument<char>(i)); break;
                case SIGCHAR_UCHAR:     val = object(pCallback->GetArgument<unsigned char>(i)); break;
                case SIGCHAR_SHORT:     val = object(pCallback->GetArgument<short>(i)); break;
                case SIGCHAR_USHORT:    val = object(pCallback->GetArgument<unsigned short>(i)); break;
                case SIGCHAR_INT:       val = object(pCallback->GetArgument<int>(i)); break;
                case SIGCHAR_UINT:      val = object(pCallback->GetArgument<unsigned int>(i)); break;
                case SIGCHAR_LONG:      val = object(pCallback->GetArgument<long>(i)); break;
                case SIGCHAR_ULONG:     val = object(pCallback->GetArgument<unsigned long>(i)); break;
                case SIGCHAR_LONGLONG:  val = object(pCallback->GetArgument<long long>(i)); break;
                case SIGCHAR_ULONGLONG: val = object(pCallback->GetArgument<unsigned long long>(i)); break;
                case SIGCHAR_FLOAT:     val = object(pCallback->GetArgument<float>(i)); break;
                case SIGCHAR_DOUBLE:    val = object(pCallback->GetArgument<double>(i)); break;
                case SIGCHAR_POINTER:   val = object(CPointer(pCallback->GetArgument<unsigned long>(i))); break;
                case SIGCHAR_STRING:    val = object(pCallback->GetArgument<const char*>(i)); break;
                default: BOOST_RAISE_EXCEPTION(PyExc_TypeError, "Unknown type."); break;
            }
            arg_list.append(val);
        }
        return eval("lambda func, args: func(*args)")(pCallback->m_oCallback, arg_list);

    END_BOOST_PY_NORET()

    // Throw an exception. We will crash now :(
    throw;
}