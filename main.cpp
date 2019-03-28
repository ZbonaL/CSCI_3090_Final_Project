#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <GL/glew.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include "apis/stb_image.h"


#include "ShaderProgram.h"
#include "ObjMesh.h"

#define SCALE_FACTOR 2.0f

int width, height;

GLuint programId;
GLuint programId2;

GLuint vertexBuffer;
GLuint indexBuffer;

GLenum positionBufferId;
GLuint positions_vbo = 0;

GLuint textureCoords_vbo = 0;
GLuint normals_vbo = 0;
GLuint colours_vbo = 0;
GLuint textureId;

unsigned int numVertices;

float angle = 0.0f;
float lightOffsetY = 0.0f;
glm::vec3 eyePosition(40, 30, 30);
bool animateLight = false;
bool rotateObject = true;
float scaleFactor = 5.0f;
float lastX = std::numeric_limits<float>::infinity();
float lastY = std::numeric_limits<float>::infinity();

glm::mat4 view;
glm::mat4 projection;
float xAngle = 0.0f;
float yAngle = 0.0f;
float zAngle = 0.0f;

static void createTexture(std::string filename) {
	int imageWidth, imageHeight;
	int numComponents;

	// load the image data into a bitmap
	unsigned char *bitmap = stbi_load(filename.c_str(),
		&imageWidth,
		&imageHeight,
		&numComponents, 4);

	// generate a texture name
	glGenTextures(1, &textureId);

	// make the texture active
	glBindTexture(GL_TEXTURE_2D, textureId);

	// make a texture mip map
	glGenerateTextureMipmap(textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	// specify the functions to use when shrinking/enlarging the texture image
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	// specify the tiling parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// send the data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);

	// bind the texture to unit 0
	glBindTexture(GL_TEXTURE_2D, textureId);
	glActiveTexture(GL_TEXTURE0);

	// free the bitmap data
	stbi_image_free(bitmap);
}


static void createGeometry(void) {
   ObjMesh mesh;
   mesh.load("meshes/newHead.obj", true, true);

   numVertices = mesh.getNumIndexedVertices();
   Vector3* vertexPositions = mesh.getIndexedPositions();
   Vector2* vertexTextureCoords = mesh.getIndexedTextureCoords();
   Vector3* vertexNormals = mesh.getIndexedNormals();

   glGenBuffers(1, &positions_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
   glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), vertexPositions, GL_STATIC_DRAW);

   glGenBuffers(1, &textureCoords_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo);
   glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector2), vertexTextureCoords, GL_STATIC_DRAW);

   glGenBuffers(1, &normals_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
   glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), vertexNormals, GL_STATIC_DRAW);

   unsigned int* indexData = mesh.getTriangleIndices();
   int numTriangles = mesh.getNumTriangles();

   glGenBuffers(1, &indexBuffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numTriangles * 3, indexData, GL_STATIC_DRAW);
}

static void update(void) {
   int milliseconds = glutGet(GLUT_ELAPSED_TIME);

   // rotate the shape about the y-axis so that we can see the shading
   if (rotateObject) {
      float degrees = (float)milliseconds / 80.0f;
      yAngle = degrees;
   }

   // move the light position over time along the x-axis, so we can see how it affects the shading
   if (animateLight) {
      float t = milliseconds / 1000.0f;
      lightOffsetY = sinf(t) * 100;
   }

   glutPostRedisplay();
}

