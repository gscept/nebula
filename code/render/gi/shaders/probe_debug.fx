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
    out vec3 WorldPosition,
    out flat int Instance 
)
{
    vec3 probeWorldPosition;
    if ((Options & RELOCATION_OPTION) != 0)
        probeWorldPosition = DDGIProbeWorldPositionWithOffset(gl_InstanceIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing, ProbeOffsets);
    else
        probeWorldPosition = DDGIProbeWorldPosition(gl_InstanceIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing);
    WorldPosition = probeWorldPosition;
    
    Normal = normal;
    Instance = gl_InstanceIndex;
    gl_Position = ViewProjection * vec4(position * DebugSize + probeWorldPosition, 1);
}

//------------------------------------------------------------------------------
/**
*/
shader void
DebugPS(
    in vec3 normal
    , in vec3 worldPos
    , in flat int instance
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
    
    if ((Options & CLASSIFICATION_OPTION) != 0)
    {
        ivec2 probeTexel = DDGIProbeTexelPosition(instance, ProbeGridDimensions);
        float status = fetch2D(ProbeStates, Basic2DSampler, probeTexel, 0).r;
        if (status == PROBE_STATE_INACTIVE)
        {
            Color = vec4(mix(vec3(1,0,0), vec3(0,0,1), vec3(hash12(gl_FragCoord.xy))), 1);
            return;
        }
    }
    
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
