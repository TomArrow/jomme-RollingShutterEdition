#version 400 compatibility
#extension GL_ARB_shader_storage_buffer_object : enable

#define TEXTURE_COUNT 14
#define VEC3_ATTRIBUTE_COUNT 3

// Geometry Shader
#extension GL_ARB_geometry_shader4 : enable
//in float realDepth[3];
float realDepth[3]; in vec4 color[3];
varying out vec4 vertColor;
varying out vec3 lightDir;
varying out vec3 ambientLight;
varying out vec3 vertexNormal;
//varying out vec3 worldVertexNormal;
out vec3 debugColor;
out vec3 texUVTransform[2];

//in vec4 geomTexCoord[3];
in geomTexCoord_interface {
	vec4 coord[TEXTURE_COUNT/2];
} geomTexCoord[];
in geomTexAttr_interface {
	vec3 attribs[VEC3_ATTRIBUTE_COUNT];
} geomTexAttr[];

in vec4 gl_TexCoordIn[3][1];

in mat4x4 projectionMatrix[3];

uniform mat4x4 worldModelViewMatrixUniform;

in mat4x4 worldModelViewMatrixReverse[3];
out mat4x4 worldModelViewMatrixReverseGeom;

out vec3 normal;
out vec3 worldNormal;

out varying vec2 my_TexCoord[TEXTURE_COUNT];

#define SETATTRIBS lightDir = geomTexAttr[i].attribs[0];ambientLight = geomTexAttr[i].attribs[1];vertexNormal = geomTexAttr[i].attribs[2];

//#define SETTEXCOORDS gl_TexCoord[0] = geomTexCoord[i].coord[0];gl_TexCoord[1] = geomTexCoord[i].coord[1];gl_TexCoord[2] = geomTexCoord[i].coord[2];gl_TexCoord[3] = geomTexCoord[i].coord[3];gl_TexCoord[4] = geomTexCoord[i].coord[4];gl_TexCoord[5] = geomTexCoord[i].coord[5];
//#define SETTEXCOORDS my_TexCoord[0] = geomTexCoord[i].coord[0];my_TexCoord[1] = geomTexCoord[i].coord[1];my_TexCoord[2] = geomTexCoord[i].coord[2];my_TexCoord[3] = geomTexCoord[i].coord[3];my_TexCoord[4] = geomTexCoord[i].coord[4];my_TexCoord[5] = geomTexCoord[i].coord[5];
#define SETTEXCOORDS for(int c=0;c<TEXTURE_COUNT;c+=2){my_TexCoord[c] = geomTexCoord[i].coord[c/2].st;my_TexCoord[c+1] = geomTexCoord[i].coord[c/2].zw;}

in vec4 eyeSpaceCoords[3];
varying out vec4 eyeSpaceCoordsGeom;

in vec4 pureVertexCoords[3];
varying out vec4 pureVertexCoordsGeom;

uniform vec3 pixelJitterUniform;
uniform vec3 dofJitterUniform;
uniform float dofFocusUniform;
uniform float dofRadiusUniform;
uniform int fishEyeModeUniform; //1= fisheye, 2=equirectangular
uniform float fovXUniform;
uniform float fovYUniform;
uniform int pixelWidthUniform;
uniform int pixelHeightUniform;
uniform int isWorldBrushUniform; 
uniform int jitterIndexUniform; 
uniform int jitterTotalFramesUniform;

float simpleJitter() {
	if(jitterTotalFramesUniform == 0){
		return 1.0f;
	}
	int segment = jitterIndexUniform % 3;
	int progress = jitterIndexUniform / 3;
	float progressMult = 1.0f / float(jitterTotalFramesUniform);
	float progressHere = float(segment) / 3.0f + float(progress) * progressMult;
	return progressHere;
}

vec2 getsideOutVec(vec2 pos0, vec2 pos1, vec2 pos2){
	vec2 sideout = (pos1-pos0).yx;
	sideout.y = -sideout.y;
	sideout = normalize(sideout);
	sideout *= -sign( dot(sideout,pos2-pos0));
	//if(dot(sideout,sideout) == 0.0f) return vec2(0.0f);
	//sideout = dot(sideout,pos2-pos0) > 0 ? -sideout : sideout;
	return sideout;
}

