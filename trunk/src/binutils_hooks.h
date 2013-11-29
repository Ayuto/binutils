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

#ifndef _BINUTILS_HOOKS_H
#define _BINUTILS_HOOKS_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include <map>

#include "DynamicHooks.h"
using namespace DynamicHooks;

#include "binutils_tools.h"

#include "boost/python.hpp"
using namespace boost::python;


// ============================================================================
// >> CLASSES
// ============================================================================
class CStackData
{
public:
    CStackData(CHook* pHook);

    object GetItem(unsigned int iIndex);
    void   SetItem(unsigned int iIndex, object value);

private:
    CHook*                m_pHook;
    std::map<int, object> m_mapCache;
};


// ============================================================================
// >> FUNCTIONS
// ============================================================================
bool binutils_HookHandler(DynamicHooks::HookType_t eHookType, CHook* pHook);

#endif // _BINUTILS_HOOKS_H
