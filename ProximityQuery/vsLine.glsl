#version 120
attribute highp vec3 vertexPosition;
attribute highp vec4 vertexColor;


uniform highp mat4 projViewModel;

varying highp vec4 color;

void main(void)
{
    vec3 P = vertexPosition;
    gl_Position = projViewModel * vec4(P, 1.0);

    color.xyz	= vertexColor.xyz;
    color.w	= 0.0;
}