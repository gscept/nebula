import os, platform, sys
import sjson
import ntpath
import subprocess
from pathlib import Path
Version = 1

import sys
if __name__ == '__main__':
    sys.path.insert(0,'../fips/generators')
    sys.path.insert(0,'../../../fips/generators')

import genutil as util
import IDLC
import IDLC.filewriter

def Error(object, msg):
    print('[Material Template Compiler] error({}): {}'.format(object, msg))
    sys.exit(-1)

def Warning(object, msg):
    print('[Material Template Compiler] warning({}): {}'.format(object, msg))

def Assert(cond, object, msg):
    if not cond:
        Error(object, msg)

def TypeToString(type, edit, val):
    typeStr = ''
    accessorStr = ''
    valueStr = ''
    if type == "vec4" or type == "vec3" or type == "vec2":

        vecStr = ''
        for v in val:
            if v is not val[-1]:
                vecStr += '{}, '.format(v)
            else:
                vecStr += '{}'.format(v)

        if type == 'vec4':
            if edit == 'color':
                typeStr = 'Color'
            else:
                typeStr = 'Vec4'
            accessorStr = 'f4'
            valueStr = 'Math::float4{{{}}}'.format(vecStr)
        elif type == 'vec3':
            if edit == 'color':
                typeStr = 'Color'
            else:
                typeStr = 'Vec3'
            valueStr = 'Math::float3{{{}}}'.format(vecStr)
            accessorStr = 'f3'
        elif type == 'vec2':
            typeStr = 'Vec2'
            accessorStr = 'f2'
            valueStr = 'Math::float2{{{}}}'.format(vecStr)
        
    elif type == "float":
        typeStr = 'Scalar'
        accessorStr = 'f'
        valueStr = '{}'.format(val)
    elif type == "int":
        typeStr = 'Int'
        accessorStr = 'i'
        valueStr = '{}'.format(val)
    elif type == "bool":
        typeStr = 'Bool'
        accessorStr = 'b'
        valueStr = '{}'.format(val).lower()
    return typeStr, accessorStr, valueStr

class StructDeclaration:
    def __init__(self, name):
        self.name = name
        self.members = list()
        self.structs = list()

    def AddMember(self, type, name):
        self.members.append(dict(name = name, type = type))

    def AddStruct(self, struct):
        self.structs.append(struct)
        
    def Format(self, indent):
        str = ''
        for struct in self.structs:
            str += struct.Format(indent + '\t')
        for member in self.members:
            str += '{}\t{} {};\n'.format(indent, member["type"], member["name"])
        return '{}struct {}\n{}{{\n{}{}}};\n'.format(indent, self.name, indent, str, indent)
    
class VariableDefinition:
    def __init__(self, name, ty, default, desc, edit):
        self.name = name
        self.type = ty
        self.default = default
        self.desc = desc
        self.edit = edit

    def __hash__(self):
        return hash(self.name)
    
    def __eq__(self, other):
        return self.name == other.name

class PassDefinition:
    def __init__(self, batch, shader, variation):
        self.batch = batch
        self.shader = shader
        self.variation = variation

    def __hash__(self):
        return hash(self.batch)

    def __eq__(self, other):
        return self.batch == other.batch

