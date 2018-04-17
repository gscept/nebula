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
template<class ... ARGTYPES> class Delegate
{
public:
    /// constructor
    Delegate();
	/// invokation operator
    void operator()(ARGTYPES ... args) const;
    /// setup a new delegate from a method call
    template<class CLASS, void (CLASS::*METHOD)(ARGTYPES ...)> static Delegate<ARGTYPES ...> FromMethod(CLASS* objPtr);
    /// setup a new delegate from a function call
    template<void(*FUNCTION)(ARGTYPES ...)> static Delegate<ARGTYPES ...> FromFunction();
	/// get object pointer
	template<class CLASS> const CLASS* GetObject() const;

	/// returns true if delegate is valid
	bool IsValid();

private:
    /// method pointer typedef
    typedef void (*StubType)(void*, ARGTYPES ...);
    /// static method-call stub 
    template<class CLASS, void(CLASS::*METHOD)(ARGTYPES ...)> static void MethodStub(void* objPtr, ARGTYPES ... args);
    /// static function-call stub
    template<void(*FUNCTION)(ARGTYPES ... )> static void FunctionStub(void* dummyPtr, ARGTYPES ... args);

    void* objPtr;
    StubType stubPtr;
};

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
Delegate<ARGTYPES ...>::Delegate() :
    objPtr(0),
    stubPtr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES> void
Delegate<ARGTYPES ...>::operator()(ARGTYPES ... args) const
{
    (*this->stubPtr)(this->objPtr, args...);
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
template<class CLASS, void (CLASS::*METHOD)(ARGTYPES ...)>
Delegate<ARGTYPES ...>
Delegate<ARGTYPES ...>::FromMethod(CLASS* objPtr_)
{
    Delegate<ARGTYPES ...> del;
    del.objPtr = objPtr_;
    del.stubPtr = &MethodStub<CLASS,METHOD>;
    return del;
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
template<void(*FUNCTION)(ARGTYPES ...)>
Delegate<ARGTYPES ...>
Delegate<ARGTYPES ...>::FromFunction()
{
    Delegate<ARGTYPES ...> del;
    del.objPtr = 0;
    del.stubPtr = &FunctionStub<FUNCTION>;
    return del;
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
template<class CLASS, void (CLASS::*METHOD)(ARGTYPES ...)>
void
Delegate<ARGTYPES ...>::MethodStub(void* objPtr_, ARGTYPES ... arg_)
{
    CLASS* obj = static_cast<CLASS*>(objPtr_);
    (obj->*METHOD)(arg_...);
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
template<void(*FUNCTION)(ARGTYPES ...)>
void
Delegate<ARGTYPES ...>::FunctionStub(void* dummyPtr, ARGTYPES ... arg_)
{
    (*FUNCTION)(arg_...);
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
template<class CLASS>
inline const CLASS* 
Delegate<ARGTYPES ...>::GetObject() const
{
	return (CLASS*)this->objPtr;
}

//------------------------------------------------------------------------------
/**
*/
template<class ... ARGTYPES>
bool 
Util::Delegate<ARGTYPES ...>::IsValid()
{
	return (0 != this->stubPtr);
}

} // namespace Util
//------------------------------------------------------------------------------
