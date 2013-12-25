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

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "binutils_macros.h"
#include "binutils_scanner.h"
#include "binutils_tools.h"
#include "binutils_hooks.h"

#include "dyncall.h"

void ExposeScanner();
void ExposeTools();
void ExposeArrays();
void ExposeDynCall();
void ExposeDynamicHooks();

// ============================================================================
// >> Expose the binutils module
// ============================================================================
BOOST_PYTHON_MODULE(_binutils)
{
    ExposeScanner();
    ExposeTools();
    ExposeArrays();
    ExposeDynCall();
    ExposeDynamicHooks();
}

// ============================================================================
// >> Expose CBinaryFile
// ============================================================================
// Overloads
BOOST_PYTHON_FUNCTION_OVERLOADS(find_binary_overload, FindBinary, 1, 2);

void ExposeScanner()
{
    class_<CBinaryFile, boost::noncopyable>("BinaryFile", no_init)

        // Class methods
        .def("find_signature",
            &CBinaryFile::FindSignature,
            "Returns the address of a signature found in memory.",
            args("signature"),
            manage_new_object_policy()
        )

        .def("find_symbol",
            &CBinaryFile::FindSymbol,
            "Returns the address of a symbol found in memory.",
            args("symbol"),
            manage_new_object_policy()
        )

        .def("find_pointer",
            &CBinaryFile::FindPointer,
            "Rips out a pointer from a function.",
            args("signature", "offset"),
            manage_new_object_policy()
        )

        // Special methods
        .def("__getitem__",
            &CBinaryFile::FindSymbol,
            "Returns the address of a symbol found in memory.",
            args("symbol"),
            manage_new_object_policy()
        )

        // Properties
        .add_property("address",
            &CBinaryFile::GetAddress,
            "Returns the base address of this binary."
        )

        .add_property("size",
            &CBinaryFile::GetSize,
            "Returns the size of this binary."
        )
    ;

    def("find_binary",
        &FindBinary,
        find_binary_overload(
            args("path", "srv_check"),
            "Returns a CBinaryFile object or None.")[reference_existing_object_policy()]
    );
}


// ============================================================================
// >> Expose CPointer
// ============================================================================
// Overloads
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_function_overload, CPointer::MakeFunction, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_virtual_function_overload, CPointer::MakeVirtualFunction, 3, 4)

// get_<type> methods
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_bool_overload, CPointer::Get<bool>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_char_overload, CPointer::Get<char>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_uchar_overload, CPointer::Get<unsigned char>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_short_overload, CPointer::Get<short>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_ushort_overload, CPointer::Get<unsigned short>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_int_overload, CPointer::Get<int>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_uint_overload, CPointer::Get<unsigned int>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_long_overload, CPointer::Get<long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_ulong_overload, CPointer::Get<unsigned long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_long_long_overload, CPointer::Get<long long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_ulong_long_overload, CPointer::Get<unsigned long long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_float_overload, CPointer::Get<float>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_double_overload, CPointer::Get<double>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_ptr_overload, CPointer::GetPtr, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_string_overload, CPointer::Get<const char *>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_string_array_overload, CPointer::GetStringArray, 0, 1)

// set_<type> methods
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_bool_overload, CPointer::Set<bool>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_char_overload, CPointer::Set<char>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_uchar_overload, CPointer::Set<unsigned char>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_short_overload, CPointer::Set<short>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_ushort_overload, CPointer::Set<unsigned short>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_int_overload, CPointer::Set<int>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_uint_overload, CPointer::Set<unsigned int>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_long_overload, CPointer::Set<long>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_ulong_overload, CPointer::Set<unsigned long>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_long_long_overload, CPointer::Set<long long>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_ulong_long_overload, CPointer::Set<unsigned long long>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_float_overload, CPointer::Set<float>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_double_overload, CPointer::Set<double>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_ptr_overload, CPointer::SetPtr, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_string_overload, CPointer::Set<const char *>, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(set_string_array_overload, CPointer::SetStringArray, 1, 3)