class MaterialTemplateDefinition:
    def __init__(self, node, parser):
        self.name = node['name']
        self.name = self.name.replace(" ", "_").replace("+", "")
        self.inherits = ""
        self.virtual = False
        self.passes = list()
        self.variables = list()
        self.interface = None
        self.vertex = "Unknown"
        self.group = "Unknown"

        if "inherits" in node:
            self.inherits = node["inherits"]
        else:
            if "interface" not in node:
                Error(self.name, "Does not define an interface and doesn't inherit one which does")
            if "interface" in node and node["interface"] not in parser.interfaceDict:
                Error(self.name, "References undefined interface '{}'".format(node['interface']))
            self.interface = parser.interfaceDict[node["interface"]]
        if "virtual" in node:
            self.virtual = node["virtual"]
        if not self.virtual:
            self.vertex = node["vertexType"]
        if "group" in node:
            self.group = node["group"]
        self.desc = node["desc"]
        if "variables" in node:
            for var in node["variables"]:
                varName = var["name"]
                if not varName in self.interface.valuesDict:
                    Error(self.name, "Defines default value for '{}' but no such value is defined in interface '{}'".format(varName, self.interface.name))
                varType = self.interface.valuesDict[varName].type

                varDef = var["defaultValue"]

                # Validate default value
                if varType == "float":
                    Assert(type(varDef) is float, self.name, "Variable '{}' is of type 'float' but initialized as '{}'".format(varName, type(varDef)))
                elif varType == "int":
                    Assert(type(varDef) is int, self.name, "Variable '{}' is of type 'int' but initialized as '{}'".format(varName, type(varDef)))
                elif varType == "vec2":
                    Assert(len(varDef) == 2, self.name, "Type 'vec2' requires 2 values")
                    for v in varDef:
                        Assert(type(v) is float, self.name, "Variable '{}' is of type 'vec2' but initialized as '{}'".format(varName, type(v)))
                elif varType == "vec3":
                    Assert(len(varDef) == 3, self.name, "Type 'vec3' requires 3 values")
                    for v in varDef:
                        Assert(type(v) is float, self.name, "Variable '{}' is of type 'vec3' but initialized as '{}'".format(varName, type(v)))
                elif varType == "vec4":
                    Assert(len(varDef) == 4, self.name, "Type 'vec4' requires 4 values")
                    for v in varDef:
                        Assert(type(v) is float, self.name, "Variable '{}' is of type 'vec4' but initialized as '{}'".format(varName, type(v)))
                elif varType == "texture2d":
                    Assert(type(varDef) is str, self.name, "Variable '{}' is of type 'texture2d' but initialized as '{}'".format(varName, type(varDef)))
                elif varType == "textureHandle":
                    Assert(type(varDef) is str, self.name, "Variable '{}' is of type 'textureHandle' but initialized as '{}'".format(varName, type(varDef)))

                varDesc = ''
                varEdit = ''
                if "desc" in var:
                    varDesc = var["desc"]
                if "edit" in var:
                    varEdit = var["edit"]
                
                self.variables.append(VariableDefinition(varName, varType, varDef, varEdit, varDesc))
        if "passes" in node:
            for p in node["passes"]:
                    batch = p["batch"]
                    shader = p["shader"]
                    variation = p["variation"]
                    self.passes.append(PassDefinition(batch, shader, variation))
    pass

    def FormatHeader(self):
        #ret = 'struct {}\n{{\n'.format(self.name)
        ret = ''

        if self.virtual:
            ret += "\t/* Virtual Material */\n"
        ret += '\tconst char* Description = "{}";\n'.format(self.desc)
        if not self.virtual:
            texCounter = 0
            for v in self.variables:
                if v.type == 'texture2d' or v.type == 'textureHandle':
                    ret += '''\tstatic constexpr MaterialTemplateTexture __{} = 
                        MaterialTemplateTexture{{
                            .bindlessOffset = {},
                            .resource = "{}"
                #ifdef WITH_NEBULA_EDITOR
                            , .desc = {},
                            .hashedName = "{}"_hash,
                            .textureIndex = {}
                #endif
                        }};\n'''.format(v.name, 'offsetof(MaterialInterfaces::{}Material, {})'.format(self.interface.name, v.name) if v.type == 'textureHandle' else '0xFFFFFFFF', v.default, 'nullptr' if len(v.desc) == 0 else '"{}"'.format(v.desc), v.name, texCounter)
                    texCounter += 1
                else:
                    typeStr, accessorStr, varStr = TypeToString(v.type, v.edit, v.default)
                    ret += '''\tstatic constexpr MaterialTemplateValue __{} = 
                        MaterialTemplateValue{{
                            .type = MaterialTemplateValue::Type::{}, 
                            .data = {{.{} = {}}},
                            .offset = offsetof(MaterialInterfaces::{}Material, {})
                #ifdef WITH_NEBULA_EDITOR
                            , .desc = {}
                #endif
                            }};\n'''.format(v.name, typeStr, accessorStr, varStr, self.interface.name, v.name, "nullptr" if len(v.desc) == 0 else '"{}"'.format(v.desc))
                    
            for p in self.passes:
                ret += '\tEntry::Pass __{};\n'.format(p.batch)
                for v in self.variables:
                    if v.type == 'texture2d':
                        ret += '\tMaterials::ShaderConfigBatchTexture __{}_{};\n'.format(p.batch, v.name)
                        
            ret += '\n\tEntry entry = {{.name = "{}", .uniqueId = "{}"_hash, .properties = {}, .bufferName="_{}", .bufferSize=sizeof(MaterialInterfaces::{}Material), .vertexLayout = CoreGraphics::{}, .numTextures = {}}};\n'.format(self.name, self.name, self.interface.uniqueId, self.interface.name, self.interface.name, self.vertex, texCounter)
            ret += '\n\tvoid Setup();\n'

        ret = 'struct {}\n{{\n{}}};\n'.format(self.name, ret)
        ret += 'extern struct {} __{};'.format(self.name, self.name)
        return ret
    
    def FormatSource(self):
        if not self.virtual:
            func = ''
            numTextures = 0
            numConstants = 0
            constLookup = 0
            texLookup = 0
            func += '\tthis->entry.texturesPerBatch.Resize({});\n'.format(len(self.passes))
            func += '\tthis->entry.textureBatchLookup.Resize({});\n'.format(len(self.passes))
            defList = list()
            for var in self.variables:
                typeStr, accessorStr, varStr = TypeToString(var.type, var.edit, var.default)
                table = ''
                
                lookup = 0
                if var.type == "vec4" or var.type == "vec3" or var.type == "vec2" or var.type == "float" or var.type == "bool" or var.type == "int":
                    lookup = constLookup
                    constLookup += 1
                    numConstants += 1
                    table = 'values'
                elif var.type == "textureHandle" or var.type == "texture2d":
                    lookup = texLookup
                    texLookup += 1
                    numConstants += 1
                    table = 'textures'

                func += '#ifdef WITH_NEBULA_EDITOR\n'
                func += '\tthis->entry.{}ByHash.Add("{}"_hash, &this->__{});\n'.format(table, var.name, var.name)
                func += '#endif\n'
                func += '\tthis->entry.{}.Add("{}", &this->__{});\n'.format(table, var.name, var.name)
                if var.type == "texture2d":
                    defList.append('&this->__{}'.format(var.name))
            
            passCounter = 0
            for p in self.passes:
                func += '\t{\n'
                func += '\t\t/* Pass {} */\n'.format(p.batch)
                func += '\t\tCoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:{}.gplb");\n'.format(p.shader)
                func += '\t\tCoreGraphics::ShaderProgramId program = CoreGraphics::ShaderGetProgram({}, CoreGraphics::ShaderFeatureMask("{}"));\n'.format('shader', p.variation)
                func += '\t\tIndexT bufferSlot = InvalidIndex;\n'
                func += '\t\tif (this->entry.bufferName != nullptr) bufferSlot = CoreGraphics::ShaderGetResourceSlot({}, this->entry.bufferName);\n'.format('shader')
                func += '\t\tthis->__{} = Entry::Pass{{.shader = shader, .program = program, .index = {}, .name = "{}", .bufferIndex=bufferSlot }};\n'.format(p.batch, passCounter, p.batch)
                func += '\t\tthis->entry.passes.Add(MaterialTemplatesGPULang::BatchGroup::{}, &this->__{});\n'.format(p.batch, p.batch)
                func += '\t\tthis->entry.texturesPerBatch[{}].Resize({});\n'.format(passCounter, numTextures)
                texCounter = 0
                for var in self.variables:
                    memStr = 'this->__{}_{}'.format(p.batch, var.name)
                    if var.type == 'texture2d':
                        func += '\t\tthis->entry.textureBatchLookup[{}].Add("{}"_hash, {});\n'.format(passCounter, var.name, texCounter)
                        func += '\t\t{} = Materials::ShaderConfigBatchTexture{{.slot = CoreGraphics::ShaderGetResourceSlot(shader, "{}"), .def = {}}};\n'.format(memStr, var.name, defList[texCounter]);
                        func += '\t\tthis->entry.texturesPerBatch[{}][{}] = &{};\n'.format(passCounter, texCounter, memStr)
                        texCounter += 1
                func += '\t}\n'
                passCounter += 1
            return '\n//------------------------------------------------------------------------------\n/**\n*/\nvoid\n{}::Setup() \n{{\n{}}}'.format(self.name, func)
        
