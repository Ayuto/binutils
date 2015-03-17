You want to create a new type or restructure an existing type? Nothing easier than that!

In this example we want to create the following struct:
```
struct PlayerInfo_t
{
    char[256] m_szName;
    int       m_iHealth;
};
```

After that we want to test the type with the following functions:
```
// Case #1
void TheFunction1(PlayerInfo_t* pInfo);

// Case #2
PlayerInfo_t* TheFunction2();
```

And here we go!
```
import binutils

# Create a new TypeManager object.
manager = binutils.TypeManager()

# Create the type
PlayerInfo = manager.create_type(
    'PlayerInfo',
    {
        # This creates an attribute of the type "string_array" at offset 0
        # The array has 256 characters
        'name': manager.attribute('string_array', 0, 256),
        
        # This creates a new "int" attribute at offset 256
        'health': manager.attribute('int', 256)
    },
    # Set the type size
    260
)


# Case #1: Create a new pointer and call a function
# Create a new pointer
info = PlayerInfo()

# Set the attributes
info.name   = 'binutils'
info.health = 100

# Call TheFunction1() with the pointer
TheFunction1(info)

# Deallocate the pointer
info.dealloc()


# Case #2: Call a function and wrap the returned pointer
# Call TheFunction2() and wrap the pointer. You can also add the type as a
# converter, so you don't need to wrap the return value everytime you call
# TheFunction()
info = PlayerInfo(TheFunction2())

# Print the attributes
print info.name
print info.health
```