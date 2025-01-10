//------------------------------------------------------------------------------
//  @file probe_debug.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/mie-rayleigh.fxh>
#include <lib/pbr.fxh>
#include <lib/ddgi.fxh>

#include "probe_shared.fxh"


//------------------------------------------------------------------------------
/**
*/
shader void
DebugVS(
    [slot=0] in vec3 position,
    [slot=2] in ivec2 uv,
    [slot=1] in vec3 normal,
    [slot=3] in vec4 tangent,
    out vec3 Normal,
    out vec3 WorldPosition
)
{
    vec3 probeWorldPosition;
    if ((Options & RELOCATION_OPTION) != 0)
        probeWorldPosition = DDGIProbeWorldPositionWithOffset(gl_InstanceIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing, ProbeOffsets);
    else
        probeWorldPosition = DDGIProbeWorldPosition(gl_InstanceIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing);
    WorldPosition = probeWorldPosition;
    
    Normal = normal;
    gl_Position = ViewProjection * vec4(position * 0.1f + probeWorldPosition, 1);
}

//------------------------------------------------------------------------------
/**
*/
shader void
DebugPS(
    in vec3 normal
    , in vec3 worldPos
    , [color0] out vec4 Color
)
{
    GIVolume volumeArg;
    volumeArg.Offset = Offset;
    volumeArg.Rotation = Rotation;
    volumeArg.GridCounts = ProbeGridDimensions;
    volumeArg.GridSpacing = ProbeGridSpacing;
    volumeArg.ScrollOffsets = ProbeScrollOffsets;
    volumeArg.NumIrradianceTexels = NumIrradianceTexels;
    volumeArg.NumDistanceTexels = NumDistanceTexels;
    volumeArg.EncodingGamma = IrradianceGamma;
    volumeArg.Irradiance = ProbeIrradiance;
    volumeArg.Distances = ProbeDistances;
    volumeArg.Offsets = ProbeOffsets;
    volumeArg.States = ProbeStates; 
    
    vec3 irradiance = EvaluateDDGIIrradiance(worldPos, vec3(0), normal, volumeArg, Options);
    Color = vec4(irradiance, 1);
}

render_state DefaultState
{
    DepthWrite = true;
    DepthEnabled = true;
    DepthFunc = LessEqual;
};

//------------------------------------------------------------------------------
/**
*/
program Debug[string Mask="Debug";]
{
    VertexShader = DebugVS();
    PixelShader = DebugPS();
    RenderState = DefaultState;
};
