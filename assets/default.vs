attribute vec3 aPosition;
attribute vec2 aTexCoord; 

uniform float uMvp[6];

varying vec2 uvVarying;

void main()
{	
	mat4 mvp = mat4(
		vec4(uMvp[0], 0.0, 0.0, 0.0),
		vec4(0.0, uMvp[1], 0.0, 0.0),
		vec4(0.0, 0.0, uMvp[2], 0.0),
		vec4(uMvp[3], uMvp[4], uMvp[5], 1.0));
	gl_Position = mvp * vec4(aPosition, 1.0);
	uvVarying = aTexCoord;
}

