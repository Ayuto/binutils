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
 * $Rev$
 * $Author$
 * $Date$
*/

// ============================================================================
// >> INCLUDES
// ============================================================================
// Python
#include "Python.h"

// C/C++
#include <sys/stat.h>

// DynCall
#include "dyncall.h"

// DynDetours
#include "detour_class.h"
#include "detourman_class.h"

// binutils
#include "binutils_scanner.h"
#include "binutils_hooks.h"
#include "binutils_utilities.h"
#include "binutils_version.h"


// ============================================================================
// >> MACROS & DEFINITIONS
// ============================================================================
// Size of the Python function table
#define MAX_PY_FUNCS 20

// Useful macro to create functions, which are exposed to Python.
// Thanks to your-name-here!
#define PY_FUNC(name, docstring) \
	static PyObject *binutils_##name(PyObject *self, PyObject *args); \
	static PyCommand binutils_##name##_cmd(#name, binutils_##name, METH_VARARGS, docstring); \
	static PyObject *binutils_##name(PyObject *self, PyObject *args)


// ============================================================================
// >> GLOBAL VARIABLES
// ============================================================================
// Create a binary data manager
BinaryManager g_BinaryManager;

// Create a VM to call functions dynamically
DCCallVM *g_pVM = dcNewCallVM(4096);

// Container for all Python functions
static PyMethodDef g_PyFuncs[MAX_PY_FUNCS];


// ============================================================================
// >> CLASSES
// ============================================================================
// Thanks to Mattie!
class PyCommand
{
public:
	PyCommand(const char *name, PyCFunction func, int method, const char *desc)
	{
		static int i = 0;
		g_PyFuncs[i].ml_name = name;
		g_PyFuncs[i].ml_meth = func;
		g_PyFuncs[i].ml_flags = method;
		g_PyFuncs[i].ml_doc = desc;
		i++;
	}
};


// ============================================================================
// >> INITIALIZATION FUNCTION
// ============================================================================
PyMODINIT_FUNC init_binutils()
{
    PyObject* _binutils = Py_InitModule("_binutils", g_PyFuncs);
    
    // Set a global variable to the latest revision
    PyObject* version = PyString_FromString(REVISION);
    PyObject_SetAttrString(_binutils, "cpp_version", version);
    Py_XDECREF(version);
}


// ============================================================================
// >> Signature Scanning/Address searching
// ============================================================================
PY_FUNC(getBinary, "Opens a binary.\n\
\n\
@param path Path to the binary.\n\
\n\
@return Tuple containing the Binary object pointer, handle and size.")
{
    const char* path = NULL;
    if (!PyArg_ParseTuple(args, "s", &path))
        Py_RETURN_NONE;

    Binary* binary = g_BinaryManager.GetBinary(path);
    if (!binary)
        Py_RETURN_NONE;

    return Py_BuildValue("kkk", binary, binary->GetHandle(), binary->GetSize());
}

PY_FUNC(findSignature, "Returns the address of a signature in the given binary.\n\
\n\
@param binary Binary pointer.\n\
@param sig    Signature to search for.\n\
@param length Length of the signature.\n\
\n\
@return None or address to the first match.")
{
    Binary* binary = NULL;
    unsigned char* sig = NULL;
    int length;
    if (!PyArg_ParseTuple(args, "ksi", &binary, &sig, &length))
        Py_RETURN_NONE;

    void* pAddr = binary->FindSignature(sig, length);
    if (!pAddr)
        Py_RETURN_NONE;

    return Py_BuildValue("k", pAddr);
}

PY_FUNC(findSymbol, "Returns the address of a symbol in the given binary.\n\
\n\
@param binary Binary pointer.\n\
@param symbol Symbol to search for.\n\
\n\
@return None or address to the symbol.")
{
    Binary* binary = NULL;
    char* symbol = NULL;
    if (!PyArg_ParseTuple(args, "ks", &binary, &symbol))
        Py_RETURN_NONE;

    void* pAddr = binary->FindSymbol(symbol);
    if (!pAddr)
        Py_RETURN_NONE;

    return Py_BuildValue("k", pAddr);
}