class InterfaceValueDefinition:
    def __init__(self, node):
        self.name = node['name']
        self.type = node['type']

class MaterialInterfaceDefinition:
    def __init__(self, node, parser):
        self.name = node['name']
        self.values = list()
        self.valuesDict = {};
        if 'inherits' in node:
            inherited = parser.interfaceDict[node['inherits']]
            for v in inherited.values:
                self.values.append(v)
                self.valuesDict[v.name] = v

        for v in node['values']:
            valueDef = InterfaceValueDefinition(v)
            if valueDef.name in self.valuesDict:
                Error(self.name, "Interface shadows inherited member '{}'".format(valueDef.name))
            self.values.append(valueDef)
            self.valuesDict[valueDef.name] = valueDef


    def FormatShader(self):
        contents = ""
        textures = ""
        texCounter = 64 - 16;
        
        typeTranslation = {
            "int": "i32",
            "float": "f32",
            "vec2": "f32x2",
            "vec3": "f32x3",
            "vec4": "f32x4",
            "bool": "u32",
            "texture2D": "texture2D",
            "textureHandle": "u32"
        }
        
        for v in self.values:
            if v.type != 'texture2d':
                contents += "\t{} : {};\n".format(v.name, typeTranslation[v.type])
            else:
                textures += "group(BATCH_GROUP) binding({}) uniform {}_{} : *texture2D;\n".format(texCounter, self.name, v.name)
                texCounter += 1
        
        ret = ""
        ret += "struct {}Material \n{{\n{}}};\n".format(self.name, contents)
        ret += textures
        ret += "\n"
        ret += "group(BATCH_GROUP) binding(MaterialBufferSlot) uniform {}Constants : *{}Material;\n".format(self.name, self.name)
        return ret


