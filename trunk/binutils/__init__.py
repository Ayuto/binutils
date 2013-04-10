'''
$Rev$
$Author$
$Date$
'''

# =============================================================================
# >> IMPORTS
# =============================================================================
# Python
import os

from functools import wraps

# binutils
from _binutils import *


# =============================================================================
# >> CLASSES
# =============================================================================
class BinaryError(Exception): pass
class PointerError(Exception): pass


class Convention:
    '''
    cdecl    - Used for normal functions.
    thiscall - Used for functions within a class.
    '''

    cdecl, thiscall = range(2)


class HookAction:
    '''
    Continue - Calls the original function as usual.
    Modified - Calls the original function with modified parameters.
    Override - Blocks the original function and uses a custom return value.
    '''

    Continue, Modified, Override = range(3)


class HookType:
    '''
    Pre  - Calls the callback before the original function.
    Post - Calls the callback after the orignal function.
    '''

    Pre, Post = range(2)


class Binary(object):
    '''
    This is a wrapper for binary specific functions, so you do not need to
    pass the binary handle and/or size to the original functions. Also, this
    wrapper includes some new functions.
    '''

    # Binary file extension depending on the current system
    FILE_EXTENSION = 'dll' if os.name == 'nt' else 'so'

    def __init__(self, path):
        '''
        Initializes the Binary object by saving the binary's handle and size.

        @param path Path to a binary file without its extension.
        '''

        self.path = os.path.abspath(path)

        # Add the file extension if it's missing
        if not self.path.endswith('.' + self.FILE_EXTENSION):
            self.path += '.' + self.FILE_EXTENSION

        try:
            self.binary, self.handle, self.size = getBinary(self.path)
        except:
            raise BinaryError('Could not open binary: %s'% self.path)

    def __repr__(self):
        '''
        Returns a string containing the binary's handle and size.

        @return String containing information about the binary.
        '''

        return 'Binary details for %s\nHandle: %i\nSize: %i'% \
            (self.path, self.handle, self.size)

    def findSignature(self, sig):
        '''
        Returns the address of the given signature.

        @param sig Signature to search for.

        @return None or address to the first match of the signature.
        '''

        return findSignature(self.binary, sig, len(sig))

    def findSymbol(self, symbol):
        '''
        Returns the address of the given symbol. Only available on Linux!

        @param symbol Name of a symbol.

        @return None or address to the symbol
        '''

        return findSymbol(self.binary, symbol)

    def getPointer(self, sig, offset):
        '''
        Returns the address of a pointer within a function.

        @param sig    Signature to search for.
        @param offset Offset to the pointer.

        @return None or address to the pointer.
        '''

        addr = self.findSignature(sig)
        return addr and getValue('p', addr + offset)

    def findString(self, searchstr):
        '''
        Returns the address of a string. Asterisks will be skipped (exactly
        like '\x2A', since their hexadecimal value is 2A).

        @param string String to search for.

        @return None or address of the string.
        '''

        # Actually, this is the same like searching a signature
        return self.findSignature(searchstr)


class Pointer(int):
    '''
    This class is the parent of all new types created by the TypeManager.
    '''

    def __new__(cls, this):
        '''
        Creates a new instance of this type by calling all accessor setup
        functions.
        '''

        if not this:
            raise PointerError('"this" pointer is NULL')

        # Try to setup the function "getPtr()" for each private method
        for name in dir(cls):
            # Filter non-private functions as only private methods are created
            # by the TypeManager
            if not name or name[0] != '_':
                continue

            try:
                getattr(cls, name)._setupGetPtr(this)
            except AttributeError:
                pass

        return super(Pointer, cls).__new__(cls, this)

    def __getitem__(self, index):
        '''
        Every pointer can be handled like an array, but be careful! The
        pointer might not be an array, so you would get an invalid address.
        '''

        if not self._size:
            raise TypeError('Size of type is unknown.')

        if not isinstance(index, int):
            raise TypeError('Indexes must be integers.')

        ptr = getValue('p', self + (self._size * index))
        return None if not ptr else self.__class__(ptr)


