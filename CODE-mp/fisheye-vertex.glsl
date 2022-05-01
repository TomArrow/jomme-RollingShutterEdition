uniform vec3 originUniform;
uniform vec3 axisUniform[3];

float angleOnPlane(vec3 point, vec3 axis1, vec3 axis2){
	
	vec2 planePosition = vec2( dot(point,axis1), dot(point,axis2));
	return acos(dot(vec2(1,0),normalize(planePosition)));
}

vec3 getPerpendicularAxis(vec3 point, vec3 mainAxis){
	return point;// - dot(point,mainAxis)*mainAxis;
}

void main(void)
{   
  vec4 nabcormaltransform = ftransform();
  vec4 normaltransform = vec4(1.0,1.0,1.0,1.0);

  float pi =radians(180);

  //gl_Position = ftransform(gl_Vertex);
  //gl_Position = transform( gl_ModelViewProjectionMatrix, gl_Vertex );
  //gl_Position.x *= (1-(abs(gl_Position.y)/540.0))*0.5+0.5;
  //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

  //vec4 test = gl_Vertex;
  //test.xyz += axisUniform[0].xyz*100;
  //gl_Position = gl_ModelViewProjectionMatrix * test;
  vec3 pointVec = originUniform-gl_Vertex.xyz;
  vec4 test;
  //test.x = dot(axisUniform[1].xyz,pointVec);
  //test.y = dot(-axisUniform[0].xyz,pointVec);
  //test.z = dot(axisUniform[2].xyz,pointVec);

  // Kinda iso perspective:
 // test.x = dot(axisUniform[1].xyz,pointVec);
  //test.y = dot(-axisUniform[2].xyz,pointVec);
  //test.z = dot(-axisUniform[0].xyz,pointVec);
  //test.xyz *= 0.005;
  //test.z *= 0.001;

  float distance = length(pointVec);
  pointVec = normalize(pointVec);
  float depth = dot(axisUniform[0].xyz,pointVec);
  float height = dot(axisUniform[2].xyz,pointVec);
  float heightSign = sign(height);
  float width = dot(-axisUniform[1].xyz,pointVec);
  float widthSign = sign(width);

  //float xAngle = acos(width)/pi;
  float xAngle = angleOnPlane(pointVec,-axisUniform[1].xyz,axisUniform[0].xyz)/3.14;
  
  //float yAngle = acos(height)/pi;
  vec3 perpendicularAxis = getPerpendicularAxis(pointVec,axisUniform[0].xyz);
  float yAngle = angleOnPlane(pointVec,axisUniform[2].xyz,perpendicularAxis)/pi;

  //xAngle = depth< 0 ? xAngle : widthSign*1.0+widthSign*(1.0-abs(xAngle));
  xAngle -= 0.5;
  xAngle *= 2;
  widthSign = sign(xAngle);
  xAngle = depth<=0 ? xAngle : widthSign*(1.0+(1.0-abs(xAngle)));
  //xAngle = depth< 0 ? xAngle : widthSign*(1.0+abs(xAngle));

  yAngle -= 0.5;
  yAngle *= 2;
  heightSign = sign(yAngle);
  //yAngle = depth< 0 ? yAngle : heightSign*(1.0+(1.0-abs(yAngle)));
  //yAngle = depth< 0 ? yAngle : -heightSign;
 // yAngle = depth< 0 ? yAngle : heightSign*(1.0+abs(yAngle));

  xAngle *= 0.5;
  //yAngle *= 0.5;

  // Kinda iso perspective:
  //test.x = acos(dot(axisUniform[1].xyz,pointVec));
  //test.x = normaltransform.x;
  //test.x = depth <0 ? (xAngle*normaltransform.w-normaltransform.w/2)*2 : widthSign*normaltransform.w;
  test.x = (xAngle*normaltransform.w);
  //test.y = depth <0 ? (yAngle*normaltransform.w)*2 : heightSign*normaltransform.w;
  test.y = yAngle*normaltransform.w*2;
  //test.y = normaltransform.y*0.5;
  //test.z = depth<0? normaltransform.z: -normaltransform.z;
  //test.z = nabcormaltransform.w-length(pointVec);
  test.z = 1.0-1.0/distance;
  test.w= normaltransform.w;
  gl_Position = test;

  gl_TexCoord[0] = gl_MultiTexCoord0;
}
