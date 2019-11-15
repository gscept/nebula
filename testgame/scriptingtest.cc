//------------------------------------------------------------------------------
//  scriptingtest.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scriptingtest.h"

#include "scripting/python/pythonserver.h"
#include "scripting/bindings.h"
#include "scripting/python/conversion.h"

#include "pybind11/embed.h"

namespace py = pybind11;

const int integer = 10;
int GetInteger()
{
    return integer;
}

Util::Variant variant;
Util::Variant GetVariant()
{
    return variant;
}

PYBIND11_EMBEDDED_MODULE(test, m)
{
    m.def("get_integer", &GetInteger);

    m.def("get_integer2",
        []()->int
        {
            return integer;
        }
    );

    m.def("get_string",
        []()->Util::String
        {
            return "constant C++ string!"_str;
        }
    );

    m.def("generate_guid",
        []()->Util::Guid
        {
            Util::Guid guid;
            guid.Generate();
            return guid;
        }
    );

    m.def("print_guid_from_py",
        [](Util::Guid const& guid)->void
        {
            n_printf(guid.AsString().AsCharPtr());
            n_printf("\n");
        }
    );

    m.def("print_string_from_py",
        [](Util::String const& str)->void
        {
            n_printf(str.AsCharPtr());
            n_printf("\n");
        }
    );

    m.def("print_uint_from_py",
        [](uint i)->void
        {
            n_printf("%i", i);
            n_printf("\n");
        }
    );

    m.def("print_variant_from_py",
        [](Util::Variant value)->void
        {
            n_printf("Variant Type: %s", Util::Variant::TypeToString(value.GetType()).AsCharPtr());
            n_printf("\n");
            n_printf("Variant Value: %s", value.ToString().AsCharPtr());
        }
    );

    m.def("get_variant", &GetVariant);

    m.def("print_mat4_from_py",
        [](Math::matrix44 const& mat)->void
        {
            Util::Variant v(mat);
            n_printf(v.ToString().AsCharPtr());
            n_printf("\n");
        }
    );

    m.def("print_int_arr_from_py",
        [](Util::Array<int> const& arr)->void
        {
            n_printf("Array: ");
            for (auto val : arr)
                n_printf("%i ", val);
            n_printf("\n");
        }
    );

    m.def("print_float_arr_from_py",
        [](Util::Array<float> const& arr)->void
        {
            n_printf("fArray: ");
            for (auto val : arr)
                n_printf("%f ", val);
            n_printf("\n");
        }
    );

    m.def("print_float4_arr_from_py",
        [](Util::Array<Math::float4> const& arr)->void
        {
            n_printf("f4Array: ");
            for (auto val : arr)
                n_printf("%s ", Util::String::FromFloat4(val).AsCharPtr());
            n_printf("\n");
        }
    );

    m.def("print_str_arr_from_py",
        [](Util::Array<Util::String> const& arr)->void
        {
            n_printf("strArray: ");
            for (auto val : arr)
                n_printf("%s ", val.AsCharPtr());
            n_printf("\n");
        }
    );

    m.def("get_native_array",
        []()->Util::Array<Util::String>
        {
            return Util::Array<Util::String>({Util::String("hello"), "from", "C++!"});
        }
    );

    m.def("print_dict_from_py",
        [](Util::Dictionary<Util::String, int> const& dict)->void
        {
            n_printf("str->int dict: \n");
            n_printf("'a' = %i\n", dict["a"]);
            n_printf("'b' = %i\n", dict["b"]);
            n_printf("'c' = %i\n", dict["c"]);
            n_printf("\n");
        }
    );

    m.def("get_variant_int",
        []()
        {
            Util::Variant v(int(10));
            return v;
        }
    );

    m.def("get_variant_float4",
        []()
        {
            Util::Variant v(Math::float4(1,2,3,4));
            return v;
        }
    );

    m.def("get_variant_guid",
        []()
        {
            Util::Guid g;
            g.Generate();
            Util::Variant v(g);
            return v;
        }
    );

    m.def("get_variant_int_arr",
        []()
        {
            Util::Variant v(Util::Array<int>({1,2,3,4,5,6}));
            return v;
        }
    );

    m.def("get_variant_matrix44_arr",
        []()
        {
            Util::Variant v(Util::Array<Math::matrix44>({Math::matrix44::identity(),Math::matrix44::identity(),Math::matrix44({1,20,2,3},{2,3,4,5},{5,4,3,2},{7,4,5,3})}));
            return v;
        }
    );

    m.def("get_variant_matrix44",
        []()
        {
            Util::Variant v(Math::matrix44({1,20,2,3},{2,3,4,5},{5,4,3,2},{7,4,5,3}));
            return v;
        }
    );

    m.def("get_float4",
        []() -> Math::float4
        {
            Math::float4 f = {1.0f,2.0f,3.0f,4.0f};
            return f;
        }
    );
}


