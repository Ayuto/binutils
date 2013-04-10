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
 * $Rev: 69 $
 * $Author: Ayuto $
 * $Date: 2013-03-24 20:51:38 +0100 (So, 24. Mrz 2013) $
*/

#ifndef BINUTILS_SCANNER_H
#define BINUTILS_SCANNER_H

// ============================================================================
// >> INCLUDES
// ============================================================================
// C/C++
#include <list>

// DynCall
#include "dynload.h"


// ============================================================================
// >> CLASSES & STRUCTS
// ============================================================================
struct Signature_t
{
    unsigned char* szSignature;
    void* pAddr;
};


class Binary
{
private:
    // Start address
    void* m_pHandle;

    // Whole size
    unsigned long m_iSize;

    // All searched signatures and their addresses (as hooking breaks the signature)
    std::list<Signature_t> m_SigCache;

public:
    Binary(void* pHandle, unsigned long iSize);

    // Returns the address of a signature
    void* FindSignature(unsigned char* sig, int length);

    // Returns the address of a symbol
    void* FindSymbol(const char* symbol);

    void* GetHandle() { return m_pHandle; }
    unsigned long GetSize() { return m_iSize; }
};


class BinaryManager
{
private:
    // Container for all binaries
    std::list<Binary *> m_Binaries;

public:
    Binary* GetBinary(const char* path);
};

#endif // BINUTILS_SCANNER_H