PY_FUNC(findVirtualFunc, "Returns the address of a virtual function at the given index.\n\
\n\
@param this  Pointer address.\n\
@param index Index to the virtual function.\n\
\n\
@return ValueError or address to the virtual function.")
{
    void *pThis = NULL;
    int index;
    if (!PyArg_ParseTuple(args, "ki", &pThis, &index))
        Py_RETURN_NONE;

    if (!pThis)
        return PyErr_Format(PyExc_ValueError, "\"this\" pointer is NULL");

    return Py_BuildValue("k", (*(void ***) pThis)[index]);
}

// ============================================================================
// >> Memory Editing
// ============================================================================
PY_FUNC(getValue, "Retrieves a value from the specified address.\n\
\n\
@param type Character that represents one of the following types:\n\
b: boolean\n\
c: character\n\
i: integer\n\
f: float\n\
d: double\n\
s: string\n\
p: pointer\n\
\n\
@param addr Address to read.\n\
\n\
@return The value the address is pointing to.")
{
    char type;
    void* addr = NULL;
    if (!PyArg_ParseTuple(args, "ck", &type, &addr))
        Py_RETURN_NONE;

    if (!addr)
        return PyErr_Format(PyExc_ValueError, "Address is NULL");

    switch (type)
    {
        case 'b':
        {
            if (ReadAddr<bool>(addr))
                Py_RETURN_TRUE;

            Py_RETURN_FALSE;
        }
        case 'c': return Py_BuildValue("c", ReadAddr<char>(addr));
        case 'i': return Py_BuildValue("i", ReadAddr<int>(addr));
        case 'f': return Py_BuildValue("f", ReadAddr<float>(addr));
        case 'd': return Py_BuildValue("d", ReadAddr<double>(addr));
        case 's': return Py_BuildValue("s", ReadAddr<char *>(addr));
        case 'p': return Py_BuildValue("k", ReadAddr<unsigned long>(addr));
    }
    return PyErr_Format(PyExc_ValueError, "Invalid data type: %c", type);
}

PY_FUNC(setValue, "Sets a value at the specified address.\n\
\n\
@param type Character that represents one of the following types:\n\
b: boolean\n\
c: character\n\
i: integer\n\
f: float\n\
d: double\n\
s: string\n\
p: pointer\n\
\n\
@param addr  Address to set.\n\
@param value Value to set.")
{
    char type;
    void* addr = NULL;
    PyObject* value = NULL;
    if (!PyArg_ParseTuple(args, "ckO", &type, &addr, &value))
        Py_RETURN_NONE;

    if (!addr)
        return PyErr_Format(PyExc_ValueError, "Address is NULL");

    switch(type)
    {
        case 'b': SetAddr<bool>(addr, PyBool_Check(value)); break;
        case 'c': SetAddr<char>(addr, *PyString_AsString(value)); break;
        case 'i': SetAddr<int>(addr, PyInt_AsLong(value)); break;
        case 'f': SetAddr<float>(addr, (float) PyFloat_AsDouble(value)); break;
        case 'd': SetAddr<double>(addr, PyFloat_AsDouble(value)); break;
        case 's': return PyErr_Format(PyExc_NotImplementedError, "Setting strings is currently not supported");
        case 'p': SetAddr<void *>(addr, (void *) PyLong_AsLong(value)); break;
        default: return PyErr_Format(PyExc_ValueError, "Invalid type: %c", type);
    }
    Py_RETURN_NONE;
}

// ============================================================================
// >> Dynamic function calling
// ============================================================================
PY_FUNC(resetVM, "Resets the virtual machine.")
{
    dcReset(g_pVM);
    Py_RETURN_NONE;
}