void jitterPixelPosMissingCorner(inout mat4 pixelPos, float jitterMult){
	
	float smallestW = min(min(pixelPos[0].w,pixelPos[1].w),pixelPos[2].w);

	mat4 outPos = pixelPos;

	float width = float(pixelWidthUniform);
	float height = float(pixelHeightUniform);

	for(int i=0;i<3;i++){
		if(pixelPos[i].w <= 0){
			continue;
		}
		int lastIndex = i > 0 ? i-1 : 2;
		int nextIndex = i == 2 ? 0 : i+1;

		// scale down the triangle
		float scaleFactor = 1.0f/(pixelPos[i].w-smallestW)*0.5f; // do 0.5f for safety that's all, so we're definitely in thhe safe zone and not approaching division by 0 etc
		mat4 scaledPos = pixelPos;
		scaledPos[lastIndex] = scaledPos[i] + (scaledPos[i] - scaledPos[lastIndex]) * scaleFactor;
		scaledPos[nextIndex] = scaledPos[i] + (scaledPos[i] - scaledPos[nextIndex]) * scaleFactor;
		
		vec2 d[3];
		d[0] = abs(vec2(2.0f / width * scaledPos[0].w,2.0f / height * scaledPos[0].w));
		d[1] = abs(vec2(2.0f / width * scaledPos[1].w,2.0f / height * scaledPos[1].w));
		d[2] = abs(vec2(2.0f / width * scaledPos[2].w,2.0f / height * scaledPos[2].w));
		
		vec2 side1out = getsideOutVec(scaledPos[lastIndex].xy/d[lastIndex],scaledPos[i].xy/d[i],scaledPos[nextIndex].xy/d[nextIndex]);
		vec2 side2out = getsideOutVec(scaledPos[i].xy/d[i],scaledPos[nextIndex].xy/d[nextIndex],scaledPos[lastIndex].xy/d[lastIndex]);

		outPos[i].xy -= d[i]*jitterMult*(side1out + side2out);
	}

	pixelPos = outPos;
}

void jitterPixelPos(inout mat4 pixelPos){
	float jittermult = 5.0f;//*simpleJitter();
	if(jitterTotalFramesUniform == 0 || isWorldBrushUniform == 0){
		return;
	} else{
		jittermult = 1.0f*simpleJitter();
	}
	// first revert the general pixel jitter
	//pixelPos[0].xy -= pixelJitterUniform.xy;
	//pixelPos[1].xy -= pixelJitterUniform.xy;
	//pixelPos[2].xy -= pixelJitterUniform.xy;
	int missingCorners = int(pixelPos[0].w <= 0) + int(pixelPos[1].w <= 0) + int(pixelPos[2].w <= 0);
	
	

	if( missingCorners == 3){
		return;
	} else if( missingCorners == 2){
		// sad but idk its too hard to figure out for me.
		jitterPixelPosMissingCorner(pixelPos,jittermult);
		return;
	} else if( missingCorners == 1){
		// sad but idk its too hard to figure out for me.
		jitterPixelPosMissingCorner(pixelPos,jittermult);
		return;
	}

	//return;

	float width = float(pixelWidthUniform);
	float height = float(pixelHeightUniform);

	vec2 d0 = abs(vec2(2.0f / width * pixelPos[0].w,2.0f / height * pixelPos[0].w));
	vec2 d1 = abs(vec2(2.0f / width * pixelPos[1].w,2.0f / height * pixelPos[1].w));
	vec2 d2 = abs(vec2(2.0f / width * pixelPos[2].w,2.0f / height * pixelPos[2].w));

	// calculate vecs pointing outward of the triangle
	vec2 side1out = getsideOutVec(pixelPos[0].xy/d0,pixelPos[1].xy/d1,pixelPos[2].xy/d2);
	vec2 side2out = getsideOutVec(pixelPos[1].xy/d1,pixelPos[2].xy/d2,pixelPos[0].xy/d0);
	vec2 side3out = getsideOutVec(pixelPos[2].xy/d2,pixelPos[0].xy/d0,pixelPos[1].xy/d1);
	//vec2 side1out = getsideOutVec(pixelPos[0].xy/pixelPos[0].w,pixelPos[1].xy/pixelPos[1].w,pixelPos[2].xy/pixelPos[2].w);
	//vec2 side2out = getsideOutVec(pixelPos[1].xy/pixelPos[1].w,pixelPos[2].xy/pixelPos[2].w,pixelPos[0].xy/pixelPos[0].w);
	//vec2 side3out = getsideOutVec(pixelPos[2].xy/pixelPos[2].w,pixelPos[0].xy/pixelPos[0].w,pixelPos[1].xy/pixelPos[1].w);
	//vec2 side1out = normalize((pixelPos[1].xy-pixelPos[0].xy).yx);
	//side1out = dot(side1out,pixelPos[2].xy-pixelPos[0].xy) > 0 ? -side1out : side1out;
	//vec2 side2out = normalize((pixelPos[2].xy-pixelPos[1].xy).yx);
	//side2out = dot(side2out,pixelPos[0].xy-pixelPos[1].xy) > 0 ? -side2out : side2out;
	//vec2 side3out = normalize((pixelPos[0].xy-pixelPos[2].xy).yx);
	//side3out = dot(side3out,pixelPos[1].xy-pixelPos[2].xy) > 0 ? -side3out : side3out;


	pixelPos[0].xy += d0*jittermult*(side1out + side3out);
	pixelPos[1].xy += d1*jittermult*(side1out + side2out);
	pixelPos[2].xy += d2*jittermult*(side2out + side3out);

	//pixelPos[0].x += dx0*50.0f; 
	//pixelPos[1].x += dx1*50.0f; 
	//pixelPos[2].x += dx2*50.0f; 
}

