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

# Read/write flags for class attributes
ATTR_READ       = 1 << 0
ATTR_WRITE      = 1 << 1
ATTR_READ_WRITE = ATTR_READ | ATTR_WRITE

# A tuple of native types
NATIVE_TYPES = (
    'bool',
    'char',
    'uchar',
    'short',
    'ushort',
    'int',
    'uint',
    'long',
    'ulong',
    'long_long',
    'ulong_long',
    'float',
    'double',
    'ptr',
    'string'
)


# =============================================================================
# >> CLASSES
# =============================================================================
class Pipe(dict):
    '''
    This class is used to create a pipe to normal functions and methods.

    LAYOUT:
    [<function name>]
    binary     = <path to a binary>
    linux      = <symbol of the function>
    nt         = <symbol or signature of the function>
    convention = <calling convention>
    parameters = <parameter string>
    '''

    def __init__(self, *files):
        config = self.__read_files(files)

        # Get all options and their values as a dict
        for section in config.sections():
            self[section] = dict((opt, config.get(section, opt)) for opt \
                in config.options(section))

    def __read_files(self, files):
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

        return config

    def __getattr__(self, attr):
        options = self[attr]

        # Is it already a function?
        if isinstance(options, Function):
            return options

        # Create the function and override the information
        func = self[attr] = make_function(
            options[KEY_BINARY],
            options[KEY_IDENTIFIER],
            getattr(Convention, options[KEY_CONVENTION]),
            options[KEY_PARAMETERS]
        )

        return func

    def add_function(self, name, binary, identifier, convention, parameters):
        self[name] = {
            KEY_BINARY:     binary,
            KEY_IDENTIFIER: identifier,
            KEY_CONVENTION: convention,
            KEY_PARAMETERS: parameters,
        }


class CustomType(Pointer):
    '''
    This is the base class for a custom type. You should inherit from this
    class if you want to create a new custom type.
    '''

    def __getattribute__(self, attr):
        '''
        This function works like a hook. Inherited functions and attributes
        are returned as usual, but added functions and virtual functions are
        treated with an extra step.
        '''

        result = super(CustomType, self).__getattribute__(attr)

        # If it's not a function, we can return here
        if not isinstance(result, (_EvalVirtualFunction, _EvalFunction)):
            return result

        return result(self)


class TypeManager(dict):
    '''
    The TypeManager is an extremely powerful class, which gives you the
    ability to create new custom types. That means you can restructure
    every possible data type. You only have to feed the manager with some
    information.
    '''

    def __init__(self):
        '''
        Initializes the manager by setting the default converter.
        '''

        # Default converter -- do nothing
        self.set_default_converter(lambda x: x)

    def set_default_converter(self, converter):
        '''
        Overrides the default converter.
        '''

        # Raise an error here, so we won't get confused if it raises later
        if not callable(converter):
            raise ValueError('The given converter is not callable.')

        self[None] = converter

    def create_type(self, name, cls_dict, override=False):
        '''
        Creates a new type. If the type already exists, an error will be
        raised unless <override> was set to True.
        '''

        return self.add_type(
            name,
            type(name, (CustomType,), cls_dict),
            override
        )

    def create_type_from_file(self, name, files, override=False):
        '''
        Same as "create_type()", but creates a type from the specified
        file(s).
        '''

        pass

    def add_type(self, name, cls, override=False):
        '''
        Adds the given type to the manager. Raises an error of the type
        already exists unless <override> was set to True.

        The type has to be a sub-class of CustomType.
        '''

        if override and name in self:
            raise NameError('Cannot create type. "%s" already exists.'% name)

        # TODO: Restrict to Pointer?
        if not issubclass(cls, CustomType):
            raise ValueError('Given class is not a subclass of CustomType.')

        self[name] = cls
        return cls

    def get_decorators(self):
        '''
        Returns the attribute, function and virtual function decorator (in
        this order).
        '''

        return (self.attribute, self.function, self.virtual_function)

    def attribute(self, str_type, offset=0, str_is_ptr=True, str_size=0,
            flags=ATTR_READ_WRITE, converter_name=None, doc=None):
        '''
        Adds an attribute to a class.
        '''
        
        if str_type not in NATIVE_TYPES:
            converter_name = str_type
            str_type = 'ptr'

        # Getter method
        def fget(ptr_self):
            result = None
            func = getattr(ptr_self, 'get_' + str_type)
            if str_type == 'string':
                result = func(offset, str_is_ptr)
            else:
                result = func(offset)

            return self[converter_name](result)

        # Setter method
        def fset(ptr_self, value):
            func = getattr(ptr_self, 'set_' + str_type)
            if str_type == 'string':
                func(value, str_size, offset, str_is_ptr)
            else:
                func(value, offset)

        # Return the proper property object depending on the flags
        if flags & ATTR_READ_WRITE:
            return property(fget, fset, doc=doc)
        elif flags & ATTR_READ:
            return property(fget, doc=doc)
        elif flags & ATTR_WRITE:
            return property(fset=fset, doc=doc)

        # Raise an error as we cannot read or write the attribute
        raise AttributeError('Attribute is not readable or writeable.')

    def function(self, binary, identifier, parameters,
            convention=Convention.THISCALL, srv_check=True,
            converter_name=None, doc=None):
        '''
        Adds a function to a class.
        '''

        func = _EvalFunction(make_function(binary, identifier, convention,
            parameters, srv_check, self.create_converter(converter_name)))

        func.__doc__ = doc
        return func

    def virtual_function(self, index, parameters,
            convention=Convention.THISCALL, converter_name=None, doc=None):
        '''
        Adds a virtual function to a class.
        '''

        func = _EvalVirtualFunction(index, convention, parameters,
            self.create_converter(converter_name))

        func.__doc__ = doc
        return func

    def create_converter(self, name):
        '''
        Creates a callable converter by name. That means the type is evaluated
        when the converter gets called.
        '''

        return lambda x: self[name](x)

