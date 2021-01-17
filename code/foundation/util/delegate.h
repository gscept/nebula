#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Delegate
    
    Nebula delegate class, allows to store a function, method or lambda call into a C++ object
    for later execution.

    Note that this does not store any objects or capture, meaning these are extremely volatile to dangling pointers.
    This is by design, since it allows us to have much better performance than for example std::function that does
    expensive runtime heap allocations for storing objects and lambda capture.

    Usage:
    // 'Foo' can be a function or static method
    Util::Delegate<int(int, float)> funcDelegate = Delegate<int(int, float)>::FromFunction<Foo>();
    // Requires an object
    Util::Delegate<int()> methodDelegate = Delegate<int()>::FromMethod<MyClass, MyMethod>(MyObject);
    // Can not utilize closures!
    Util::Delegate<void(int)> lambdaDelegate = [](int a, int b) { return a + b; };

    See http://www.codeproject.com/KB/cpp/ImpossiblyFastCppDelegate.aspx
    for details.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
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
    /// copy constructor
    Delegate(Delegate<RETTYPE(ARGTYPES...)> const& rhs);
    /// move constructor
    Delegate(Delegate<RETTYPE(ARGTYPES...)>&& rhs);
    /// lambda constructor
    template <typename LAMBDA>
    Delegate(LAMBDA const& lambda);
    
    /// assignment operator
    void operator=(Delegate<RETTYPE(ARGTYPES...)> const& rhs);
    /// move operator
    void operator=(Delegate<RETTYPE(ARGTYPES...)>&& rhs);
    /// check if null
    bool operator==(std::nullptr_t);
    /// check if null
    bool operator!();

    /// lambda assignment operator
    template <typename LAMBDA>
    Delegate<RETTYPE(ARGTYPES ...)>& operator=(LAMBDA const& instance);
    
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
    objPtr(nullptr),
    stubPtr(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
Delegate<RETTYPE(ARGTYPES ...)>::Delegate(Delegate<RETTYPE(ARGTYPES...)> const& rhs) :
    objPtr(rhs.objPtr),
    stubPtr(rhs.stubPtr)
{

}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
Delegate<RETTYPE(ARGTYPES ...)>::Delegate(Delegate<RETTYPE(ARGTYPES...)>&& rhs) :
    objPtr(rhs.objPtr),
    stubPtr(rhs.stubPtr)
{
    rhs.objPtr = nullptr;
    rhs.stubPtr = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template <typename LAMBDA>
Delegate<RETTYPE(ARGTYPES...)>::Delegate(LAMBDA const& lambda)
{
    static_assert(sizeof(LAMBDA) == 1ULL, "Util::Delegate does accept lambdas carrying capture variables! Read the description of at the top of util/delegate.h");
    Assign((void*)(&lambda), LambdaStub<LAMBDA>);
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
void
Delegate<RETTYPE(ARGTYPES...)>::operator=(Delegate<RETTYPE(ARGTYPES...)> const& rhs)
{
    if (this != &rhs)
    {
        this->objPtr = rhs.objPtr;
        this->stubPtr = rhs.stubPtr;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
void
Delegate<RETTYPE(ARGTYPES...)>::operator=(Delegate<RETTYPE(ARGTYPES...)>&& rhs)
{
    if (this != &rhs)
    {
        this->objPtr = rhs.objPtr;
        this->stubPtr = rhs.stubPtr;
        rhs.objPtr = nullptr;
        rhs.stubPtr = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
template <typename LAMBDA>
Delegate<RETTYPE(ARGTYPES...)>&
Delegate<RETTYPE(ARGTYPES...)>::operator=(LAMBDA const& instance)
{
    static_assert(sizeof(LAMBDA) == 1ULL, "Util::Delegate does accept lambdas carrying capture variables! Read the description of at the top of util/delegate.h");
    Assign((void*)(&instance), LambdaStub<LAMBDA>);
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
bool
Delegate<RETTYPE(ARGTYPES...)>::operator==(std::nullptr_t)
{   
    return this->stubPtr == nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<typename RETTYPE, typename ... ARGTYPES>
bool
Delegate<RETTYPE(ARGTYPES...)>::operator!()
{
    return this->stubPtr == nullptr;
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
    static_assert(sizeof(LAMBDA) == 1ULL, "Util::Delegate does accept lambdas carrying capture variables! Read the description of at the top of util/delegate.h");
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