PY_FUNC(setMode, "Sets a calling convention of the virtual machine.\n\
\n\
@param mode Integer that represents a calling convention.")
{
    int mode;
    if (PyArg_ParseTuple(args, "i", &mode))
        dcMode(g_pVM, mode);

    Py_RETURN_NONE;
}

PY_FUNC(setArg, "Sets an argument type of the virtual machine.\n\
\n\
@param type Character that represents one of the following types:\n\
b: boolean\n\
c: character\n\
i: integer\n\
f: float\n\
d: double\n\
s: string\n\
p: pointer")
{
    char type;
    PyObject *value = NULL;
    if (!PyArg_ParseTuple(args, "cO", &type, &value))
        Py_RETURN_NONE;

    switch(type)
    {
        case 'b': dcArgBool(g_pVM, PyBool_Check(value)); break;
        case 'c': dcArgChar(g_pVM, *PyString_AsString(value)); break;
        case 'i': dcArgInt(g_pVM, PyInt_AsLong(value)); break;
        case 'f': dcArgFloat(g_pVM, (float) PyFloat_AsDouble(value)); break;
        case 'd': dcArgFloat(g_pVM, PyFloat_AsDouble(value)); break;
        case 's': dcArgPointer(g_pVM, (void *) PyString_AsString(value)); break;
        case 'p': dcArgPointer(g_pVM, (void *) PyLong_AsLong(value)); break;
        default: return PyErr_Format(PyExc_ValueError, "Invalid type: %c", type);
    }
    Py_RETURN_NONE;
}

PY_FUNC(call, "Calls the virtual machine.\n\
\n\
@param func Address to the function.\n\
@param type Character that represents one of the following types:\n\
v: void\n\
b: boolean\n\
c: character\n\
i: integer\n\
f: float\n\
d: double\n\
s: string\n\
p: pointer\n\
\n\
@return Value returned by the called function.")
{
    void *pFunc = NULL;
    char type;
    if (!PyArg_ParseTuple(args, "kc", &pFunc, &type))
        Py_RETURN_NONE;

    if (!pFunc)
        return PyErr_Format(PyExc_ValueError, "Function pointer is NULL");

    switch(type)
    {
        case 'v': dcCallVoid(g_pVM, pFunc); Py_RETURN_NONE;
        case 'b':
        {
            // dcCallBool is the same like dcCallInt
            if (dcCallBool(g_pVM, pFunc) % 2)
                Py_RETURN_TRUE;

            Py_RETURN_FALSE;
        }
        case 'c': return Py_BuildValue("c", dcCallChar(g_pVM, pFunc));
        case 'i': return Py_BuildValue("i", dcCallInt(g_pVM, pFunc));
        case 'f': return Py_BuildValue("f", dcCallFloat(g_pVM, pFunc));
        case 'd': return Py_BuildValue("d", dcCallDouble(g_pVM, pFunc));
        case 's': return Py_BuildValue("s", dcCallPointer(g_pVM, pFunc));
        case 'p': return Py_BuildValue("k", dcCallPointer(g_pVM, pFunc));
    }
    return PyErr_Format(PyExc_ValueError, "Invalid type: %c", type);
}

// ============================================================================
// >> Memory space allocating/deallocating
// ============================================================================
PY_FUNC(allocate, "Reserves memory space and returns the pointer to it.\n\
\n\
@param size Bytes, which should be allocated.\n\
\n\
@return Address to the reserved memory space.")
{
    int size;
    if (!PyArg_ParseTuple(args, "i", &size))
        Py_RETURN_NONE;

    return Py_BuildValue("k", malloc(size));
}

PY_FUNC(deallocate, "Frees reserved memory space.\n\
\n\
@param addr Address of the reserved memory space.")
{
    void *ptr = NULL;
    if (PyArg_ParseTuple(args, "k", &ptr) && ptr)
        free(ptr);

    Py_RETURN_NONE;
}

