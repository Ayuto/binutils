# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
from __future__ import with_statement

import os
import re

from distutils.core import Extension
from distutils.core import setup


# =============================================================================
# >> PROJECT SOURCES
# =============================================================================
SOURCES = [
    'src/binutils_exports.cpp',
    'src/binutils_scanner.cpp',
    'src/binutils_hooks.cpp',
]


# =============================================================================
# >> EXTRA INCLUDES
# =============================================================================
INCLUDE_DIRS = [
    'src/dyncall/include',
    'src/DynDetours/include',
    'src/ASMJit',
]


# =============================================================================
# >> LIBRARY SEARCH DIRECTORIES
# =============================================================================
LIBRARY_DIRS = [
    'src/dyncall/lib',
    'src/ASMJit/lib',
    'src/DynDetours/lib',
]


# =============================================================================
# >> LIBRARY NAMES
# =============================================================================
# Windows
if os.name == 'nt':
    LIBRARIES = [
        'libdyncall_s',
        'libdynload_s',
        'libDynDetours',
        'libAsmJit',
    ]

# Linux
else:
    LIBRARIES = [
        'dyncall_s',
        'dynload_s',
        'DynDetours',
        'AsmJit',
    ]


# =============================================================================
# >> COMPILER FLAGS
# =============================================================================
COMPILER_FLAGS = [
]


# =============================================================================
# >> LINKER FLAGS
# =============================================================================
LINKER_FLAGS = [
    '-static-libgcc',
    '-static-libstdc++',
]


# =============================================================================
# >> GLOBAL VARIABLES
# =============================================================================
RE_REVISION = re.compile(r'\$Rev: (\d+) \$')


# =============================================================================
# >> FUNCTIONS
# =============================================================================
def updateCppVersion():
    max_revs = [0]
    for filename in os.listdir('src'):
        if not os.path.isfile('src/' + filename):
            continue
            
        with open('src/' + filename) as f:
            data = f.read()
            
        max_revs += map(int, RE_REVISION.findall(data))
        
    rev = str(max(max_revs))
    with open('src/binutils_version.h', 'r') as f:
        data = f.read()
        
    with open('src/binutils_version.h', 'w') as f:
        f.write(RE_REVISION.sub(rev, data))


# =============================================================================
# >> SETUP
# =============================================================================
updateCppVersion()

module = Extension(
    '_binutils',
    sources=SOURCES,
    library_dirs=LIBRARY_DIRS,
    libraries=LIBRARIES,
    include_dirs=INCLUDE_DIRS,
    extra_compile_args=COMPILER_FLAGS,
    extra_link_args=LINKER_FLAGS,
)

setup(
    name='_binutils',
    ext_modules=[module],
)