void drawHead(glm::mat4 model_matrix) {
	// model-view-projection matrix
	glm::mat4 mvp = projection * view * model_matrix;
	GLuint mvpMatrixId = glGetUniformLocation(programId, "u_MVPMatrix");
	glUniformMatrix4fv(mvpMatrixId, 1, GL_FALSE, &mvp[0][0]);

	// model-view matrix
	glm::mat4 mv = view * model_matrix;
	GLuint mvMatrixId = glGetUniformLocation(programId, "u_MVMatrix");
	glUniformMatrix4fv(mvMatrixId, 1, GL_FALSE, &mv[0][0]);
	// the position of our light
	GLuint lightPosId = glGetUniformLocation(programId, "u_LightPos");
	glUniform3f(lightPosId, 0, 0, 0);
	// the position of our camera/eye
	GLuint eyePosId = glGetUniformLocation(programId, "u_EyePosition");
	glUniform3f(eyePosId, eyePosition.x, eyePosition.y, eyePosition.z);

	// the colour of our object
	GLuint diffuseColourId = glGetUniformLocation(programId, "u_DiffuseColour");
	glUniform4f(diffuseColourId, 1.0, 1.0, 1.0, 1.0);

	// the shininess of the object's surface
	GLuint shininessId = glGetUniformLocation(programId, "u_Shininess");
	glUniform1f(shininessId, 25);

	// find the names (ids) of each vertex attribute
	GLint positionAttribId = glGetAttribLocation(programId, "position");
	GLint textureCoordsAttribId = glGetAttribLocation(programId, "textureCoords");
	GLint normalAttribId = glGetAttribLocation(programId, "normal");

	// provide the vertex positions to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
	glEnableVertexAttribArray(positionAttribId);
	glVertexAttribPointer(positionAttribId, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// provide the vertex texture coordinates to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo);
	glEnableVertexAttribArray(textureCoordsAttribId);
	glVertexAttribPointer(textureCoordsAttribId, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// provide the vertex normals to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
	glEnableVertexAttribArray(normalAttribId);
	glVertexAttribPointer(normalAttribId, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// draw the triangles
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, (void*)0);

	// disable the attribute arrays
	glDisableVertexAttribArray(positionAttribId);
	glDisableVertexAttribArray(textureCoordsAttribId);
	glDisableVertexAttribArray(normalAttribId);
}

void drawHead2(glm::mat4 model_matrix) {
	// model-view-projection matrix
	glm::mat4 mvp = projection * view * model_matrix;
	GLuint mvpMatrixId = glGetUniformLocation(programId2, "u_MVPMatrix");
	glUniformMatrix4fv(mvpMatrixId, 1, GL_FALSE, &mvp[0][0]);

	// model-view matrix
	glm::mat4 mv = view * model_matrix;
	GLuint mvMatrixId = glGetUniformLocation(programId2, "u_MVMatrix");
	glUniformMatrix4fv(mvMatrixId, 1, GL_FALSE, &mv[0][0]);
	// the position of our light
	GLuint lightPosId = glGetUniformLocation(programId2, "u_LightPos");
	glUniform3f(lightPosId,1, 8 + lightOffsetY, -2);
	// the position of our camera/eye
	GLuint eyePosId = glGetUniformLocation(programId2, "u_EyePosition");
	glUniform3f(eyePosId, eyePosition.x, eyePosition.y, eyePosition.z);

	// the colour of our object
	GLuint diffuseColourId = glGetUniformLocation(programId2, "u_DiffuseColour");
	glUniform4f(diffuseColourId, 1.0, 1.0, 1.0, 1.0);

	// the shininess of the object's surface
	GLuint shininessId = glGetUniformLocation(programId2, "u_Shininess");
	glUniform1f(shininessId, 25);

	// find the names (ids) of each vertex attribute
	GLint positionAttribId = glGetAttribLocation(programId2, "position");
	GLint textureCoordsAttribId = glGetAttribLocation(programId2, "textureCoords");
	GLint normalAttribId = glGetAttribLocation(programId2, "normal");

	// provide the vertex positions to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
	glEnableVertexAttribArray(positionAttribId);
	glVertexAttribPointer(positionAttribId, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// provide the vertex texture coordinates to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo);
	glEnableVertexAttribArray(textureCoordsAttribId);
	glVertexAttribPointer(textureCoordsAttribId, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// provide the vertex normals to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
	glEnableVertexAttribArray(normalAttribId);
	glVertexAttribPointer(normalAttribId, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// draw the triangles
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, (void*)0);

	// disable the attribute arrays
	glDisableVertexAttribArray(positionAttribId);
	glDisableVertexAttribArray(textureCoordsAttribId);
	glDisableVertexAttribArray(normalAttribId);
}
static void render(void) {
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // activate our shader program
	glUseProgram(programId);

   // turn on depth buffering
   glEnable(GL_DEPTH_TEST);

   // projection matrix - perspective projection
   float aspectRatio = (float)width / (float)height;
   projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

   // view matrix - orient everything around our preferred view
   view = glm::lookAt(
      eyePosition,
      glm::vec3(0,0,0),    // where to look
      glm::vec3(0,1,0)     // up
   );


   createTexture("textures/sun.jpg");
   // model matrix: translate, scale, and rotate the model
   glm::vec3 rotationAxis(0,1,0);
   glm::mat4 model = glm::mat4(1.0f);
   model = glm::rotate(model, glm::radians(xAngle), glm::vec3(1, 0, 0)); // rotate about the x-axis
   model = glm::rotate(model, glm::radians(yAngle), glm::vec3(0, 1, 0)); // rotate about the y-axis
   model = glm::rotate(model, glm::radians(zAngle), glm::vec3(0, 0, 1)); // rotate about the z-axis
   model = glm::scale(model, glm::vec3(2*scaleFactor, 2*scaleFactor, 2*scaleFactor));

   drawHead(model, colour);


	glUseProgram(programId2);


   model = glm::translate(model, glm::vec3(scaleFactor * 0.5, 0.0f, 0.0));
   model = glm::rotate(model, glm::radians(xAngle), glm::vec3(1, 0, 0)); // rotate about the x-axis
   model = glm::rotate(model, glm::radians(yAngle), glm::vec3(0, 1, 0)); // rotate about the y-axis
   model = glm::rotate(model, glm::radians(zAngle), glm::vec3(0, 0, 1)); // rotate about the z-axis
   model = glm::scale(model, glm::vec3(scaleFactor/ 7, scaleFactor / 7, scaleFactor / 7));
   drawHead2(model);

   model = glm::mat4(1.0f);
   model = glm::rotate(model, glm::radians(xAngle), glm::vec3(1, 0, 0)); // rotate about the x-axis
   model = glm::rotate(model, glm::radians(yAngle + 100), glm::vec3(0, 1, 0)); // rotate about the y-axis
   model = glm::rotate(model, glm::radians(zAngle), glm::vec3(0, 0, 1)); // rotate about the z-axis
   model = glm::scale(model, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
   model = glm::translate(model, glm::vec3(scaleFactor * 1, 0.0f, 0.0));
   model = glm::rotate(model, glm::radians(xAngle), glm::vec3(1, 0, 0)); // rotate about the x-axis
   model = glm::rotate(model, glm::radians(yAngle), glm::vec3(0, 1, 0)); // rotate about the y-axis
   model = glm::rotate(model, glm::radians(zAngle), glm::vec3(0, 0, 1)); // rotate about the z-axis
   model = glm::scale(model, glm::vec3(scaleFactor / 8, scaleFactor / 8, scaleFactor / 8));

   drawHead2(model);

	// make the draw buffer to display buffer (i.e. display what we have drawn)
	glutSwapBuffers();




}

static void reshape(int w, int h) {
   glViewport(0, 0, w, h);

   width = w;
   height = h;
}

static void drag(int x, int y) {
   if (!isinf(lastX) && !isinf(lastY)) {
      float dx = lastX - (float)x;
      float dy = lastY - (float)y;
      float distance = sqrt(dx * dx + dy * dy);

      if (dy > 0.0f) {
         scaleFactor = SCALE_FACTOR / distance;
      } else {
         scaleFactor = distance / SCALE_FACTOR;
      }
   } else {
      lastX = (float)x;
      lastY = (float)y;
   }
}

static void mouse(int button, int state, int x, int y) {
   if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
      lastX = std::numeric_limits<float>::infinity();
      lastY = std::numeric_limits<float>::infinity();
   }
}

static void keyboard(unsigned char key, int x, int y) {
   std::cout << "Key pressed: " << key << std::endl;
   if (key == 'l') {
      animateLight = !animateLight;
   } else if (key == 'r') {
      rotateObject = !rotateObject;
   }
}

int main(int argc, char** argv) {
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   glutInitWindowSize(800, 600);
   glutCreateWindow("CSCI 4110u Shading in OpenGL");
   glutIdleFunc(&update);
   glutDisplayFunc(&render);
   glutReshapeFunc(&reshape);
   glutMotionFunc(&drag);
   glutMouseFunc(&mouse);
   glutKeyboardFunc(&keyboard);

   glewInit();
   if (!GLEW_VERSION_2_0) {
      std::cerr << "OpenGL 2.0 not available" << std::endl;
      return 1;
   }
   std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
	std::cout << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

   createGeometry();

	// this creates program that uses the phong shader
   ShaderProgram program;
   program.loadShaders("shaders/phong_vertex.glsl", "shaders/phong_fragment.glsl");

  	programId = program.getProgramId();
   
	// this creates program that uses the gouraud shader
   ShaderProgram program2;
   program2.loadShaders("shaders/gouraud_vertex.glsl", "shaders/gouraud_fragment.glsl");

  	programId2 = program2.getProgramId();

   glutMainLoop();

   return 0;
}