interfaceCounter = 0
class MaterialTemplateGenerator:
    def __init__(self):
        self.document = None
        self.version = 0
        self.materials = list()
        self.materialDict = {}
        self.interfaces = list()
        self.interfaceDict = {}
        self.batchGroupCounter = 0
        self.batchGroupDict = {}
        self.materialLists = []
        self.name = ""

    #------------------------------------------------------------------------------
    ##
    #
    def SetVersion(self, v):
        self.version = v

    #------------------------------------------------------------------------------
    ##
    #
    def SetName(self, n):
        self.name = n

    #------------------------------------------------------------------------------
    ##
    #    
    def SetDocument(self, input):
        fstream = open(input, 'r')
        self.document = sjson.loads(fstream.read())
        fstream.close()

    #------------------------------------------------------------------------------
    ##
    #
    def Parse(self):
        global interfaceCounter;
        self.materialDict.clear()
        self.materials.clear()
        self.interfaceDict.clear()
        self.interfaces.clear()

        if "Nebula" in self.document:
            main = self.document["Nebula"]
            for name, node in main.items():
                if name == "Templates":
                    for mat in node:
                        matDef = MaterialTemplateDefinition(mat, self)
                        for p in matDef.passes:
                            if p.batch not in self.batchGroupDict:
                                self.batchGroupDict[p.batch] = self.batchGroupCounter
                                self.batchGroupCounter += 1 
                        if matDef.inherits:
                            matDef.variables = self.materialDict[matDef.inherits].variables + matDef.variables
                            matDef.passes = self.materialDict[matDef.inherits].passes + matDef.passes
                            if matDef.interface == None:
                                matDef.interface = self.materialDict[matDef.inherits].interface;                        

                        self.materialDict[matDef.name] = matDef
                        self.materials.append(matDef)

                        if matDef.interface == None:
                            Error(matDef.name, "Template must either reference a valid interface or inherit from a template that does")
                            
                if name == "Interfaces":
                    for int in node:
                        intDef = MaterialInterfaceDefinition(int, self)
                        intDef.uniqueId = interfaceCounter
                        interfaceCounter += 1
                        self.interfaceDict[intDef.name] = intDef
                        self.interfaces.append(intDef)
                    
    #------------------------------------------------------------------------------
    ##
    #
    def FormatHeader(self, f):
        f.WriteLine('namespace {}\n{{\n'.format(self.name))

        f.WriteLine('#define ENUM_{}\\'.format(self.name))
        for int in self.interfaces:
            if int == self.interfaces[-1]:
                f.WriteLine('\t{},'.format(int.name))
            else:
                f.WriteLine('\t{},\\'.format(int.name))

        f.WriteLine("")
        f.WriteLine('void SetupMaterialTemplates();\n')

        for mat in self.materials:
            f.WriteLine(mat.FormatHeader())

        f.WriteLine('}} // namespace {}\n'.format(self.name))

    #------------------------------------------------------------------------------
    ##
    #
    def BeginHeader(self, f):
        f.WriteLine("// Material Template #version:{}#".format(self.version))
        f.WriteLine("#pragma once")
        f.WriteLine("//------------------------------------------------------------------------------")
        f.WriteLine("/**")
        f.IncreaseIndent()
        f.WriteLine("This file was generated with Nebula's Material Template compiler tool.")
        f.WriteLine("DO NOT EDIT")
        f.DecreaseIndent()
        f.WriteLine("*/")
        f.WriteLine('#include "util/string.h"')
        f.WriteLine('#include "util/dictionary.h"')
        f.WriteLine('#include "util/tupleutility.h"')
        f.WriteLine('#include "coregraphics/vertexlayout.h"')
        f.WriteLine('#include "materials/materialtemplatetypes.h"')
        f.WriteLine('#include "materials/gpulang/material_interfaces.h"')
        f.WriteLine('#include "materials/shaderconfig.h"')
        f.WriteLine('#include "coregraphics/shader.h"')
        f.WriteLine('#include "math/scalar.h"')
        f.WriteLine('#include "math/vec2.h"')
        f.WriteLine('#include "math/vec3.h"')
        f.WriteLine('#include "math/vec4.h"')
        f.WriteLine('using namespace Util;')
        f.WriteLine('namespace MaterialTemplatesGPULang\n{\n')
        f.WriteLine('enum class BatchGroup;\n')
        

    #------------------------------------------------------------------------------
    ##
    #
    def EndHeader(self, f):
        f.WriteLine('} // namespace MaterialTemplatesGPULang\n')

    #------------------------------------------------------------------------------
    ##
    #
    def GenerateHeader(self, outPath):
        f = IDLC.filewriter.FileWriter()
        f.Open(outPath)
        self.BeginHeader(f)
        self.FormatHeader(f)
        self.EndHeader(f)
        f.Close()

    #------------------------------------------------------------------------------
    ##
    #
    def FormatSource(self, f):
        f.WriteLine('namespace {}\n{{\n'.format(self.name))
        setupStr = ''
        for mat in self.materials:
            if not mat.virtual:
                f.WriteLine(mat.FormatSource())
                setupStr += '\t__{}.Setup();\n'.format(mat.name)
                setupStr += '\tLookup.Add("{}"_hash, &__{}.entry);\n'.format(mat.name, mat.name)
                for p in mat.passes:
                    setupStr += '\tConfigs[(uint)MaterialTemplatesGPULang::BatchGroup::{}].Append(&__{}.entry);\n'.format(p.batch, mat.name)
                setupStr += '\n'
                f.WriteLine('struct {}::{} __{};'.format(self.name, mat.name, mat.name))
        f.WriteLine('//------------------------------------------------------------------------------\n/**\n*/\nvoid\nSetupMaterialTemplates()\n{{\n{}}}\n'.format(setupStr))

        f.WriteLine('}} // namespace {}\n'.format(self.name))

    #------------------------------------------------------------------------------
    ##
    #  
    def BeginSource(self, f):
        f.WriteLine("// Material Template #version:{}#".format(self.version))
        f.WriteLine("//------------------------------------------------------------------------------")
        f.WriteLine("/**")
        f.IncreaseIndent()
        f.WriteLine("This file was generated with Nebula's Material Template compiler tool.")
        f.WriteLine("DO NOT EDIT")
        f.DecreaseIndent()
        f.WriteLine("*/")
        f.WriteLine('#include "{}.h"'.format(self.name))
        f.WriteLine('#include "util/string.h"')
        f.WriteLine('#include "util/dictionary.h"')
        f.WriteLine('#include "coregraphics/vertexlayout.h"')
        f.WriteLine('#include "materials/materialtemplatetypes.h"')
        f.WriteLine('#include "math/vec2.h"')
        f.WriteLine('#include "math/vec3.h"')
        f.WriteLine('#include "math/vec4.h"')
        f.WriteLine('using namespace Util;')
        f.WriteLine('namespace MaterialTemplatesGPULang\n{\n')
        

    #------------------------------------------------------------------------------
    ##
    #  
    def EndSource(self, f):
        f.WriteLine('} // namespace MaterialTemplatesGPULang\n')

    #------------------------------------------------------------------------------
    ##
    #
    def GenerateSource(self, outPath):
        f = IDLC.filewriter.FileWriter()
        f.Open(outPath)
        self.BeginSource(f)
        self.FormatSource(f)
        self.EndSource(f)
        f.Close()

    #------------------------------------------------------------------------------
    ##
    #
    def FormatShader(self, f):
        materialNames = "\\\n"
        for i in self.interfaces:
            f.WriteLine(i.FormatShader())
            materialNames += "\t\t{}Materials : *{}Material;\\\n".format(i.name, i.name);
            self.materialLists.append(i.name)
        

    #------------------------------------------------------------------------------
    ##
    #
    def BeginShader(self, f):
        f.WriteLine("// Material Interface #version:{}#".format(self.version))
        f.WriteLine("//------------------------------------------------------------------------------")
        f.WriteLine("/**")
        f.IncreaseIndent()
        f.WriteLine("This file was generated with Nebula's Material Template compiler tool.")
        f.WriteLine("DO NOT EDIT")
        f.DecreaseIndent()
        f.WriteLine("*/")
        f.WriteLine("#include <lib/std.gpuh>")
        f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def EndShader(self, f):
        f.WriteLine("")