// ============================================================================
// >> Dynamic function hooking
// ============================================================================
PY_FUNC(hookFunction, "Hooks a function dynamically.\n\
\n\
@param addr     Address of the function to hook.\n\
@param conv     Calling convention of that function.\n\
@param params   String that represents the functions arguments.\n\
@param type     Hooking type (pre-/post-hook).\n\
@param callback Python callback function.")
{
    void* addr = NULL;
    eCallConv conv;
    char* params = NULL;
    eHookType type;
    PyObject* callback = NULL;

    if (!PyArg_ParseTuple(args, "kisiO", &addr, &conv, &params, &type, &callback))
        Py_RETURN_NONE;

    if (!addr)
        return PyErr_Format(PyExc_ValueError, "Function pointer is NULL");

    CDetour* pDetour = g_DetourManager.Add_Detour(addr, params, conv);
    ICallbackManager* mngr = pDetour->GetManager("Python", type);
    if (!mngr)
    {
        mngr = new CCallbackManager;
        pDetour->AddManager(mngr, type);
    }

    mngr->Add((void *) callback, type);
    Py_RETURN_NONE;
}

PY_FUNC(unhookFunction, "Un-hooks a function dynamically.\n\
\n\
@param addr     Address of the function to hook.\n\
@param type     Hooking type (pre-/post-hook).\n\
@param callback Python callback function.")
{
    void* addr = NULL;
    PyObject* callback = NULL;
    eHookType type;

    if (!PyArg_ParseTuple(args, "kiO", &addr, &type, &callback))
        Py_RETURN_NONE;

    if (!addr)
        return PyErr_Format(PyExc_ValueError, "Function pointer is NULL");

    CDetour* pDetour = g_DetourManager.Find_Detour(addr);
    if (!pDetour)
        Py_RETURN_NONE;

    ICallbackManager* mngr = pDetour->GetManager("Python", type);
    if (mngr)
        mngr->Remove((void *) callback, type);

    Py_RETURN_NONE;
}

PY_FUNC(getTrampoline, "Returns the address of the trampoline of a hooked function.\n\
\n\
@param addr None or address of the trampoline of a hooked function.")
{
    void* addr = NULL;
    if (!PyArg_ParseTuple(args, "k", &addr))
        Py_RETURN_NONE;

    if (!addr)
        return PyErr_Format(PyExc_ValueError, "Function pointer is NULL");

    CDetour* pDetour = g_DetourManager.Find_Detour(addr);
    if (!pDetour)
        Py_RETURN_NONE;

    void* trampoline = pDetour-> GetTrampoline();
    return Py_BuildValue("k", trampoline);
}

// ============================================================================
// >> Various functions
// ============================================================================
PY_FUNC(getByteAt, "Returns the hexadecimal character at the given address.\n\
\n\
@param addr Address to read.\n\
\n\
@return Character at the given memory location.")
{
    void* addr = NULL;
    if (!PyArg_ParseTuple(args, "k", &addr))
        Py_RETURN_NONE;

    if (!addr)
        return PyErr_Format(PyExc_ValueError, "Address is NULL");

    return Py_BuildValue("c", *(unsigned char *) addr);
}

PY_FUNC(uLong2Int, "Converts the address represention \"unsigned long\" to \"int\".\n\
\n\
@param addr Address represented as an unsigned long.\n\
\n\
@return Address represented as an integer (int).")
{
    unsigned long mylong;
    if (!PyArg_ParseTuple(args, "k", &mylong))
        Py_RETURN_NONE;

    if (!mylong)
        return PyErr_Format(PyExc_ValueError, "Address is NULL");

    return Py_BuildValue("i", mylong);
}

PY_FUNC(int2uLong, "Converts the address represention \"int\" to \"unsigned long\".\n\
\n\
@param addr Address represented as an integer (int).\n\
\n\
@return Address represented as an unsigned long.")
{
    int myint;
    if (!PyArg_ParseTuple(args, "i", &myint))
        Py_RETURN_NONE;

    if (!myint)
        return PyErr_Format(PyExc_ValueError, "Address is NULL");

    return Py_BuildValue("k", myint);
}