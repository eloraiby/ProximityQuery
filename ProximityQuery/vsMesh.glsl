#version 120
attribute highp vec3 vertexPosition;
attribute highp vec3 vertexNormal;
attribute highp vec4 vertexColor;

attribute highp float faceId;
attribute highp float mtlId;

uniform highp mat4 projViewModel;
//uniform highp vec3 eyePosition;
uniform highp vec3 lightPosition;
uniform highp vec4 lightColor;
uniform highp vec4 ambientColor;

varying highp vec4 weightColor;
varying highp vec4 fmMultiplier;

varying highp vec4 color;

void main(void)
{
    vec3 P = vertexPosition;
    vec3 N = vertexNormal;
    vec3 L = normalize(P - lightPosition);
    gl_Position = projViewModel * vec4(P, 1.0);

    float diffuseIntensity = max(dot(L, N), 0);
    color.xyz	= lightColor.xyz * diffuseIntensity + vertexColor.xyz * ambientColor.xyz;
    color.w	= 0.0;
}