uniform float serverTimeUniform;
uniform float soundDeformTimeUniform;
uniform float soundDeformIntensityUniform;
uniform float soundDeformDistanceScaleUniform;
uniform float soundDeformShortDistanceReductionUniform;
uniform float soundDeformSpreadSpeedUniform;
uniform int soundDeformSampleAvgWidthUniform;
uniform int soundDeformModeUniform; // 1= Z axis, 2 = normal, 3= direct view, 4= normal with direct view scale
uniform vec3 soundDeformOriginUniform;
uniform int soundDeformSampleRateUniform;
uniform int soundDeformSampleCountUniform;

layout(std430, binding = 4) buffer soundDeformSampleLayout
{
    vec4 soundDeformSamples[]; // theyre actually shorts but glsl doesnt knoww what that is :) we just shift things around
};

//
//
// General stuff
//
//
float angleOnPlane(vec3 point, vec3 axis1, vec3 axis2)
{

	vec2 planePosition = vec2(dot(point, axis1), dot(point, axis2));
	return acos(dot(vec2(1, 0), normalize(planePosition)));
}

vec3 getPerpendicularAxis(vec3 point, vec3 mainAxis)
{
	return normalize(point - dot(point, mainAxis) *mainAxis);
}


float getNormalSign(vec3 observer,vec3 triangle[3]){
    vec3 pseudoAxis  = normalize(triangle[1]-triangle[0]);
	vec3 pseudoPoint = normalize(triangle[2]-triangle[0]);
	vec3 relativeObserver = observer - triangle[0];
	vec3 rightAngleAxisToThirdPoint= getPerpendicularAxis(pseudoPoint,pseudoAxis);
	vec3 normal = cross(rightAngleAxisToThirdPoint,pseudoAxis);
	return sign(dot(normal,relativeObserver));
}


void setDebugColor(float x, float y, float z)
{
	debugColor = vec3(max(0.0,min(1.0,x)),max(0.0,min(1.0,y)),max(0.0,min(1.0,z)));
}

float calcDofFactor(float pointDistance, float dofFocus)
{
	return abs(pointDistance - dofFocus);	// /pointDistance;
}

