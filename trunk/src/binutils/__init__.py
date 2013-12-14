# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
import ConfigParser
import os
import binascii

# binutils
from binutils import *


# =============================================================================
# >> CONSTANTS
# =============================================================================
# *.ini files layout
KEY_BINARY     = 'binary'
KEY_CONVENTION = 'convention'
KEY_PARAMETERS = 'parameters'
KEY_IDENTIFIER = os.name


# =============================================================================
# >> CLASSES
# =============================================================================
class Pipe(dict):
    def __init__(self, *files):
        config = ConfigParser.ConfigParser()

        for f in files:
            if not hasattr(f, 'readline'):
                if not os.path.isfile(f):
                    raise ValueError('"%s" is no file or readable object.'% f)

                config.read(f)
            else:
                config.readfp(f)
                try:
                    f.close()
                except:
                    pass

        # Get all options and their values as a dict
        for section in config.sections():
            self[section] = dict((opt, config.get(section, opt)) for opt \
                in config.options(section))

    def __getattr__(self, attr):
        options    = self[attr]
        binary     = binutils.find_binary(options[KEY_BINARY])
        identifier = options[KEY_IDENTIFIER]
        func_ptr   = None

        # Is it a signature?
        if os.name == 'nt' and ' ' in identifier:
            sig = binascii.unhexlify(identifier.replace(' ', ''))
            func_ptr = binary.find_signature(sig)

            # Raise an error here. Maybe the user wanted to use a symbol, but
            # accidentally added a space
            if not bool(func_ptr):
                raise ValueError('Could not find signature "%s".'% repr(sig))
        else:
            func_ptr = binary[identifier]

        # Return a new Function object
        return func_ptr.make_function(
            getattr(Convention, options[KEY_CONVENTION]),
            options[KEY_PARAMETERS]
        )

    def add_function(self, name, binary, identifier, convention, parameters):
        self[name] = {
            KEY_BINARY:     binary,
            KEY_IDENTIFIER: identifier,
            KEY_CONVENTION: convention,
            KEY_PARAMETERS: parameters,
        }