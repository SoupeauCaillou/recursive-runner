#ifdef GL_ES
precision lowp float;
#endif
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform vec4 vColor;

varying vec2 uvVarying;

void main()
{
    gl_FragColor = vec4(0.0,0.0,0.0,0.0);
}