// make_<type>_array methods
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_bool_array_overload, CPointer::MakeArray<bool>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_char_array_overload, CPointer::MakeArray<char>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_uchar_array_overload, CPointer::MakeArray<unsigned char>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_short_array_overload, CPointer::MakeArray<short>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_ushort_array_overload, CPointer::MakeArray<unsigned short>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_int_array_overload, CPointer::MakeArray<int>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_uint_array_overload, CPointer::MakeArray<unsigned int>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_long_array_overload, CPointer::MakeArray<long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_ulong_array_overload, CPointer::MakeArray<unsigned long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_long_long_array_overload, CPointer::MakeArray<long long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_ulong_long_array_overload, CPointer::MakeArray<unsigned long long>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_float_array_overload, CPointer::MakeArray<float>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_double_array_overload, CPointer::MakeArray<double>, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_string_array_overload, CPointer::MakeArray<const char *>, 0, 1)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(make_ptr_array_overload, CPointer::MakePtrArray, 2, 3)

void ExposeTools()
{
    // CPointer class
    class_<CPointer>("Pointer", init< optional<unsigned long> >())
        .def(init<const CPointer&>())

        // Class methods
        .def("get_virtual_func",
            &CPointer::GetVirtualFunc,
            "Returns the address (as a CPointer instance) of a virtual function at the given index.",
            args("index"),
            manage_new_object_policy()
        )

        .def("realloc",
            &CPointer::Realloc,
            "Reallocates a memory block.",
            args("size")
        )

        .def("dealloc",
            &CPointer::Dealloc,
            "Deallocates a memory block."
        )

        .def("make_function",
            &CPointer::MakeFunction,
            make_function_overload(
                args("convention", "params", "return_type"),
                "Creates a new Function object.")[manage_new_object_policy()]
        )

        .def("make_virtual_function",
            &CPointer::MakeVirtualFunction,
            make_virtual_function_overload(
                args("index", "convention", "params", "return_type"),
                "Creates a new Function object.")[manage_new_object_policy()]
        )

        .def("compare",
            &CPointer::Compare,
            "Compares the first <num> bytes of both pointers. Returns 0 if they are equal. A value greater than zero indicates that the first "\
            "byte that does not match in both pointers has a greater value in <self> than in <other>. A value less than zero indicates the opposite.",
            args("other", "num")
        )

        .def("is_overlapping",
            &CPointer::IsOverlapping,
            "Returns True if the pointers are overlapping each other.",
            args("destination", "num_bytes")
        )

        .def("search_bytes",
            &CPointer::SearchBytes,
            "Searches within the first <num_bytes> of this memory block for the first occurence of <bytes> and returns a pointer it.",
            args("bytes", "num_bytes"),
            manage_new_object_policy()
        )

        .def("copy",
            &CPointer::Copy,
            "Copies <num_bytes> from <self> to the pointer <destination>. Overlapping is not allowed!",
            args("destination", "num_bytes")
        )

        .def("move",
            &CPointer::Move,
            "Copies <num_bytes> from <self> to the pointer <destination>. Overlapping is allowed!",
            args("destination", "num_bytes")
        )

        // Make array methods
        .def("make_bool_array",
            &CPointer::MakeArray<bool>,
            make_bool_array_overload(
                args("length"),
                "Creates a new BoolArray object."
            )
        )

        .def("make_char_array",
            &CPointer::MakeArray<char>,
            make_char_array_overload(
                args("length"),
                "Creates a new CharArray object."
            )
        )

        .def("make_uchar_array",
            &CPointer::MakeArray<unsigned char>,
            make_uchar_array_overload(
                args("length"),
                "Creates a new UCharArray object."
            )
        )

        .def("make_short_array",
            &CPointer::MakeArray<short>,
            make_short_array_overload(
                args("length"),
                "Creates a new ShortArray object."
            )
        )

        .def("make_ushort_array",
            &CPointer::MakeArray<unsigned short>,
            make_ushort_array_overload(
                args("length"),
                "Creates a new UShortArray object."
            )
        )

        .def("make_int_array",
            &CPointer::MakeArray<int>,
            make_int_array_overload(
                args("length"),
                "Creates a new IntArray object."
            )
        )

        .def("make_uint_array",
            &CPointer::MakeArray<unsigned int>,
            make_uint_array_overload(
                args("length"),
                "Creates a new UIntArray object."
            )
        )

        .def("make_long_array",
            &CPointer::MakeArray<long>,
            make_long_array_overload(
                args("length"),
                "Creates a new LongArray object."
            )
        )

        .def("make_ulong_array",
            &CPointer::MakeArray<unsigned long>,
            make_ulong_array_overload(
                args("length"),
                "Creates a new ULongArray object."
            )
        )

        .def("make_long_long_array",
            &CPointer::MakeArray<long long>,
            make_long_long_array_overload(
                args("length"),
                "Creates a new LongLongArray object."
            )
        )

        .def("make_ulong_long_array",
            &CPointer::MakeArray<unsigned long long>,
            make_ulong_long_array_overload(
                args("length"),
                "Creates a new ULongLongArray object."
            )
        )

        .def("make_float_array",
            &CPointer::MakeArray<float>,
            make_float_array_overload(
                args("length"),
                "Creates a new FloatArray object."
            )
        )

        .def("make_double_array",
            &CPointer::MakeArray<double>,
            make_double_array_overload(
                args("length"),
                "Creates a new DoubleArray object."
            )
        )

        .def("make_string_array",
            &CPointer::MakeArray<const char *>,
            make_string_array_overload(
                args("length"),
                "Creates a new StringArray object."
            )
        )

        .def("make_ptr_array",
            &CPointer::MakePtrArray,
            make_ptr_array_overload(
                args("size", "length", "converter"),
                "Creates a new PtrArray object."
            )
        )

        // get_<type> methods
        .def("get_bool",
            &CPointer::Get<bool>,
            get_bool_overload(
                args("offset"),
                "Returns the value at the given memory location as a boolean.")
        )

        .def("get_char",
            &CPointer::Get<char>,
            get_char_overload(
                args("offset"),
                "Returns the value at the given memory location as a char.")
        )

        .def("get_uchar",
            &CPointer::Get<unsigned char>,
            get_uchar_overload(
                args("offset"),
                "Returns the value at the given memory location as an unsgined char.")
        )

        .def("get_short",
            &CPointer::Get<short>,
            get_short_overload(
                args("offset"),
                "Returns the value at the given memory location as a short.")
        )

        .def("get_ushort",
            &CPointer::Get<unsigned short>,
            get_ushort_overload(
                args("offset"),
                "Returns the value at the given memory location as a unsigned short.")
        )

        .def("get_int",
            &CPointer::Get<int>,
            get_int_overload(
                args("offset"),
                "Returns the value at the given memory location as an integer.")
        )

        .def("get_uint",
            &CPointer::Get<unsigned int>,
            get_uint_overload(
                args("offset"),
                "Returns the value at the given memory location as an unsigned integer.")
        )

        .def("get_long",
            &CPointer::Get<long>,
            get_long_overload(
                args("offset"),
                "Returns the value at the given memory location as a long.")
        )

        .def("get_ulong",
            &CPointer::Get<unsigned long>,
            get_ulong_overload(
                args("offset"),
                "Returns the value at the given memory location as an unsigned long.")
        )

        .def("get_long_long",
            &CPointer::Get<long long>,
            get_long_long_overload(
                args("offset"),
                "Returns the value at the given memory location as a long long.")
        )

        .def("get_ulong_long",
            &CPointer::Get<unsigned long long>,
            get_ulong_long_overload(
                args("offset"),
                "Returns the value at the given memory location as an unsigned long long.")
        )

        .def("get_float",
            &CPointer::Get<float>,
            get_float_overload(
                args("offset"),
                "Returns the value at the given memory location as a float.")
        )

        .def("get_double",
            &CPointer::Get<double>,
            get_double_overload(
                args("offset"),
                "Returns the value at the given memory location as a double.")
        )

        .def("get_ptr",
            &CPointer::GetPtr,
            get_ptr_overload(
                args("offset"),
                "Returns the value at the given memory location as a CPointer instance.")[manage_new_object_policy()]
        )

        .def("get_string",
            &CPointer::Get<const char *>,
            get_string_overload(
                args("offset"),
                "Returns the value at the given memory location as a string.")
        )

        .def("get_string_array",
            &CPointer::GetStringArray,
            get_string_array_overload(
                args("offset"),
                "Returns the value at the given memory location as a string.")
        )

        // set_<type> methods
        .def("set_bool",
            &CPointer::Set<bool>,
            set_bool_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a boolean.")
        )

        .def("set_char",
            &CPointer::Set<char>,
            set_char_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a char.")
        )

        .def("set_uchar",
            &CPointer::Set<unsigned char>,
            set_uchar_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as an unsigned char.")
        )

        .def("set_short",
            &CPointer::Set<short>,
            set_short_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a short.")
        )

        .def("set_ushort",
            &CPointer::Set<unsigned short>,
            set_ushort_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as an unsigned short.")
        )

        .def("set_int",
            &CPointer::Set<int>,
            set_int_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as an integer.")
        )

        .def("set_uint",
            &CPointer::Set<unsigned int>,
            set_uint_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as an unsigned integer.")
        )

        .def("set_long",
            &CPointer::Set<long>,
            set_long_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a long.")
        )

        .def("set_ulong",
            &CPointer::Set<unsigned long>,
            set_ulong_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as an unsigned long.")
        )

        .def("set_long_long",
            &CPointer::Set<long long>,
            set_long_long_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a long long.")
        )

        .def("set_ulong_long",
            &CPointer::Set<unsigned long long>,
            set_ulong_long_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as an unsigned long long.")
        )

        .def("set_float",
            &CPointer::Set<float>,
            set_float_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a float.")
        )

        .def("set_double",
            &CPointer::Set<double>,
            set_double_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a double.")
        )

        .def("set_ptr",
            &CPointer::SetPtr,
            set_ptr_overload(
                args("value", "offset"),
                "Sets the value at the given memory location as a pointer.")
        )

        .def("set_string",
            &CPointer::Set<const char *>,
            set_string_overload(
            args("text", "offset"),
                "Sets the value at the given memory location as a string.")
        )

        .def("set_string_array",
            &CPointer::SetStringArray,
            set_string_array_overload(
            args("text", "offset", "size"),
                "Sets the value at the given memory location as a string.")
        )

        // Special methods
        .def("__int__",
            &CPointer::GetAddress,
            "Returns the address of this memory block."
        )

        .def(!self)

        .def(self == other<unsigned long>())
        .def(self == self)

        .def(self != other<unsigned long>())
        .def(self != self)

        .def(self += int())
        .def(self += self)

        .def(self + int())
        .def(self + self)
        .def(int() + self)

        .def(self -= int())
        .def(self -= self)

        .def(self - int())
        .def(self - self)
        .def(int() - self)

        // Attributes
        .def_readwrite("address",
            &CPointer::m_ulAddr,
           "Returns the address of this memory block."
        )

        // Properties
        .add_property("size",
            &CPointer::GetSize,
            "Returns the size of this memory block."
        )
    ;


    // CFunction class
    class_<CFunction, bases<CPointer> >("Function", init<unsigned long, Convention_t, char*, optional<PyObject*> >())
        .def(init<const CFunction&>())

        // Class methods
        CLASS_METHOD_VARIADIC("__call__",
            &CFunction::__call__,
            "Calls the function dynamically."
        )

        CLASS_METHOD_VARIADIC("call_trampoline",
            &CFunction::CallTrampoline,
            "Calls the trampoline function dynamically."
        )

        .def("add_pre_hook",
            &CFunction::AddPreHook,
            "Adds a pre-hook callback."
        )

        .def("add_post_hook",
            &CFunction::AddPostHook,
            "Adds a post-hook callback."
        )

        .def("remove_pre_hook",
            &CFunction::RemovePreHook,
            "Removes a pre-hook callback."
        )

        .def("remove_post_hook",
            &CFunction::RemovePostHook,
            "Removes a post-hook callback."
        )

        // Attributes
        .add_property("parameters",
            &CFunction::GetParams,
            &CFunction::SetParams,
            "Returns the parameter string."
        )

        .def_readwrite("convention",
            &CFunction::m_eConv,
            "Returns the calling convention."
        )

        .def_readwrite("converter",
            &CFunction::m_oConverter,
            "Returns the converter."
        )
    ;

    DEFINE_CLASS_METHOD_VARIADIC(Function, __call__);
    DEFINE_CLASS_METHOD_VARIADIC(Function, call_trampoline);

    def("alloc",
        Alloc,
        args("size"),
        "Allocates a memory block.",
        manage_new_object_policy()
    );
}


