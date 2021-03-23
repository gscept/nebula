//------------------------------------------------------------------------------
//  lights_cluster_cull.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/clustering.fxh"
#include "lib/pbr.fxh"
#include "lib/stencil.fxh"
#include "lib/decals.fxh"

sampler_state ScreenspaceSampler
{
    Filter = MinMagMipPoint;
};

// only enable blend for the albedo and material output
render_state PBRState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    BlendEnabled[1] = true;
    SrcBlend[1] = One;
    DstBlend[1] = Zero;
    BlendEnabled[2] = true;
    SrcBlend[2] = One;
    DstBlend[2] = Zero;
    DepthWrite = false;
    DepthEnabled = false;
};

// only enable blend for the albedo and material output
render_state EmissiveState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = One;
    DepthWrite = false;
    DepthEnabled = false;
};

write rgba16f image2D Decals;
write rgba16f image2D DebugOutput;

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader 
void csCull()
{
    uint index1D = gl_GlobalInvocationID.x;

    if (index1D > NumDecalClusters)
        return;

    ClusterAABB aabb = AABBs[index1D];

    uint flags = 0;

    // update PBR decals
    uint numDecals = 0;
    for (uint i = 0; i < NumPBRDecals; i++)
    {
        const PBRDecal decal = PBRDecals[i];
        if (TestAABBAABB(aabb, decal.bboxMin.xyz, decal.bboxMax.xyz))
        {
            PBRDecalIndexList[index1D * MAX_DECALS_PER_CLUSTER + numDecals] = i;
            numDecals++;
        }
    }
    PBRDecalCountList[index1D] = numDecals;

    // update feature flags if we have any decals
    if (numDecals > 0)
        flags |= CLUSTER_PBR_DECAL_BIT;

    // update emissive decals
    numDecals = 0;
    for (uint i = 0; i < NumEmissiveDecals; i++)
    {
        const EmissiveDecal decal = EmissiveDecals[i];
        if (TestAABBAABB(aabb, decal.bboxMin.xyz, decal.bboxMax.xyz))
        {
            EmissiveDecalIndexList[index1D * MAX_DECALS_PER_CLUSTER + numDecals] = i;
            numDecals++;
        }       
    }
    EmissiveDecalCountList[index1D] = numDecals;

    // update feature flags if we have any decals
    if (numDecals > 0)
        flags |= CLUSTER_EMISSIVE_DECAL_BIT;

    atomicOr(AABBs[index1D].featureFlags, flags);
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csDebug()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;

    // convert screen coord to view-space position
    vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth, InvProjection);

    uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    uint flag = AABBs[idx].featureFlags; // add 0 so we can read the value
    vec4 color = vec4(0, 0, 0, 0);
    if (CHECK_FLAG(flag, CLUSTER_PBR_DECAL_BIT))
    {
        uint count = PBRDecalCountList[idx];
        color.r = count / float(NumPBRDecals);
    }
    if (CHECK_FLAG(flag, CLUSTER_EMISSIVE_DECAL_BIT))
    {
        uint count = EmissiveDecalCountList[idx];
        color.g = count / float(NumEmissiveDecals);
    }
    
    imageStore(DebugOutput, int2(coord), color);
}

//------------------------------------------------------------------------------
/**
*/
shader
void vsRender(
    [slot = 0] in vec3 position,
    [slot = 2] in vec2 uv,
    out vec2 UV)
{
    gl_Position = vec4(position, 1);
    UV = uv;
}


