# Calling a C function #
Imagine you have a shared library called "testlib" which defines this public function.
```
int multiply(int x, int y)
{
    return x * y;
}
```
It's pretty easy to call this function.
```
import binutils

from binutils import Convention

# Find the library
testlib = binutils.find_binary('testlib')

# Get the function we want to call
multiply_ptr = testlib['multiply']

# Convert the Pointer object to a Function object (a subclass of Pointer).
# The calling convention of this function is CDECL.
# The two "i" mean that the function requires two integers.
# The last "i" means that it returns an integer.
# See also: http://code.google.com/p/binutils/source/browse/trunk/src/thirdparty/DynamicHooks/DynamicHooks.h#42
# The ")" just seperates the argument types from the return type.
multiply = multiply_ptr.make_function(Convention.CDECL, 'ii)i')

# We can now call it like a normal Python function.
print multiply(5, 5) # Output: 25
```