# Entry point for generator
if __name__ == '__main__':
    globals()

    generator = MaterialTemplateGenerator()
    generator.SetVersion(Version)
    files = sys.argv[1:-3]
    shaderC = sys.argv[-3]
    shaderInclude = sys.argv[-2]
    outDir = sys.argv[-1]

    print(f'{shaderC}\n')
    print(f'{shaderInclude}\n')
    print(f'{outDir}\n')

    outHeaderPath = '{}/materialtemplatesgpulang.h'.format(outDir)
    outSourcePath = '{}/materialtemplatesgpulang.cc'.format(outDir)
    outShaderPath = '{}/material_interfaces.gpul'.format(outDir)
    compilerChangeTime = os.path.getmtime(sys.argv[0])

    hasModifications = False
    try:
        outputHeaderChangeTime = os.path.getmtime(outHeaderPath)
        outputSourceChangeTime = os.path.getmtime(outSourcePath)
        outputShaderChangeTime = os.path.getmtime(outShaderPath)

        for file in files:
            inputChangeTime = os.path.getmtime(file)
            if compilerChangeTime < outputHeaderChangeTime and compilerChangeTime < outputSourceChangeTime and compilerChangeTime < outputShaderChangeTime:
                if inputChangeTime <= outputHeaderChangeTime and inputChangeTime <= outputSourceChangeTime and inputChangeTime <= outputShaderChangeTime:
                    continue

            hasModifications = True
    except FileNotFoundError:
        hasModifications = True
    

    if hasModifications:
        generator.SetName("materialtemplatesgpulang")
        headerF = IDLC.filewriter.FileWriter()
        headerF.Open(outHeaderPath)
        generator.BeginHeader(headerF)

        sourceF = IDLC.filewriter.FileWriter()
        sourceF.Open(outSourcePath)
        generator.BeginSource(sourceF)

        sourceF.WriteLine('Util::Dictionary<uint, Entry*> Lookup;')
        sourceF.WriteLine('Util::Array<Entry*> Configs[(uint)MaterialTemplatesGPULang::BatchGroup::Num];')

        shaderF = IDLC.filewriter.FileWriter()
        shaderF.Open(outShaderPath)
        generator.BeginShader(shaderF)
        shaderF.WriteLine("const MaterialBindingSlot = 51u;")
        shaderF.WriteLine("const MaterialBufferSlot = 52u;")

        for file in files:
            path = Path(file).stem

            print("[Material Template Compiler] '{}' -> '{}/materialtemplatesgpulang.h' & '{}/materialtemplatesgpulang.cc & '{}/material_interfaces.gpul' ".format(file, outDir, outDir, outDir))
            generator.SetDocument(file)
            generator.SetName(path)
            generator.Parse()
            generator.FormatHeader(headerF)
            generator.FormatSource(sourceF)
            generator.FormatShader(shaderF)

        enumStr = '\tInvalid = -1,\n'
        for batch in generator.batchGroupDict:
            enumStr += '\t{} = {},\n'.format(batch, generator.batchGroupDict[batch])
        enumStr += '\tNum\n'
        headerF.WriteLine('enum class BatchGroup\n{{\n{}}};'.format(enumStr))

        conversionStr = ''
        for batch in generator.batchGroupDict:
            conversionStr += '\t\tcase "{}"_hash: return BatchGroup::{};\n'.format(batch, batch)
        conversionStr += '\t\tdefault: return BatchGroup::Invalid;'
        headerF.WriteLine('''
    //------------------------------------------------------------------------------
    /**
    */
    inline BatchGroup\nBatchGroupFromName(const char* name)\n{{\n\tuint code = Util::String::Hash(name, strlen(name));\n\tswitch(code)\n\t{{\n{}\n\t}}\n}}\n'''.format(conversionStr))

        # Finish header
        enumStr = ''
        for file in files:
            name = Path(file).stem
            enumStr += '\tENUM_{}\n'.format(name)
        enumStr += '\tNum\n'

        headerF.WriteLine('enum class MaterialProperties\n{{\n{}}};'.format(enumStr))
        headerF.WriteLine('extern Util::Dictionary<uint, Entry*> Lookup;')
        headerF.WriteLine('extern Util::Array<Entry*> Configs[(uint)MaterialTemplatesGPULang::BatchGroup::Num];\n')
        headerF.WriteLine('void SetupMaterialTemplates();\n')
        
        generator.EndHeader(headerF)
        headerF.Close()

        # Finish source
        setupStr = ''
        for file in files:
            name = Path(file).stem
            setupStr += '\t{}::SetupMaterialTemplates();\n'.format(name)

        sourceF.WriteLine('//------------------------------------------------------------------------------\n/**\n*/\nvoid\nSetupMaterialTemplates() \n{{\n{}}}'.format(setupStr))

        generator.EndSource(sourceF)
        sourceF.Close()

        # Finish shader


        bindingsContent = "\n".join(["\t{}Materials : address {}Material;".format(i, i) for i in generator.materialLists])
        shaderF.WriteLine("struct MaterialBinding\n{{\n{}\n}};\n".format(bindingsContent));
        shaderF.WriteLine("group(BATCH_GROUP) binding(MaterialBindingSlot) uniform MaterialPointers : *MaterialBinding;")
        generator.EndShader(shaderF)
        shaderF.Close()

    
    # Finally, run the AnyFX compiler
    shaderBinaryOutput = "{}/material_interfaces.gplb".format(outDir)
    shaderHeaderOutput = "{}/material_interfaces.h".format(outDir)

    try:
        outputShaderBinaryChangeTime = os.path.getmtime(shaderBinaryOutput)
        outputShaderHeaderChangeTime = os.path.getmtime(shaderHeaderOutput)
        shaderCompilerChangeTime = os.path.getmtime(shaderC)
        if shaderCompilerChangeTime < outputShaderBinaryChangeTime and shaderCompilerChangeTime < outputShaderHeaderChangeTime:
            if outputShaderChangeTime <= outputShaderBinaryChangeTime and outputShaderChangeTime <= outputShaderHeaderChangeTime:
                exit(0)
    except FileNotFoundError:
        pass

    subprocess.run([shaderC, outShaderPath, "-I", shaderInclude, "-I", outDir, "-o", shaderBinaryOutput, "-h", shaderHeaderOutput, "-t", "shader"])