class AttrFlag:
    '''
    READ       - Attribute is readable
    WRITE      - Attribute is writeable
    READ_WRITE - Attribute is readable and writeable
    '''

    READ, WRITE, READ_WRITE = range(1, 4)


class TypeManager(dict):
    '''
    This class will hold all information about registered types. These types
    are dynamically created, so they are comfortable to use. You can handle
    them like normal classes (because they are normal classes) - e.g. you can
    subclass them to provide an easier API.
    '''

    def createType(self, cname, vtable={}, methods={}, attrs={}, doc=None,
            size=None):
        '''
        Creates a new type.

        @param cname   Name of the class
        @param vtable  Information about the virtual function table.
        @param methods Information about methods.
        @param attrs   Information about class attributes.
        @param doc     Documentation about the class.
        @param size    Size of the type.

        @return Python class, which works as a bridge to a valid pointer.

        vtable format:
        {<function's name>: {
            'offset'  : <offset to the function>,
            'params'  : <parameter signature>,
            ['rettype': <class, which will wrap pointers>,]
            ['doc'    : <documentation about the function>,]
            },
        }

        methods format:
        {<function's name>: {
            'addr'    : <address of the function>,
            'params'  : <parameter signature>,
            ['rettype': <class, which will wrap pointers>,]
            ['doc'    : <documentation about the function>,]
            },
        }

        attrs format:
        {<attribute's name>: {
            'offset': <offset from the this pointer to the attribute>,
            'type'  : <class, which will wrap pointers>,
            ['flags': <read/write/read-write flag>,]
            ['doc'  : <documentation about the attribute>,]
            },
        }
        '''

        clsdict = {'__doc__': doc, '_size': size, '_manager': self}

        # Create virtual functions
        for name, vfunc in vtable.iteritems():
            if name.startswith('_'):
                raise NameError('Names, which start with a leading undersco' \
                    're are reserved.')

            clsdict[name] = self.__createVirtualFunction(vfunc['offset'],
                vfunc['params'], vfunc.get('rettype', None),
                vfunc.get('doc', None))

        # Create methods
        for name, method in methods.iteritems():
            if name.startswith('_'):
                raise NameError('Names, which start with a leading undersco' \
                    're are reserved.')

            clsdict[name] = self.__createMethod(method['addr'],
                method['params'], vfunc.get('rettype', None),
                vfunc.get('doc', None))

        # Create attributes
        for name, attr in attrs.iteritems():
            if name.startswith('_'):
                raise NameError('Names, which start with a leading undersco' \
                    're are reserved.')

            clsdict[name] = self.__createAttribute(attr['offset'],
                attr['type'], attr.get('flags', AttrFlag.READ_WRITE),
                attr.get('doc', None))

        cls = type(cname, (Pointer,), clsdict)
        self[cname] = {
            'type'   : cls,
            'vtable' : vtable,
            'methods': methods,
            'attrs'  : attrs,
            'doc'    : doc,
            'size'   : size,
        }

        return cls

    def deleteType(self, name):
        '''
        Deletes a type by its name.

        @param name Name of the type.
        '''

        self.pop(name, 0)

    def getType(self, name):
        '''
        Returns the requested type, if it was registered, else None.

        @param name Name of the registered type.

        @return KeyError or the requested type.
        '''

        try:
            return self[name]['type']
        except KeyError:
            raise KeyError('Unknown type "%s"'% name)

    def getTypeData(self, name):
        '''
        Returns the data of the requested type.

        @param name Name of the registered type.

        @return KeyError or data of the requested type.
        '''

        try:
            return self[name]
        except KeyError:
            raise KeyError('Unknown type "%s"'% name)

    def __createVirtualFunction(self, offset, params, rettype=None, doc=None):
        '''
        Creates a bridge to a virtual function.

        @param offset  Offset to the virtual function.
        @param params  Parameter string, which represents the arguments.
        @param rettype Classname, which will wrap returned pointers.
        @param doc     Documentation about the function.

        @return New virtual function bridge.
        '''

        def method(this, *args):
            retval = callFunction(method.getPtr(), Convention.thiscall,
                params, this, *args)

            # No pointer returned?
            if params[~0] != 'p':
                return retval

            # Invalid pointer?
            if not retval:
                return None

            try:
                cls = self.getType(rettype)
            except KeyError:
                return Pointer(retval)
            else:
                return cls(retval)

        # Setup function
        def _setupGetPtr(this):
            addr = findVirtualFunc(this, offset)
            method.getPtr = lambda: addr

        method.__doc__       = doc
        method._setupGetPtr  = _setupGetPtr
        method.isVirtual     = lambda: True

        def raiseError():
            raise PointerError('Only instances are able to return the funct' \
                'ion\'s pointer')
        method.getPtr        = lambda: raiseError()

        method.getOffset     = lambda: offset
        method.getParams     = lambda: params
        method.getRetType    = lambda: self.getType(rettype)
        method.getTrampoline = lambda: getTrampoline(method.getPtr())
        method.unhook        = lambda callback, htype=HookType.Pre: \
            unhookFunction(method.getPtr(), htype, callback)

        method.hook = lambda callback, htype=HookType.Pre: hookFunction(
            method.getPtr(), Convention.thiscall, params, htype, callback)

        return method

    def __createMethod(self, addr, params, rettype=None, doc=None):
        '''
        Creates a bridge to a class method.

        @param addr    Address of the method.
        @param params  Parameter string, which represents the arguments.
        @param rettype Classname, which will wrap returned pointers.
        @param doc     Documentation about the method.

        @return New class method bridge.
        '''

        def method(this, *args):
            retval = callFunction(addr, Convention.thiscall, params, this,
                *args)

            # No pointer returned?
            if params[~0] != 'p':
                return retval

            # Invalid pointer?
            if not retval:
                return None

            try:
                cls = self.getType(rettype)
            except KeyError:
                return Pointer(retval)
            else:
                return cls(retval)

        method.__doc__       = doc
        method._setupGetPtr  = _setupGetPtr
        method.isVirtual     = lambda: False
        method.getPtr        = lambda: addr
        method.getParams     = lambda: params
        method.getRetType    = lambda: self.getType(rettype)
        method.getTrampoline = lambda: getTrampoline(addr)
        method.unhook        = lambda callback, htype=HookType.Pre: \
            unhookFunction(addr, htype, callback)

        method.hook = lambda callback, htype=HookType.Pre: hookFunction(addr,
            Convention.thiscall, params, htype, callback)

        return method

    def __createAttribute(self, offset, attrtype, flags=AttrFlag.READ_WRITE,
            doc=None):
        '''
        Creates a bridge to a class attribute.

        @param offset   Offset to the attribute.
        @param attrtype Type of the attribute.
        @param flags    Read/write/read-write flag.
        @param doc      Documentation about the function.

        @return New attribute bridge.
        '''

        param = attrtype if attrtype in 'bifs' else 'p'

        def getAttr(this):
            retval = getValue(param, this + offset)

            # No pointer returned?
            if param != 'p':
                return retval

            # Invalid pointer?
            if not retval:
                return None

            try:
                cls = self.getType(attrtype)
            except KeyError:
                return Pointer(retval)
            else:
                return cls(retval)

        def setAttr(this, value):
            setValue(param, this + offset, value)

        if flags == AttrFlag.READ:
            return property(getAttr, doc=doc)
        elif flags == AttrFlag.WRITE:
            return property(fset=setAttr, doc=doc)
        elif flags == AttrFlag.READ_WRITE:
            return property(getAttr, setAttr, doc=doc)

        raise ValueError('Invalid flags given: %i'% flags)


