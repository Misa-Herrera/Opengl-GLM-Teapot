#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <vector>

#include "SOIL2\SOIL2\SOIL2.h"
#include <shader.h>
#include <OBJ_Loader.h>

using namespace std;
using namespace glm;

char t;
//Window variables
int
CurrentWidth = 800,
CurrentHeight = 600,
WindowHandle = 0,
numOfTris,
numOfVert;




//open Gl Integers to be used
GLuint
VertexShaderId,
FragmentShaderId,
ProgramId,
VaoId,
VboId,
ColorBufferId,
NormalBufferId,
MVPMatrixID,
NormalMatrixID,
ModelViewMatrixID,
LightArrayID;




const GLchar* VertexShader = R"glsl(
#version 330
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec4 in_Color;
layout(location = 2) in vec3 VertexNormal;
 
out Data
{
	vec3 Position;
	vec3 Normal;
	vec4 ex_Color;
} data;
 
uniform mat4 ModelViewMatrix;
uniform mat4 NormalMatrix;
uniform mat4 MVP;
 

void main()
{
	data.Normal = normalize(vec3( NormalMatrix * vec4(VertexNormal,0) ) );
	data.Position = vec3( ModelViewMatrix * vec4( VertexPosition, 1 ) );
	data.ex_Color = in_Color;
 
	gl_Position = MVP * vec4( VertexPosition, 1 );
}

)glsl";



const GLchar* FragmentShader = R"glsl(
#version 330
in Data
{
	vec3 Position;
	vec3 Normal;
	vec4 ex_Color;
} data;

out vec4 out_Color;

struct light{
	vec3 dir;
	vec3 col;
};

uniform mat4 ModelViewMatrix;

 
void main(void)
{
	float ka = 0.1f;
	float kd = 0.3f;
	float ks = 0.6f;
	float sp = 3.0f;

	light lights[3];

	lights[0].dir = vec3(0.f, -1.f, 0.f);
	lights[1].dir = vec3(-1.f, 0.f, 0.f);
	lights[2].dir = vec3(0.f, 0.f, -1.f);

	lights[0].col = vec3(1.f, 0.1f, 0.1f);
	lights[1].col = vec3(0.1f, 1.f, 0.1f);
	lights[2].col = vec3(0.1f, 0.1f, 1.f);

	
	vec3 ambientSum = vec3(0);
	vec3 diffuseSum = vec3(0);
	vec3 specSum = vec3(0);

	vec3 ambient, diffuse, specular;
 
	vec3 N = normalize(data.Normal);
	vec3 V = normalize(- data.Position );
	

    ambientSum = ka*vec3(data.ex_Color);
	for(int i=0;i<3;i++)
	{	
		vec3 L = normalize(vec3(ModelViewMatrix * -vec4(lights[i].dir,0)));
		float diff = max(dot(N,L),0.0f);
		diffuse =  kd*diff * lights[i].col ;
		diffuseSum += diffuse;

		vec3 R = reflect(-L,N);
		specular = lights[i].col * ks * pow( max( dot(R,V), 0.0), sp);
		specSum += specular;

	}	

		vec4 result = vec4(ambientSum + diffuseSum+specSum,1);
		out_Color = result;
	}

)glsl";



void Initialize(int, char*[]);
void InitWindow(int, char*[]);
void ResizeFunction(int, int);
void RenderFunction(void);

void Cleanup(void);
void CreateVBO(void);
void DestroyVBO(void);
void CreateShaders(const GLchar* vs, const GLchar* fs);
void DestroyShaders(void);
void viewChange(unsigned char key, int xmouse, int ymouse);


int main(int argc, char* argv[])
{

	objl::Loader Loader;
	bool loudout = Loader.LoadFile("teapot2.obj");
	for (int i = 0; i < Loader.LoadedMeshes.size(); i++)
	{
		objl::Mesh curMesh = Loader.LoadedMeshes[i];
	}

	Initialize(argc, argv);

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	int width, height;
	unsigned char* image = SOIL_load_image("brick.png", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);




	glutMainLoop();

	exit(EXIT_SUCCESS);
}

void Initialize(int argc, char* argv[])
{
	GLenum GlewInitResult;

	glewExperimental = GL_TRUE;
	InitWindow(argc, argv);

	GlewInitResult = glewInit();		//initializing glew

	if (GLEW_OK != GlewInitResult) {
		fprintf(
			stderr,
			"ERROR: %s\n",
			glewGetErrorString(GlewInitResult)
		);
		exit(EXIT_FAILURE);
	}

	fprintf(
		stdout,
		"INFO: OpenGL Version: %s\n",
		glGetString(GL_VERSION)
	);
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);


	CreateShaders(VertexShader, FragmentShader);
	CreateVBO();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

}

