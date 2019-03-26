#version 110

uniform mat4 u_MVPMatrix;
uniform mat4 u_MVMatrix;
uniform vec4 u_DiffuseColour;

attribute vec4 position;
attribute vec3 normal;

varying vec3 v_Position;
varying vec3 v_Normal;

varying vec2 v_TextureCoords;
attribute vec2 textureCoords;
uniform sampler2D textureSampler;

void main() {
   // interpolated parameters
   v_TextureCoords = textureCoords;

    // interpolate the eye space position and normal
    v_Position = vec3(u_MVMatrix * position);
    v_Normal = vec3(u_MVMatrix * vec4(normal, 0.0));

    // perform the usual model-view-projection transformation for the vertex position
    gl_Position = u_MVPMatrix * position;
}