class FunctionManager(dict):
    '''
    This class will hold all information about registered functions.
    '''

    def __init__(self, typemngr=None):
        '''
        Creates a new FunctionManager instance.

        @param typemngr A TypeManager instance, so functions can return the
                        proper type.
        '''

        self.typemngr = typemngr

    def createFunction(self, name, addr, conv, params, rettype=None,
            doc=None):
        '''
        Creates a function dynamically.

        @param name    Name of the function.
        @param addr    Address of the function.
        @param conv    Function's calling convention.
        @param params  Parameter string, which represents the arguments.
        @param rettype Classname, which will wrap returned pointers.
        @param doc     Documentation about the method.

        @return A callable Python function.
        '''

        def call(*args):
            retval = callFunction(addr, conv, params, *args)

            # No pointer, no class to wrap the pointer or no TypeManager?
            if rettype is None or params[~0] != 'p' or self.typemngr is None:
                return retval

            # Invalid pointer?
            if not retval:
                return None

            try:
                cls = self.typemngr.getType(rettype)
            except KeyError:
                return Pointer(retval)
            else:
                return cls(retval)

        call.__doc__       = doc
        call.isVirtual     = lambda: False
        call.getPtr        = lambda: addr
        call.getConv       = lambda: conv
        call.getParams     = lambda: params
        call.getRetType    = lambda: self.typemngr.getType(rettype)
        call.getTrampoline = lambda: getTrampoline(addr)
        call.unhook        = lambda callback, htype=HookType.Pre: \
            unhookFunction(addr, htype, callback)

        call.hook = lambda callback, htype=HookType.Pre: hookFunction(addr,
            conv, params, htype, callback)

        self[name] = {
            'func'   : call,
            'addr'   : addr,
            'conv'   : conv,
            'params' : params,
            'rettype': rettype,
            'doc'    : doc,
        }

        return call

    def getFunction(self, name):
        '''
        Returns the requested function.

        @param name Name of the function.

        @return KeyError or a callable Python function.
        '''

        try:
            return self[name]['func']
        except KeyError:
            raise KeyError('Unknown function "%s"'% name)

    def getFunctionData(self, name):
        '''
        Returns the data of the requested function.

        @param name Name of the registered function.

        @return KeyError or data of the requested function.
        '''

        try:
            return self[name]
        except KeyError:
            raise KeyError('Unknown function "%s"'% name)


