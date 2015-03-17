**_Read DynamicFunctionCalls before!_**


---


# Pre-hooking a C function #
We will now do some pre-hook operations on the previous defined function.

## Modifying arguments ##
You can modify the arguments before they are passed to the original function.
```
def my_pre_hook(args):
   # Print all arguments passed to the function
   print tuple(args)

   # Add +1 to the first argument
   args[0] += 1

# Add a pre-hook
multiply.add_pre_hook(my_pre_hook)

# Call "multiply" and print the output
# Output:
# (5, 5)
# 30
print multiply(5, 5)

# Remove the pre-hook
multiply.remove_pre_hook(my_pre_hook)
```

## Overriding a function ##
You can also override the function, so it will never get called.
```
def my_pre_hook(args):
    # Continue as usual if the first argument is not 5
    if args[0] != 5:
        # "return" or "return None" doesn't override the function
        return

    # In every other case we want to return 42. Note the orignal function
    # will not get called!
    return 42

# Add the pre-hook
multiply.add_pre_hook(my_pre_hook)

# Call the function with something else than 5 as the first parameter
print multiply(10, 5) # Output: 50

# Call the function with 5 as the first parameter
print multiply(5, 10) # Output: 42

# Remove the pre-hook
multiply.remove_pre_hook(my_pre_hook)
```

# Post-hooking a C function #
Post-hooking is also a great thing. binutils changes the return address of the original function, so your callback will be called before it returns to the caller. That way you are able to change the return value. You can also change the arguments, but it would only have an effect if the argument is a pointer.

## Returning a custom value depending on the orignal return value ##
```
def my_post_hook(args, return_value):
    # Print the passed arguments and the return value
    print tuple(args), return_value

    # Return 42 if the result is greater than 42
    if return_value > 42:
        return 42

    # Else we let the function return as usual. So, we don't need to do anything

# Add the post-hook
multiply.add_post_hook(my_post_hook)

# Call the function with arguments which result in a value less than 42
print multiply(5, 5) # Output: 25

# Call the function with arguments which result in a value greater than 42
print multiply(10, 6) # Output: 42

# Remove the post-hook
multiply.remove_post_hook(my_post_hook)
```