using namespace Scripting;

namespace Test
{
__ImplementClass(Test::ScriptingTest, 'scrT', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
ScriptingTest::Run()
{
	Ptr<ScriptServer> server = Scripting::PythonServer::Create();
    if (!server->Open())
    {
        n_printf("[ERROR]: Could not open python script server!\n");
        return;
    }
    
    const auto EVAL = [&server](auto str) {
        n_printf(">> %s\n", str);
        if (!server->Eval(str))
        {
            // n_printf("[WARNING]: Could not evaluate input!\n");
        }
    };

    // TODO: Setup verification in python
    // TODO: Verify input/output from python

    EVAL("import foundation");
    EVAL("import util");
    EVAL("import game");
    EVAL("import test");
    EVAL("import nmath");
    EVAL("import numpy");
    EVAL("import numpy.matlib");
    EVAL("import numpy.linalg");
    EVAL("import numpy.random");

    EVAL("print(\"Hello world from Python!\\n\")");
    EVAL("i = test.get_integer()");
    EVAL("print(i)");
    EVAL("i = test.get_integer2()");
    EVAL("print(i)");
    
    // Util::String
    EVAL("test.print_string_from_py('constant python string!')");
    EVAL("str = test.get_string()");
    EVAL("print(type(str))");
    EVAL("print(str)");
    EVAL("str = 'dynamic python string'");
    EVAL("test.print_string_from_py(str)");
    
    // Util::Guid
    EVAL("guid = util.Guid()");
    EVAL("guid.generate()");
    EVAL("s = guid.to_string()");
    EVAL("print(guid.to_string())");
    EVAL("print(guid)");

    // Util::FourCC
    EVAL("fcc1 = util.FourCC(\"abcd\")");
    EVAL("fcc2 = util.FourCC(\"cdba\")");
    EVAL("fcc3 = util.FourCC(\"abcd\")");
    EVAL("print(fcc1)");
    EVAL("print(fcc2)");
    EVAL("print(fcc3)");
    EVAL("print(fcc1 == fcc2)");
    EVAL("print(fcc1 == fcc3)");
    EVAL("print(fcc1 < fcc3)");
    EVAL("print(fcc1 > fcc3)");
    EVAL("print(fcc1 >= fcc3)");
    EVAL("print(fcc1 <= fcc3)");
    EVAL("print(fcc1 != fcc2)");
    EVAL("print(util.FourCC.lookup('Scripting::PythonServer'))");

    // Math::float4
    EVAL("tf4 = test.get_float4()");
    EVAL("print(type(tf4))");
    EVAL("print(tf4)");
    EVAL("f4 = nmath.Float4(1.0, 1.0, 1.0, 1.0)");
    EVAL("print(f4)");
    EVAL("f4 = nmath.Float4(10.0, 0.5, 0.75, 2.0)");
    EVAL("print(f4)");
    EVAL("f4 = f4 + nmath.Float4(10.0, 0.5, 0.75, 2.0)");
    EVAL("print(f4)");
    EVAL("dot = nmath.Float4.dot3(f4, f4)");
    EVAL("print(dot)");
    EVAL("npf4 = numpy.array([1.0, 2.0, 3.0, 4.0])");
    EVAL("print(npf4)");
    EVAL("f4res = npf4 + f4");
    EVAL("print(f4res)");
    EVAL("f4res = nmath.Float4(f4 + npf4)");
    EVAL("print(f4res)");
    EVAL("print(f4res[0])");
    EVAL("print(f4res[1])");
    EVAL("print(f4res[2])");
    EVAL("print(f4res[3])");

    // Math::point
    EVAL("p = nmath.Point(test.get_float4());");
    EVAL("print(type(p))");
    EVAL("print(p)");
    EVAL("p = nmath.Point(30,20,10)");
    EVAL("print(p)");
    EVAL("p = nmath.Point(npf4)");
    EVAL("print(type(p))");
    EVAL("print(p)");

    // Math::point
    EVAL("p = nmath.Vector(test.get_float4());");
    EVAL("print(type(p))");
    EVAL("print(p)");
    EVAL("p = nmath.Vector(30,20,10)");
    EVAL("print(p)");
    EVAL("p = nmath.Vector(npf4)");
    EVAL("print(type(p))");
    EVAL("print(p)");

    EVAL("p = p * 100");
    EVAL("print(p)");

    // Math::matrix44
    EVAL("mat = nmath.Matrix44.identity()");
    EVAL("print(mat)");
    EVAL("mat = nmath.Matrix44(1,2,3,4, 5,1,7,8, 9,10,1,12, 13,14,15,1)");
    EVAL("print(mat)");
    EVAL("mat = nmath.Matrix44.inverse(mat)");
    EVAL("print(mat)");
    EVAL("mat = nmath.Matrix44.transpose(mat)");
    EVAL("print(mat)");
    
    // Util::Array
    EVAL("arr = [1,2,3,4,5,6,7,8,9,10]");
    EVAL("test.print_int_arr_from_py(arr)");
    EVAL("arr = [1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0]");
    EVAL("test.print_float_arr_from_py(arr)");
    EVAL("f4arr = [nmath.Float4(0,1,2,3),nmath.Float4(1,2,3,4),nmath.Float4(4,3,2,1)]");
    EVAL("print(type(f4arr[0]))");
    EVAL("print(f4arr)");
    EVAL("test.print_float4_arr_from_py(f4arr)");
    EVAL("arr = ['test1', 'test2', 'hellofrom3']");
    EVAL("test.print_str_arr_from_py(arr)");
    EVAL("arr = test.get_native_array()");
    EVAL("print(type(arr))");    
    EVAL("print(arr)");    
    EVAL("dict = { 'a':0, 'b':1, 'c':2 }");
    EVAL("test.print_dict_from_py(dict)");

    // Util::Variant
    EVAL("val = 10");
    EVAL("test.print_variant_from_py(val)");
    EVAL("val = 5.2");
    EVAL("test.print_variant_from_py(val)");
    EVAL("val = nmath.Float4(1.0,2.0,3.0,4.0)");
    EVAL("test.print_variant_from_py(val)");
    EVAL("pyvar = test.get_variant_int()");
    EVAL("print(type(pyvar))");
    EVAL("print(pyvar)");
    EVAL("pyvar = test.get_variant_float4()");
    EVAL("print(type(pyvar))");
    EVAL("print(pyvar)");
    EVAL("pyvar = test.get_variant_guid()");
    EVAL("print(type(pyvar))");
    EVAL("print(pyvar)");
    EVAL("pyvar = test.get_variant_int_arr()");
    EVAL("print(type(pyvar))");
    EVAL("print(pyvar)");
    EVAL("pyvar = test.get_variant_matrix44_arr()");
    EVAL("print(type(pyvar))");
    EVAL("print(pyvar)");
    EVAL("pyvar = test.get_variant_matrix44()");
    EVAL("print(type(pyvar))");
    EVAL("print(pyvar)");

    n_printf("// Game system test ---------------------------------------------------------------\n");

    EVAL("entity = game.create_entity()");
    EVAL("print(entity)");
    EVAL("print(entity.is_valid())");
    EVAL("print(entity.has_component(\"TransformComponent\"))");
    EVAL("entity.register_component(\"TransformComponent\");");
    EVAL("print(entity.has_component(\"TransformComponent\"))");
    EVAL("print(entity.world_transform.position)");
    EVAL("print(entity.local_transform.position)");
    EVAL("print(entity.local_transform.forward)");
    EVAL("m = nmath.Matrix44.translation(10,3,4);");
    EVAL("entity.local_transform = m;");
    EVAL("print(entity.local_transform.position)");
    EVAL("print(entity.local_transform.forward)");
    EVAL("entity.deregister_component(\"TransformComponent\")");
    EVAL("print(entity.has_component(\"TransformComponent\"))");
    EVAL("game.destroy_entity(entity)");
}

}