vec3 generateTransformationMatrixRow(in vec3 vec1i, in vec3 vec2i, in vec3 vec3i, float resultValue1, float resultValue2, float resultValue3){
	vec3 transformVec;
	
	transformVec.x = (vec2i.z * vec3i.y * resultValue1 - vec2i.y * vec3i.z * resultValue1 - vec1i.z * vec3i.y * resultValue2 + vec1i.y * vec3i.z * resultValue2 + vec1i.z * vec2i.y * resultValue3 - vec1i.y * vec2i.z * resultValue3) / (vec1i.z * vec2i.y * vec3i.x - vec1i.y * vec2i.z * vec3i.x - vec1i.z * vec2i.x * vec3i.y + vec1i.x * vec2i.z * vec3i.y + vec1i.y * vec2i.x * vec3i.z - vec1i.x * vec2i.y * vec3i.z);
	transformVec.y = (-vec2i.z * vec3i.x * resultValue1 + vec2i.x * vec3i.z * resultValue1 + vec1i.z * vec3i.x * resultValue2 - vec1i.x * vec3i.z * resultValue2 - vec1i.z * vec2i.x * resultValue3 + vec1i.x * vec2i.z * resultValue3) / (vec1i.z * vec2i.y * vec3i.x - vec1i.y * vec2i.z * vec3i.x - vec1i.z * vec2i.x * vec3i.y + vec1i.x * vec2i.z * vec3i.y + vec1i.y * vec2i.x * vec3i.z - vec1i.x * vec2i.y * vec3i.z);
	transformVec.z = (-vec2i.y * vec3i.x * resultValue1 + vec2i.x * vec3i.y * resultValue1 + vec1i.y * vec3i.x * resultValue2 - vec1i.x * vec3i.y * resultValue2 - vec1i.y * vec2i.x * resultValue3 + vec1i.x * vec2i.y * resultValue3) / (-vec1i.z * vec2i.y * vec3i.x + vec1i.y * vec2i.z * vec3i.x + vec1i.z * vec2i.x * vec3i.y - vec1i.x * vec2i.z * vec3i.y - vec1i.y * vec2i.x * vec3i.z + vec1i.x * vec2i.y * vec3i.z);
	
	return transformVec;
}

// Calculates matrix from 2 pairs of vectors that transforms the i vec3 ones to the o vec2 ones.
void makeUVTransformationMatrix(in vec3 vec1i, in vec2 vec1o, in vec3 vec2i, in vec2 vec2o, in vec3 vec3i, in vec2 vec3o, inout vec3 matrix[2]){
	
	matrix[0] = generateTransformationMatrixRow(vec1i,vec2i,vec3i,vec1o.s,vec2o.s,vec3o.s);
	matrix[1] = generateTransformationMatrixRow(vec1i,vec2i,vec3i,vec1o.t,vec2o.t,vec3o.t);

}