//------------------------------------------------------------------------------
/**
    Update GBuffers
*/
shader
void psRenderPBR(
    in vec2 UV,
    [color0] out vec4 Albedo,
    [color1] out vec3 Normal,
    [color2] out vec4 Material)
{
    ivec2 coord = ivec2(gl_FragCoord.xy);
    float depth = fetch2D(DepthBuffer, ScreenspaceSampler, coord, 0).r;
    uint stencil = fetchStencil(StencilBuffer, ScreenspaceSampler, coord, 0);
    vec3 normal = fetch2D(NormalBufferCopy, ScreenspaceSampler, coord, 0).rgb;

    if (CHECK_FLAG(stencil, STENCIL_BIT_CHARACTER))
        discard;

    // convert screen coord to view-space position
    vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth, InvProjection);
    vec4 worldPos = ViewToWorld(viewPos, InvView);
    vec3 worldViewVec = normalize(EyePos.xyz - worldPos.xyz);
    vec3 viewVec = -normalize(viewPos.xyz);
    vec3 viewNormal = (View * vec4(normal, 0)).xyz;

    uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 totAlbedo = vec3(0);
    vec3 totNormal = vec3(0);
    vec4 totMaterial = vec4(0);
    float totWeight = 0; 

    // calculate custom derivatives for the decal sampling method
    vec2 ddx = dFdx(viewPos.xy) * FocalLengthNearFar.z * 2.0f;
    vec2 ddy = dFdy(viewPos.xy) * FocalLengthNearFar.z * 2.0f;

    // adjust gradient based on dFdx in depth, if the distance is too big, clip the gradient
    float dFdDepth = fwidth(depth);
    if (dFdDepth > 0.001f)
    {
        ddx = vec2(0.0f);
        ddy = vec2(0.0f);
    }

    uint flag = AABBs[idx].featureFlags;
    if (CHECK_FLAG(flag, CLUSTER_PBR_DECAL_BIT))
    {
        uint count = PBRDecalCountList[idx];
        for (int i = 0; i < count; i++)
        {
            uint didx = PBRDecalIndexList[idx * MAX_DECALS_PER_CLUSTER + i];
            PBRDecal decal = PBRDecals[didx];
            vec4 localPos = decal.invModel * worldPos;

            // check if pixel is inside the bounding box
            vec3 dist = vec3(0.5f) - abs(localPos.xyz);
            if (all(greaterThan(dist, vec3(0))))
            {
                // assume the XY are the texture coordinates
                vec2 uv = localPos.xy + vec2(0.5f);

                // calculate a weight which serves as a smooth blend off the normal
                float weight = saturate(dot(decal.direction, -normal.xyz));
                vec4 d_albedo = sample2DGrad(decal.albedo, DecalSampler, uv, ddx, ddy);
                vec4 d_normal = sample2DGrad(decal.normal, DecalSampler, uv, ddx, ddy);
                vec4 d_material = sample2DGrad(decal.material, DecalSampler, uv, ddx, ddy);
                weight *= d_albedo.a;

                if (weight > 0.0f)
                {
                    // calculate tbn
                    vec3 d_tangent = decal.tangent;
                    vec3 binormal = cross(d_tangent, normal);
                    vec3 tangent = cross(binormal, normal);
                    mat3 tbn = mat3(tangent, binormal, normal);

                    // calculate normal map in TBN space
                    vec3 tNormal = vec3(0, 0, 0);
                    tNormal.xy = (d_normal.ag * 2.0f) - 1.0f;
                    tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
                    tNormal = tbn * tNormal;

                    totAlbedo += d_albedo.xyz * weight;
                    totNormal += tNormal * weight;
                    totMaterial += d_material * weight;
                    totWeight += weight;
                }
            }

            // no need to go on if the pixel is saturated with decals
            if (totWeight >= 1.0f)
                break;
        }
    }

    if (totWeight == 0)
    {
        discard;
        return;
    }

    // normalize the normal
    totNormal = normalize(totNormal);

    // write outputs, make sure to include the total weight so we can do a proper blending
    Albedo = vec4(totAlbedo, totWeight);
    Normal = totNormal;
    Material = vec4(totMaterial.xyz, totWeight);
}

//------------------------------------------------------------------------------
/**
    Render emissive
*/
shader
void psRenderEmissive(
    in vec2 UV,
    [color0] out vec4 emissive)
{
    ivec2 coord = ivec2(UV.xy * FramebufferDimensions);
    float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;
    vec4 material = fetch2D(SpecularBuffer, PosteffectSampler, coord, 0).rgba;

    // convert screen coord to view-space position
    vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth, InvProjection);
    vec4 worldPos = ViewToWorld(viewPos, InvView);
    vec3 worldViewVec = normalize(EyePos.xyz - worldPos.xyz);
    vec3 viewVec = -normalize(viewPos.xyz);

    vec3 viewPos_ddx = dFdx(viewPos.xyz);
    vec3 viewPos_ddy = dFdy(viewPos.xyz);

    // calculate the normal using derivatives of the world position
    vec3 viewNormal = normalize(cross(viewPos_ddx, viewPos_ddy));
    vec3 normal = (InvView * vec4(viewNormal.xyz, 0)).xyz;

    uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec4 totEmissive = vec4(0);
    float totWeight = 0;

    uint flag = AABBs[idx].featureFlags;
    if (CHECK_FLAG(flag, CLUSTER_EMISSIVE_DECAL_BIT))
    {
        uint count = EmissiveDecalCountList[idx];
        for (int i = 0; i < count; i++)
        {
            uint didx = EmissiveDecalIndexList[idx * MAX_DECALS_PER_CLUSTER + i];
            EmissiveDecal decal = EmissiveDecals[didx];

            // transform world position to the local space of the 
            vec4 localPos = decal.invModel * worldPos;
            vec3 dist = vec3(0.5f) - abs(localPos.xyz);

            // check if pixel is inside the bounding box
            if (all(greaterThan(dist, vec3(0))))
            {
                // assume the XY are the texture coordinates
                vec2 uv = localPos.xy;
                float weight = dot(decal.direction, normal.xyz);
                vec4 emissive = sample2D(decal.emissive, DecalSampler, uv);

                weight = min(max(weight, 0.0f), 1.0f);
                totEmissive += emissive * weight;
                totWeight += weight;
            }

            // no need to go on if the pixel is saturated with decals
            if (totWeight >= 1.0f)
                break;
        }
    }

    // write emissive
    emissive = totEmissive * vec4(1,1,1, totWeight);
}

//------------------------------------------------------------------------------
/**
*/
program CullDecals [ string Mask = "Cull"; ]
{
    ComputeShader = csCull();
};

//------------------------------------------------------------------------------
/**
*/
program ClusterDebug [ string Mask = "Debug"; ]
{
    ComputeShader = csDebug();
};

//------------------------------------------------------------------------------
/**
*/
program RenderPBRDecals [ string Mask = "RenderPBR"; ]
{
    VertexShader = vsRender();
    PixelShader = psRenderPBR();
    RenderState = PBRState;
};

//------------------------------------------------------------------------------
/**
*/
program RenderEmissiveDecals[string Mask = "RenderEmissive";]
{
    VertexShader = vsRender();
    PixelShader = psRenderEmissive();
    RenderState = EmissiveState;
};
