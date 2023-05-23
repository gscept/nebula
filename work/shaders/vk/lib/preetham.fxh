//------------------------------------------------------------------------------
//  preetham.fxh
//
//	Common functions used to calculate a Preetham sky scattering
//
//	Source: http://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf
//
//  (C) 2016 Individual contributors, see AUTHORS file
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
*/
vec3 YxyToXYZ( in vec3 Yxy )
{
	float Y = Yxy.r;
	float x = Yxy.g;
	float y = Yxy.b;

	float X = x * ( Y / y );
	float Z = ( 1.0 - x - y ) * ( Y / y );

	return vec3(X,Y,Z);
}

//------------------------------------------------------------------------------
/**
	@param sphereDir The direction from the surface to the sky dome.
	@param lightDir The direction of the global light (sky light) in world space (so multiply by InvView).
*/
vec3
Preetham(vec3 sphereDir, vec3 lightDir, vec4 A, vec4 B, vec4 C, vec4 D, vec4 E, vec4 Z)
{
	
	float cosThetaSun = saturate(dot(lightDir, vec3(0, 1, 0)));
	float cosTheta = saturate(dot(sphereDir, vec3(0, 1, 0)));
	float cosGamma = saturate(dot(sphereDir, lightDir.xyz));
	float thetaSun = acos(cosThetaSun);
	float gamma = acos(cosGamma);
	vec3 r_xyY = Z.xyz * (perez(cosTheta, gamma, cosGamma, A.xyz, B.xyz, C.xyz, D.xyz, E.xyz));
	vec3 zeroThetaS = perez(0.0, thetaSun, cosThetaSun, A.xyz, B.xyz, C.xyz, D.xyz, E.xyz);

	vec3 r_XYZ = YxyToXYZ(r_xyY / zeroThetaS);
	vec3 ret = XYZToRGB(r_XYZ);
	return ret * ONE_OVER_PI;
}
