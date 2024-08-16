import os, platform, sys
import sjson
import ntpath
import argparse
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
    print('[Frame Script Compiler] error({}): {}'.format(object, msg))
    sys.exit(-1)

def Warning(object, msg):
    print('[Frame Script Compiler] warning({}): {}'.format(object, msg))

def Assert(cond, object, msg):
    if not cond:
        Error(object, msg)

class TextureImportDefinition:
    def __init__(self, parser, name):
        self.name = name.replace(" ", "")

    def FormatHeader(self, file):
        file.WriteLine("void Bind_{}(const Frame::TextureImport& imp);".format(self.name))

    def FormatSource(self, file):
        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Bind_{}(const Frame::TextureImport& imp)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("Textures[(uint)TextureIndex::{}] = imp.tex;".format(self.name))
        file.WriteLine("TextureCurrentStage[(uint)TextureIndex::{}] = imp.stage;".format(self.name))
        file.WriteLine("TextureOriginalStage[(uint)TextureIndex::{}] = imp.stage;".format(self.name))
        file.WriteLine("TextureImageBits[(uint)TextureIndex::{}] = CoreGraphics::PixelFormat::ToImageBits(CoreGraphics::TextureGetPixelFormat(imp.tex));".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatAssign(self):
        pass

class BufferImportDefinition:
    def __init__(self, parser, name):
        self.name = name.replace(" ", "")

    def FormatHeader(self, file):
        file.WriteLine("void Bind_{}(const CoreGraphics::BufferId id);".format(self.name))

    def FormatSource(self, file):
        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Bind_{}(const CoreGraphics::BufferId id)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("Buffers[(uint)BufferIndex::{}] = id;".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatAssign(self):
        pass
    
class TextureExportDefinition:
    def __init__(self, parser, name):
        self.name = name.replace(" ", "")
        parser.externs.append(self)

    def FormatExtern(self, file):
        file.WriteLine("Frame::TextureExport Export_{};".format(self.name))

    def FormatHeader(self, file):
        file.WriteLine("extern Frame::TextureExport Export_{};".format(self.name))

    def FormatSource(self, file):
        file.WriteLine("Frame::TextureExport Export_{} = {{ .index = (uint)TextureIndex::{}, .tex = Textures[(uint)TextureIndex::{}], .stage = TextureCurrentStage[(uint)TextureIndex::{}] }};".format(self.name, self.name, self.name, self.name))

class DependencyDefinition:
    def __init__(self, parser, name):
        self.name = name.replace(" ", "")

    def FormatHeader(self, file):
        file.WriteLine("void Bind_{}(const CoreGraphics::SubmissionWaitEvent event);".format(self.name))
        
    def FormatSource(self, file):

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("CoreGraphics::SubmissionWaitEvent Submission_{};".format(self.name))
        file.WriteLine("void")
        file.WriteLine("Bind_{}(const CoreGraphics::SubmissionWaitEvent event)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("Submission_{} = event;".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatAssign(self):
        return "Submission_{} = Import_{};".format(self.name, self.name)

class LocalTextureDefinition:
    def __init__(self, parser, node):
        self.name = node["name"].replace(" ", "")
        self.fixedSize = None
        self.relativeSize = None
        self.depthFormat = False
        self.stencilFormat = False
        self.bits = list()

        if 'fixedSize' in node:
            self.fixedSize = node['fixedSize']
        elif 'relativeSize' in node:
            self.relativeSize = node['relativeSize']
        else:
            Error(self.name, "Does not specify any size, please define either fixedSize or relativeSize")
        self.mips = 1
        self.layers = 1
        if 'mips' in node:
            self.mips = node['mips']
        if 'layers' in node:
            self.layers = node['layers']

        if 'format' not in node:
            Error(self.name, 'Local texture must declare format')
        self.format = node['format']
        if self.format in parser.depthFormats:
            self.depthFormat = True
            self.bits.append("DepthBits")
        if self.format in parser.stencilFormats:
            self.stencilFormat = True
            self.bits.append("StencilBits")
        if len(self.bits) == 0:
            self.bits.append("ColorBits")

        if 'usage' in node:
            self.usage = node['usage']
        else:
            self.usage = 'Render'

        usageBits = self.usage.split("|")
        self.usage = ""
        for bit in usageBits:
            self.usage += "CoreGraphics::TextureUsage::{}Texture".format(bit)
            if bit != usageBits[-1]:
                self.usage += " | "

        if 'type' in node:
            self.type = node['type']
        else:
            self.type = 'Texture2D'

    def HasDepthFormat(self):
        return self.depthFormat
    
    def HasStencilFormat(self):
        return self.depthFormat

    def FormatHeader(self, file):
        pass

    def FormatSource(self, file):
        file.WriteLine("")
        if self.relativeSize:
            file.WriteLine("//------------------------------------------------------------------------------")
            file.WriteLine("/**")
            file.WriteLine("*/")
            file.WriteLine("void")
            file.WriteLine("Initialize_Texture_{}(const uint frameWidth, const uint frameHeight)".format(self.name))
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("if (Textures[(uint)TextureIndex::{}] != CoreGraphics::InvalidTextureId)".format(self.name))
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::DestroyTexture(Textures[(uint)TextureIndex::{}]);".format(self.name))
            file.DecreaseIndent()
            file.WriteLine("}")
            file.WriteLine("CoreGraphics::TextureCreateInfo info;")
            file.WriteLine('info.name = "{}";'.format(self.name))
            file.WriteLine("info.type = CoreGraphics::{};".format(self.type))
            file.WriteLine("info.format = CoreGraphics::PixelFormat::Code::{};".format(self.format))
            file.WriteLine("info.width = {} * frameWidth;".format(self.relativeSize[0]))
            file.WriteLine("info.height = {} * frameHeight;".format(self.relativeSize[1]))
            file.WriteLine("info.usage = {};".format(self.usage))
            file.WriteLine("info.mips = {};".format("CoreGraphics::TextureAutoMips" if self.mips == "auto" else self.mips))
            file.WriteLine("info.layers = {};".format(self.layers))
            file.WriteLine("Textures[(uint)TextureIndex::{}] = CoreGraphics::CreateTexture(info);".format(self.name))
            file.WriteLine("TextureImageBits[(uint)TextureIndex::{}] = CoreGraphics::PixelFormat::ToImageBits(info.format);".format(self.name, self.name))
            file.WriteLine("TextureCurrentStage[(uint)TextureIndex::{}] = CoreGraphics::PipelineStage::InvalidStage;".format(self.name))
            file.WriteLine("TextureRelativeScale[(uint)TextureIndex::{}] = {{ {}, {} }};".format(self.name, self.relativeSize[0], self.relativeSize[1]))
            file.WriteLine("TextureRelativeSize[(uint)TextureIndex::{}] = {{ {} * frameWidth, {} * frameHeight }};".format(self.name, self.relativeSize[0], self.relativeSize[1]))
            file.DecreaseIndent()
            file.WriteLine("}")
        else:
            file.WriteLine("//------------------------------------------------------------------------------")
            file.WriteLine("/**")
            file.WriteLine("*/")
            file.WriteLine("void")
            file.WriteLine("Initialize_Texture_{}()".format(self.name))
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("if (Textures[(uint)TextureIndex::{}] != CoreGraphics::InvalidTextureId)".format(self.name))
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::DestroyTexture(Textures[(uint)TextureIndex::{}]);".format(self.name))
            file.DecreaseIndent()
            file.WriteLine("}")
            file.WriteLine("CoreGraphics::TextureCreateInfo info;")
            file.WriteLine('info.name = "{}";'.format(self.name))
            file.WriteLine("info.type = CoreGraphics::{};".format(self.type))
            file.WriteLine("info.format = CoreGraphics::PixelFormat::Code::{};".format(self.format))
            file.WriteLine("info.width = {};".format(self.fixedSize[0]))
            file.WriteLine("info.height = {};".format(self.fixedSize[1]))
            file.WriteLine("info.usage = {};".format(self.usage))
            file.WriteLine("info.mips = {};".format("TextureAutoMips" if self.mips == "auto" else self.mips))
            file.WriteLine("info.layers = {};".format(self.layers))
            file.WriteLine("Textures[(uint)TextureIndex::{}] = CoreGraphics::CreateTexture(info);".format(self.name))
            file.WriteLine("TextureImageBits[(uint)TextureIndex::{}] = CoreGraphics::PixelFormat::ToImageBits(info.format);".format(self.name, self.name))
            file.DecreaseIndent()
            file.WriteLine("}")

class ResourceDependencyDefinition:
    def __init__(self, parser, node):
        self.name = node['name']
        self.stage = node['stage']

    @classmethod
    def raw(self, name, stage):
        return self(None, dict(name = name, stage = stage))

class SubgraphDefinition:
    def __init__(self, parser, node, p, subp):
        self.name = "{}_{}".format(node['name'].replace(" ", ""), "Pass" if p is not None else "Compute")
        self.disabledBindings = list()
        self.p = p
        self.subp = subp
        if "disabled" in node:
            for binding in node['disabled']:
                self.disabledBindings.append(dict(name = binding['name'], value = binding['value']))

        parser.externs.append(self)

        if self.p is not None and self.subp is not None:
            parser.pipelines.append(self)

    def FormatExtern(self, file):
        file.WriteLine("")
        if len(self.disabledBindings) > 0:
            file.WriteLine("bool SubgraphEnabled_{};".format(self.name))
        file.WriteLine('Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> SubgraphTextureDependencies_{};'.format(self.name))
        file.WriteLine('Util::Array<Util::Pair<BufferIndex, CoreGraphics::PipelineStage>> SubgraphBufferDependencies_{};'.format(self.name))
        file.WriteLine("void (*Subgraph_{})(const CoreGraphics::CmdBufferId, const Math::rectangle<int>& viewport, const IndexT, const IndexT);\n".format(self.name))
        file.WriteLine("void (*SubgraphPipelines_{})(const CoreGraphics::PassId, const uint);\n".format(self.name))

        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine('void')
        file.WriteLine('RegisterSubgraph_{}(void(*func)(const CoreGraphics::CmdBufferId, const Math::rectangle<int>& viewport, const IndexT, const IndexT), Util::Array<Util::Pair<BufferIndex, CoreGraphics::PipelineStage>> bufferDeps, Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> textureDeps)'.format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("Subgraph_{} = func;".format(self.name))
        file.WriteLine("SubgraphTextureDependencies_{} = textureDeps;".format(self.name))
        file.WriteLine("SubgraphBufferDependencies_{} = bufferDeps;".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

        if self.p is not None and self.subp is not None:
            file.WriteLine("//------------------------------------------------------------------------------")
            file.WriteLine("/**")
            file.WriteLine("*/")
            file.WriteLine('void')
            file.WriteLine('RegisterSubgraphPipelines_{}(void(*func)(const CoreGraphics::PassId, const uint))'.format(self.name))
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("SubgraphPipelines_{} = func;".format(self.name))
            file.DecreaseIndent()
            file.WriteLine("}")     

    def FormatPipeline(self, file):
        file.WriteLine("")
        file.WriteLine("if (SubgraphPipelines_{} != nullptr)".format(self.name))
        file.IncreaseIndent()
        file.WriteLine("SubgraphPipelines_{}(Pass_{}, {});".format(self.name, self.p.name, self.subp.index))
        file.DecreaseIndent()
        file.WriteLine("else")
        file.IncreaseIndent()
        file.WriteLine('n_warning("No registered pipeline creation callback is registered for {}\\n");'.format(self.name))
        file.DecreaseIndent()

    def FormatHeader(self, file):
        if self.p is not None and self.subp is not None:
            file.WriteLine('void RegisterSubgraphPipelines_{}(void(*func)(const CoreGraphics::PassId, const uint));'.format(self.name))
        file.WriteLine('void RegisterSubgraph_{}(void(*func)(const CoreGraphics::CmdBufferId, const Math::rectangle<int>& viewport, const IndexT, const IndexT), Util::Array<Util::Pair<BufferIndex, CoreGraphics::PipelineStage>> bufferDeps = nullptr, Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> textureDeps = nullptr);'.format(self.name))
    
    def FormatSource(self, file):
        file.WriteLine("")
        file.WriteLine("if (Subgraph_{} != nullptr)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()

        file.WriteLine('CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "{}");'.format(self.name))
        file.IncreaseIndent()
        if self.subp == None:
            file.WriteLine('Synchronize("Subgraph_{}_Sync", cmdBuf, SubgraphTextureDependencies_{}, SubgraphBufferDependencies_{});'.format(self.name, self.name, self.name))
        file.WriteLine('Subgraph_{}(cmdBuf, viewport, frameIndex, bufferIndex);'.format(self.name))
        file.DecreaseIndent()
        file.WriteLine('CoreGraphics::CmdEndMarker(cmdBuf);')

        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatSetup(self, file):
        pass

class BatchDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")

    def FormatHeader(self, file):
        pass

    def FormatSource(self, file):
        file.WriteLine("Frame::DrawBatch(cmdBuf, MaterialTemplates::BatchGroup::{}, view->GetCamera(), bufferIndex);".format(self.name))    
    
    def FormatSetup(self, file):
        pass

class FullscreenEffectDefinition:
    def __init__(self, parser, node, p):
        self.name = node['name'].replace(" ", "")
        self.shader = node['shader']
        self.mask = node['mask']
        self.constantBlockName = node['constantBlockName']
        self.namespace = node['namespace']
        self.variables = list()
        for var in node['variables']:
            self.variables.append(dict(name = var['semantic'], value = var['value'], type = var['type']))
        self.p = p
        parser.externs.append(self)

    def FormatHeader(self, file):
        pass

    def FormatSource(self, file):
        file.WriteLine("CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, FullScreenEffect_{}_Pipeline);".format(self.name))
        file.WriteLine("CoreGraphics::CmdSetResourceTable(cmdBuf, FullScreenEffect_{}_ResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);".format(self.name))
        file.WriteLine("RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);")
        file.WriteLine("CoreGraphics::CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());")

    def FormatExtern(self, file):
        file.WriteLine("")
        file.WriteLine('#include "{}.h"'.format(self.shader))
        file.WriteLine("CoreGraphics::PipelineId FullScreenEffect_{}_Pipeline;".format(self.name))
        file.WriteLine("CoreGraphics::BufferId FullScreenEffect_{}_Constants;".format(self.name))
        file.WriteLine("CoreGraphics::ResourceTableId FullScreenEffect_{}_ResourceTable;".format(self.name))
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Initialize_FullscreenEffect_{}()".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()

        file.WriteLine('CoreGraphics::ShaderId shad = CoreGraphics::ShaderGet("shd:{}.fxb");'.format(self.shader))
        file.WriteLine('CoreGraphics::ShaderProgramId prog = CoreGraphics::ShaderGetProgram(shad, CoreGraphics::ShaderFeatureMask("{}"));'.format(self.mask))
        file.WriteLine("FullScreenEffect_{}_Pipeline = CoreGraphics::CreateGraphicsPipeline({{prog, Pass_{}, 0, CoreGraphics::InputAssemblyKey{{ CoreGraphics::PrimitiveTopology::TriangleList, false}} }});".format(self.name, self.p.name))
        file.WriteLine('FullScreenEffect_{}_ResourceTable = CoreGraphics::ShaderCreateResourceTable(shad, NEBULA_BATCH_GROUP, 1);'.format(self.name))
        file.WriteLine('{}::{} state;'.format(self.namespace, self.constantBlockName))
        for var in self.variables:
            type = var['type']
            if type == "textureHandle":
                file.WriteLine('state.{} = CoreGraphics::TextureGetBindlessHandle(Textures[(uint)TextureIndex::{}]);'.format(var['name'], var['value']))
            elif type == "texture":
                file.WriteLine('CoreGraphics::ResourceTableSetTexture(FullScreenEffect_Finalize_ResourceTable, CoreGraphics::ResourceTableTexture(Textures[(uint)TextureIndex::{}], {}::Table_Batch::{}_SLOT));'.format(var['value'], self.namespace, var['name']))
            else:
                file.WriteLine('state.{} = {};'.format(var['name'], var['value']))
        file.WriteLine('CoreGraphics::BufferCreateInfo bufInfo;')
        file.WriteLine('bufInfo.name = "FullscreenEffect_{}_Constants";'.format(self.name))
        file.WriteLine('bufInfo.byteSize = sizeof({}::{});'.format(self.namespace, self.constantBlockName))
        file.WriteLine('bufInfo.data = &state;')
        file.WriteLine('bufInfo.dataSize = bufInfo.byteSize;')
        file.WriteLine('bufInfo.mode = CoreGraphics::HostLocal;')
        file.WriteLine('bufInfo.usageFlags = CoreGraphics::ConstantBuffer;')
        file.WriteLine('bufInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;')
        file.WriteLine('FullScreenEffect_{}_Constants = CoreGraphics::CreateBuffer(bufInfo);'.format(self.name))
        file.WriteLine('CoreGraphics::ResourceTableSetConstantBuffer(FullScreenEffect_{}_ResourceTable, CoreGraphics::ResourceTableBuffer(FullScreenEffect_{}_Constants, Finalize::Table_Batch::{}_SLOT));'.format(self.name, self.name, self.constantBlockName))
        file.WriteLine('CoreGraphics::ResourceTableCommitChanges(FullScreenEffect_{}_ResourceTable);'.format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatSetup(self, file):
        file.WriteLine("Initialize_FullscreenEffect_{}();".format(self.name))

class CopyDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")
        self.sourceTex = ""
        self.sourceBits = ""
        self.destTex = ""
        self.destBits = ""
        parser.externs.append(self)

        if not "from" in node:
            Error(self.name, "Must define a 'from' source")
            source = node['from']
            if not 'tex' in source:
                Error(self.name, "Texture source must define a texture resource")
            if not 'bits' in source:
                Error(self.name, "Texture source must define image bits")

            self.sourceTex = source['tex']
            self.sourceBits = source['bits']

        if not "to" in node:
            Error(self.name, "copy must define a 'to' source")
            dest = node['to']
            if not 'tex' in dest:
                Error(self.name, "Texture destination must define a texture resource")
            if not 'bits' in dest:
                Error(self.name, "Texture source must define image bits")                

            self.destTex = dest['tex']
            self.destBits = dest['bits']                

    def FormatHeader(self, file):
        file.WriteLine("void Copy_{}(const CoreGraphics::CmdBufferId cmdBuf);".format(self.name))

    def FormatExtern(self, file):
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Copy_{}(const CoreGraphics::CmdBufferId cmdBuf)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::TextureDimensions fromDims = CoreGraphics::TextureGetDimensions(Textures[TextureIndex::{}]);".format(self.sourceTex))
        file.WriteLine("CoreGraphics::TextureDimensions toDims = CoreGraphics::TextureGetDimensions(Textures[TextureIndex::{}]);".format(self.destTex))
        file.WriteLine("CoreGraphics::TextureCopy from {{.region = {{0, 0, fromDims.width, fromDims.height}}, .mip = 0, .layer = 0, .bits = CoreGraphics::ImageBits::{}}};".format(self.sourceBits))
        file.WriteLine("CoreGraphics::TextureCopy to {{.region = {{0, 0, toDims.width, toDims.height}}, .mip = 0, .layer = 0, .bits = CoreGraphics::ImageBits::{}}};".format(self.destBits))
        file.WriteLine("CoreGraphics::CmdCopy(cmdBuf, Textures[TextureIndex::{}], {{from}}, Textures[TextureIndex::{}], {{to}});".format(self.sourceTex, self.destTex))
        file.DecreaseIndent()
        file.WriteLine("}")
    
    def FormatSource(self, file):
        file.WriteLine("Copy_{}(cmdBuf);".format(self.name))

    def FormatSetup(self, file):
        pass

class BlitDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")
        self.sourceTex = ""
        self.sourceBits = ""
        self.destTex = ""
        self.destBits = ""

        parser.externs.append(self)
        if not "from" in node:
            Error(self.name, "Must define a 'from' source")
        source = node['from']
        if not 'tex' in source:
            Error(self.name, "Texture source must define a texture resource")

        self.sourceTex = source['tex']

        if not 'bits' in source:
            self.sourceBits = "ColorBits"
        else:
            self.sourceBits = source['bits']

        if not "to" in node:
            Error(self.name, "copy must define a 'to' source")
        dest = node['to']
        if not 'tex' in dest:
            Error(self.name, "Texture destination must define a texture resource")

        self.destTex = dest['tex']

        if not 'bits' in dest:
            self.destBits = "ColorBits"
        else:
            self.destBits = dest['bits']     

        self.resourceDependencies = [
            ResourceDependencyDefinition.raw(name = self.sourceTex, stage = "TransferRead"), 
            ResourceDependencyDefinition.raw(name = self.destTex, stage = "TransferWrite")
        ]          

    def FormatHeader(self, file):
        pass

    def FormatExtern(self, file):
        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        
        if len(self.resourceDependencies) > 0:
            file.WriteLine("Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> Blit_{}_TextureDependencies;".format(self.name))

        file.WriteLine("void")
        file.WriteLine("Initialize_Blit_{}()".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        if len(self.resourceDependencies) > 0:
            file.WriteLine("Blit_{}_TextureDependencies.Clear();".format(self.name))
        for dep in self.resourceDependencies:
            file.WriteLine('Blit_{}_TextureDependencies.Append({{ TextureIndex::{}, CoreGraphics::PipelineStage::{} }});'.format(self.name, dep.name, dep.stage))
        file.DecreaseIndent()
        file.WriteLine("}")
        
        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Blit_{}(const CoreGraphics::CmdBufferId cmdBuf)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::TextureDimensions fromDims = CoreGraphics::TextureGetDimensions(Textures[(uint)TextureIndex::{}]);".format(self.sourceTex))
        file.WriteLine("CoreGraphics::TextureDimensions toDims = CoreGraphics::TextureGetDimensions(Textures[(uint)TextureIndex::{}]);".format(self.destTex))
        file.WriteLine("CoreGraphics::TextureCopy from {{.region = {{0, 0, fromDims.width, fromDims.height}}, .mip = 0, .layer = 0, .bits = CoreGraphics::ImageBits::{}}};".format(self.sourceBits))
        file.WriteLine("CoreGraphics::TextureCopy to {{.region = {{0, 0, toDims.width, toDims.height}}, .mip = 0, .layer = 0, .bits = CoreGraphics::ImageBits::{}}};".format(self.destBits))
        file.WriteLine('N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_TRANSFER, "{}");'.format(self.name))
        file.WriteLine("CoreGraphics::CmdBlit(cmdBuf, Textures[(uint)TextureIndex::{}], from, Textures[(uint)TextureIndex::{}], to);".format(self.sourceTex, self.destTex))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatSource(self, file):
        file.WriteLine('Synchronize("Blit_{}_Sync", cmdBuf, Blit_{}_TextureDependencies, nullptr);'.format(self.name, self.name))
        file.WriteLine("Blit_{}(cmdBuf);".format(self.name))

    def FormatSetup(self, file):
        file.WriteLine("Initialize_Blit_{}();".format(self.name))

class ResolveDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")
        self.sourceTex = ""
        self.sourceBits = ""
        self.destTex = ""
        self.destBits = ""
        parser.externs.append(self)
        if not "from" in node:
            Error(self.name, "Must define a 'from' source")
        source = node['from']
        if not 'tex' in source:
            Error(self.name, "Texture source must define a texture resource")

        self.sourceTex = source['tex']

        if not 'bits' in source:
            self.sourceBits = "ColorBits"
        else:
            self.sourceBits = source['bits']

        if not "to" in node:
            Error(self.name, "copy must define a 'to' source")
        dest = node['to']
        if not 'tex' in dest:
            Error(self.name, "Texture destination must define a texture resource")

        self.destTex = dest['tex']

        if not 'bits' in dest:
            self.destBits = "ColorBits"
        else:
            self.destBits = dest['bits']                

    def FormatHeader(self, file):
        pass

    def FormatExtern(self, file):

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Resolve_{}(const CoreGraphics::CmdBufferId cmdBuf)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::TextureDimensions fromDims = CoreGraphics::TextureGetDimensions(Textures[(uint)TextureIndex::{}]);".format(self.sourceTex))
        file.WriteLine("CoreGraphics::TextureDimensions toDims = CoreGraphics::TextureGetDimensions(Textures[(uint)TextureIndex::{}]);".format(self.destTex))
        file.WriteLine("CoreGraphics::TextureCopy from {{.region = {{0, 0, fromDims.width, fromDims.height}}, .mip = 0, .layer = 0, .bits = CoreGraphics::ImageBits::{}}};".format(self.sourceBits))
        file.WriteLine("CoreGraphics::TextureCopy to {{.region = {{0, 0, toDims.width, toDims.height}}, .mip = 0, .layer = 0, .bits = CoreGraphics::ImageBits::{}}};".format(self.destBits))
        file.WriteLine("CoreGraphics::CmdResolve(cmdBuf, Textures[(uint)TextureIndex::{}], from, Textures[(uint)TextureIndex::{}], to);".format(self.sourceTex, self.sourceBits, self.destTex, self.destBits))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatSource(self, file):
        file.WriteLine("Blit_{}(cmdBuf);".format(self.name))

class SwapDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")
        self.source = node['from']
        parser.externs.append(self)

        self.resourceDependencies = [
            ResourceDependencyDefinition.raw(name = self.source, stage = "TransferRead"), 
        ]         
    
    def FormatHeader(self, file):
        pass 

    def FormatExtern(self, file):
        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        
        if len(self.resourceDependencies) > 0:
            file.WriteLine("Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> Swap_{}_TextureDependencies;".format(self.name))

        file.WriteLine("void")
        file.WriteLine("Initialize_Swap_{}()".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        if len(self.resourceDependencies) > 0:
            file.WriteLine("Swap_{}_TextureDependencies.Clear();".format(self.name))
        for dep in self.resourceDependencies:
            file.WriteLine('Swap_{}_TextureDependencies.Append({{ TextureIndex::{}, CoreGraphics::PipelineStage::{} }});'.format(self.name, dep.name, dep.stage))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatSource(self, file):
        file.WriteLine('CoreGraphics::QueueBeginMarker(CoreGraphics::GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Swap");')
        file.WriteLine('CoreGraphics::SwapchainId swapchain = WindowGetSwapchain(CoreGraphics::CurrentWindow);')
        file.WriteLine('CoreGraphics::SwapchainSwap(swapchain);')
        file.WriteLine('Synchronize("Swap_{}_Sync", cmdBuf, Swap_{}_TextureDependencies, nullptr);'.format(self.name, self.name))
        file.WriteLine('CoreGraphics::SwapchainCopy(swapchain, cmdBuf, Textures[(uint)TextureIndex::{}]);'.format(self.source))
        file.WriteLine('CoreGraphics::QueueEndMarker(CoreGraphics::GraphicsQueueType);')

    def FormatSetup(self, file):
        file.WriteLine("Initialize_Swap_{}();".format(self.name))
    
class TransitionDefinition:
    def __init__(self, parser, node):
        self.name = node['name']
        self.stage = node['stage']

    def FormatHeader(self, file):
        pass

    def FormatSetup(self, file):
        pass

    def FormatSource(self, file):
        file.WriteLine('Synchronize("Transition_{}", cmdBuf, {{ {{ TextureIndex::{}, CoreGraphics::PipelineStage::{} }} }}, nullptr);'.format(self.name, self.name, self.stage))


class AttachmentDefinition:
    def __init__(self, parser, node):
        self.name = node['name']
        self.ref = None
        self.clearColor = None
        self.clearDepth = None
        self.clearStencil = None
        self.storeLoadFlags = list()
        if self.name not in parser.localTextureDict:
            Error(self.name, "Attachment references undeclared texture")
        else:
            self.ref = parser.localTextureDict[self.name]

        if "clear" in node:
            self.clearColor = node['clear']
            if type(self.clearColor) is not list:
                Error(self.name, "clear is not given as an array")
            elif len(self.clearColor) != 4:
                Error(self.name, "clear must be an array of 4 values")
        if "clear_depth" in node:
            self.clearDepth = node['clear_depth']
            if self.clearColor != None:
                Error(self.name, "clear_depth is exclusive of clear")
        if "clear_stencil" in node:
            self.clearStencil = node['clear_stencil']
            if self.clearColor != None:
                Error(self.name, "clear_stencil is exclusive of clear")
        if "flags" in node:
            storeLoadFlags = node['flags']
            for flag in storeLoadFlags.split("|"):
                self.storeLoadFlags.append(flag)
                if flag == "Load" and (self.clearColor is not None  or self.clearDepth is not None):
                    Error(self.name, "Attachment can't both clear and load")
                if flag == "LoadStencil" and self.clearStencil is not None:
                    Error(self.name, "Attachment can't both clear and load")
        
class SubpassDefinition:
    def __init__(self, parser, node, parent, idx):
        self.name = node['name'].replace(" ", "")
        self.dependencies = list()
        self.index = idx
        if 'resource_dependencies' in node:
            for dependency in node['resource_dependencies']:
                dep = ResourceDependencyDefinition(parser = parser, node = dependency)
                parent.resourceDependencies.append(dep)
        if 'subpass_dependencies' in node:
            for dependency in node['subpass_dependencies']:
                if dependency not in parent.subpassDict:
                    Error(self.name, "Depends on {} which is not defined".format(dependency))
                subpass_dep = parent.subpassDict[dependency]
                for idx, subpass in enumerate(parent.subpasses):
                    if subpass.name == subpass_dep.name:
                        self.dependencies.append(dict(index = idx, name = subpass_dep.name))

        self.attachments = list()
        if 'attachments' in node:
            for attachment in node['attachments']:
                if attachment not in parent.attachmentDict:
                    Error(self.name, "References attachment {} which is not defined in Pass {}".format(self.name, parent.name))
                subpass_attach = parent.attachmentDict[attachment]
                for idx, attachment in enumerate(parent.attachments):
                    if attachment.name == subpass_attach.name:
                        self.attachments.append(dict(index = idx, name = subpass_attach.name, ref = subpass_attach))

        self.inputs = list()
        if 'inputs' in node:
            for inp in node['inputs']:
                if inp not in parent.attachmentDict:
                    Error(self.name, "References input attachment {} which is not defined in Pass {}".format(self.name, parent.name))
                subpass_input = parent.attachmentDict[inp]
                for idx, attachment in enumerate(parent.attachments):
                    if attachment.name == subpass_input.name:
                        self.inputs.append(dict(index = idx, name = subpass_input.name))

        self.resolves = list()
        if 'resolves' in node:
            for res in node['resolves']:
                if res not in parent.attachmentDict:
                    Error(self.name, "References resolve attachment {} which is not defined in Pass {}".format(self.name, parent.name))
                subpass_resolve = parent.attachmentDict[res]
                for idx, attachment in enumerate(parent.attachments):
                    if attachment.name == subpass_resolve.name:
                        self.inputs.append(dict(index = idx, name = subpass_resolve.name))

        self.depth = None
        if 'depth' in node:
            depth = node['depth']
            if depth not in parent.attachmentDict:
                Error(self.name, "References depth attachment {} which is not defined in Pass {}".format(self.name, parent.name))
            subpass_depth = parent.attachmentDict[depth]
            for idx, attachment in enumerate(parent.attachments):
                if attachment.name == subpass_depth.name:
                    self.depth = dict(index = idx, name = subpass_depth.name, ref = subpass_depth)

        self.ops = list()
        for op in node["ops"]:
            if 'subgraph' in op:
                self.ops.append(SubgraphDefinition(parser, op['subgraph'], parent, self))
            elif 'batch' in op:
                self.ops.append(BatchDefinition(parser, op['batch']))
            elif 'fullscreen_effect' in op:
                self.ops.append(FullscreenEffectDefinition(parser, op['fullscreen_effect'], parent))


    def FormatHeader(self, file):
        numAttachments =  len(self.attachments) + (1 if self.depth != None else 0)
        file.WriteLine('static Util::FixedArray<Math::rectangle<int>> Subpass_{}_Viewports({});'.format(self.name, numAttachments))
        for op in self.ops:
            op.FormatHeader(file)

    def FormatSource(self, file):
        file.IncreaseIndent()
        file.WriteLine("// Subpass {}".format(self.name))
        for op in self.ops:
            op.FormatSource(file)
        file.DecreaseIndent()

    def FormatSetup(self, file):
        for op in self.ops:
            op.FormatSetup(file)

class PassDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")
        self.attachments = list()
        self.attachmentDict = {}
        self.resourceDependencies = list()
        parser.externs.append(self)
        parser.passes.append(self)
        for at in node["attachments"]:
            attachment = AttachmentDefinition(parser, at)
            self.attachments.append(attachment)
            self.attachmentDict[attachment.name] = attachment
            self.resourceDependencies.append(ResourceDependencyDefinition.raw(name = attachment.name, stage = "DepthStencilWrite" if attachment.ref.HasDepthFormat() or attachment.ref.HasStencilFormat() else "ColorWrite"))
        
        self.subpasses = list()
        self.subpassDict = {}
        for sub in node["subpasses"]:
            subpass = SubpassDefinition(parser, sub, self, len(self.subpasses))
            self.subpasses.append(subpass)
            self.subpassDict[subpass.name] = subpass

    def FormatExtern(self, file):

        file.WriteLine("")
        file.WriteLine('CoreGraphics::PassId Pass_{} = CoreGraphics::InvalidPassId;'.format(self.name))

        
        if len(self.resourceDependencies) > 0:
            file.WriteLine("Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> Pass_{}_TextureDependencies;".format(self.name))

        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Initialize_Pass_{}()".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("if (Pass_{} != CoreGraphics::InvalidPassId)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::DestroyPass(Pass_{});".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

        file.WriteLine("Pass_{}_TextureDependencies.Clear();".format(self.name))
        for dependency in self.resourceDependencies:
            file.WriteLine("Pass_{}_TextureDependencies.Append({{TextureIndex::{}, CoreGraphics::PipelineStage::{}}});".format(self.name, dependency.name, dependency.stage))
        file.WriteLine("CoreGraphics::PassCreateInfo info;")
        file.WriteLine('info.name = "Pass_{}";'.format(self.name))
        file.WriteLine("info.attachments.Resize({});".format(len(self.attachments)))
        file.WriteLine("info.attachmentFlags.Resize({});".format(len(self.attachments)))
        file.WriteLine("info.attachmentClears.Resize({});".format(len(self.attachments)))
        file.WriteLine("info.attachmentDepthStencil.Resize({});".format(len(self.attachments)))

        file.WriteLine("info.subpasses.Resize({});".format(len(self.subpasses)))
        for idx, subpass in enumerate(self.subpasses):
            file.WriteLine("static const int Subpass_{} = {};".format(subpass.name, idx))

        for attachment in self.attachments:
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::TextureViewCreateInfo viewInfo;")
            file.WriteLine('viewInfo.name = "[Attachment] {} in {}";'.format(attachment.name, self.name))
            file.WriteLine("viewInfo.tex = Textures[(uint)TextureIndex::{}];".format(attachment.name))
            file.WriteLine("viewInfo.format = CoreGraphics::PixelFormat::Code::{};".format(attachment.ref.format))
            file.WriteLine("viewInfo.startMip = 0;")
            file.WriteLine("viewInfo.numMips = 1;")
            file.WriteLine("viewInfo.startLayer = 0;")
            file.WriteLine("viewInfo.numLayers = {};".format(attachment.ref.layers))
            bits = ""
            for bit in attachment.ref.bits:
                bits += "CoreGraphics::ImageBits::{}".format(bit)
                if bit != attachment.ref.bits[-1]:
                    bits += " | "
            file.WriteLine("viewInfo.bits = {};".format(bits))
            file.WriteLine("info.attachments[Pass_{}_Attachment_{}] = CoreGraphics::CreateTextureView(viewInfo);".format(self.name, attachment.name))

            file.WriteLine("Math::vec4 clearValue = Math::vec4(0.0f);")
            flags = "CoreGraphics::AttachmentFlagBits::NoFlags"
            if attachment.clearColor is not None or attachment.clearDepth is not None:
                flags += " | CoreGraphics::AttachmentFlagBits::Clear"
            if attachment.clearStencil is not None:
                flags += " | CoreGraphics::AttachmentFlagBits::ClearStencil"

            for flag in attachment.storeLoadFlags:
                flags += " | CoreGraphics::AttachmentFlagBits::{}".format(flag) 
            file.WriteLine("CoreGraphics::AttachmentFlagBits flags = {};".format(flags))

            if attachment.clearColor is not None:
                file.WriteLine("clearValue = Math::vec4({}, {}, {}, {});".format(attachment.clearColor[0], attachment.clearColor[1], attachment.clearColor[2], attachment.clearColor[3]))
            if attachment.clearDepth is not None:
                file.WriteLine("clearValue.x = {};".format(attachment.clearDepth))
            if attachment.clearStencil is not None:
                file.WriteLine("clearValue.y = {};".format(attachment.clearStencil))

            file.WriteLine("info.attachmentFlags[Pass_{}_Attachment_{}] = flags;".format(self.name, attachment.name))
            file.WriteLine("info.attachmentClears[Pass_{}_Attachment_{}] = clearValue;".format(self.name, attachment.name))
            file.WriteLine("info.attachmentDepthStencil[Pass_{}_Attachment_{}] = CoreGraphics::PixelFormat::IsDepthStencilFormat(CoreGraphics::PixelFormat::Code::{});".format(self.name, attachment.name, attachment.ref.format))

            file.DecreaseIndent()
            file.WriteLine("}")

        for subpass in self.subpasses:
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::Subpass subpass; // {}".format(subpass.name))
            numTargets = len(subpass.attachments)
            for dep in subpass.dependencies:
                file.WriteLine("subpass.dependencies.Append(Subpass_{});".format(dep['name']))
            for inp in subpass.inputs:
                file.WriteLine("subpass.inputs.Append(Pass_{}_Attachment_{});".format(self.name, inp['name']))
            for attachment in subpass.attachments:
                file.WriteLine("subpass.attachments.Append(Pass_{}_Attachment_{});".format(self.name, attachment['name']))
            for resolve in subpass.resolves:
                file.WriteLine("subpass.resolves.Append(Pass_{}_Attachment_{});".format(self.name, resolve['name']))
            if subpass.depth is not None:
                file.WriteLine("subpass.depth = Pass_{}_Attachment_{};".format(self.name, subpass.depth['name']))
                numTargets += 1
            file.WriteLine("subpass.numViewports = {};".format(numTargets))
            file.WriteLine("subpass.numScissors = {};".format(numTargets))
            file.WriteLine("info.subpasses[Subpass_{}] = subpass;".format(subpass.name))
            file.DecreaseIndent()
            file.WriteLine("}")
        file.WriteLine("Pass_{} = CoreGraphics::CreatePass(info);".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")

    def FormatHeader(self, file):
        file.WriteLine('static Util::FixedArray<Shared::RenderTargetParameters> Pass_{}_RenderTargetDimensions({});'.format(self.name, len(self.attachments)))

        for idx, attachment in enumerate(self.attachments):
            file.WriteLine("static const int Pass_{}_Attachment_{} = {};".format(self.name, attachment.name, idx))
        for subpass in self.subpasses:
            subpass.FormatHeader(file)

    def FormatSource(self, file):
        file.WriteLine("")
        if len(self.resourceDependencies) > 0:
            file.WriteLine('Synchronize("Pass_{}_Sync", cmdBuf, Pass_{}_TextureDependencies, nullptr);'.format(self.name, self.name))
        file.WriteLine('CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_GREEN, "{}");'.format(self.name))
        file.IncreaseIndent()

        for subpass in self.subpasses:
            for op in subpass.ops:
                if type(op) == SubgraphDefinition:
                    file.WriteLine('Synchronize("Subgraph_{}_Sync", cmdBuf, SubgraphTextureDependencies_{}, SubgraphBufferDependencies_{});'.format(op.name, op.name, op.name))

        for attachment in self.attachments: 
            file.WriteLine('Pass_{}_RenderTargetDimensions[Pass_{}_Attachment_{}] = Shared::RenderTargetParameters{{ {{ viewport.width() * {}f, viewport.height() * {}f, 1 / float(viewport.width()) * {}f, 1 / float(viewport.height()) * {}f }}, {{ viewport.width() / TextureRelativeSize[(uint)TextureIndex::{}].first, viewport.height() / TextureRelativeSize[(uint)TextureIndex::{}].second }} }};'.format(self.name, self.name, attachment.name, attachment.ref.relativeSize[0], attachment.ref.relativeSize[1], attachment.ref.relativeSize[0], attachment.ref.relativeSize[1], attachment.ref.name, attachment.ref.name))
        file.WriteLine('CoreGraphics::PassSetRenderTargetParameters(Pass_{}, Pass_{}_RenderTargetDimensions);'.format(self.name, self.name))
        file.WriteLine('CoreGraphics::CmdBeginPass(cmdBuf, Pass_{});'.format(self.name))
        
        for subpass in self.subpasses:
            if subpass.depth:
                file.WriteLine('Subpass_{}_Viewports[Pass_{}_Attachment_{}] = Math::rectangle<int>(0, 0, viewport.width() * {}, viewport.height() * {});'.format(subpass.name, self.name, subpass.depth['name'], subpass.depth['ref'].ref.relativeSize[0], subpass.depth['ref'].ref.relativeSize[1]))
            for attachment in subpass.attachments:
                file.WriteLine('Subpass_{}_Viewports[Pass_{}_Attachment_{}] = Math::rectangle<int>(0, 0, viewport.width() * {}, viewport.height() * {});'.format(subpass.name, self.name, attachment['name'], attachment['ref'].ref.relativeSize[0], attachment['ref'].ref.relativeSize[1]))
            file.WriteLine('CoreGraphics::CmdSetViewports(cmdBuf, Subpass_{}_Viewports);'.format(subpass.name))
            file.WriteLine('CoreGraphics::CmdSetScissors(cmdBuf, Subpass_{}_Viewports);'.format(subpass.name))
            if subpass != self.subpasses[0]:
                file.WriteLine("")
                file.WriteLine('CoreGraphics::CmdNextSubpass(cmdBuf);')
            subpass.FormatSource(file)
        file.WriteLine('CoreGraphics::CmdEndPass(cmdBuf);')
        file.DecreaseIndent()
    
        for attachment in self.attachments:
            file.WriteLine("TextureCurrentStage[(uint)TextureIndex::{}] = CoreGraphics::PipelineStage::{};".format(attachment.name, "DepthStencilWrite" if attachment.ref.HasDepthFormat() or attachment.ref.HasStencilFormat() else "ColorWrite"))
        file.WriteLine('CoreGraphics::CmdEndMarker(cmdBuf);')

    def FormatSetup(self, file):
        file.WriteLine("Initialize_Pass_{}();".format(self.name))
        for sub in self.subpasses:
            sub.FormatSetup(file)

       

class SubmissionDefinition:
    def __init__(self, parser, node):
        self.name = node['name'].replace(" ", "")
        self.queue = node['queue']
        self.lastSubmit = False
        if 'last_submit' in node:
            self.lastSubmit = node['last_submit']
        self.waitForQueue = None
        self.waitForSubmissions = list()

        if 'wait_for_queue' in node:
            self.waitForQueue = node['wait_for_queue']
        if 'wait_for_submissions' in node:
            for wait in node['wait_for_submissions']:
                self.waitForSubmissions.append(wait.replace(" ", ""))
        self.ops = list()
        parser.externs.append(self)

        global queues
        if self.queue not in queues:
            Error(self.name, '"queue" must be either Graphics, Compute, Transfer or Sparse')

        for op in node['ops']:
            if 'subgraph' in op:
                self.ops.append(SubgraphDefinition(parser, op['subgraph'], None, None))
            elif 'pass' in op:
                self.ops.append(PassDefinition(parser, op['pass']))
            elif 'copy' in op:
                self.ops.append(CopyDefinition(parser, op['copy']))
            elif 'blit' in op:
                self.ops.append(BlitDefinition(parser, op['blit']))
            elif 'resolve' in op:
                self.ops.append(ResolveDefinition(parser, op['resolve']))
            elif 'swap' in op:
                self.ops.append(SwapDefinition(parser, op['swap']))
            elif 'transition' in op:
                self.ops.append(TransitionDefinition(parser, op['transition']))

    def FormatExtern(self, file):
        file.WriteLine("CoreGraphics::CmdBufferPoolId CmdPool_{} = CoreGraphics::InvalidCmdBufferPoolId;\n".format(self.name))

    def FormatHeader(self, file):
        for op in self.ops:
            op.FormatHeader(file)

    def FormatSetup(self, file):
        file.WriteLine("if (CmdPool_{} != CoreGraphics::InvalidCmdBufferPoolId)".format(self.name))
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::DestroyCmdBufferPool(CmdPool_{});".format(self.name))
        file.DecreaseIndent()
        file.WriteLine("}")
        file.WriteLine("CmdPool_{} = CoreGraphics::CreateCmdBufferPool({{ .queue = CoreGraphics::QueueType::{}QueueType, .resetable = false, .shortlived = true }});".format(self.name, self.queue))
        for op in self.ops:
            op.FormatSetup(file)

    def FormatSource(self, file, imports):
        file.WriteLine("{{   // Submission {}".format(self.name))
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::CmdBufferCreateInfo cmdBufInfo;")
        file.WriteLine("cmdBufInfo.pool = CmdPool_{};".format(self.name))
        file.WriteLine("cmdBufInfo.usage = CoreGraphics::QueueType::{}QueueType;".format(self.queue))
        file.WriteLine("cmdBufInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::Timestamps;")
        file.WriteLine("#if NEBULA_GRAPHICS_DEBUG")
        file.WriteLine('cmdBufInfo.name = "{}";'.format(self.name))
        file.WriteLine("#endif // NEBULA_GRAPHICS_DEBUG")
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::CreateCmdBuffer(cmdBufInfo);")
        file.WriteLine("CoreGraphics::CmdBufferBeginInfo beginInfo = { true, false, false };")
        file.WriteLine("CoreGraphics::CmdBeginRecord(cmdBuf, beginInfo);")
        deps = ""
        if len(self.waitForSubmissions) > 0:
            deps = "- waits for: "
            for dep in self.waitForSubmissions:
                deps += "{}".format(dep)
                if dep != self.waitForSubmissions[-1]:
                    deps += ", "
        if self.queue == "Graphics":
            file.WriteLine('CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_PURPLE, "{} {}");'.format(self.name, deps))
        else:
            file.WriteLine('CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TURQOISE, "{} {}");'.format(self.name, deps))
        file.IncreaseIndent()
        file.WriteLine("Graphics::FlushUpdates(cmdBuf, CoreGraphics::QueueType::{}QueueType);".format(self.queue))
        file.WriteLine("Materials::MaterialLoader::FlushMaterialBuffers(cmdBuf, CoreGraphics::QueueType::{}QueueType);".format(self.queue))

        for op in self.ops:
            op.FormatSource(file)

        if len(imports) > 0:
            file.WriteLine("static Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>> EndOfFrameSyncs;")
            file.WriteLine("EndOfFrameSyncs.Clear();")
            for imp in imports:
                file.WriteLine("EndOfFrameSyncs.Append({{ TextureIndex::{}, TextureOriginalStage[(uint)TextureIndex::{}] }});".format(imp.name, imp.name))
            file.WriteLine('Synchronize("End Of Frame Sync", cmdBuf, EndOfFrameSyncs, nullptr);')

        file.DecreaseIndent()
        file.WriteLine("CoreGraphics::CmdEndMarker(cmdBuf);")
        file.WriteLine("CoreGraphics::CmdFinishQueries(cmdBuf);")
        file.WriteLine("CoreGraphics::CmdEndRecord(cmdBuf);")
        file.WriteLine("Submission_{} = CoreGraphics::SubmitCommandBuffer(cmdBuf, CoreGraphics::QueueType::{}QueueType".format(self.name, self.queue))
        file.WriteLine("#if NEBULA_GRAPHICS_DEBUG")
        file.WriteLine(', "{}"'.format(self.name))
        file.WriteLine("#endif")
        file.WriteLine(");")
        if self.waitForQueue is not None:
            file.WriteLine("CoreGraphics::WaitForLastSubmission(CoreGraphics::QueueType::{}QueueType, CoreGraphics::QueueType::{}QueueType);".format(self.queue, self.waitForQueue))
        for wait in self.waitForSubmissions:
            file.WriteLine("if (Submission_{} != nullptr)".format(wait))
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::WaitForSubmission(Submission_{}, CoreGraphics::QueueType::{}QueueType);".format(wait, self.queue))
            file.DecreaseIndent()

        file.WriteLine("CoreGraphics::DestroyCmdBuffer(cmdBuf);")

        file.DecreaseIndent()
        file.WriteLine("}")

        file.DecreaseIndent()
        file.WriteLine("}")

class FrameScriptGenerator:
    def __init__(self):
        self.document = None
        self.depthFormats = set(["D32S8", "D32", "D24S8", "D16"])
        self.stencilFormats = set(["D32S8", "D24S8"])


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

        self.textures = list()
        self.buffers = list()
        self.importBuffers = list()
        self.importTextures = list()
        self.localTextures = list()
        self.localTextureDict = {}
        self.exportTextures = list()
        self.dependencies = list()
        self.submissions = list()
        self.externs = list()
        self.passes = list()
        self.pipelines = list()
        if "Nebula" in self.document:
            main = self.document["Nebula"]
            for name, node in main.items():
                if name == "ImportTextures":
                    for imp in node:
                        tex = TextureImportDefinition(self, imp)
                        self.importTextures.append(tex)
                elif name == "ImportBuffers":
                    for imp in node:
                        buf = BufferImportDefinition(self, imp)
                        self.importBuffers.append(buf)
                elif name == "Dependencies":
                    for imp in node:
                        dep = DependencyDefinition(self, imp)
                        self.dependencies.append(dep)
                elif name == "LocalTextures":
                    for imp in node:
                        tex = LocalTextureDefinition(self, imp)
                        self.localTextures.append(tex)
                        self.localTextureDict[tex.name] = tex
                elif name == "ExportTextures":
                    for exp in node:
                        tex = TextureExportDefinition(self, exp)
                        self.exportTextures.append(tex)
                elif name == "Submissions":
                    for imp in node:
                        sub = SubmissionDefinition(self, imp)
                        self.submissions.append(sub)

    #------------------------------------------------------------------------------
    ##
    #
    def FormatHeader(self, file):

        file.WriteLine("// Frame Script #version:{}#".format(self.version))
        file.WriteLine("#pragma once")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.IncreaseIndent()
        file.WriteLine("This file was generated with Nebula's Frame Script compiler tool.")
        file.WriteLine("DO NOT EDIT")
        file.DecreaseIndent()
        file.WriteLine("*/")
        file.WriteLine("")
        file.WriteLine('#include "framescripts/framescripts.h"')
        file.WriteLine('#include "coregraphics/texture.h"')
        file.WriteLine('#include "coregraphics/buffer.h"')
        file.WriteLine('#include "coregraphics/graphicsdevice.h"')
        file.WriteLine('#include "coregraphics/pipeline.h"')
        file.WriteLine('#include "system_shaders/shared.h"')

        file.WriteLine("namespace FrameScript_{}".format(self.name))
        file.WriteLine("{")
        file.WriteLine("")

        file.WriteLine("enum class TextureIndex")
        file.WriteLine("{")
        file.IncreaseIndent()
        for idx, texture in enumerate(self.localTextures + self.importTextures):
            file.WriteLine("{} = {},".format(texture.name, idx))
        file.WriteLine("Num")
        file.DecreaseIndent()
        file.WriteLine("};")

        file.WriteLine("")
        file.WriteLine("enum class BufferIndex")
        file.WriteLine("{")
        file.IncreaseIndent()
        for idx, buffer in enumerate(self.importBuffers):
            file.WriteLine("{} = {},".format(buffer.name, idx))
        file.WriteLine("Num")
        file.DecreaseIndent()
        file.WriteLine("};")        
        file.WriteLine("void Synchronize(const char* name, const CoreGraphics::CmdBufferId buf, const Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>>& textureDeps, const Util::Array<Util::Pair<BufferIndex, CoreGraphics::PipelineStage>>& bufferDeps);")

    
        for importBuffer in self.importBuffers:
            importBuffer.FormatHeader(file)

        for importTexture in self.importTextures:
            importTexture.FormatHeader(file)

        for dependency in self.dependencies:
            dependency.FormatHeader(file)

        for texture in self.localTextures:
            texture.FormatHeader(file)

        file.WriteLine("")
        file.WriteLine("/// Setup FrameScript_{}".format(self.name))
        file.WriteLine("void Initialize(const uint frameWidth, const uint frameHeight);")
        file.WriteLine("void SetupPipelines();")


        file.WriteLine("")
        for submission in self.submissions:
            submission.FormatHeader(file)

            
        file.WriteLine("")
        if len(self.localTextures + self.importTextures) > 0:
            file.WriteLine("extern CoreGraphics::TextureId Textures[(uint)TextureIndex::Num];")
            file.WriteLine("extern Util::Pair<float,float> TextureRelativeScale[(uint)TextureIndex::Num];")
        if len(self.importBuffers) > 0:
            file.WriteLine("extern CoreGraphics::BufferId Buffers[(uint)BufferIndex::Num];")

        for texture in (self.localTextures + self.importTextures):
            file.WriteLine("inline CoreGraphics::TextureId Texture_{}() {{ return Textures[(uint)TextureIndex::{}]; }}".format(texture.name, texture.name))
            file.WriteLine("inline Util::Pair<float, float> TextureRelativeScale_{}() {{ return TextureRelativeScale[(uint)TextureIndex::{}]; }}".format(texture.name, texture.name))
        for buffer in (self.importBuffers):
            file.WriteLine("inline CoreGraphics::BufferId Buffer_{}() {{ return Buffers[(uint)BufferIndex::{}]; }}".format(buffer.name, buffer.name))
        for export in self.exportTextures:
            export.FormatHeader(file)
        for submission in self.submissions:
            file.WriteLine("extern CoreGraphics::SubmissionWaitEvent Submission_{};".format(submission.name))

        file.WriteLine("")
        file.WriteLine("/// Execute FrameScript_{}".format(self.name))
        file.WriteLine("void Run(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex);")

        file.WriteLine("}} // namespace FrameScript_{}".format(self.name))

    #------------------------------------------------------------------------------
    ##
    #
    def FormatSource(self, file):
        file.WriteLine("// Frame Script #version:{}#".format(self.version))
        file.WriteLine("#pragma once")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.IncreaseIndent()
        file.WriteLine("This file was generated with Nebula's Frame Script compiler tool.")
        file.WriteLine("DO NOT EDIT")
        file.DecreaseIndent()
        file.WriteLine("*/")
        file.WriteLine("")
        file.WriteLine('#include "{}.h"'.format(self.name))
        file.WriteLine('#include "materials/materialtemplates.h"')
        file.WriteLine('#include "materials/materialloader.h"')
        file.WriteLine('#include "graphics/globalconstants.h"')
        file.WriteLine('#include "coregraphics/graphicsdevice.h"')
        file.WriteLine('#include "frame/framesubpassbatch.h"')
        file.WriteLine('#include "graphics/view.h"')
        file.WriteLine('#include "coregraphics/swapchain.h"')
        file.WriteLine('#include "coregraphics/barrier.h"')
        file.WriteLine('#include "renderutil/drawfullscreenquad.h"')

        file.WriteLine("namespace FrameScript_{}".format(self.name))
        file.WriteLine("{")

        for submission in self.submissions:
            file.WriteLine("CoreGraphics::SubmissionWaitEvent Submission_{};".format(submission.name))

        file.WriteLine("")
        if len(self.localTextures + self.importTextures) > 0:
            file.WriteLine("CoreGraphics::ImageBits TextureImageBits[(uint)TextureIndex::Num] = {};")
            file.WriteLine("CoreGraphics::PipelineStage TextureCurrentStage[(uint)TextureIndex::Num] = {};")
            file.WriteLine("CoreGraphics::PipelineStage TextureOriginalStage[(uint)TextureIndex::Num] = {};")
            file.WriteLine("CoreGraphics::TextureId Textures[(uint)TextureIndex::Num] = {};")
            file.WriteLine("Util::Pair<float, float> TextureRelativeScale[(uint)TextureIndex::Num] = {};")
            file.WriteLine("Util::Pair<float, float> TextureRelativeSize[(uint)TextureIndex::Num] = {};")
        if len(self.importBuffers) > 0:
            file.WriteLine("CoreGraphics::PipelineStage BufferCurrentStage[(uint)BufferIndex::Num] = {};")
            file.WriteLine("CoreGraphics::BufferId Buffers[(uint)BufferIndex::Num] = {};")

        file.WriteLine("")
        for extern in self.externs:
            extern.FormatExtern(file)

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("inline void")
        file.WriteLine("Synchronize(const char* name, const CoreGraphics::CmdBufferId buf, const Util::Array<Util::Pair<TextureIndex, CoreGraphics::PipelineStage>>& textureDeps, const Util::Array<Util::Pair<BufferIndex, CoreGraphics::PipelineStage>>& bufferDeps)")
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("CoreGraphics::BarrierScope scope = CoreGraphics::BarrierScope::Begin(name, buf);")

        if len(self.importTextures + self.localTextures) > 0:
            file.WriteLine("for (const auto [index, stage] : textureDeps)")
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::PipelineStage lastStage = TextureCurrentStage[(uint)index];")
            file.WriteLine("if ((stage != lastStage) && Textures[(uint)index] != CoreGraphics::InvalidTextureId)")
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("scope.AddTexture(TextureImageBits[(uint)index], Textures[(uint)index], lastStage, stage);")
            file.DecreaseIndent()
            file.WriteLine("}")
            file.WriteLine("TextureCurrentStage[(uint)index] = stage;")
    
            file.DecreaseIndent()
            file.WriteLine("}")

        if len(self.importBuffers) > 0:
            file.WriteLine("for (const auto [index, stage] : bufferDeps)")
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("CoreGraphics::PipelineStage lastStage = BufferCurrentStage[(uint)index];")
            file.WriteLine("if ((stage != lastStage) && Buffers[(uint)index] != CoreGraphics::InvalidBufferId)")
            file.WriteLine("{")
            file.IncreaseIndent()
            file.WriteLine("scope.AddBuffer(Buffers[(uint)index], lastStage, stage);")
            file.DecreaseIndent()
            file.WriteLine("}")
            file.WriteLine("BufferCurrentStage[(uint)index] = stage;")

            file.DecreaseIndent()
            file.WriteLine("}")

        file.WriteLine("scope.Flush();")
        file.DecreaseIndent()
        file.WriteLine("}")

        for importBuffer in self.importBuffers:
            importBuffer.FormatSource(file)

        for importTexture in self.importTextures:
            importTexture.FormatSource(file)

        for dependency in self.dependencies:
            dependency.FormatSource(file)

        for localTexture in self.localTextures:
            localTexture.FormatSource(file)

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("InitializeTextures(const uint windowWidth, const uint windowHeight)")
        file.WriteLine("{")
        file.IncreaseIndent()
        for localTexture in self.localTextures:
            if localTexture.relativeSize:
                file.WriteLine("Initialize_Texture_{}(windowWidth, windowHeight);".format(localTexture.name))
            else:
                file.WriteLine("Initialize_Texture_{}();".format(localTexture.name))
        file.DecreaseIndent()
        file.WriteLine("}")

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("InitializeSubmissions()")
        file.WriteLine("{")
        file.IncreaseIndent()
        for p in self.submissions:
            p.FormatSetup(file)
        file.DecreaseIndent()
        file.WriteLine("}")

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("InitializePipelines()")
        file.WriteLine("{")
        file.IncreaseIndent()
        for pipeline in self.pipelines:
            pipeline.FormatPipeline(file)
        file.DecreaseIndent()
        file.WriteLine("}")


        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Initialize(const uint frameWidth, const uint frameHeight)")
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("InitializeTextures(frameWidth, frameHeight);")
        file.WriteLine("InitializeSubmissions();")
        file.DecreaseIndent()
        file.WriteLine("}")

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("SetupPipelines()")
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("InitializePipelines();")
        file.DecreaseIndent()
        file.WriteLine("}")

        file.WriteLine("")
        file.WriteLine("//------------------------------------------------------------------------------")
        file.WriteLine("/**")
        file.WriteLine("*/")
        file.WriteLine("void")
        file.WriteLine("Run(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex)")
        file.WriteLine("{")
        file.IncreaseIndent()
        file.WriteLine("const Ptr<Graphics::View>& view = Graphics::GraphicsServer::Instance()->GetCurrentView();")
        for submission in self.submissions:
            if submission.lastSubmit:
                submission.FormatSource(file, self.importTextures)
            else:
                submission.FormatSource(file, [])

        for exportTexture in self.exportTextures:
            file.WriteLine("Export_{} = {{ .index = (uint)TextureIndex::{}, .tex = Textures[(uint)TextureIndex::{}], .stage = TextureCurrentStage[(uint)TextureIndex::{}] }};".format(exportTexture.name, exportTexture.name, exportTexture.name, exportTexture.name))
        file.DecreaseIndent()
        file.WriteLine("}")

        file.WriteLine("}} // namespace FrameScript_{}".format(self.name))

# Entry point for generator
if __name__ == '__main__':
    globals()

    queues = set(['Graphics', 'Compute', 'Transfer', 'Sparse'])

    generator = FrameScriptGenerator()
    generator.SetVersion(Version)
    file = sys.argv[-3]
    outH = sys.argv[-2]
    outS = sys.argv[-1]

    print("Compiling frame script '{}' -> '{}' & '{}'".format(file, outH, outS))

    headerF = IDLC.filewriter.FileWriter()
    headerF.Open(outH)

    sourceF = IDLC.filewriter.FileWriter()
    sourceF.Open(outS)

    generator.SetDocument(file)
    generator.SetName(Path(file).stem)
    generator.Parse()

    generator.FormatHeader(headerF)
    headerF.Close()
    generator.FormatSource(sourceF)
    sourceF.Close()
