# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
import os
import sys

from distutils.core import Extension
from distutils.core import setup


# =============================================================================
# >> PROJECT SOURCES
# =============================================================================
SOURCES = [
    # binutils
    'src/binutils_wrap.cpp',
    'src/binutils_tools.cpp',
    'src/binutils_hooks.cpp',
    'src/binutils_scanner.cpp',
    
    # DynamicHooks
    'src/thirdparty/DynamicHooks/DynamicHooks.cpp',
    'src/thirdparty/DynamicHooks/utilities.cpp',
    'src/thirdparty/DynamicHooks/asm.cpp',
]


# =============================================================================
# >> EXTRA INCLUDES
# =============================================================================
INCLUDE_DIRS = [
    'src/thirdparty/boost',
    'src/thirdparty/dyncall/include',
    'src/thirdparty/AsmJit/include',
    'src/thirdparty/DynamicHooks',
]


# =============================================================================
# >> LIBRARY SEARCH DIRECTORIES
# =============================================================================
LIBRARY_DIRS = [
    'src/thirdparty/boost/lib',
    'src/thirdparty/dyncall/lib',
    'src/thirdparty/AsmJit/lib',
]


# =============================================================================
# >> LIBRARY NAMES
# =============================================================================
# Windows
if os.name == 'nt':
    LIBRARIES = [
        'libboost_python-mgw47-mt-1_54',
        'libdyncall_s',
        'libdyncallback_s',
        'libdynload_s',
        'libAsmJit',
    ]

# Linux
else:
    LIBRARIES = [
        'boost_python',
        'dyncall_s',
        'dyncallback_s',
        'dynload_s',
        'AsmJit',
    ]


# =============================================================================
# >> COMPILER FLAGS
# =============================================================================
COMPILER_FLAGS = [
    # This disables annoying visibility warnings
    '-Wno-attributes',

    # Disable parentheses suggestions
    '-Wno-parentheses',
]


# =============================================================================
# >> LINKER FLAGS
# =============================================================================
if os.name == 'nt':
    LINKER_FLAGS = [
        '-static-libgcc',
        '-static-libstdc++',
    ]
else:
    LINKER_FLAGS = [
    ]


# =============================================================================
# >> MACROS
# =============================================================================
MACROS = [
    ('BOOST_PYTHON_STATIC_LIB', ''),
    ('PYTHON_VERSION', sys.version_info[0]),
]


# =============================================================================
# >> SETUP
# =============================================================================
setup(
    name='binutils',
    ext_modules=[
        Extension(
            'binutils',
            sources=SOURCES,
            library_dirs=LIBRARY_DIRS,
            libraries=LIBRARIES,
            include_dirs=INCLUDE_DIRS,
            extra_compile_args=COMPILER_FLAGS,
            extra_link_args=LINKER_FLAGS,
            define_macros=MACROS
        ),
    ]
)