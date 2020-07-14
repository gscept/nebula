//------------------------------------------------------------------------------
//  ScriptTestApplication.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scripttestapplication.h"
#include "math/mat4.h"
#include "io/ioserver.h"
#include "util/stringatom.h"
#include "io/fswrapper.h"
#include "math/vector.h"

#include "Python.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
using namespace Math;
using namespace Base;
using namespace Test;

namespace py = pybind11;



struct foo
{
    int a, b;
    void set(int aa, int bb) {
        a = aa; 
        b = bb; }
};

PYBIND11_EMBEDDED_MODULE(FooMod, m)
{    
    py::class_<foo>(m, "foo").def("set", &foo::set);    
}

//------------------------------------------------------------------------------
/*
*/
ScriptTestApplication::ScriptTestApplication(int a, char**v):argc(a),argv(v)
{
}

//------------------------------------------------------------------------------
/*
*/
ScriptTestApplication::~ScriptTestApplication()
{
}

//------------------------------------------------------------------------------
/*
*/
bool 
ScriptTestApplication::Open()
{
    this->coreServer = Core::CoreServer::Create();
    this->coreServer->SetCompanyName(Util::StringAtom("Radon Labs GmbH"));
    this->coreServer->SetAppName(Util::StringAtom("PS3 Audio Test Simple"));
    this->coreServer->Open();


    this->masterTime.Start();
    
    
    
    
    //PyImport_AppendInittab("FooMod", PyInit_FooMod);
    Py_Initialize();

    //py::module::import("FooMod");
    PyRun_SimpleString("print('a')");
            
    //PyRun_SimpleString("import FooMod");
    PyRun_SimpleString("print('a')");

    PyRun_SimpleString("import FooMod");
    py::module main = py::module::import("__main__");
    foo f;
    main.attr("f") = &f;    
    PyRun_SimpleString("f.set(5, 3)");
   

    return true;
}

//------------------------------------------------------------------------------
/*
*/
void 
ScriptTestApplication::Close()
{    
    Py_Finalize();
    this->masterTime.Stop();

    this->coreServer->Close();
    this->coreServer = nullptr;
}

//------------------------------------------------------------------------------
/*
*/
void 
ScriptTestApplication::Run()
{
    // waiting for game pad
    

   // while(true)
    {
    
        Core::SysFunc::Sleep(0.01);
    }
}

//------------------------------------------------------------------------------
/*
*/
