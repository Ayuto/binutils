# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
import os
import binascii

from configobj import ConfigObj

# binutils
from binutils import *


# =============================================================================
# >> CONSTANTS
# =============================================================================
# All available keys in data files
KEY_BINARY            = 'binary'
KEY_IDENTIFIER        = 'identifier'
KEY_CONVENTION        = 'convention'
KEY_PARAMETERS        = 'parameters'
KEY_SRV_CHECK         = 'srv_check'
KEY_CONVERTER         = 'converter'
KEY_STR_SIZE          = 'str_size'
KEY_TYPE_SIZE         = 'type_size'
KEY_ATTR_FLAGS        = 'flags'
KEY_DOCUMENTATION     = 'documentation'
KEY_ALIGNED           = 'aligned'

# Sub information keys
KEY_ATTRIBUTES        = 'attributes'
KEY_FUNCTIONS         = 'functions'
KEY_VIRTUAL_FUNCTIONS = 'virtual_functions'

# A tuple of all supported native types
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
    'string',
    'string_array'
)

# Read/write flags for class attributes
class AttrFlags:
    READ       = 1 << 0
    WRITE      = 1 << 1
    READ_WRITE = READ | WRITE


# =============================================================================
# >> CLASSES
# =============================================================================
class Pipe(dict):
    '''
    This class is mostly used to create a pipe to global functions. But you
    can also create member functions as global functions.

    You cannot create virtual functions or attributes with this class!
    '''

    def __init__(self, manager, *files):
        '''
        Initializes the pipe and parses all given files. <manager> must be a
        TypeManager object.

        Example for a file:

        [<function name>]
        # Required:
        binary     = <path to binary>
        identifier = <symbol or signature>
        parameters = <parameters>
        converter  = <type or converter>

        # Optional:
        srv_check     = <default: True>
        convention    = <calling convention, default: CDECL>
        documentation = <default: ''>
        '''

        # Save the manager for later converter access
        self.type_manager = manager

        # Parse all data from the given files
        data = parse_data(
            read_files(*files),
            (
                (KEY_BINARY, str, None),
                (KEY_IDENTIFIER, str, None),
                (KEY_CONVENTION, lambda x: getattr(Convention, x), 'CDECL'),
                (KEY_PARAMETERS, str, None),
                (KEY_SRV_CHECK, as_bool, 'True'),
                (KEY_CONVERTER, lambda x: None if x == 0 else x, 0),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        # Add all functions to the pipe
        for func_name, func_data in data:
            self.add_function(func_name, *func_data)

    def __getattr__(self, attr):
        '''
        Redirects to __getitem__, which returns a function called <attr>.
        '''

        return self[attr]

    def add_function(self, name, binary, identifier, convention, parameters,
            srv_check=True, converter_name=None, doc=None):
        '''
        Adds a function to the pipe.
        '''

        func = self[name] = make_function(binary, identifier,
            convention,
            parameters,
            srv_check,
            self.type_manager.create_converter(converter_name),
            doc
        )

        return func


class TypeManager(dict):
    '''
    The TypeManager is an extremely powerful class, which gives you the
    ability to create new custom types. That means you can reconstruct
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

    def create_converter(self, name):
        '''
        Creates a callable converter by name. That means the type is evaluated
        when the converter gets called.
        '''

        return lambda x: self[name](x)

    def create_pipe_from_file(self, *files):
        '''
        Creates a new Pipe object.

        For more information take a look at the Pipe documentation.
        '''

        return Pipe(self, *files)

    def create_type(self, name, cls_dict, size=None, override=False):
        '''
        Creates a new type. If the type already exists, an error will be
        raised unless <override> was set to True.
        '''

        return self.add_type(
            type(name, (Pointer,), cls_dict),
            size,
            override
        )

    def create_type_from_file(self, type_name, *files):
        '''
        This is the same like TypeManager.create_type(), but creates a type
        from the given files.

        If you have a type that inherits another one you can pass the files in
        the following order to emulate inheritance:
        InheritedType = manager.create_type_from_file('InheritedType',
            'BaseType.ini', 'InheritedType.ini')

        Example for a file:

        # Optional -- required for type creation
        type_size = 72

        [attributes]
            [[<attribute name>]]
            # Required:
            converter  = <type or converter>
            identifier = <offset>

            # Optional:
            str_size      = <size of string array, default: 0>
            flags         = <attribute flags, default: READ_WRITE>
            documentation = <default: ''>

        [functions]
            [[<function name>]]
            # Required
            binary = <path to binary>
            identifier = <signature or symbol>
            parameters = <parameters>

            # Optional
            convention    = <calling convention, default: THISCALL>
            srv_check     = <default: True>
            converter     = <converter name, default: None>
            documentation = <default: ''>

        [virtual_functions]
            [[<function name>]]
            # Required:
            identifier = <index in the virtual table>
            parameters = <parameters>

            # Optional:
            convention    = <calling convention, default: THISCALL>
            converter     = <converter name, default: None>
            documentation = <default: ''>


        All sections (attributes, functions, virtual_functions) are optional.
        '''

        cls_dict = {}
        raw_data = read_files(*files)
        size     = raw_data.get(KEY_TYPE_SIZE)

        # Parse the attributes
        attributes = parse_data(
            raw_data.get(KEY_ATTRIBUTES, {}),
            (
                (KEY_CONVERTER, str, None),
                (KEY_IDENTIFIER, int, None),
                (KEY_STR_SIZE, int, 0),
                (KEY_ATTR_FLAGS, lambda x: getattr(AttrFlags, x), 'READ_WRITE'),
                (KEY_ALIGNED, as_bool, 'False'),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        for name, data in attributes:
            cls_dict[name] = self.attribute(*data)

        # Parse the functions
        functions = parse_data(
            raw_data.get(KEY_FUNCTIONS, {}),
            (
                (KEY_BINARY, str, None),
                (KEY_IDENTIFIER, str, None),
                (KEY_PARAMETERS, str, None),
                (KEY_CONVENTION, lambda x: getattr(Convention, x), 'THISCALL'),
                (KEY_SRV_CHECK, as_bool, 'True'),
                (KEY_CONVERTER, lambda x: None if x == 0 else x, 0),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        for name, data in functions:
            cls_dict[name] = self.function(*data)

        # Parse the virtual functions
        virtual_functions = parse_data(
            raw_data.get(KEY_VIRTUAL_FUNCTIONS, {}),
            (
                (KEY_IDENTIFIER, int, None),
                (KEY_PARAMETERS, str, None),
                (KEY_CONVENTION, lambda x: getattr(Convention, x), 'THISCALL'),
                (KEY_CONVERTER, lambda x: None if x == 0 else x, 0),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        for name, data in virtual_functions:
            cls_dict[name] = self.virtual_function(*data)

        # Create the type and return it
        return self.create_type(type_name, cls_dict, size and int(size))

    def add_type(self, cls, size=None, override=False):
        '''
        Adds the given type to the manager. Raises an error of the type
        already exists unless <override> was set to True.

        The type has to be a sub-class of Pointer.

        Be careful when you implement an __init__ function. It requires the
        the following signature:
        __init__(self, ptr)

        This function overrides your function with an __init__ function that
        will eventually allocate space (if <ptr> is None and <size> was
        given).
        '''

        name = cls.__name__
        if not override and name in self:
            raise NameError('Cannot create type. "%s" already exists.'% name)

        if not issubclass(cls, Pointer):
            raise ValueError('Given class is not a subclass of Pointer.')

        # Save the old __init__
        old_init = cls.__init__ if '__init__' in cls.__dict__ else \
            lambda self, ptr: None

        def __init__(ptr_self, ptr=None):
            # Call the old __init__ function
            old_init(ptr_self, ptr)

            # Do we want to wrap a pointer?
            if ptr is not None:
                super(cls, ptr_self).__init__(ptr)
                return

            # Do we have the size information?
            if size is not None:
                super(cls, ptr_self).__init__(alloc(size))
                return

            raise ValueError('Cannot allocate space for type "%s". Missing ' \
                'size information.'% cls.__name__)

        cls.__init__ = __init__
        self[name] = cls
        return cls

    def get_decorators(self):
        '''
        Returns the attribute, function and virtual function decorator (in
        this order).
        '''

        return (self.attribute, self.function, self.virtual_function)

    def attribute(self, str_type, offset=0, str_size=0,
            flags=AttrFlags.READ_WRITE, aligned=False, doc=None):
        '''
        Adds an attribute to a class.
        '''

        # We don't want the user to pass redundant information
        if str_type != 'string_array' and str_size != 0:
            raise ValueError('Parameter "str_size" is only required for att' \
                'ributes of type "string_array".')

        converter_name = None
        if str_type not in NATIVE_TYPES:
            converter_name = str_type
            str_type = 'ptr'
        elif aligned:
            raise ValueError('You cannot align an attribute of type "%s".'% \
                str_type)

        # Getter method
        def fget(ptr_self):
            if aligned:
                return self[converter_name](ptr_self + offset)

            result = getattr(ptr_self, 'get_' + str_type)(offset)
            if str_type == 'ptr':
                return self[converter_name](result)

            return result

        # Setter method
        def fset(ptr_self, value):
            if aligned:
                # Using memcpy() is probably the best, but that would require
                # a size, which we cannot know here.
                raise NotImplementedError('Setting aligned types is current' \
                    'ly not supported.')

            func = getattr(ptr_self, 'set_' + str_type)
            if str_type == 'string_array':
                func(value, offset, str_size)
            else:
                func(value, offset)

        # Return the proper property object depending on the flags
        if flags & AttrFlags.READ_WRITE:
            return property(fget, fset, doc=doc)
        elif flags & AttrFlags.READ:
            return property(fget, doc=doc)
        elif flags & AttrFlags.WRITE:
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

# Create a manager that can be used by all programs
type_manager = TypeManager()


class _EvalFunction(Function):
    '''
    This is a wrapper for the Thiscall constructor.
    '''

    def __init__(self, func):
        super(_EvalFunction, self).__init__(func)
        self.is_virtual = False

    def __get__(self, this, cls):
        '''
        Returns a new Thiscall object.
        '''

        func = Thiscall(self, this)
        func.__doc__ = self.__doc__
        return func


class _EvalVirtualFunction(object):
    '''
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

    def __get__(self, this, cls):
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
        Calls the function. The this-pointer is automatically passed to the
        function.
        '''

        return super(Thiscall, self).__call__(self.this, *args)

    def call_trampoline(self, *args):
        '''
        Calls the trampoline. The this-pointer is automatically passed to the
        trampoline.
        '''

        return super(Thiscall, self).call_trampoline(self.this, *args)


# =============================================================================
# >> FUNCTIONS
# =============================================================================
def make_function(binary, identifier, convention, parameters, srv_check=True,
        converter=lambda x: x, doc=None):
    '''
    Creates a new function. Signatures have to be passed with spaces.
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

    func = func_ptr.make_function(convention, parameters, converter)
    func.__doc__ = doc
    return func

def create_string(text, size=None):
    '''
    Creates a new string. If <size> is None len(<text>) + 1 bytes are allocated.
    Otherwise it will allocate <size> bytes.
    '''

    ptr = alloc(len(text) + 1 if size is None else size)
    try:
        ptr.set_string_array(text)
    except ValueError:
        ptr.dealloc()
        raise ValueError('String exceeds size of memory block.')

    return ptr


# =============================================================================
# >> HELPER FUNCTIONS
# =============================================================================
def read_files(*files):
    '''
    Reads all passed data files and converts them to a dictionary. If the
    given files provides a close() function, it will be called.
    '''

    data = {}
    for f in files:
        data.update(ConfigObj(f))
        try:
            f.close()
        except AttributeError:
            pass

    return data

def parse_data(raw_data, keys):
    '''
    Parses the data dictionary by converting the values of the given keys into
    the proper type or assigning them default values. Raises a KeyError if a
    key does not exist and if no default value is available.

    Returns a generator: (<name>, [<value of key0>, <value of key1>, ...])

    <keys> must have the following structure:
    ((<key name>, <converter>, <default value or None>), ...)


    Information about data which comes from a file:

    You can specialize every key by adding a "_nt" (for Windows) or a
    "_posix" (for Linux) to the end a key.

    For example:
    If you are using a signature on Windows, but a symbol on Linux, you have
    three possibilities to do that:

    1.
    identifier_nt    = <signature for Windows>
    identifier       = <symbol for Linux>

    2.
    identifier       = <signature for Windows>
    identifier_posix = <symbol for Linux>

    3.
    identifier_nt    = <signature for Windows>
    identifier_posix = <symbol for Linux>
    '''

    for func_name, func_data in raw_data.iteritems():
        data = []
        for key, converter, default in keys:
            # Get the OS specific key. If that fails, fall back to the shared
            # key. If that fails too, use the default value
            value = func_data.get(key + '_' + os.name, func_data.get(key, default))

            # If this is still None, we are missing that information
            if value is None:
                raise KeyError('Missing information for key "%s".'% key)

            data.append(converter(value))

        yield (func_name, data)

def as_bool(value):
    '''
    Converts a string that represents a boolean value into a boolean value.
    Raises a ValueError if the string doesn't represent such a value.
    '''

    value = value.lower()
    if value == 'true':
        return True

    if value == 'false':
        return False

    raise ValueError('Cannot convert "%s" to a boolean value.'% value)