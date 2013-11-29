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

#ifndef _BINUTILS_SCANNER_H
#define _BINUTILS_SCANNER_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include <list>
#include "binutils_tools.h"


// ============================================================================
// >> CLASSES
// ============================================================================
struct Signature_t
{
    unsigned char* m_szSignature;
    unsigned long  m_ulAddr;
};


class CBinaryFile
{
public:
    CBinaryFile(unsigned long ulAddr, unsigned long ulSize);

    CPointer* FindSignature(object szSignature);
    CPointer* FindSymbol(char* szSymbol);
    CPointer* FindPointer(object szSignature, int iOffset);

    unsigned long GetAddress() { return m_ulAddr; }
    unsigned long GetSize() { return m_ulSize; }

private:
    unsigned long          m_ulAddr;
    unsigned long          m_ulSize;
    std::list<Signature_t> m_Signatures;
};


class CBinaryManager
{
public:
    CBinaryFile* FindBinary(char* szPath, bool bSrvCheck = true);

private:
    std::list<CBinaryFile*> m_Binaries;
};

// ============================================================================
// >> FUNCTIONS
// ============================================================================
CBinaryFile* FindBinary(char* szPath, bool bSrvCheck = true);

#endif // _BINUTILS_SCANNER_H