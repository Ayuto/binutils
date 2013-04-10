/**
* =============================================================================
* binutils
* Copyright(C) 2012 Ayuto. All rights reserved.
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

/*
 * $Rev: 51 $
 * $Author: Ayuto $
 * $Date: 2013-02-08 18:34:40 +0100 (Fr, 08. Feb 2013) $
*/

// ============================================================================
// >> INCLUDES
// ============================================================================
// C/C++
#include "Python.h"

// DynDetours
#include "conv_interface.h"
#include "detour_class.h"
#include "func_class.h"
#include "func_stack.h"
#include "func_types.h"
#include "register_class.h"

// binutils
#include "binutils_hooks.h"
#include "binutils_utilities.h"


// ============================================================================
// >> CLASS MEMBERS
// ============================================================================
void CCallbackManager::Add(void* pFunc, eHookType type)
{
    if (!pFunc)
        return;

    switch (type)
    {
        case TYPE_PRE:  m_PreCalls.push_front((PyObject*) pFunc); break;
        case TYPE_POST: m_PostCalls.push_front((PyObject*) pFunc); break;
    }
}

void CCallbackManager::Remove(void* pFunc, eHookType type)
{
    if (!pFunc)
        return;

    switch (type)
    {
        case TYPE_PRE:  m_PreCalls.remove((PyObject *) pFunc); break;
        case TYPE_POST: m_PostCalls.remove((PyObject *) pFunc); break;
    }
}

HookRetBuf_t* CCallbackManager::DoPreCalls(CDetour* pDetour)
{
    if (!pDetour)
        return NULL;

    HookRetBuf_t* buffer = new HookRetBuf_t;
    buffer->eRes = HOOKRES_NONE;
    buffer->pRetBuf = 0;

    PyObject* pArgList = GetArgList(pDetour);
    list<PyObject *>::iterator iter;
    for (iter=m_PreCalls.begin(); iter != m_PreCalls.end(); iter++)
    {
	    PyObject* pTuple = PyObject_CallFunction(*iter, "O", pArgList);
	    if (PyErr_Occurred() || !pTuple)
        {
            PyErr_Print();
            continue;
	    }

        eHookRes action = HOOKRES_ERROR;
        void* result    = NULL;
        if (!PyArg_ParseTuple(pTuple, "ik", &action, &result))
            continue;

        if (action >= buffer->eRes)
        {
            buffer->eRes = action;
            buffer->pRetBuf = result;
        }
    }

    if (buffer->eRes == HOOKRES_NEWPARAMS)
        SetNewArgs(pDetour, pArgList);

    Py_XDECREF(pArgList);
    return buffer;
}

HookRetBuf_t* CCallbackManager::DoPostCalls(CDetour* pDetour)
{
    HookRetBuf_t* buffer = new HookRetBuf_t;
    buffer->eRes = HOOKRES_NONE;
    buffer->pRetBuf = 0;
    return buffer;
}


// ============================================================================
// >> FUNCTIONS
// ============================================================================
void SetNewArgs(CDetour* pDetour, PyObject* pArgList)
{
    CRegisterObj* pRegisters = pDetour->GetAsmBridge()->GetConv()->GetRegisters();
    CFuncObj* pFunction      = pDetour->GetFuncObj();
    CFuncStack* pStack       = pFunction->GetStack();
    eCallConv convention     = pFunction->GetConvention();

    unsigned int i = 0;

    // Read and set thisptr at first
    if (convention == CONV_THISCALL)
    {
        // If it's a thiscall, we skip the first entry in our list
        i++;

        unsigned long thisptr = PyInt_AsLong(PyList_GetItem(pArgList, 0));
        if (thisptr)
        {
        #ifdef _WIN32
            pRegisters->r_ecx = thisptr;
        #else
            unsigned long* addr = (unsigned long *) (pRegisters->r_esp + 4);
            *addr = thisptr;
        #endif
        }
    }

    for(; i < pFunction->GetNumArgs(); i++)
    {
        ArgNode_t* pArgNode = pStack->GetArgument(convention == CONV_THISCALL ? i-1: i);
        CFuncArg* pArg      = pArgNode->m_pArg;

        int offset = pArgNode->m_nOffset;

        // Hack for thiscalls on Linux
        #ifdef __linux__
        if (convention == CONV_THISCALL)
            offset += 4;
        #endif

        void* addr = (void *) (pRegisters->r_esp + 4 + offset);
        PyObject* value = PyList_GetItem(pArgList, i);
        switch (pArg->GetType())
        {
            case TYPE_BOOL:	     SetAddr<bool>(addr, PyBool_Check(value)); break;
            case TYPE_INT32:     SetAddr<int>(addr, PyInt_AsLong(value)); break;
            case TYPE_INT32_PTR: SetAddr<void *>(addr, (void *) PyInt_AsLong(value)); break;
            case TYPE_CHAR_PTR:  SetAddr<char *>(addr, PyString_AsString(value)); break;
            default: printf("Unknown argument eArgType %i\n", pArg->GetType()); break;
        }
    }
}

PyObject* GetArgList(CDetour* pDetour)
{
    CRegisterObj* pRegisters = pDetour->GetAsmBridge()->GetConv()->GetRegisters();
    CFuncObj* pFunction      = pDetour->GetFuncObj();
    CFuncStack* pStack       = pFunction->GetStack();

    // List to save all arguments
    PyObject* pArgList = PyList_New(0);

    if (pFunction->GetConvention() == CONV_THISCALL)
    {
    #ifdef __linux__
        unsigned long thisptr =  pRegisters->r_esp + 4;
    #else
        unsigned long thisptr = pRegisters->r_ecx;
    #endif

        PyObject* pyThis = Py_BuildValue("k", thisptr);
        PyList_Append(pArgList, pyThis);
        Py_XDECREF(pyThis);
    }

    for(unsigned int i=0; i < pFunction->GetNumArgs(); i++)
    {
        ArgNode_t* pArgNode = pStack->GetArgument(i);
        CFuncArg* pArg      = pArgNode->m_pArg;

        int offset = pArgNode->m_nOffset;

        #ifdef __linux__
        // Hack for thiscalls on Linux
        if (pFunction->GetConvention() == CONV_THISCALL)
            offset += 4;
        #endif

        void* addr      = (void *) (pRegisters->r_esp + 4 + offset);
        PyObject* value = NULL;
        switch(pArg->GetType())
		{
			case TYPE_BOOL:      value = Py_BuildValue("i", ReadAddr<int>(addr) % 2); break;
			case TYPE_INT32:     value = Py_BuildValue("i", ReadAddr<int>(addr)); break;
			case TYPE_INT32_PTR: value = Py_BuildValue("k", ReadAddr<unsigned long>(addr)); break;
			case TYPE_FLOAT:     value = Py_BuildValue("f", ReadAddr<float>(addr)); break;
            case TYPE_CHAR_PTR:  value = Py_BuildValue("s", ReadAddr<char *>(addr)); break;
            default: printf("Unknown argument eArgType %i\n", pArg->GetType()); break;
        }
        PyList_Append(pArgList, value);
    }
    return pArgList;
}