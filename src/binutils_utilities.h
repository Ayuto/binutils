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

#ifndef BINUTILS_UTILITIES_H
#define BINUTILS_UTILITIES_H

// ============================================================================
// >> FUNCTIONS
// ============================================================================
template <class T>
inline T ReadAddr(void* addr)
{
    return *((T *) addr);
}

template <class T>
inline void SetAddr(void* addr, T value)
{
    *(T *) addr = value;
}

#endif // BINUTILS_UTILITIES_H