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

// ============================================================================
// >> INCLUDES
// ============================================================================
// C/C++
#ifdef _WIN32
    #include <windows.h>
#else
    #include <string.h>
    #include <iostream>
    #include <fcntl.h>
    #include <link.h>
    #include <sys/mman.h>
#endif

// DynCall
#include "dynload.h"

// binutils
#include "binutils_scanner.h"


// ============================================================================
// >> BinaryManager
// ============================================================================
Binary* BinaryManager::GetBinary(const char* path)
{
    void* pAddr = (void *) dlLoadLibrary(path);
    if (!pAddr)
        return NULL;

    // Search for an existing Binary object
    for (std::list<Binary *>::iterator iter=m_Binaries.begin(); iter != m_Binaries.end(); iter++)
    {
        Binary* binary = *iter;
        if (binary->GetHandle() == pAddr)
        {
            // We don't need to open it several times
            dlFreeLibrary((DLLib *) pAddr);
            return binary;
        }
    }

    // Determine the binary size
    unsigned long iSize;

#ifdef _WIN32
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER *) pAddr;
    IMAGE_NT_HEADERS* nt  = (IMAGE_NT_HEADERS *) ((BYTE *) dos + dos->e_lfanew);
    iSize = nt->OptionalHeader.SizeOfImage;

#else
    // This method does not retrieve the whole size
    struct stat buf;
    if (stat(path, &buf) == -1)
    {
        // Free it!
        dlFreeLibrary((DLLib *) pAddr);
        return NULL;
    }

    iSize = buf.st_size;
#endif

    // Create a new Binary object and add it to the list
    Binary* binary = new Binary(pAddr, iSize);
    m_Binaries.push_front(binary);
    return binary;
}


// ============================================================================
// >> Binary
// ============================================================================
Binary::Binary(void* pAddr, unsigned long iSize)
{
    m_pHandle = pAddr;
    m_iSize   = iSize;
}

void* Binary::FindSignature(unsigned char* sig, int length)
{
    // Search for a cached signature
    for (std::list<Signature_t>::iterator iter=m_SigCache.begin(); iter != m_SigCache.end(); iter++)
    {
        Signature_t sig_t = *iter;
        if (strcmp((const char *) sig_t.szSignature, (const char *) sig) == 0)
            return sig_t.pAddr;
    }

    unsigned char* base = (unsigned char *) m_pHandle;
    unsigned char* end  = (unsigned char *) (base + m_iSize - length);
    while(base < end)
    {
        int i = 0;
        for(; i < length; i++)
        {
            if (sig[i] == '\x2A')
                continue;

            if (sig[i] != base[i])
                break;
        }

        if (i == length)
        {
            // Add our signature to the cache
            Signature_t sig_t = {sig, (void *) base};
            m_SigCache.push_back(sig_t);
            return base;
        }
        base++;
    }
    return NULL;
}

/**
  * Copied from here:
  * http://code.google.com/p/source-python/source/browse/src/core/modules/binutils/binutils_scanner.cpp#159
  *
  * Thanks!
**/
void* Binary::FindSymbol(const char* symbol)
{
#ifdef _WIN32
    return dlFindSymbol((DLLib *) m_pHandle, symbol);

#else
    struct link_map *dlmap;
    struct stat dlstat;
    int dlfile;
    uintptr_t map_base;
    Elf32_Ehdr *file_hdr;
    Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
    Elf32_Sym *symtab;
    const char *shstrtab, *strtab;
    uint16_t section_count;
    uint32_t symbol_count;

    dlmap = (struct link_map *) m_pHandle;
    symtab_hdr = NULL;
    strtab_hdr = NULL;

    dlfile = open(dlmap->l_name, O_RDONLY);
    if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
    {
        close(dlfile);
        return NULL;
    }

    /* Map library file into memory */
    file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
    map_base = (uintptr_t)file_hdr;
    if (file_hdr == MAP_FAILED)
    {
        close(dlfile);
        return NULL;
    }
    close(dlfile);

    if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
    {
        munmap(file_hdr, dlstat.st_size);
        return NULL;
    }

    sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
    section_count = file_hdr->e_shnum;
    /* Get ELF section header string table */
    shstrtab_hdr = &sections[file_hdr->e_shstrndx];
    shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

    /* Iterate sections while looking for ELF symbol table and string table */
    for (uint16_t i = 0; i < section_count; i++)
    {
        Elf32_Shdr &hdr = sections[i];
        const char *section_name = shstrtab + hdr.sh_name;

        if (strcmp(section_name, ".symtab") == 0)
            symtab_hdr = &hdr;

        else if (strcmp(section_name, ".strtab") == 0)
            strtab_hdr = &hdr;
    }

    /* Uh oh, we don't have a symbol table or a string table */
    if (symtab_hdr == NULL || strtab_hdr == NULL)
    {
        munmap(file_hdr, dlstat.st_size);
        return NULL;
    }

    symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
    strtab = (const char *)(map_base + strtab_hdr->sh_offset);
    symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;
    void* sym_addr = NULL;

    /* Iterate symbol table starting from the position we were at last time */
    for (uint32_t i = 0; i < symbol_count; i++)
    {
        Elf32_Sym &sym = symtab[i];
        unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
        const char *sym_name = strtab + sym.st_name;

        /* Skip symbols that are undefined or do not refer to functions or objects */
        if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
            continue;

        if (strcmp(symbol, sym_name) == 0)
        {
            sym_addr = (void *)(dlmap->l_addr + sym.st_value);
            break;
        }
    }

    // Unmap the file now.
    munmap(file_hdr, dlstat.st_size);
    return sym_addr ? sym_addr : NULL;
#endif
}