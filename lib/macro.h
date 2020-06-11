#pragma once

#ifndef DIV_ROUND_UP
#    define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))
#endif

#ifndef STRUCT_OFFSET
#    define STRUCT_OFFSET(Type, member) (size_t)(&((Type*)NULLPTR)->member)
#endif

#ifndef GET_STRUCT_BY_MBMBER
#    define GET_STRUCT_BY_MBMBER(Type, member, pMember) (Type*)((size_t)pMember - STRUCT_OFFSET(Type, member))
#endif

#ifndef UNUSED
#    define UNUSED(x) (void)(x);
#endif