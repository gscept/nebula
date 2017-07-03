#pragma once
//------------------------------------------------------------------------------
/**
    @file core/rttimacros.h
    
    This defines the macros for Nebula3's RTTI mechanism 
    (__DeclareClass, __ImplementClass, etc...).
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file	
*/

//------------------------------------------------------------------------------
/**
    Declaration macro. Put this into the class declaration.
*/
#define __DeclareClass(type) \
public: \
    void* operator new(size_t size) \
    { \
        return RTTI.AllocInstanceMemory(); \
    }; \
    void operator delete(void* p) \
    { \
        RTTI.FreeInstanceMemory(p); \
    }; \
    static Core::Rtti RTTI; \
    static Core::RefCounted* FactoryCreator(); \
    static type* Create(); \
    static bool RegisterWithFactory(); \
    virtual Core::Rtti* GetRtti() const; \
private:

#define __DeclareTemplateClass(type, temp) \
public: \
    void* operator new(size_t size) \
    { \
        return Memory::Alloc(Memory::ObjectHeap, size); \
    }; \
    void operator delete(void* p) \
    { \
		Memory::Free(Memory::ObjectHeap, p); \
    }; \
    static type<temp>* Create(); \
private:

#define __DeclareAbstractClass(class_name) \
public: \
    static Core::Rtti RTTI; \
    virtual Core::Rtti* GetRtti() const; \
private:

//------------------------------------------------------------------------------
/**
    Register a class with the factory. This is only necessary for classes
    which can create objects by name or fourcc.
*/
#define __RegisterClass(type) \
    static const bool type##_registered = type::RegisterWithFactory(); \

