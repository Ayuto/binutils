# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
import os
import binascii

# binutils
import helpers

from _binutils import *


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
KEY_SIZE              = 'size'
KEY_LENGTH            = 'length'
KEY_IS_ARRAY          = 'is_array'
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
class CustomType(Pointer):
    '''
    Subclass this type in order to create custom types. You have to set the
    attribute "__metaclass__" to an instance of TypeManager. This is required,
    because that registers the class automatically at the manager instance.
    '''

    def __init__(self, ptr=None):
        '''
        If <ptr> is not None the pointer will be wrapped by this class.
        Otherwise it allocates space and wraps the allocated space. This
        requires the <size> attribute to be set. If it is still None, a
        ValueError will be raised.
        '''

        if not hasattr(self, '__metaclass__') or \
                not isinstance(self.__metaclass__, TypeManager):

            raise ValueError('Attribute __metaclass__ must be an instance o' \
                'f "TypeManager".')

        if ptr is not None:
            return super(CustomType, self).__init__(ptr)

        if self.size is not None:
            return super(CustomType, self).__init__(alloc(self.size))

        raise ValueError('Cannot allocate space for type "%s". Missing size' \
            ' information.'% self.__class__.__name__)

    def __add__(self, other):
        return self.__class__(int(self) + int(other))

    def __radd__(self, other):
        return self + other

    def __sub__(self, other):
        return self.__class__(int(self) - int(other))

    def __rsub__(self, other):
        return self - other

    # Overload Pointer's size property
    size = None


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

    def __getattr__(self, attr):
        '''
        Redirection to TypeManager.__getitem__.
        '''

        return self[attr]

    def __call__(self, name, bases, cls_dict):
        '''
        Creates a new class and registers it automatically. The type has to be
        a subclass of CustomType.
        '''

        cls = type(name, bases, cls_dict)
        if not issubclass(cls, CustomType):
            raise ValueError('Custom type "%s" has to be a subclass of "Cus' \
                'tomType".'% name)

        self[name] = cls
        return cls

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

    def create_pipe(self, **cls_dict):
        '''
        This function is mostly used to create a pipe to global functions. But
        you can also create member functions as global functions (they will
        require a valid this-pointer as the first argument).
        '''

        return type('Pipe', (object,), cls_dict)

    def create_pipe_from_file(self, *files):
        '''
        Parses all given files and creates a new Pipe object.

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

        data = helpers.parse_data(
            helpers.read_files(*files),
            (
                (KEY_BINARY, str, None),
                (KEY_IDENTIFIER, str, None),
                (KEY_PARAMETERS, str, None),
                (KEY_CONVERTER, lambda x: None if x == 0 else x, 0),
                (KEY_SRV_CHECK, helpers.as_bool, 'True'),
                (KEY_CONVENTION, lambda x: getattr(Convention, x), 'CDECL'),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        cls_dict = {}
        for func_name, func_data in data:
            cls_dict[func_name] = self.pipe_function(*func_data)

        return self.create_pipe(**cls_dict)

    def create_type(self, name, cls_dict):
        '''
        Creates a new subclass of CustomType. Setting __metaclass__ is not
        necessary and would have no effect as it will be overriden.
        '''

        cls_dict['__metaclass__'] = self
        return self(name, (CustomType,), cls_dict)

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
        size = 72

        [attributes]
            [[<attribute name>]]
            # Required:
            converter  = <type or converter>
            identifier = <offset>

            # Optional:
            size          = <size of string array, default: 0>
            is_array      = <default: False>
            length        = <length of array, default: -1>
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

        raw_data = helpers.read_files(*files)
        size     = raw_data.get(KEY_SIZE)
        cls_dict = { 'size':  size and int(size)}

        # Parse the attributes
        attributes = helpers.parse_data(
            raw_data.get(KEY_ATTRIBUTES, {}),
            (
                (KEY_CONVERTER, str, None),
                (KEY_IDENTIFIER, int, None),
                (KEY_LENGTH, int, -1),
                (KEY_IS_ARRAY, helpers.as_bool, 'False'),
                (KEY_ALIGNED, helpers.as_bool, 'False'),
                (KEY_ATTR_FLAGS, lambda x: getattr(AttrFlags, x), 'READ_WRITE'),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        for name, data in attributes:
            cls_dict[name] = self.attribute(*data)

        # Parse the functions
        functions = helpers.parse_data(
            raw_data.get(KEY_FUNCTIONS, {}),
            (
                (KEY_BINARY, str, None),
                (KEY_IDENTIFIER, str, None),
                (KEY_PARAMETERS, str, None),
                (KEY_CONVERTER, lambda x: None if x == 0 else x, 0),
                (KEY_SRV_CHECK, helpers.as_bool, 'True'),
                (KEY_CONVENTION, lambda x: getattr(Convention, x), 'THISCALL'),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        for name, data in functions:
            cls_dict[name] = self.function(*data)

        # Parse the virtual functions
        virtual_functions = helpers.parse_data(
            raw_data.get(KEY_VIRTUAL_FUNCTIONS, {}),
            (
                (KEY_IDENTIFIER, int, None),
                (KEY_PARAMETERS, str, None),
                (KEY_CONVERTER, lambda x: None if x == 0 else x, 0),
                (KEY_CONVENTION, lambda x: getattr(Convention, x), 'THISCALL'),
                (KEY_DOCUMENTATION, str, '')
            )
        )

        for name, data in virtual_functions:
            cls_dict[name] = self.virtual_function(*data)

        # Create the type and return it
        return self.create_type(type_name, cls_dict)

    def create_function_typedef(self, name, convention, parameters,
            converter_name=None):
        '''
        Creates a function typedef. That means you can use the returned
        converter to wrap a pointer of a function without setting the function
        data again.
        '''

        cls = self[name] = lambda ptr: ptr.make_function(convention,
            parameters, self.create_converter(converter_name))

        return cls

    def pipe_function(self, binary, identifier, parameters,
            converter_name=None, srv_check=True, convention=Convention.CDECL,
            doc=None):
        '''
        Returns a new Function object.
        '''

        return make_function(binary, identifier, convention, parameters,
            self.create_converter(converter_name), srv_check, doc)

    def attribute(self, str_type, offset=0, length=-1, is_array=False,
            aligned=False, flags=AttrFlags.READ_WRITE, doc=None):
        '''
        Adds an attribute to a class.
        '''

        # We don't want the user to pass redundant information
        if length != -1 and (not is_array and str_type != 'string_array'):
            raise ValueError('A length is optional for arrays.')

        converter_name = None
        if str_type not in NATIVE_TYPES:
            converter_name = str_type
            str_type = 'ptr'

        elif aligned and not is_array:
            raise ValueError('You cannot align an attribute of type "%s".'% \
                str_type)

        # Getter method
        def fget(ptr_self):
            # Handle arrays
            if is_array:
                # This is a check for NULL pointers and required for unaligned
                # arrays
                ptr_self2 = ptr_self.get_ptr(offset)
                if aligned:
                    ptr_self = Pointer(int(ptr_self) + offset)
                else:
                    ptr_self = ptr_self2

                if str_type == 'ptr':
                    return ptr_self.make_ptr_array(
                        self[converter_name].size,
                        length,
                        self.create_converter(converter_name)
                    )

                return getattr(ptr_self, 'make_%s_array'% str_type)(length)

            # Handle aligned data types
            if aligned:
                return self[converter_name](ptr_self + offset)

            # Handle unaligned data types
            result = getattr(ptr_self, 'get_' + str_type)(offset)
            if str_type == 'ptr':
                return self[converter_name](result)

            return result

        # Setter method
        def fset(ptr_self, value):
            # Handle arrays
            if is_array:
                if isinstance(value, Pointer):
                    if length == -1:
                        if value.length == -1:
                            raise ValueError('Setting arrays requires a len' \
                                'gth.')

                        # If <length> is -1, but <value> has a length we are
                        # going to use its length instead
                        value = tuple(value)
                    else:
                        value = (value[x] for x in xrange(length))

                # Get the array and set all values
                array = fget(ptr_self)
                for index, val in enumerate(value):
                    array[index] = val

                return

            # Handle aligned data types
            if aligned:
                Pointer(value).copy(
                    ptr_self + offset, self[converter_name].size * length
                )
                return

            # Handle unaligned data types
            if str_type == 'string_array':
                ptr_self.set_string_array(value, offset, length)
            else:
                getattr(ptr_self, 'set_' + str_type)(value, offset)

        # Return the proper property object depending on the flags
        if flags & AttrFlags.READ_WRITE:
            return property(fget, fset, doc=doc)
        elif flags & AttrFlags.READ:
            return property(fget, doc=doc)
        elif flags & AttrFlags.WRITE:
            return property(fset=fset, doc=doc)

        # Raise an error as we cannot read or write the attribute
        raise AttributeError('Attribute is not readable or writeable.')

    def function(self, binary, identifier, parameters, converter_name=None,
            srv_check=True, convention=Convention.THISCALL, doc=None):
        '''
        Adds a function to a class.
        '''

        func = helpers._EvalFunction(
            make_function(
                binary,
                identifier,
                convention,
                parameters,
                self.create_converter(converter_name),
                srv_check
            )
        )

        func.__doc__ = doc
        return func

    def virtual_function(self, index, parameters,
            converter_name=None, convention=Convention.THISCALL, doc=None):
        '''
        Adds a virtual function to a class.
        '''

        func = helpers._EvalVirtualFunction(index, convention, parameters,
            self.create_converter(converter_name))

        func.__doc__ = doc
        return func

# Create a manager that can be used by all programs
type_manager = TypeManager()


# =============================================================================
# >> FUNCTIONS
# =============================================================================
def make_function(binary, identifier, convention, parameters,
        converter=lambda x: x, srv_check=True, doc=None):
    '''
    Creates a new function. Signatures have to be passed with spaces.
    '''

    binary = find_binary(binary, srv_check)

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

def callback(convention, params):
    '''
    This decorator is used to create C++ functions, which call back to the
    decorated Python function:

    EXAMPLE:

    @binutils.callback(Convention.CDECL, 'ii)i')
    def add(x, y):
        return x + y

    Don't forget to call add.free() when the function is not needed
    anymore.
    '''

    def wait_for_py_func(py_func):
        return Callback(py_func, convention, params)

    return wait_for_py_func