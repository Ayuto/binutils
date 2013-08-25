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
#include "hook_types.h"
#include "dyncall.h"
#include "boost/python.hpp"
using namespace boost::python;


// ============================================================================
// >> Convention enum
// ============================================================================
enum Convention
{
	_CONV_CDECL = DC_CALL_C_DEFAULT,
#ifdef _WIN32
	_CONV_STDCALL = DC_CALL_C_X86_WIN32_STD,
	_CONV_FASTCALL = DC_CALL_C_X86_WIN32_FAST_MS,
	_CONV_THISCALL = DC_CALL_C_X86_WIN32_THIS_MS
#else
	_CONV_FASTCALL = DC_CALL_C_X86_WIN32_FAST_GNU,
	_CONV_THISCALL = DC_CALL_C_X86_WIN32_THIS_GNU
#endif
};

// ============================================================================
// >> CLASSES
// ============================================================================
class CFunction;

class CPointer
{
public:
	CPointer(unsigned long ulAddr = 0);

	template<class T>
	T Get(int iOffset = 0)
	{
		if (!IsValid())
			BOOST_RAISE_EXCEPTION(PyExc_ValueError, "Pointer is NULL.")

		return *(T *) (m_ulAddr + iOffset);
	}

	template<class T>
	void Set(T value, int iOffset = 0)
	{
		if (!IsValid())
			BOOST_RAISE_EXCEPTION(PyExc_ValueError, "Pointer is NULL.")

		unsigned long newAddr = m_ulAddr + iOffset;
		*(T *) newAddr = value;
	}

	const char *        GetString(int iOffset = 0, bool bIsPtr = true);
	void                SetString(char* szText, int iSize = 0, int iOffset = 0, bool bIsPtr = true);
	CPointer*           GetPtr(int iOffset = 0);
	void                SetPtr(CPointer* ptr, int iOffset = 0);

	unsigned long       GetSize();
	unsigned long       GetAddress() { return m_ulAddr; }

	CPointer*           Add(int iValue);
	CPointer*           Sub(int iValue);
	bool                IsValid() { return m_ulAddr ? true: false; }

	CPointer*           GetVirtualFunc(int iIndex, bool bPlatformCheck = true);

	void                Alloc(unsigned long ulSize);
	void                Realloc(unsigned long ulSize);
	void                Dealloc();

	CFunction*          MakeFunction(Convention eConv, char* szParams);

protected:
	unsigned long m_ulAddr;
};

class CFunction: public CPointer
{
public:
	CFunction(unsigned long ulAddr, Convention eConv, char* szParams);

	object __call__(object args);
	object CallTrampoline(object args);

	void Hook(eHookType eType, PyObject* pCallable);
	void Unhook(eHookType eType, PyObject* pCallable);

	void AddPreHook(PyObject* pCallable);
	void AddPostHook(PyObject* pCallable);

	void RemovePreHook(PyObject* pCallable);
	void RemovePostHook(PyObject* pCallable);

private:
	char*      m_szParams;
	Convention m_eConv;
};


// ============================================================================
// >> FUNCTIONS
// ============================================================================
int GetError();

inline unsigned long ExtractPyPtr(object obj)
{
	if (strcmp(extract<char *>(obj.attr("__class__").attr("__name__")), "Pointer") == 0)
	{
		CPointer* pPtr = extract<CPointer *>(obj);
		return pPtr->GetAddress();
	}
	return extract<unsigned long>(obj);
}

#endif // _BINUTILS_TOOLS_H