//
//
// Standard stuff 
//
//
void standard(vec3 myNormal){
	
	int musicDeformSampleCount = soundDeformSampleCountUniform;//soundDeformSamples.length()*2; 

	mat4 outPos;

	//setDebugColor(1,0,0);
	for (int i = 0; i < 3; i++)
	{
		vec4 positionAdjustment = vec4(0.0);
		//vec3 colorAdd = vec3(0.0);
		//float redAdd = 0.0;
		if(musicDeformSampleCount>0 && isWorldBrushUniform > 0){
			// TODO implement soundDeformSampleAvgWidthUniform
			vec3 eyeSpaceSoundDeformOrigin = (worldModelViewMatrixUniform*vec4(soundDeformOriginUniform,1.0)).xyz;
			vec3 pointToOrigin = eyeSpaceSoundDeformOrigin-eyeSpaceCoords[i].xyz;
			float distanceToPoint = length(pointToOrigin);
			//redAdd+= distanceToPoint/1000.0f;
			//colorAdd += eyeSpaceSoundDeformOrigin/1000.0f;
			float samplePosition = (float(soundDeformTimeUniform))*float(soundDeformSampleRateUniform)-(distanceToPoint/soundDeformSpreadSpeedUniform*float(soundDeformSampleRateUniform));
			int realSamplePos = samplePosition< 0 ? 0: int(samplePosition)%musicDeformSampleCount;
			vec4 vecAtPos = soundDeformSamples[realSamplePos/4]; // gotta work with vec4 due to alignment
			float actualValue =vecAtPos[realSamplePos % 4];
			float shortDistanceRatio = clamp((distanceToPoint/soundDeformShortDistanceReductionUniform),0.0,1.0);
			float deformIntensity = soundDeformIntensityUniform*float(actualValue)/32767.0*(1.0+soundDeformDistanceScaleUniform*distanceToPoint/1000.0f)*shortDistanceRatio*shortDistanceRatio;
			switch(soundDeformModeUniform){
				default:
				case 1: // Z axis
					vec4 worldPos = (worldModelViewMatrixReverse[0]*eyeSpaceCoords[i]);
					worldPos.z += deformIntensity;
					worldPos.w = 1.0;
					worldPos = worldModelViewMatrixUniform*worldPos;
					positionAdjustment = worldPos-gl_PositionIn[i];
					positionAdjustment.w = 0.0;
				break;
				case 2: // normal
					positionAdjustment = vec4(myNormal*deformIntensity,0.0);
				break;
				case 3: // direct view
					positionAdjustment = vec4(-normalize(pointToOrigin)*deformIntensity,0.0);
				break;
				case 4: // normal with direct view scale
					positionAdjustment = vec4(-deformIntensity*myNormal*dot(normalize(pointToOrigin),myNormal),0.0);
				break;
				case 5: // mixture of 1 and 4
					vec4 worldPos2 = (worldModelViewMatrixReverse[0]*eyeSpaceCoords[i]);
					worldPos2.z += deformIntensity;
					worldPos2.w = 1.0;
					worldPos2 = worldModelViewMatrixUniform*worldPos2;
					positionAdjustment = worldPos2-gl_PositionIn[i];
					positionAdjustment.w = 0.0;
					positionAdjustment += vec4(-deformIntensity*myNormal*dot(normalize(pointToOrigin),myNormal),0.0);
				break;
				case 6: // deform on all 3 axes in direction of movement, sorta. meh? ( doesnt work)
					vec4 worldPos3 = (worldModelViewMatrixReverse[0]*eyeSpaceCoords[i]);
					vec3 originToPointWorldNormal = normalize(worldPos3.xyz-soundDeformOriginUniform);
					worldPos3.x += deformIntensity * sign(originToPointWorldNormal.x);
					worldPos3.y += deformIntensity * sign(originToPointWorldNormal.y);
					worldPos3.z += deformIntensity * sign(originToPointWorldNormal.z);
					worldPos3.w = 1.0;
					worldPos3 = worldModelViewMatrixUniform*worldPos3;
					positionAdjustment = worldPos-gl_PositionIn[i];
					positionAdjustment.w = 0.0;
				break;
				case 7: // mixture of 1 and 3
					vec4 worldPos4 = (worldModelViewMatrixReverse[0]*eyeSpaceCoords[i]);
					worldPos4.z += deformIntensity;
					worldPos4.w = 1.0;
					worldPos4 = worldModelViewMatrixUniform*worldPos4;
					positionAdjustment = worldPos4-gl_PositionIn[i];
					positionAdjustment.w = 0.0;
					positionAdjustment += vec4(-normalize(pointToOrigin)*deformIntensity,0.0);
				break;
			}
		}
		outPos[i] = projectionMatrix[0]* (gl_PositionIn[i]+positionAdjustment);
	}

	//jitterPixelPos(outPos); // "intelligent" antialias geometry jitter?

	for (int i = 0; i < 3; i++)
	{
		gl_Position = outPos[i];
		SETTEXCOORDS
		eyeSpaceCoordsGeom = eyeSpaceCoords[i];
		pureVertexCoordsGeom = pureVertexCoords[i];

		vertColor = color[i];
		SETATTRIBS
		//vertColor.xyz += colorAdd;
		//vertColor.x += redAdd;
		EmitVertex();
	}
	

	EndPrimitive();

}



