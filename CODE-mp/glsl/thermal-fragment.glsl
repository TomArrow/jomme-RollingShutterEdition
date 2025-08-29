#version 400 compatibility

uniform sampler2D text_in;


uniform int thermalVisionUniform;
uniform int shaderDebugUniform;
uniform int blurEarlyStageUniform; // 0 = blurearly disabled. 1 = prepostprocessing (before early blur, not supported atm). 2 = postprocessing, 3 = postprocessing, final frame ( not supported atm)
uniform int serverTimeUniform;
uniform float serverTimeFractionUniform;
uniform int jitterIndexUniform; 
uniform int jitterTotalFramesUniform;

#define FLOATSERVERTIME ((float(serverTimeUniform)+serverTimeFractionUniform)*1000.0f)

precision highp float;


// based on https://www.shadertoy.com/view/MlVSzw
const float PI = 3.1415926535;

const float ALPHA = 0.14;
const float INV_ALPHA = 1.0 / ALPHA;
const float K = 2.0 / (PI * ALPHA);
float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}
float inv_error_function(float x)
{
	float y = log(1.0 - x*x);
	float z = K + 0.5 * y;
	return sqrt(sqrt(max(0.0001f,z*z - y * INV_ALPHA)) - z) * sign(x);
}

float gaussian_rand( vec2 n )
{
    int a = serverTimeUniform & 65535;
	float t = fract(fract(float(a) *13.4326426f) + fract( serverTimeFractionUniform*13.4326426f )); // MEH
	float x = nrand( n + 0.07*t );

    float mult= 0.20f;
    
	float tmp = inv_error_function(x*2.0-1.0)*mult;
    if(isinf(tmp) || isnan(tmp)){
        return 0.5;
    }
    if(jitterTotalFramesUniform > 1 && blurEarlyStageUniform == 0){
       //tmp *= max(1.0f,0.33f*sqrt(float(jitterTotalFramesUniform)));
       //tmp *= pow(float(jitterTotalFramesUniform),0.125f);
       tmp *= pow(float(jitterTotalFramesUniform),0.5f);
    }
    if(isinf(tmp) || isnan(tmp)){
        return 0.5;
    }
    return tmp + 0.5;
}




// thermal vision
//
// intensity table:
// where,L,a,b
// 0,6,24,-30
// 0.15,30,63,-108
// 0.4,63,-10,-49
// 0.65,41,-42,41
// 0.85,93,-20,89
// 1.0,54,73,66
//
// distance table:
// 0,76,-12,-32
// 0.07,65,-11,-47
// fade alpha to 0 towards 1.0
//
const float Epsilon = 0.008856f; // Intent is 216/24389
const float Kappa = 903.3f; // Intent is 24389/27
const float KappaInv = 1.0f/Kappa; // Intent is 24389/27
const vec3 white = vec3(0.95047f,1.000f,1.08883f);
const float something = 16.0f / 116.0f;
const float something2 = 1.0f / 7.787f;
//const mat3 xyztorgb = mat3(3.2406f, -0.9689f, 0.0557f,-1.5372f, 1.8758f, -0.2040f, -0.4986f, 0.0415f, 1.0570f);
const mat3 xyztorgb = mat3(3.2406f,-1.5372f,-0.4986f,-0.9689f,1.8758f,0.0415f,0.0557f,-0.2040f,1.0570f);
vec3 lab2rgb( vec3 c ) {
    float y = ( c.x + 16.0f ) / 116.0f;
    float x = c.y / 500.0f + y;
    float z = y - c.z / 200.0f;
	float x3 = x * x * x;
	float z3 = z * z * z;
    vec3 outVal = vec3(
        //white.x * (x3 > Epsilon ? x3 : (116.0f*x-16.0f)/Kappa),
        white.x * (x3 > Epsilon ? x3 : (x - something)*something2),
        white.y * (c.x > (Kappa * Epsilon) ? y*y*y : c.x *KappaInv),
        white.z * (z3 > Epsilon ? z3 : (z - something) *something2));
	return outVal*xyztorgb ;
};
const vec3 heatLUT[21] = {
	// 0,6,24,-30
	vec3(6.0f,24.0f,-30.0f),vec3(13.92f,36.87f,-55.74),vec3(21.84f,49.74f,-81.48f),
	// 0.15,30,63,-108
	vec3(30.0f,63.0f,-108.0f),vec3(36.6f,48.4f,-96.2f),vec3(43.2f,33.8f,-84.4f),vec3(49.8f,19.2f,-72.6f),vec3(56.4f,4.6f,-60.8f),
	// 0.4,63,-10,-49
	vec3(63.0f,-10.0f,-49.0f),	vec3(58.6f,-16.4f,-31.0f),	vec3(54.2f,-22.8f,-13.0f),	vec3(49.8f,-29.2f,5.0f),vec3(45.4f,-35.6f,23.0f),
	// 0.65,41,-42,41
	vec3(41.0f,-42.0f,41.0f),	vec3(54.0f,-36.5f,53.0f),vec3(67.0f,-31.0f,65.0f),vec3(80.0f,-25.5f,77.0f),
	// 0.85,93,-20,89
	vec3(93.0f,-20.0f,89.0f),	vec3(80.13f,10.69f,81.41f),vec3(67.26f,41.38f,73.82f),
	// 1.0,54,73,66
	vec3(54.0f,73.0f,66.0f),
};







