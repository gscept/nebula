#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Delegate
    
    Nebula3 delegate class, allows to store a method call into a C++ object
    for later execution.

    See http://www.codeproject.com/KB/cpp/ImpossiblyFastCppDelegate.aspx
    for details.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class ARGTYPE> class Delegate
{
public:
    /// constructor
    Delegate();
    /// invokation operator
    void operator()(ARGTYPE arg) const;
    /// setup a new delegate from a method call
    template<class CLASS, void (CLASS::*METHOD)(ARGTYPE)> static Delegate<ARGTYPE> FromMethod(CLASS* objPtr);
    /// setup a new delegate from a function call
    template<void(*FUNCTION)(ARGTYPE)> static Delegate<ARGTYPE> FromFunction();
	/// get object pointer
	template<class CLASS> const CLASS* GetObject() const;

	/// returns true if delegate is valid
	bool IsValid();

private:
    /// method pointer typedef
    typedef void (*StubType)(void*, ARGTYPE);
    /// static method-call stub 
    template<class CLASS, void(CLASS::*METHOD)(ARGTYPE)> static void MethodStub(void* objPtr, ARGTYPE arg);
    /// static function-call stub
    template<void(*FUNCTION)(ARGTYPE)> static void FunctionStub(void* dummyPtr, ARGTYPE arg);

    void* objPtr;
    StubType stubPtr;
};

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
Delegate<ARGTYPE>::Delegate() :
    objPtr(0),
    stubPtr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE> void
Delegate<ARGTYPE>::operator()(ARGTYPE arg) const
{
    (*this->stubPtr)(this->objPtr, arg);
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
template<class CLASS, void (CLASS::*METHOD)(ARGTYPE)>
Delegate<ARGTYPE>
Delegate<ARGTYPE>::FromMethod(CLASS* objPtr_)
{
    Delegate<ARGTYPE> del;
    del.objPtr = objPtr_;
    del.stubPtr = &MethodStub<CLASS,METHOD>;
    return del;
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
template<void(*FUNCTION)(ARGTYPE)>
Delegate<ARGTYPE>
Delegate<ARGTYPE>::FromFunction()
{
    Delegate<ARGTYPE> del;
    del.objPtr = 0;
    del.stubPtr = &FunctionStub<FUNCTION>;
    return del;
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
template<class CLASS, void (CLASS::*METHOD)(ARGTYPE)>
void
Delegate<ARGTYPE>::MethodStub(void* objPtr_, ARGTYPE arg_)
{
    CLASS* obj = static_cast<CLASS*>(objPtr_);
    (obj->*METHOD)(arg_);
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
template<void(*FUNCTION)(ARGTYPE)>
void
Delegate<ARGTYPE>::FunctionStub(void* dummyPtr, ARGTYPE arg_)
{
    (*FUNCTION)(arg_);
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
template<class CLASS>
inline const CLASS* 
Delegate<ARGTYPE>::GetObject() const
{
	return (CLASS*)this->objPtr;
}

//------------------------------------------------------------------------------
/**
*/
template<class ARGTYPE>
bool 
Util::Delegate<ARGTYPE>::IsValid()
{
	return (0 != this->stubPtr);
}

} // namespace Util
//------------------------------------------------------------------------------
