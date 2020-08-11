//------------------------------------------------------------------------------
//  mie-rayleigh.fxh
//
//	Common functions used to calculate a Mie and Rayleigh sky scattering
//
//	Source: https://github.com/Tw1ddle/Sky-Shader
//
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------

#define PI 3.141592653589793238462643383279502884197169

//------------------------------------------------------------------------------
/**
*/
vec3 
RayleighIntegrate(vec3 lambda)
{
	return (8.0f * pow(PI, 3.0f) * pow(pow(RefractiveIndex, 2.0f) - 1.0f, 2.0f) * (6.0f + 3.0f * DepolarizationFactor)) / (3.0f * Molecules * pow(lambda, vec3(4.0f)) * (6.0f - 7.0f * DepolarizationFactor));
}

//------------------------------------------------------------------------------
/**
*/
vec3
MieIntegrate(vec3 lambda, vec3 k, float t)
{
	float c = 0.2f * t * 10e-18;
	return 0.434 * c * PI * pow((2.0f * PI) / lambda, vec3(MieV - 2.0f)) * k;
}

//------------------------------------------------------------------------------
/**
*/
float 
RayleighPhase(float cosTheta)
{
	return (3.0f / (16.0f * PI)) * (1.0f + pow(cosTheta, 2.0f));
}

//------------------------------------------------------------------------------
/**
*/
float 
HenyeyGreensteinPhase(float cosTheta, float g)
{
	return (1.0f / (4.0f * PI)) * ((1.0f - pow(g, 2.0f)) / pow(1.0f - 2.0f * g * cosTheta + pow(g, 2.0f), 1.5f));
}

//------------------------------------------------------------------------------
/**
*/
float
SunIntensity(float zenithAngle)
{
	float cutoff = PI / 1.95f;
	return SunIntensityFactor * max(0.0f, 1.0f - exp(-((cutoff - acos(zenithAngle)) / SunIntensityFalloff)));
}

const float Av = 0.15; // Shoulder strength
const float Bv = 0.50; // Linear strength
const float Cv = 0.10; // Linear angle
const float Dv = 0.20; // Toe strength
const float Ev = 0.02; // Toe numerator
const float Fv = 0.30; // Toe denominator
vec3 Uncharted2Tonemap(vec3 W)
{
	return ((W * (Av * W + Cv * Bv) + Dv * Ev) / (W * (Av * W + Bv) + Dv * Fv)) - Ev / Fv;
}

//------------------------------------------------------------------------------
/**
*/
vec3 
CalculateAtmosphericScattering(vec3 p, vec3 lp) 
{
	float sunfade = 1.0 - clamp(1.0f - exp(lp.y), 0.0f, 1.0f);
	float rayleighCoefficient = RayleighFactor - (1.0f - sunfade);
	vec3 betaR = RayleighIntegrate(PrimaryColors) * rayleighCoefficient;

	vec3 betaM = MieIntegrate(PrimaryColors, MieKCoefficient, Turbidity) * MieCoefficient;

	float zenith = acos(max(0.0f, dot(vec3(0, 1, 0), p)));
	float denom = cos(zenith) + 0.15f * pow(93.885f - ((zenith * 180.0f) / PI), -1.253f);
	float sr = RayleighZenithLength / denom;
	float sm = MieZenithLength / denom;

	vec3 fex = exp(-(betaR * sr + betaM * sm));

	float cosTheta = dot(p, lp);
	vec3 betaRTheta = betaR * RayleighPhase(cosTheta * 0.5f + 0.5f);
	vec3 betaMTheta = betaM * HenyeyGreensteinPhase(cosTheta, MieDirectionalG);
	float sunE = SunIntensity(dot(lp, vec3(0, 1, 0)));
	vec3 lin = pow(sunE * ((betaRTheta + betaMTheta) / (betaR + betaM)) * (1.0f - fex), vec3(1.5f));
	lin *= mix(vec3(1.0f), pow(sunE * ((betaRTheta + betaMTheta) / (betaR + betaM)) * fex, vec3(0.5f)), clamp(pow(1.0f - dot(vec3(0, 1, 0), lp), 5.0f), 0.0f, 1.0f));

	float sunAngleDiameterCos = cos(SunDiscSize);
	float sundisk = smoothstep(sunAngleDiameterCos, sunAngleDiameterCos + 0.00002, cosTheta);
	vec3 l0 = vec3(0.1f) * fex;
	l0 += sunE * fex * sundisk;
	vec3 texColor = lin + l0;
	texColor *= 0.04f;
	texColor += vec3(0.0f, 0.001f, 0.0025f) * 0.3f;

	/*
	vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(TonemapWeight));
	vec3 curr = Uncharted2Tonemap((log2(2.0 / pow(Lum, 4.0))) * texColor);
	vec3 color = curr * whiteScale;
	vec3 retColor = pow(color, vec3(1.0 / (1.2 + (1.2 * sunfade))));
	*/

	return texColor;
}
