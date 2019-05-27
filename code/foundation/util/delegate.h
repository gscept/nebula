#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Delegate
    
    Nebula delegate class, allows to store a method call into a C++ object
    for later execution.

	Usage:
	// 'Foo' can be a function or static method
	Util::Delegate<int(int, float)> funcDelegate = Delegate<int(int, float)>::FromFunction<Foo>();
	// Requires an object
    Util::Delegate<int()> methodDelegate = Delegate<int()>::FromMethod<MyClass, MyMethod>(MyObject);
	// Can utilize closures!
    Util::Delegate<void(int)> lambdaDelegate = [a](int b) { return a + b; };

    See http://www.codeproject.com/KB/cpp/ImpossiblyFastCppDelegate.aspx
    for details.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2019 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{

template<typename T>
class Delegate; // This is soooo dirty.

template<typename RETTYPE, typename ... ARGTYPES>
class Delegate<RETTYPE(ARGTYPES...)>
{
public:
    /// constructor
    Delegate();
	
	/// lambda constructor
	template <typename LAMBDA>
	Delegate(const LAMBDA& lambda);
	
	/// lambda assignment operator
	template <typename LAMBDA>
	Delegate<RETTYPE(ARGTYPES ...)>& operator=(const LAMBDA& instance);
	
	/// invokation operator
    RETTYPE operator()(ARGTYPES ... args) const;

    /// setup a new delegate from a method call
    template<class CLASS, RETTYPE (CLASS::*METHOD)(ARGTYPES ...)> static Delegate<RETTYPE(ARGTYPES ...)> FromMethod(CLASS* objPtr);
    /// setup a new delegate from a function call
    template<RETTYPE(*FUNCTION)(ARGTYPES ...)> static Delegate<RETTYPE(ARGTYPES ...)> FromFunction();
	/// setup a new delegate from lambda
	template <typename LAMBDA> static Delegate<RETTYPE(ARGTYPES ...)> FromLambda(const LAMBDA & instance);

	/// get object pointer
	template<class CLASS> const CLASS* GetObject() const;

	/// returns true if delegate is valid
	bool IsValid();

private:
    /// method pointer typedef
	using StubType = RETTYPE(*)(void*, ARGTYPES...);
    //typedef void (*StubType)(void*, ARGTYPES ...);

    /// static method-call stub 
    template<class CLASS, RETTYPE(CLASS::*METHOD)(ARGTYPES ...)> static RETTYPE MethodStub(void* objPtr, ARGTYPES ... args);
    /// static function-call stub
    template<RETTYPE(*FUNCTION)(ARGTYPES ... )> static RETTYPE FunctionStub(void* dummyPtr, ARGTYPES ... args);
	/// static lambda-call stub
	template <typename LAMBDA> static RETTYPE LambdaStub(void* objPtr, ARGTYPES... arg);
	
	/// assignment method
	void Assign(void* obj, typename Delegate<RETTYPE(ARGTYPES...)>::StubType stub);
	
    void* objPtr;
    StubType stubPtr;
};

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
Delegate<RETTYPE(ARGTYPES ...)>::Delegate() :
    objPtr(0),
    stubPtr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template <typename LAMBDA>
Delegate<RETTYPE(ARGTYPES...)>::Delegate(const LAMBDA& lambda)
{
	Assign((void*)(&lambda), LambdaStub<LAMBDA>);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template <typename LAMBDA>
Delegate<RETTYPE(ARGTYPES...)>&
Delegate<RETTYPE(ARGTYPES...)>::operator=(const LAMBDA& instance)
{
	Assign((void*)(&instance), LambdaStub<LAMBDA>);
	return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES> RETTYPE
Delegate<RETTYPE(ARGTYPES...)>::operator()(ARGTYPES ... args) const
{
    return (*this->stubPtr)(this->objPtr, args...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<class CLASS, RETTYPE (CLASS::*METHOD)(ARGTYPES ...)>
Delegate<RETTYPE(ARGTYPES...)>
Delegate<RETTYPE(ARGTYPES...)>::FromMethod(CLASS* objPtr_)
{
    Delegate<RETTYPE(ARGTYPES...)> del;
    del.objPtr = objPtr_;
    del.stubPtr = &MethodStub<CLASS,METHOD>;
    return del;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<RETTYPE(*FUNCTION)(ARGTYPES ...)>
Delegate<RETTYPE(ARGTYPES...)>
Delegate<RETTYPE(ARGTYPES...)>::FromFunction()
{
    Delegate<RETTYPE(ARGTYPES...)> del;
    del.objPtr = 0;
    del.stubPtr = &FunctionStub<FUNCTION>;
    return del;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<typename LAMBDA>
Delegate<RETTYPE(ARGTYPES...)>
Delegate<RETTYPE(ARGTYPES...)>::FromLambda(const LAMBDA & instance)
{
	return ((void*)(&instance), LambdaStub<LAMBDA>);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<class CLASS, RETTYPE (CLASS::*METHOD)(ARGTYPES ...)>
RETTYPE
Delegate<RETTYPE(ARGTYPES...)>::MethodStub(void* objPtr_, ARGTYPES ... arg_)
{
    CLASS* obj = static_cast<CLASS*>(objPtr_);
    return (obj->*METHOD)(arg_...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<RETTYPE(*FUNCTION)(ARGTYPES ...)>
RETTYPE
Delegate<RETTYPE(ARGTYPES...)>::FunctionStub(void* dummyPtr, ARGTYPES ... arg_)
{
    return (*FUNCTION)(arg_...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<typename LAMBDA>
RETTYPE
Delegate<RETTYPE(ARGTYPES...)>::LambdaStub(void* objPtr, ARGTYPES ... arg_)
{
	LAMBDA* p = static_cast<LAMBDA*>(objPtr);
	return (p->operator())(arg_...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template<class CLASS>
inline const CLASS* 
Delegate<RETTYPE(ARGTYPES...)>::GetObject() const
{
	return (CLASS*)this->objPtr;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
bool 
Util::Delegate<RETTYPE(ARGTYPES...)>::IsValid()
{
	return (0 != this->stubPtr);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
void
Delegate<RETTYPE(ARGTYPES...)>::Assign(void* obj, typename Delegate<RETTYPE(ARGTYPES...)>::StubType stub)
{
	this->objPtr = obj;
	this->stubPtr = stub;
}

} // namespace Util
//------------------------------------------------------------------------------