void InitWindow(int argc, char* argv[])
{
	glutInit(&argc, argv);		//initialized glut

								//glut version, context and profile control
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutSetOption(
		GLUT_ACTION_ON_WINDOW_CLOSE,
		GLUT_ACTION_GLUTMAINLOOP_RETURNS
	);

	//creating window
	glutInitWindowSize(CurrentWidth, CurrentHeight);


	//defines what type of OpenGL context we would like and how the device should render our scene
	//GLUT_DEPTH: enables usage of the depth buffer
	//GLUT_DOUBLE: enabless usage of double buffereing. a feature that reduces image flickering
	//GLUT_RGBA: flag that defines how we use colors
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	WindowHandle = glutCreateWindow("Name");

	//window isnt valid if handle is less than one
	if (WindowHandle < 1) {
		fprintf(
			stderr,
			"ERROR: Could not create a new rendering window.\n"
		);
		exit(EXIT_FAILURE);
	}

	glutReshapeFunc(ResizeFunction);//function that handles window resizing
	glutDisplayFunc(RenderFunction); //rendering function
	glutKeyboardFunc(viewChange); //pressing a button
	glutCloseFunc(Cleanup);
}

void ResizeFunction(int Width, int Height)
{
	CurrentWidth = Width;
	CurrentHeight = Height;
	glViewport(0, 0, CurrentWidth, CurrentHeight);
}

void RenderFunction(void)
{


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/*
	void glDrawArrays(GLenum mode, GLint first, GLsizei count);
	mode specifies the type of array data thats going to be drawn to the screen
	first specifies teh first index of the enabled vertex attribute arrays that we want to draw
	count specifies how many of the vertexes there are to draw


	*/
	glUseProgram(ProgramId);

	glDrawArrays(GL_TRIANGLES, 0, numOfVert);

	glutSwapBuffers();
}



void Cleanup(void)
{
	DestroyShaders();
	DestroyVBO();
}

void CreateVBO()
{

	vector<GLfloat> Vertices;
	vector<GLfloat> Colors;
	vector<GLfloat> Normals;



	GLenum ErrorCheckValue = glGetError();


	//creates vertex array objects in the GPU memory and passes back their identifiers
	//first paramter describes the ammount of vertex arrays to generate
	// second is the address we want it stored
	glGenVertexArrays(1, &VaoId);
	glBindVertexArray(VaoId);


	//creating vertex buffers, first parameter is number of buffers to be generated
	//second, the names are stored as an array in the second parameter
	glGenBuffers(1, &VboId);

	//binds binds teh buffer to the specified target
	glBindBuffer(GL_ARRAY_BUFFER, VboId);

	//copies vertex data to the gpus memory
	glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(GLfloat), &Vertices[0], GL_STATIC_DRAW);
	//now that all data is in GPU, we have to tell it how to use the block of data
	//done with this next function
	//void glVertexAttribPointer(GLuint index,GLint size,	GLenum type,GLboolean normalized,GLsizei stride,const GLvoid* pointer);
	//first parameter index, is equal to the GLSL shaders layout number; usermaintained and must match
	//next parameter measures the amount of componets of each vertex, 4 = x,y,z,w
	//next is the type that each component uses (GL_FLOAT)
	// next parameter is whether or not we would want to normalize them to a range
	//stride i dont reall get
	//last one is numerical offset in bytes in the block of data supplied i gl buffer data ==?????
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, 0);
	glEnableVertexAttribArray(0);


	//color data, same as vertex data
	glGenBuffers(1, &ColorBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, ColorBufferId);
	glBufferData(GL_ARRAY_BUFFER, Colors.size() * sizeof(GLfloat), &Colors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);


	//normal data
	glGenBuffers(1, &NormalBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, NormalBufferId);
	glBufferData(GL_ARRAY_BUFFER, Normals.size() * sizeof(GLfloat), &Normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);


	//error checking
	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);

		exit(-1);
	}
}

