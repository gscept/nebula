//------------------------------------------------------------------------------
//  preetham.fxh
//
//	Common functions used to calculate a Preetham sky scattering
//
//	Source: http://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf
//
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#define TINY (0.0001f)
//------------------------------------------------------------------------------
/**
*/
vec3
perez(float cosTheta, float gamma, float cosGamma, vec3 A, vec3 B, vec3 C, vec3 D, vec3 E)
{
	return (1 + A * exp(B / (cosTheta + TINY))) * (1 + C * exp(D * gamma) + E * cosGamma * cosGamma);
}

//------------------------------------------------------------------------------
/**
	@param sphereDir The direction from the surface to the sky dome.
	@param lightDir The direction of the global light (sky light) in world space (so multiply by InvView).
*/
vec3
Preetham(vec3 sphereDir, vec3 lightDir, vec4 A, vec4 B, vec4 C, vec4 D, vec4 E, vec4 Z)
{
	float cosTheta = clamp(sphereDir.y, 0.0f, 1.0f);
	float cosGamma = dot(sphereDir, lightDir.xyz);
	float gamma = acos(cosGamma);
	vec3 r_xyY = Z.xyz * perez(cosTheta, gamma, cosGamma, A.xyz, B.xyz, C.xyz, D.xyz, E.xyz);
	vec3 r_XYZ = vec3(r_xyY.x, r_xyY.y, 1 - r_xyY.x - r_xyY.y) * r_xyY.z / r_xyY.y;
	float red = dot(vec3( 3.240479, -1.537150, -0.498535), r_XYZ);
	float green = dot(vec3(-0.969256,  1.875992,  0.041556), r_XYZ);
	float blue = dot(vec3( 0.055648, -0.204043,  1.057311), r_XYZ);
	return vec3(red, green, blue);
}