//
//
// Equirectangular stuff.
//
//
vec4 equirect_getPos(vec3 pointVec, int iter)
{

	float pi = radians(180);

	vec3 axis[3];
	axis[0] = vec3(0.0, 0.0, -1.0);
	axis[1] = vec3(-1.0, 0.0, 0.0);
	axis[2] = vec3(0.0, 1.0, 0.0);

	float distance = length(pointVec);

	vec3 perpendicularZAxisToPoint = getPerpendicularAxis(normalize(pointVec), axis[2].xyz);

	// Apply DOF jitter
	float dofFactor = calcDofFactor(distance, dofFocusUniform) *dofRadiusUniform *0.001;	// 0.001 to get it roughly in line with normal dof in mme. 
	if(dofFactor != 0.0){
		vec3 perpendicularXAxisToPoint = cross(axis[2].xyz, perpendicularZAxisToPoint);
		pointVec.x += dofFactor* dot(dofJitterUniform, perpendicularXAxisToPoint);
		pointVec.y += dofFactor* dot(dofJitterUniform, axis[2].xyz);
		pointVec.z += dofFactor* dot(dofJitterUniform, perpendicularZAxisToPoint);
		pointVec = normalize(pointVec);
		perpendicularZAxisToPoint = getPerpendicularAxis(pointVec, axis[2].xyz); // Calculate it again because position has changed through jitter.
	} else {
		pointVec = normalize(pointVec);
	}


	vec4 outputPos;

	float depth = dot(axis[0].xyz, pointVec);
	realDepth[iter] = depth;

	float xAngle = angleOnPlane(pointVec, -axis[1].xyz, axis[0].xyz) / pi;

	float yAngle = angleOnPlane(pointVec, axis[2].xyz, perpendicularZAxisToPoint) / pi;

	xAngle -= 0.5;
	xAngle *= 2;
	float widthSign = sign(xAngle);
	xAngle = depth <= 0 ? xAngle : widthSign *(1.0 + (1.0 - abs(xAngle)));
	xAngle *= 0.5;

	yAngle -= 0.5;
	yAngle *= 2;
	float heightSign = sign(yAngle);

	outputPos.x = xAngle;
	outputPos.y = yAngle;
	outputPos.z = 1.0 - 1.0 / distance;
	outputPos.w = 1.0;
	
	// This is pixel jitter. It's applied for atni-aliasing, hence it's the last step and applied onto actual screen coordinates
	outputPos.x += pixelJitterUniform.x;
	outputPos.y += pixelJitterUniform.y;
	outputPos.z += pixelJitterUniform.z;

	return outputPos;
}



void equirect(){
	bool someInBack = false;
	bool someLeft = false;
	bool someRight = false;

	vec4 positions[3];
	for (int i = 0; i < 3; i++)
	{
		positions[i] = equirect_getPos(-gl_PositionIn[i].xyz, i);
	}

	for (int i = 0; i < 3; i++)
	{
		if (realDepth[i] > 0) someInBack = true;
		if (positions[i].x <= 0) someLeft = true;
		if (positions[i].x > 0) someRight = true;
	}

	bool wrappedAround = false;
	if (someInBack && someLeft && someRight)
	{
		wrappedAround = true;
	}

	if (!wrappedAround)
	{
		for (int i = 0; i < 3; i++)
		{
			gl_Position = positions[i];
			//gl_TexCoord[0] = gl_TexCoordIn[i][0];
			SETTEXCOORDS
			vertColor = color[i];
			SETATTRIBS
			
			eyeSpaceCoordsGeom = eyeSpaceCoords[i];
			pureVertexCoordsGeom = pureVertexCoords[i];
			EmitVertex();
		}
		EndPrimitive();
	}
	else
	{
		// Emit 2 separate vertices.
		for (int i = 0; i < 3; i++)
		{
			vec4 thisPosition = positions[i];
			if (realDepth[i] > 0)
			{
				if (thisPosition.x <= 0) thisPosition.x += 2.0;
			}
			gl_Position = thisPosition;
			//gl_TexCoord[0] = gl_TexCoordIn[i][0];
			SETTEXCOORDS
			vertColor = color[i];
			SETATTRIBS
			
			eyeSpaceCoordsGeom = eyeSpaceCoords[i];
			pureVertexCoordsGeom = pureVertexCoords[i];
			EmitVertex();
		}
		EndPrimitive();
		for (int i = 0; i < 3; i++)
		{
			vec4 thisPosition = positions[i];
			if (realDepth[i] > 0)
			{
				if (thisPosition.x > 0) thisPosition.x -= 2.0;
			}
			gl_Position = thisPosition;
			//gl_TexCoord[0] = gl_TexCoordIn[i][0];
			SETTEXCOORDS
			vertColor = color[i];
			SETATTRIBS
			
			eyeSpaceCoordsGeom = eyeSpaceCoords[i];
			pureVertexCoordsGeom = pureVertexCoords[i];
			EmitVertex();
		}
		EndPrimitive();
	}
}

