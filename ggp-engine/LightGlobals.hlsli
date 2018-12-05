//LIGHT STRUCTS
struct DirectionalLight {
	float4 color;
	float3 direction;
	float diffuseIntensity;
	float ambientIntensity;
	float3 empty;
};

struct PointLight {
	float4 color;
	float3 position;
	float intensity;
	float range;
	float3 empty;
};

struct SpotLight {
	float4 color;
	float3 direction;
	float cone;
	float3 position;
	float range;
};

//LIGHTING DATA REGISTERS
static const uint maxDirLightCount = 4;
cbuffer dirLightData : register(b1) {
	DirectionalLight dirLights[maxDirLightCount];
	uint dirLightCount;
}

static const uint maxPointLightCount = 128;
cbuffer pointLightData : register(b2) {
	PointLight pointLights[maxPointLightCount];
	uint pointLightCount;
}

static const uint maxSpotLightCount = 128;
cbuffer spotLightData : register(b3) {
	SpotLight spotLights[maxSpotLightCount];
	uint spotLightCount;
}

/*
	BASIC LIGHTING
*/
////HELPERS
//Diffuse calculation
float diffuse(float3 _normal, float3 _lightDir) {
	//Return N*L
	return saturate(dot(_normal, _lightDir));
}

//Specular calculation (full phong)
float specular(float3 _normal, float3 _lightDir, float3 _camPos, float3 _worldPos, float _shininess = 0.0f) {
	//Reflect the light direction across the normal to the surface
	float3 refl = reflect(-_lightDir, _normal);
	//Get a normalized direction vector pointing at the camera
	float3 camDir = normalize(_camPos - _worldPos);
	//If there is any specularity on this surface, calculate it using phong
	float spec = _shininess == 0.0f ? 0.0f : _shininess * pow(saturate(dot(refl, camDir)), 64);
	//Return specularity scalar
	return spec;
}

////CALC FOR INDIVIDUAL TYPES
//Directional light
float3 calcDirLight(DirectionalLight _light, float3 _normal, float3 _surfaceColor, float3 _camPos, float3 _worldPos, float _shininess = 0.0f) {
	//Calculate light amounts
	float diff = diffuse(_normal, _light.direction);
	float spec = specular(_normal, _light.direction, _camPos, _worldPos, _shininess);
	//Combine
	return (diff * _surfaceColor + spec) * _light.color * _light.diffuseIntensity + _surfaceColor * _light.color * _light.ambientIntensity;
}

//Point Light
float3 calcPointLight(PointLight _light, float3 _normal, float3 _surfaceColor, float3 _camPos, float3 _worldPos, float _shininess = 0.0f) {
	//Get direction from the pixel to the point light
	float3 lightDir = _light.position - _worldPos;
	//Get the distance to the light
	float4 lightDist = length(lightDir);
	//After getting distance, normalize the dir vector
	lightDir = normalize(lightDir);
	//Attenuation calculation
	float lightAtten = pow(saturate(1.0f - (lightDist * lightDist / (_light.range * _light.range))), 2);

	//Calculate lighting 
	float diff = diffuse(_normal, lightDir);
	float spec = specular(_normal, lightDir, _camPos, _worldPos, _shininess);

	//Return the combined lighting
	return (diff * _surfaceColor + spec) * lightAtten * _light.intensity * _light.color;
}

//Spot Light
float3 calcSpotLight(SpotLight _light, float3 _normal, float3 _surfaceColor, float3 _camPos, float3 _worldPos, float _shininess = 0.0f) {
	//Get direction from the pixel to the point light
	float3 lightDir = normalize(_worldPos - _light.position);
	//Get the distance to the light
	float lightDist = length(_worldPos - _light.position);
	float NdotL = dot(_normal, -lightDir);
	NdotL = saturate(NdotL);
	//Attenuation calculation
	float lightAtten = saturate(1.0f - (lightDist * lightDist / (_light.range * _light.range)));
	lightAtten *= lightAtten;
	float angleFromCenter = max(dot(-lightDir, normalize(-_light.direction)), 0.0f);
	float spotAmount = pow(angleFromCenter, 45.0f - _light.cone);
	//Calculate lighting 
	//Calculate specular light
	float3 toCam = normalize(_camPos - _worldPos);
	float3 refl = reflect(lightDir, _normal);
	float spec = pow(max(dot(refl, toCam), 0.0f), 64.0f);

	//Return the combined lighting
	return (_light.color * NdotL * _surfaceColor * spotAmount + spec) * lightAtten;
} 

/*
	PHYSICALLY BASED LIGHTING
*/
//Some basic constants
static const float DIELECTRIC_FRESNEL_CONST = 0.04f;
static const float MIN_ROUGHNESS = 0.0000001f;
static const float PI = 3.14159264349f;

//Trowbridge-Reits specular. D(h)
//http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// D(m) = (a^2) / (pi * ((N o M)^2 * (a^2 - 1) + 1)^2)
float ggxSpecular(float _a2, float _NdotM) {
	float NdotM2 = _NdotM * _NdotM;
	float denom = NdotM2 * (_a2 - 1) + 1;
	return _a2 / (PI * denom * denom);
}

//Schlick fresnel calculation. F(v, m)
//http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// F(v,m) = F0 + (1 - F0) * (1 - (V o M))^5
float3 fresnel(float3 _camDir, float3 _m, float3 _f0) {
	float VdotM = saturate(dot(_camDir, _m));
	float3 fres = _f0 + (1 - _f0) * pow(1 - VdotM, 5);
	return fres;
}

