attribute vec3 aPosition;
attribute vec2 aTexCoord; 

uniform mat4 uMvp;
uniform vec4 uvScaleOffset;
uniform float uRotation;
uniform vec2 uScale;

varying vec2 uvVarying;

void main()
{
	float r = uRotation;
	mat4 rot;
	rot[0] = vec4(cos(r), -sin(r), 0.0, 0.0);
	rot[1] = vec4(sin(r), cos(r), 0.0, 0.0);
	rot[2] = vec4(0.0, 0.0, 1.0, 0.0);
	rot[3] = vec4(0.0, 0.0, 0.0, 1.0);
	gl_Position = (uMvp * rot) * vec4(aPosition.xy * uScale, aPosition.z, 1.0);
	uvVarying = uvScaleOffset.zw + aTexCoord * uvScaleOffset.xy;
	uvVarying.y = 1.0 - uvVarying.y;
}