// from http://www.java-gaming.org/index.php?topic=35123.0
vec4 cubic(float v){
    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) * (1.0/6.0);
}

vec4 textureBicubic(sampler2D sampler, vec2 texCoords, int lod){

   vec2 texSize = textureSize(sampler, lod);
   vec2 invTexSize = 1.0 / texSize;
   
   texCoords = texCoords * texSize - 0.5;

   
    vec2 fxy = fract(texCoords);
    texCoords -= fxy;

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = texCoords.xxyy + vec2 (-0.5, +1.5).xyxy;
    
    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4 (xcubic.yw, ycubic.yw) / s;
    
    offset *= invTexSize.xxyy;
    
    vec4 sample0 = textureLod(sampler, offset.xz,lod);
    vec4 sample1 = textureLod(sampler, offset.yz,lod);
    vec4 sample2 = textureLod(sampler, offset.xw,lod);
    vec4 sample3 = textureLod(sampler, offset.yw,lod);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(
       mix(sample3, sample2, sx), mix(sample1, sample0, sx)
    , sy);
}
float fracs[6] = {
    1.0f,
    0.5f,
    0.333333f,
    0.25f,
    0.2f,
    0.165f
};

const vec3 veryFarColor = vec3(76,-12,-32);
const vec3 farColor = vec3(65,-11,-47);
void applyThermal(inout vec3 color){
    float intensity = color.y;
    float distanceFactor = color.z;
    float multiplier = 1.0f;
    
	//if(intensity > 1.0f){
	//	multiplier = intensity;
	//	intensity=1.0f;
	//}
    intensity *= 20.0f;
	int index = clamp(int(intensity),0,19);
	float lerp = intensity - float(index);
	vec3 result = mix(heatLUT[index],heatLUT[index+1],lerp);
	vec3 distcolor = mix(veryFarColor,farColor,clamp(distanceFactor*14.28f,0.0f,1.0f));
    //distanceFactor = 100.0f;
	result = mix(distcolor,result,min(1.0f,distanceFactor*1.3f));
    //result.x *= multiplier;
	result = lab2rgb(result);
	//result *= multiplier;
	result.x = max(0.0f,result.x);
	result.y = max(0.0f,result.y);
	result.z = max(0.0f,result.z);
	color =  result*0.25f;
}

void main(void)
{

	// 1.0f in the source would mean 10,000 nits. 
	// Let's assume 400 nits for a typical gaming monitor (so the target for 1.0f from source buffer)
	// 400/10000 = 0.04f				
	vec3 inputColorTmp = textureBicubic(text_in, gl_TexCoord[0].st,0).xyz;
	vec3 inputColorTmpBlurred = inputColorTmp;
    for(int i=1;i<5;i++){
        vec3 newval = textureBicubic(text_in, gl_TexCoord[0].st,i).xyz;
        if(thermalVisionUniform ==4){
            inputColorTmpBlurred += newval;
        } else{
            inputColorTmpBlurred = mix(inputColorTmpBlurred,newval,fracs[i]);
        }
    }
    if(thermalVisionUniform ==2||thermalVisionUniform ==3){
        inputColorTmp = inputColorTmpBlurred;
        applyThermal(inputColorTmp);
        //inputColorTmp.xyz *= 2.0f*gaussian_rand(gl_TexCoord[0].st);
        inputColorTmp.xyz *= 2.0f*clamp(gaussian_rand(gl_TexCoord[0].st),0.1f,10.0f);
    } else if(thermalVisionUniform ==4){
        //applyThermal(inputColorTmp);
        //inputColorTmp.xyz = vec3(inputColorTmp.z);
        inputColorTmp.xyz = vec3(inputColorTmp.y*2.0f+inputColorTmpBlurred.z);
        inputColorTmp.xz *= 0.7f;
        //inputColorTmp.xyz *= noise1(gl_FragCoord.x*10000.0f);
        inputColorTmp.xyz *= 2.0f*clamp(gaussian_rand(gl_TexCoord[0].st),0.1f,10.0f);
        //inputColorTmp.xyz *= 2.0f*gaussian_rand(gl_TexCoord[0].st);
    }

	gl_FragColor = vec4(inputColorTmp,1.0f);
    
}