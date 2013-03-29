attribute vec3 aPosition;
attribute vec2 aTexCoord; 

uniform float uMvp[6];

varying vec2 uvVarying;

void main()
{	
    vec3 camera = vec3(10.0, 0.0, 0.5);
	mat4 worldToCamRot = mat4(
		vec4(cos(-camera.z), -sin(-camera.z), 0.0, 0.0),
		vec4(sin(-camera.z), cos(-camera.z), 0.0, 0.0),
		vec4(0.0, 0.0, 1, 0.0),
		vec4(0, 0, 0, 1.0));
    mat4 worldToCamTrans = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(-camera.xy, 0.0, 1.0));
	mat4 mvp = mat4(
		vec4(uMvp[0], 0.0, 0.0, 0.0),
		vec4(0.0, uMvp[1], 0.0, 0.0),
		vec4(0.0, 0.0, uMvp[2], 0.0),
		vec4(uMvp[3], uMvp[4], uMvp[5], 1.0));
	gl_Position = mvp * worldToCamTrans * worldToCamRot * vec4(aPosition, 1.0);
	uvVarying = aTexCoord;
}

