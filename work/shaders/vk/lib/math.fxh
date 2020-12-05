//------------------------------------------------------------------------------
//  math.fxh
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
*/
vec4
makequat(vec3 axis, float sine, float cosine)
{
    return vec4(axis * vec3(sine, sine, sine), cosine);
}

//------------------------------------------------------------------------------
/**
*/
vec3
quatrotate(vec4 quat, vec3 position)
{
    vec3 temp = cross(quat.xyz, position) + quat.w * position;
    return position + 2.0f * cross(quat.xyz, temp);
}
