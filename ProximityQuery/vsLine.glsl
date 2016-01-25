#version 120
attribute highp vec4 vertexPosition;
attribute highp vec4 vertexColor;

varying highp vec4 color;

void main(void)
{
    gl_Position = vertexPosition;

    color.xyz	= vertexColor.xyz;
    color.w	= 0.0;
}