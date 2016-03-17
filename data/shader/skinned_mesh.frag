
varying vec3 vNormal;

void main()
{
	float c = dot(normalize(vNormal), normalize(vec3(0, 1, 0))) * 0.5 + 0.5;
	gl_FragColor = vec4(c, c, c, 1);
}