# =============================================================================
# >> FUNCTIONS
# =============================================================================
def callFunction(func, mode, sig, *args):
    '''
    Calls a function dynamically.

    @param func Address of a function.
    @param mode Calling convention (Convention class).
    @param sig  Function signature representing the function's arguments.
    @param args Arguments specified by the function's signature.

    @return Return value specified by the function's signature.
    '''

    argssig, rettype = sig.split(')')
    if len(argssig) != len(args):
        raise ValueError('Signature (%i) and arguments (%i) do not line up'% \
            (len(argssig), len(args)))

    resetVM()
    setMode(__DYNCALL_MODES.get(mode))
    for type, arg in zip(argssig, args):
        setArg(type, arg)

    return call(func, rettype)


# =============================================================================
# >> DECORATORS
# =============================================================================
def Types(*types):
    '''
    Decorator to convert arguments to the given types. This is not restricted
    to pure types, but callable objects (a NoneType will be skipped).

    EXAMPLE:

    @Types(int, float, str)
    def test(x, y, z):
        print (x, y, z)

    test(1.3, 5, 10)
    # Output: (1, 5.0, '10')
    '''

    def call(func):
        @wraps(func)
        def wrapper(*args):
            return func(*formatArgs(types, args))
        return wrapper
    return call


# =============================================================================
# >> HELPER FUNCTIONS
# =============================================================================
def formatArgs(targs, args):
    '''
    Returns an iterator of (targs[0](args[0]), targs[1](args[1]), ...). A
    NoneType will be skipped.
    '''

    for t, a in zip(types, args):
        if t is None:
            yield a
        else:
            yield t(a)


# =============================================================================
# >> GLOBAL VARIABLES
# =============================================================================
# Grabbed from dyncall.h
__DYNCALL_MODES = {
    Convention.cdecl: 0,
    Convention.thiscall: 5 if os.name == 'nt' else 6
}