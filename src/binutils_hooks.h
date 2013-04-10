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

#ifndef BINUTILS_HOOKS_H
#define BINUTILS_HOOKS_H

// ============================================================================
// >> INCLUDES
// ============================================================================
// Python
#include "Python.h"

// C/C++
#include <list>

// DynDetours
#include "callback_manager.h"


// ============================================================================
// >> CLASSES
// ============================================================================
class CCallbackManager: public ICallbackManager
{
	private:
        list<PyObject *> m_PreCalls;
        list<PyObject *> m_PostCalls;

	public:
		virtual void Add(void* pFunc, eHookType type);
		virtual void Remove(void* pFunc, eHookType type);

		virtual HookRetBuf_t* DoPreCalls(CDetour* pDetour);
		virtual HookRetBuf_t* DoPostCalls(CDetour* pDetour);

		virtual const char* GetLang() { return "Python"; }
};


// ============================================================================
// >> FUNCTIONS
// ============================================================================
PyObject* GetArgList(CDetour* pDetour);
void SetNewArgs(CDetour* pDetour, PyObject* pArgList);

#endif // BINUTILS_HOOKS_H