//
//
// Real fisheye stuff
//
//
vec4 fisheye_getPos(vec3 pointVec, int iter, inout vec2 fovless2DPos)
{

	float pi = radians(180);

	vec3 axis[3];
	axis[0] = vec3(0.0, 0.0, -1.0);
	axis[1] = vec3(-1.0, 0.0, 0.0);
	axis[2] = vec3(0.0, 1.0, 0.0);

	float distance = length(pointVec);

	vec3 perpendicularZAxisToPoint = getPerpendicularAxis(normalize(pointVec), axis[2].xyz); 

	// Apply DOF jitter
	float dofFactor = calcDofFactor(distance, dofFocusUniform) *dofRadiusUniform *0.001;	// 0.001 to get it roughly in line with normal dof in mme. 
	vec3 perpendicularXAxisToPoint = cross(axis[2].xyz, perpendicularZAxisToPoint);
	// old faulty way:
	//pointVec.x += dofFactor* dot(dofJitterUniform, perpendicularXAxisToPoint);
	//pointVec.y += dofFactor* dot(dofJitterUniform, axis[2].xyz);
	//pointVec.z += dofFactor* dot(dofJitterUniform, perpendicularZAxisToPoint);
	// This might still be wrong, not sure.  (yeah it doesnt angle vertically i think)
	//pointVec += dofFactor * dofJitterUniform.x * perpendicularXAxisToPoint;
	//pointVec += dofFactor * dofJitterUniform.y * axis[2].xyz;
	//pointVec += dofFactor * dofJitterUniform.z * perpendicularZAxisToPoint;
	// Hmm the getperpendicularaxis might actually not work if the two axes supplied are identical. Might result in Null vector. Do we just ignore that for now? Hmm
	vec3 pointPseudoZAxis = normalize(pointVec);
	vec3 pseudoYAxisToPoint = getPerpendicularAxis(axis[2].xyz, pointPseudoZAxis); 
	vec3 pseudoXAxisToPoint = cross(pointPseudoZAxis,pseudoYAxisToPoint);
	pointVec += dofFactor * dofJitterUniform.x * pseudoXAxisToPoint;
	pointVec += dofFactor * dofJitterUniform.y * pseudoYAxisToPoint;
	pointVec += dofFactor * dofJitterUniform.z * pointPseudoZAxis;

	pointVec = normalize(pointVec);

	vec4 outputPos;

	float depth = dot(axis[0].xyz, pointVec);
	realDepth[iter] = depth;

	vec3 perpendicularRotatedAxisToPoint = getPerpendicularAxis(pointVec, axis[0].xyz);
	float angleFromCenter = angleOnPlane(pointVec, -axis[0].xyz, perpendicularRotatedAxisToPoint)/pi; // Distance from center, basically.
	vec2 pseudoAngleOnXYplane = vec2(dot(pointVec, -axis[1].xyz),dot(pointVec, axis[2].xyz));


	pseudoAngleOnXYplane = normalize(pseudoAngleOnXYplane);


	vec2 position = pseudoAngleOnXYplane*angleFromCenter;
	fovless2DPos = position;
	float positionX = -position.x*360.0/fovXUniform/2.0;
	//float positionY = -position.y*360.0/fovYUniform/2.0; // TODO do a proper fix to make it use the fovY so it will be correct underwater etc. It's calculated in a weird way tho.
	float positionY = -position.y*360.0/fovXUniform/pixelHeightUniform*pixelWidthUniform/2.0;

	outputPos.x = positionX;
	outputPos.y = positionY;
	outputPos.z = 1.0 - 1.0 / distance;
	outputPos.w = 1.0;

	// This is pixel jitter. It's applied for atni-aliasing, hence it's the last step and applied onto actual screen coordinates
	outputPos.x += pixelJitterUniform.x;
	outputPos.y += pixelJitterUniform.y;
	outputPos.z += pixelJitterUniform.z;

	return outputPos;
}