//------------------------------------------------------------------------------
/**
    Implementation macro for default memory pool sizes. Put this into the source file.
*/
#if NEBULA3_DEBUG
#define __ImplementClass(type, fourcc, baseType) \
    Core::Rtti type::RTTI(#type, fourcc, type::FactoryCreator, &baseType::RTTI, sizeof(type)); \
    Core::Rtti* type::GetRtti() const { return &this->RTTI; } \
    Core::RefCounted* type::FactoryCreator() { return type::Create(); } \
    type* type::Create() \
    { \
        RefCounted::criticalSection.Enter(); \
        RefCounted::isInCreate = true; \
        type* newObject = n_new(type); \
        RefCounted::isInCreate = false; \
        RefCounted::criticalSection.Leave(); \
        return newObject; \
    }\
    bool type::RegisterWithFactory() \
    { \
        Core::SysFunc::Setup(); \
        if (!Core::Factory::Instance()->ClassExists(#type)) \
        { \
            Core::Factory::Instance()->Register(&type::RTTI, #type, fourcc); \
        } \
        return true; \
    }
#define __ImplementClassTemplate(type, baseType) \
	template <class TEMP> \
    inline type<TEMP>* type<TEMP>::Create() \
    { \
        RefCounted::criticalSection.Enter(); \
        RefCounted::isInCreate = true; \
        type<TEMP>* newObject = n_new(type<TEMP>); \
        RefCounted::isInCreate = false; \
        RefCounted::criticalSection.Leave(); \
        return newObject; \
    }
#else
#define __ImplementClass(type, fourcc, baseType) \
    Core::Rtti type::RTTI(#type, fourcc, type::FactoryCreator, &baseType::RTTI, sizeof(type)); \
    Core::Rtti* type::GetRtti() const { return &this->RTTI; } \
    Core::RefCounted* type::FactoryCreator() { return type::Create(); } \
    type* type::Create() \
    { \
        return n_new(type); \
    }\
    bool type::RegisterWithFactory() \
    { \
        Core::SysFunc::Setup(); \
        if (!Core::Factory::Instance()->ClassExists(#type)) \
        { \
            Core::Factory::Instance()->Register(&type::RTTI, #type, fourcc); \
        } \
        return true; \
    }
#endif

//------------------------------------------------------------------------------
/**
*/
#define __ImplementAbstractClass(type, fourcc, baseType) \
    Core::Rtti type::RTTI(#type, fourcc, 0, &baseType::RTTI, 0); \
    Core::Rtti* type::GetRtti() const { return &this->RTTI; }

//------------------------------------------------------------------------------
/**
    Type implementation of topmost type in inheritance hierarchy (source file).
*/
#if NEBULA3_DEBUG
#define __ImplementRootClass(type, fourcc) \
    Core::Rtti type::RTTI(#type, fourcc, type::FactoryCreator, 0, sizeof(type)); \
    Core::Rtti* type::GetRtti() const { return &this->RTTI; } \
    Core::RefCounted* type::FactoryCreator() { return type::Create(); } \
    type* type::Create() \
    { \
        RefCounted::criticalSection.Enter(); \
        RefCounted::isInCreate = true; \
        type* newObject = n_new(type); \
        RefCounted::isInCreate = false; \
        RefCounted::criticalSection.Leave(); \
        return newObject; \
    }\
    bool type::RegisterWithFactory() \
    { \
        if (!Core::Factory::Instance()->ClassExists(#type)) \
        { \
            Core::Factory::Instance()->Register(&type::RTTI, #type, fourcc); \
        } \
        return true; \
    }
#else
#define __ImplementRootClass(type, fourcc) \
    Core::Rtti type::RTTI(#type, fourcc, type::FactoryCreator, 0, sizeof(type)); \
    Core::Rtti* type::GetRtti() const { return &this->RTTI; } \
    Core::RefCounted* type::FactoryCreator() { return type::Create(); } \
    type* type::Create() \
    { \
        return n_new(type); \
    }\
    bool type::RegisterWithFactory() \
    { \
        if (!Core::Factory::Instance()->ClassExists(#type)) \
        { \
            Core::Factory::Instance()->Register(&type::RTTI, #type, fourcc); \
        } \
        return true; \
    }
#endif
    
#define __SetupExternalAttributes() \
	public: \
	virtual void SetupExternalAttributes(); \
	private: 

//------------------------------------------------------------------------------
/**
	Neat macro to make enums act as bit flags, be able to check if bits are set, and convert to integers
*/
#define __ImplementEnumBitOperators(type) \
	inline TYPE operator|(type a, type b) { return static_cast<type>(static_cast<unsigned>(a) | static_cast<unsigned>(b)); }\
	inline TYPE operator&(type a, type b) { return static_cast<type>(static_cast<unsigned>(a) & static_cast<unsigned>(b)); }\
	inline TYPE& operator|=(type& a, type b) { a = static_cast<type>(static_cast<unsigned>(a) | static_cast<unsigned>(b)); return a; }\
	inline TYPE& operator&=(type& a, type b) { a = static_cast<type>(static_cast<unsigned>(a) & static_cast<unsigned>(b)); return a; }\
	inline TYPE operator|(type a, unsigned b) { return static_cast<type>(static_cast<unsigned>(a) | b); }\
	inline TYPE operator&(type a, unsigned b) { return static_cast<type>(static_cast<unsigned>(a) & b); }\
	inline TYPE& operator|=(type& a, unsigned b) { a = static_cast<type>(static_cast<unsigned>(a) | b); return a; }\
	inline TYPE& operator&=(type& a, unsigned b) { a = static_cast<type>(static_cast<unsigned>(a) & b); return a; }\
	inline unsigned operator|(unsigned a, type b) { return a | static_cast<unsigned>(b); }\
	inline unsigned operator&(unsigned a, type b) { return a & static_cast<unsigned>(b); }\
	inline unsigned& operator|=(unsigned& a, type b) { a = a | static_cast<unsigned>(b); return a; }\
	inline unsigned& operator&=(unsigned& a, type b) { a = a & static_cast<unsigned>(b); return a; }

#define __ImplementEnumComparisonOperators(type) \
	inline bool operator>(type a, unsigned b) { return static_cast<unsigned>(a) > b; }\
	inline bool operator>(unsigned a, type b) { return a > static_cast<unsigned>(b); }\
	inline bool operator<(type a, unsigned b) { return static_cast<unsigned>(a) < b; }\
	inline bool operator<(unsigned a, type b) { return a < static_cast<unsigned>(b); }\
	inline bool operator==(type a, unsigned b) { return static_cast<unsigned>(a) == b; }\
	inline bool operator==(unsigned a, type b) { return a == static_cast<unsigned>(b); }\
	inline bool operator!=(type a, unsigned b) { return static_cast<unsigned>(a) != b; }\
	inline bool operator!=(unsigned a, type b) { return a != static_cast<unsigned>(b); }

	/*
namespace TYPE {\
	inline bool HasFlags(const TYPE& a, TYPE flags) { return (a & flags) == flags; }\
	constexpr typename std::underlying_type<TYPE>::type ToInteger(TYPE a) { return static_cast<typename std::underlying_type<TYPE>::type>(a); }\
	constexpr TYPE FromInteger(typename std::underlying_type<TYPE>::type a) { return static_cast<TYPE>(a); }}
	*/
