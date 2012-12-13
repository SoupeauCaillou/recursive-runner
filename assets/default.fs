#ifdef GL_ES
precision lowp float;
#endif
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform vec4 vColor;

varying vec2 uvVarying;

void main()
{
    vec3 rgb = texture2D(tex0, uvVarying).rgb * vColor.rgb;
    gl_FragColor = vec4(rgb, texture2D(tex1, uvVarying).a * vColor.a);
    gl_FragColor.b = 0.0;
    gl_FragColor.a += 0.3;
}