# Create a manager that can be used by all programs
type_manager = TypeManager()


class _EvalFunction(Function):
    '''
    Intern use only.

    This is a wrapper for the Thiscall constructor.
    '''

    def __init__(self, func):
        super(_EvalFunction, self).__init__(func)
        self.is_virtual = False

    def __call__(self, this):
        '''
        Returns a new Thiscall object.
        '''

        func = Thiscall(self, this)
        func.__doc__ = self.__doc__
        return func


class _EvalVirtualFunction(object):
    '''
    Intern use only!

    This is a wrapper for the Thiscall constructor. We can only evaluate the
    virtual function when we get a valid this-pointer.

    Step 1: Save the given information and wait for a this-pointer.
    Step 2: Make a function and convert it to a Thiscall object.
    '''

    def __init__(self, index, convention, parameters, converter):
        '''
        Step 1.
        '''

        self.index      = index
        self.convention = convention
        self.parameters = parameters
        self.converter  = converter
        self.is_virtual = True

    def __call__(self, this):
        '''
        Step 2.
        '''

        func = Thiscall(
            this.make_virtual_function(
                self.index,
                self.convention,
                self.parameters,
                self.converter
            ),
            this
        )
        func.__doc__ = self.__doc__
        return func

    def __getattr__(self, attr):
        '''
        Emulates the Function class by raising an error if you try to access
        an attribute or function of the Function class.
        '''

        if attr in dir(Function):
            raise AttributeError('This function is virtual. You need a poin' \
                'ter to access this attribute.')

        raise AttributeError('"%s" has no attribute "%s"'% (
            self.__class__.__name__, attr))


class Thiscall(Function):
    '''
    This class is used to emulate functions which require a this-pointer. By
    using this class you don't always need to pass the pointer to the
    function. This is done behind the scene.
    '''

    def __init__(self, func, this):
        '''
        Initializes the function and saves the this-pointer.
        '''

        super(Thiscall, self).__init__(func)
        self.this = this

    def __call__(self, *args):
        '''
        Calls the function.
        '''

        return super(Thiscall, self).__call__(self.this, *args)

    def call_trampoline(self, *args):
        '''
        Calls the trampoline
        '''

        return super(Thiscall, self).call_trampoline(self.this, *args)


# =============================================================================
# >> FUNCTIONS
# =============================================================================
def make_function(binary, identifier, convention, parameters, srv_check=True,
        converter=lambda x: x):
    '''
    This is a shortcut for creating new functions.
    '''

    binary = binutils.find_binary(binary, srv_check)

    # Is it a signature?
    if os.name == 'nt' and ' ' in identifier:
        sig = binascii.unhexlify(identifier.replace(' ', ''))
        func_ptr = binary.find_signature(sig)

        # Raise an error here. Maybe the user wanted to use a symbol, but
        # accidentally added a space
        if not func_ptr:
            raise ValueError('Could not find signature "%s".'% repr(sig))
    else:
        func_ptr = binary[identifier]

        # Same here. Maybe the user wanted to use a signature, but forgot
        # to add spaces
        if not func_ptr:
            raise ValueError('Could not find symbol "%s".'% identifier)

    # Return a new Function object
    return func_ptr.make_function(convention, parameters, converter)