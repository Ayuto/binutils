# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
import os

from configobj import ConfigObj

# binutils
from _binutils import *


# =============================================================================
# >> CLASSES
# =============================================================================
class _EvalFunction(Function):
    '''
    Emulates a bound method.
    '''

    def __init__(self, func):
        super(_EvalFunction, self).__init__(func)
        self.is_virtual = False

    def __get__(self, this, cls):
        '''
        Returns <self> if <this> is None. Otherwise it returns a new Thiscall
        object.
        '''

        func = self if this is None else Thiscall(self, this, False)
        func.__doc__ = self.__doc__
        return func


class _EvalVirtualFunction(object):
    '''
    Emulates a bound method. We can only evaluate the function's address if a
    valid this-pointer was passed.
    '''

    def __init__(self, index, convention, parameters, converter):
        self.index      = index
        self.convention = convention
        self.parameters = parameters
        self.converter  = converter
        self.is_virtual = True

    def __call__(self, this, *args):
        '''
        This will be called when the function was accessed via the class. We
        still have to wait for a valid this-pointer.
        '''

        return self.__eval_func(this)(*args)

    def __get__(self, this, cls):
        '''
        Returns <self> if <this> is None. Otherwise it returns a new Thiscall
        object.
        '''

        return self if this is None else self.__eval_func(this)

    def __eval_func(self, this):
        '''
        Evaluates the function's address by using the given this-pointer.
        '''

        this = Pointer(this)
        func = Thiscall(
            this.make_virtual_function(
                self.index,
                self.convention,
                self.parameters,
                self.converter
            ),
            this,
            True
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

    def __init__(self, func, this, is_virtual):
        '''
        Initializes the function and saves the this-pointer.
        '''

        super(Thiscall, self).__init__(func)
        self.this = this
        self.is_virtual = is_virtual

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

    You can specialize every key by adding "_nt" (for Windows) or "_posix"
    (for Linux) to the end a key.

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