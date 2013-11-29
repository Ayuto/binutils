/**
* =============================================================================
* binutils
* Copyright(C) 2013 Ayuto. All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef _BINUTILS_MACROS_H
#define _BINUTILS_MACROS_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "boost/python.hpp"
using namespace boost::python;


// ============================================================================
// This macro allows us to call a Python function OR Python method. It decides
// what we need to use.
// boost::python::object retval = CALL_PY_FUNC(PyObject*, ...);
// ============================================================================
#define CALL_PY_FUNC(pCallable, ...) \
    PyObject_HasAttrString(pCallable, "__self__") ? call_method<object>(PyObject_GetAttrString(pCallable, "__self__"), extract<const char*>(PyObject_GetAttrString(pCallable, "__name__")), ##__VA_ARGS__) : call<object>(pCallable, ##__VA_ARGS__)

// ============================================================================
// Surround boost python statements with this macro in order to handle
// exceptions.
// ============================================================================
#define BEGIN_BOOST_PY() \
    try {

#define END_BOOST_PY( ... ) \
    } catch( ... ) { \
        PyErr_Print(); \
        PyErr_Clear(); \
        return __VA_ARGS__; \
    }

#define END_BOOST_PY_NORET( ... ) \
    } catch( ... ) { \
        PyErr_Print(); \
        PyErr_Clear(); \
    }

// ============================================================================
// Use this macro to expose a variadic function.
// ============================================================================
#define BOOST_VARIADIC_FUNCTION(name, function, ...) \
    def("__" name, &function, ##__VA_ARGS__); \
    exec("def " name "(*args): __" name "(args)", scope().attr("__dict__"))

// ============================================================================
// Use this macro to expose a variadic class method. Don't forget to call
// DEFINE_CLASS_METHOD_VARIADIC after that.
// ============================================================================
#define CLASS_METHOD_VARIADIC(name, method, ...) \
    .def("__" name, method, ##__VA_ARGS__)

// ============================================================================
// Use this macro to define a variadic class method.
// ============================================================================
#define DEFINE_CLASS_METHOD_VARIADIC(classname, method) \
    scope().attr(#classname).attr(#method) = eval("lambda self, *args: self.__" #method "(args)")

// ============================================================================
// Use this macro to raise a Python exception.
// ============================================================================
#define BOOST_RAISE_EXCEPTION( exceptionName, exceptionString ) \
    { \
        PyErr_SetString(exceptionName, exceptionString); \
        throw_error_already_set(); \
    }

// ============================================================================
// These typedefs save some typing. Use this policy for any functions that return
// a newly allocated instance of a class which you need to delete yourself.
// ============================================================================
typedef return_value_policy<manage_new_object> manage_new_object_policy;

// ============================================================================
// Use this policy for objects that someone else will free.
// ============================================================================
typedef return_value_policy<reference_existing_object> reference_existing_object_policy;

// ============================================================================
// Use this policy for functions that return const objects.
// ============================================================================
typedef return_value_policy<copy_const_reference> copy_const_reference_policy;

#endif // _BINUTILS_MACROS_H