void fisheye(){
	int inBack = 0;
	//bool someLeft = false;
	//bool someRight = false;

	vec4 positions[3];
	vec3 originalPositions3[3];
	vec3 positions3[3];
	//vec3 preFisheye2DPos[3];
	vec3 postFisheye2DPos[3];
	for (int i = 0; i < 3; i++)
	{
		originalPositions3[i] = -gl_PositionIn[i].xyz;
		positions[i] = fisheye_getPos(-gl_PositionIn[i].xyz, i,postFisheye2DPos[i].xy);
		positions3[i] = positions[i].xyz;
		//postFisheye2DPos[i] = vec3( positions[i].xy,-1);
		postFisheye2DPos[i].z = -1;
	}

	// Some attempt to get rid of artifacts...
	float longestSidePost = max(max(length(postFisheye2DPos[0].xyz-postFisheye2DPos[1].xyz),length(postFisheye2DPos[1].xyz-postFisheye2DPos[2].xyz)),length(postFisheye2DPos[2].xyz-postFisheye2DPos[0].xyz));


	for (int i = 0; i < 3; i++)
	{
		if (realDepth[i] > 0) inBack++;
	}

	
	//float originalSign = getNormalSign(vec3(0,0,0),originalPositions3);
	//float newSign = getNormalSign(vec3(0,0,0),positions3);

	//setDebugColor(longestSidePost/10,sizeRatio,relativeLongestSide/100);
	bool wrappedAround = false;// originalSign != newSign;// || original2DSign != new2DSign;

	if (!(wrappedAround || (longestSidePost > 1.0f))) // The longest side thing feels like a dirty hack. But it kidna works. And can be further improved with better TCS I think.
	{
		for (int i = 0; i < 3; i++)
		{
			gl_Position = positions[i];
			//gl_TexCoord[0] = gl_TexCoordIn[i][0];
			SETTEXCOORDS
			vertColor = color[i];
			SETATTRIBS
			
			eyeSpaceCoordsGeom = eyeSpaceCoords[i];
			pureVertexCoordsGeom = pureVertexCoords[i];
			EmitVertex();
		}
		EndPrimitive();
	}
}


void main()
{
	/*normal = normalize(cross(normalize(gl_PositionIn[2].xyz-gl_PositionIn[0].xyz),normalize(gl_PositionIn[1].xyz-gl_PositionIn[0].xyz)));

	// Calculate UV vectors (dunno what im doing, i want parallax mapping lol)
	vec3 uvtransform[2];
	makeUVTransformationMatrix(gl_PositionIn[0].xyz,geomTexCoord[0].st,gl_PositionIn[1].xyz,geomTexCoord[1].st,gl_PositionIn[2].xyz,geomTexCoord[2].st,uvtransform);
	texUVTransform= uvtransform;*/
	vec3 myNormal = normalize(cross(normalize(eyeSpaceCoords[2].xyz-eyeSpaceCoords[0].xyz),normalize(eyeSpaceCoords[1].xyz-eyeSpaceCoords[0].xyz)));
	normal = myNormal;

	vec4 worldSpaceCoords0 = worldModelViewMatrixReverse[0]*eyeSpaceCoords[0];
	vec4 worldSpaceCoords1 = worldModelViewMatrixReverse[0]*eyeSpaceCoords[1];
	vec4 worldSpaceCoords2 = worldModelViewMatrixReverse[0]*eyeSpaceCoords[2];
	
	worldNormal = normalize(cross(normalize(worldSpaceCoords2.xyz-worldSpaceCoords0.xyz),normalize(worldSpaceCoords1.xyz-worldSpaceCoords0.xyz)));

	// Calculate UV vectors (dunno what im doing, i want parallax mapping lol)
	vec3 uvtransform[2];
	// TODO in multitexture environment, we need to make sure to use the corret one
	const int maintexnum = 0;
	makeUVTransformationMatrix(eyeSpaceCoords[0].xyz,geomTexCoord[0].coord[maintexnum].st,eyeSpaceCoords[1].xyz,geomTexCoord[1].coord[maintexnum].st,eyeSpaceCoords[2].xyz,geomTexCoord[2].coord[maintexnum].st,uvtransform);
	texUVTransform= uvtransform;


	worldModelViewMatrixReverseGeom = worldModelViewMatrixReverse[0];

// TODO Apply distortion in TES instead? To have more multithreading? And then send it over as in/out variable?
	if(fishEyeModeUniform == 2){
		equirect();
	} else if(fishEyeModeUniform == 1 ){
		fisheye();
	} else {
		standard(myNormal);
	}
}