void DestroyVBO(void)
{
	GLenum ErrorCheckValue = glGetError();
	//disable vertex attibutes in reverse order of what we made them in
	//using appropiate indices
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	//bind the GL_ARRAY_BUffer to 0
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//and then we can delete our buffers
	//similar to glGenBuffers
	glDeleteBuffers(1, &NormalBufferId);
	glDeleteBuffers(1, &ColorBufferId);
	glDeleteBuffers(1, &VboId);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &VaoId);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not destroy the VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);

		exit(-1);
	}
}

void CreateShaders(const GLchar* vs, const GLchar* fs)
{
	GLenum ErrorCheckValue = glGetError();
	//creat a shader object
	VertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	/*
	void glShaderSource(
	GLuint shader,
	GLsizei count,
	const GLchar** string,
	const GLint * length
	);
	this function copies the source code in the string (&vertexShader) which we declared earlier
	and associates it with the shader object identified by parameter shader
	count represents how many strings are present in the string, 1 since VertexShader is one long string
	length is an array of integers denoting the lengths of the strings in string parameter
	leave NULL if you use normal null-termintated strings
	*/
	glShaderSource(VertexShaderId, 1, &vs, NULL);

	//after source is in memory, need to compile
	glCompileShader(VertexShaderId);


	//process repeated for fragment shader
	FragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShaderId, 1, &fs, NULL);
	glCompileShader(FragmentShaderId);


	//generate a program object
	ProgramId = glCreateProgram();
	//now attach our shaders to the program object
	glAttachShader(ProgramId, VertexShaderId);

	glAttachShader(ProgramId, FragmentShaderId);
	//need to link our program
	glLinkProgram(ProgramId);
	//after this process completes we can use program by calling the following:
	glUseProgram(ProgramId);
	//this takees in the shader program objects identifiere and makes it current
	//the current program remains active until glUseProgram is called with another shader program object's identifier
	//MatrixID = glGetUniformLocation(ProgramId, "MVP");
	ErrorCheckValue = glGetError();

	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create the shaders: %s \n",
			gluErrorString(ErrorCheckValue)
		);
		char t;
		cin >> t;

		exit(-1);
	}
}

void DestroyShaders(void)
{
	GLenum ErrorCheckValue = glGetError();
	//stop using the shader program
	glUseProgram(0);

	//detach our shaders from the program
	glDetachShader(ProgramId, VertexShaderId);
	glDetachShader(ProgramId, FragmentShaderId);


	//delete shaders
	glDeleteShader(FragmentShaderId);
	glDeleteShader(VertexShaderId);

	//deete program
	glDeleteProgram(ProgramId);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not destroy the shaders: %s \n",
			gluErrorString(ErrorCheckValue)
		);

		exit(-1);
	}
}



void viewChange(unsigned char key, int xmouse, int ymouse)
{
	mat4x4 proj, cam, model, MVP, normalMatrix, MVMatrix;
	vec3 eye, opt, upv;
	DestroyShaders();
	switch (key) {

	case '1':
		CreateShaders(VertexShader, FragmentShader);
		proj = ortho(-2.4f, 2.4f, -1.8f, 1.8f, 1.f, 50.f);

		eye = { 10.f,10.f,10.f };
		opt = { 0.f,0.f,0.f };
		upv = { 0.f,1.f,0.f };

		cam = lookAt(eye, opt, upv);
		model = translate(mat4(1.f), vec3(0.f, -1.f, 0.f));
		MVMatrix = cam*model;
		normalMatrix = transpose(inverse(MVMatrix));

		break;
	case '2':
		CreateShaders(VertexShader, FragmentShader);
		proj = perspective(radians(50.f), 4.f / 3.f, 1.f, 50.f);

		eye = { 3.f,3.f,3.f };
		opt = { 0.f,0.f,0.f };
		upv = { 0.f,1.f,0.f };

		cam = lookAt(eye, opt, upv);
		model = translate(rotate(mat4(1.f), radians(45.f), vec3(1.f, 0.f, 0.f)), vec3(0.f, -1.f, 0.f));
		MVMatrix = cam*model;
		normalMatrix = transpose(inverse(MVMatrix));
		break;

	default:
		break;
	}
	MVP = proj*cam*model;
	MVPMatrixID = glGetUniformLocation(ProgramId, "MVP");
	NormalMatrixID = glGetUniformLocation(ProgramId, "NormalMatrix");
	ModelViewMatrixID = glGetUniformLocation(ProgramId, "ModelViewMatrix");

	glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(NormalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
	glUniformMatrix4fv(ModelViewMatrixID, 1, GL_FALSE, &MVMatrix[0][0]);


	glutPostRedisplay();
}