//Schlick-Beckman approximation of Smith's geometric shadowing equation. G(x), where x is lightDir or cameraDir
//http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// k = a*sqrt(2/pi)
// G(x) = (N o X) / ((N o X) * (1 - k) + k)
float geomShadow(float _NdotX, float _k) {
	float gs = _NdotX / (_NdotX * (1 - _k) + _k);
	return gs;
}


//Cook-Torrence microfacet BRDF calculation
//Shared code between all light types
float3 microfacetSpec(float3 _normal, float3 _lightDir, float3 _camDir, float _NdotL, float _roughness, float _metalness, float3 _f0) {
	//Store calculations used more than once in variables
	float3 m = normalize(_camDir + _lightDir);
	float a = _roughness * _roughness;
	float a2 = max(a * a, MIN_ROUGHNESS);
	float k = pow(_roughness + 1, 2) / 8.0f;
	//float k = a * 0.7978845608f;
	//Dot products
	float NdotV = saturate(dot(_normal, _camDir));
	float NdotM = saturate(dot(_normal, m));

	//Calculate individual parts of the Cook-Torrance microfacet BRDF equation
	//f(L,V) = (D(m)*F(V,m)*G(L)*G(V)) / (4 * (N o L) * (N o V))
	float spec = ggxSpecular(a2, NdotM); //D(m) - Specular
	float3 fres = fresnel(_camDir, m, _f0); //F(V,m) - Fresnel
	float gsNL = geomShadow(_NdotL, k); //G(L) - Geometric Shadowing for L
	float gsNV = geomShadow(NdotV, k); //G(V) - Geometric Shadowing for V
	//Put all the terms together into the microfacet BRDF calculation
	return (spec * fres * gsNL * gsNV) / (4 * max(_NdotL, NdotV));
}

////CALC FOR INDIVIDUAL TYPES
float3 calcDirLightPBR(DirectionalLight _light, float3 _normal, float3 _camPos, float3 _worldPos, float3 _albedo, float _roughness, float _metalness, float3 _specColor) {
	float3 lightDir = normalize(-_light.direction);
	float3 camDir = normalize(_camPos - _worldPos);
	//Calculate diffuse lighting
	float diff = diffuse(_normal, lightDir);
	//Calculate specular lighting
	float3 spec = microfacetSpec(_normal, lightDir, camDir, diff, _roughness, _metalness, _specColor);
	//Incorporate conservation of energy
	float3 balancedDiffuse = diff * ((1 - saturate(spec)) * (1 - _metalness));

	return (balancedDiffuse * _albedo + spec) * _light.color * _light.diffuseIntensity;
}

float3 calcPointLightPBR(PointLight _light, float3 _normal, float3 _camPos, float3 _worldPos, float3 _albedo, float _roughness, float _metalness, float3 _specColor) {
	float3 lightDir = normalize(_light.position - _worldPos);
	float lightDist = distance(_light.position, _worldPos);
	float3 camDir = normalize(_camPos - _worldPos);
	//Attenuation calculation
	float lightAtten = pow(saturate(1.0f - (lightDist * lightDist / (_light.range * _light.range))), 2);
	//Calculate diffuse lighting
	float diff = diffuse(_normal, lightDir);
	//Calculate specular lighting
	float3 spec = microfacetSpec(_normal, lightDir, camDir, diff, _roughness, _metalness, _specColor);
	//Incorporate conservation of energy
	float3 balancedDiffuse = diff * ((1 - saturate(spec)) * (1 - _metalness));

	return (balancedDiffuse * _albedo + spec) * lightAtten * _light.intensity * _light.color;
}

float3 calcSpotLightPBR(SpotLight _light, float3 _normal, float3 _camPos, float3 _worldPos, float3 _albedo, float _roughness, float _metalness, float3 _specColor) {
	float3 lightDir = normalize(_worldPos - _light.position);
	float lightDist = distance(_light.position, _worldPos);
	float3 camDir = normalize(_camPos - _worldPos);
	
	//Attenuation calculation
	float lightAtten = pow(saturate(1.0f - (lightDist * lightDist / (_light.range * _light.range))), 2);
	//Calculate diffuse lighting
	float diff = dot(_normal, -lightDir);
	diff = saturate(diff);
	//calculate spotlight amount based on cone shape
	float angleFromCenter = max(dot(-lightDir, normalize(-_light.direction)), 0.0f);
	float spotAmount = pow(angleFromCenter, 45.0f - _light.cone);
	//Calculate specular lighting
	float3 spec = microfacetSpec(_normal, -lightDir, camDir, diff, _roughness, _metalness, _specColor);
	//Incorporate conservation of energy
	float3 balancedDiffuse = diff * ((1 - saturate(spec)) * (1 - _metalness));
	return (balancedDiffuse * _albedo * spotAmount + spec) * lightAtten * _light.color;
}

/*
	UTILS
*/
//Apply the normal from a texture to the normal and tangent vectors from the mesh
float3 applyNormalMap(float3 _normal, float3 _tangent, float3 _mapNormal) {
	//Construct TBN matrix
	//NOTE: I followed the demo on mycourses for this (Game Graphics Programming)
	//Normal
	float3 N = normalize(_normal);
	//Tangent
	float3 T = normalize(_tangent - N * dot(_tangent, N));
	//Bi-tangent
	float3 B = cross(T, N);
	//TBN Matrix
	float3x3 TBN = float3x3(T, B, N);
	//Adjust normal vector using the data from the normal map
	//Transform the normal map vector into local pixel space
	return normalize(mul(_mapNormal, TBN));
}
