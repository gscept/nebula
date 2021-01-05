//------------------------------------------------------------------------------
//  decals.fxh
//  (C) 2021 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------

#ifndef DECALS_FXH
#define DECALS_FXH

// increase if we need more decals in close proximity, for now, 128 is more than enough
#define MAX_DECALS_PER_CLUSTER 128

sampler_state DecalSampler
{
    Filter = MinMagMipLinear;
    AddressU = Border;
    AddressV = Border;
    BorderColor = Transparent;
};

struct PBRDecal
{
    textureHandle albedo;
    vec4 bboxMin;
    vec4 bboxMax;
    mat4 invModel;
    vec3 direction;
    textureHandle material;
    vec3 tangent;
    textureHandle normal;
};

struct EmissiveDecal
{
    vec4 bboxMin;
    vec4 bboxMax;
    mat4 invModel;
    vec3 direction;
    textureHandle emissive;
};

group(BATCH_GROUP) rw_buffer DecalLists [ string Visibility = "CS|PS"; ]
{
    EmissiveDecal EmissiveDecals[128];
    PBRDecal PBRDecals[128];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant DecalUniforms [ string Visibility = "CS|PS"; ]
{
    uint NumPBRDecals;
    uint NumEmissiveDecals;
    uint NumClusters;
    textureHandle NormalBufferCopy;
    textureHandle StencilBuffer;
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(BATCH_GROUP) rw_buffer DecalIndexLists [ string Visibility = "CS|PS"; ]
{
    uint EmissiveDecalCountList[NUM_CLUSTER_ENTRIES];
    uint EmissiveDecalIndexList[NUM_CLUSTER_ENTRIES * MAX_DECALS_PER_CLUSTER];
    uint PBRDecalCountList[NUM_CLUSTER_ENTRIES];
    uint PBRDecalIndexList[NUM_CLUSTER_ENTRIES * MAX_DECALS_PER_CLUSTER];
};

//------------------------------------------------------------------------------
/**
*/
void
ApplyDecals(uint idx, vec4 viewPos, vec4 worldPos, float depth, inout vec4 albedo, inout vec3 normal, inout vec4 material)
{
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
        return;
    }

    // normalize the normal
    totNormal = normalize(totNormal);
    
    // write outputs, make sure to include the total weight so we can do a proper blending
    albedo.xyz = (1.0f - totWeight) * albedo.xyz + (totAlbedo * totWeight);
    normal = totNormal;
    material.xyz = (1.0f - totWeight) * material.xyz + totMaterial.xyz * totWeight;
}

#endif