// ============================================================================
// >> Expose Arrays
// ============================================================================
#define EXPOSE_ARRAY(type, classname) \
    class_< CArray<type>, bases<CPointer> >(classname, init<unsigned long, optional<int> >()) \
        .def("__getitem__", &CArray<type>::GetItem) \
        .def("__setitem__", &CArray<type>::SetItem) \
    ;

void ExposeArrays()
{
    EXPOSE_ARRAY(bool, "BoolArray");
    EXPOSE_ARRAY(char, "CharArray");
    EXPOSE_ARRAY(unsigned char, "UCharArray");
    EXPOSE_ARRAY(short, "ShortArray");
    EXPOSE_ARRAY(unsigned short, "UShortArray");
    EXPOSE_ARRAY(int, "IntArray");
    EXPOSE_ARRAY(unsigned int, "UIntArray");
    EXPOSE_ARRAY(long, "LongArray");
    EXPOSE_ARRAY(unsigned long, "ULongArray");
    EXPOSE_ARRAY(long long, "LongLongArray");
    EXPOSE_ARRAY(unsigned long long, "ULongLongArray");
    EXPOSE_ARRAY(float, "FloatArray");
    EXPOSE_ARRAY(double, "DoubleArray");
    EXPOSE_ARRAY(const char *, "StringArray");

    class_< CPtrArray, bases<CArray<unsigned long> > >("PtrArray", init<unsigned long, unsigned int, optional<int, PyObject *> >())
        .def("__getitem__", &CPtrArray::GetItem)
        .def("__setitem__", &CPtrArray::SetItem)
    ;
}

// ============================================================================
// >> Expose DynCall
// ============================================================================
void ExposeDynCall()
{
    enum_<Convention_t>("Convention")
        .value("CDECL", CONV_CDECL)
        .value("STDCALL", CONV_STDCALL)
        .value("THISCALL", CONV_THISCALL)
    ;

    // Other constants that are very useful.
    scope().attr("DC_ERROR_NONE") = DC_ERROR_NONE;
    scope().attr("DC_ERROR_UNSUPPORTED_MODE") = DC_ERROR_UNSUPPORTED_MODE;

    def("get_error",
        &GetError,
        "Returns the last DynCall error ID."
    );
}

// ============================================================================
// >> Expose DynamicHooks
// ============================================================================
void ExposeDynamicHooks()
{
    class_<CStackData>("StackData", init<CHook*>())

        // Special methods
        .def("__getitem__",
            &CStackData::GetItem,
            "Returns the argument at the specified index."
        )

        .def("__setitem__",
            &CStackData::SetItem,
            "Sets the argument at the specified index."
        )
    ;
}