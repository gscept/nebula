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



#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "nanobind/nanobind.h"

using namespace Math;
using namespace Base;
using namespace Test;

namespace py = nanobind;



struct foo
{
    int a, b;
    void set(int aa, int bb) {
        a = aa; 
        b = bb; }
};

NB_MODULE(FooMod, m)
{    
    py::class_<foo>(m, "foo").def("set", &foo::set).def(py::init<int,int>())
    .def("__repr__", [](foo& p)
    {
        Util::String form;
        form.Format("Foo: %d, %d", p.a, p.b);
        return form.AsCharPtr();
    }
    );
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
    
    
    
    
    PyImport_AppendInittab("FooMod", PyInit_FooMod);
    Py_Initialize();

    //py::module::import("FooMod");
    PyRun_SimpleString("print('a')");
            
    //PyRun_SimpleString("import FooMod");
    PyRun_SimpleString("print('a')");

    PyRun_SimpleString("import FooMod");
    PyRun_SimpleString("f = FooMod.foo(2,4)");
    PyRun_SimpleString("print(f)");
    py::object main = py::module_::import_("__main__");
    foo f;
    main.attr("g") = &f;    
    PyRun_SimpleString("g.set(5, 3)");
   

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
