//------------------------------------------------------------------------------
//  @file fullscreen.fxh    
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
vec4
GenerateFullscreenTri(uint vertexId)
{
    vec4 ret;

    // UV [0..2]
    ret.zw = vec2((vertexId << 1) & 2, vertexId & 2);

    // Triangle position [-1..3]
    ret.xy = ret.zw * vec2(2, 2) + vec2(-1, -1);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
vec4
GenerateQuad(uint vertexId)
{
    vec4 ret;

    switch (vertexId)
    {
        case 0:
        case 5:
            ret.zw = vec2(0, 0);
            break;
        case 1:
            ret.zw = vec2(0, 1);
            break;
        case 2:
        case 3:
            ret.zw = vec2(1, 1);
            break;
        case 4:
            ret.zw = vec2(1, 0);
            break;
    }

    // Put UV in [0..1]
    ret.xy = ret.zw * vec2(2, 2) + vec2(-1, -1);
    return ret;
}