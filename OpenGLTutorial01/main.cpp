#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL2/SOIL2.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include "cShaderProgram.h"
#include "cCamera.h"
#include "cMesh.h"
#include "cModel.h"
#include "cSkybox.h"
#include "cSkinnedMesh.h"
#include "cSkinnedGameObject.h"
#include "cAnimationState.h"
#include "cScreenQuad.h"
#include "cPlaneObject.h"
#include "cFrameBuffer.h"

//Setting up a camera GLOBAL
cCamera Camera(glm::vec3(0.0f, 0.0f, 3.0f),		//Camera Position
			glm::vec3(0.0f, 1.0f, 0.0f),		//World Up vector
			0.0f,								//Pitch
			-90.0f);							//Yaw

cCamera StaticCamera(glm::vec3(12.0f, 0.0f, 36.0f),		//Camera Position
	glm::vec3(0.0f, 1.0f, 0.0f),		//World Up vector
	0.0f,								//Pitch
	-90.0f);							//Yaw

cCamera RotatingCamera(glm::vec3(12.0f, 0.0f, 36.0f),		//Camera Position
	glm::vec3(0.0f, 1.0f, 0.0f),		//World Up vector
	0.0f,								//Pitch
	-90.0f);							//Yaw

float cameraRotAngle = 0.0f;
glm::vec3 forwardCamera;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;
float lastX = 400, lastY = 300;

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

int drawType = 1;
bool TV1Channel = 0;
bool TV2Channel = 0;
bool spaceLock = false;
bool enterLock = false;
float staticTime = 0.0f;
float staticTime2 = 0.0f;

glm::vec3 roverPos = glm::vec3(0.0f);

std::map<std::string, cModel*> mapModelsToNames;
std::map<std::string, cSkinnedMesh*> mapSkinnedMeshToNames;
std::map<std::string, cShaderProgram*> mapShaderToName;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadCubeMap(std::string directory, std::vector<std::string> faces);

int main()
{
	glfwInit();

	srand(time(NULL));

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialzed GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	//Setting up global openGL state
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//Set up all our programs
	cShaderProgram* myProgram = new cShaderProgram();
	myProgram->compileProgram("assets/shaders/", "vertShader.glsl", "fragShader.glsl");
	mapShaderToName["mainProgram"] = myProgram;

	myProgram = new cShaderProgram();
	myProgram->compileProgram("assets/shaders/", "animVert.glsl", "animFrag.glsl");
	mapShaderToName["skinProgram"] = myProgram;

	myProgram = new cShaderProgram();
	myProgram->compileProgram("assets/shaders/", "skyBoxVert.glsl", "skyBoxFrag.glsl");
	mapShaderToName["skyboxProgram"] = myProgram;

	myProgram = new cShaderProgram();
	myProgram->compileProgram("assets/shaders/", "modelVert.glsl", "modelFrag.glsl");
	mapShaderToName["simpleProgram"] = myProgram;

	myProgram = new cShaderProgram();
	myProgram->compileProgram("assets/shaders/", "quadVert.glsl", "quadFrag.glsl");
	mapShaderToName["quadProgram"] = myProgram;

	//Assemble all our models
	std::string path = "assets/models/landing_site/CuriosityQR_xyz_n_uv.obj";
	mapModelsToNames["Surface"] = new cModel(path);

	path = "assets/models/lander/Entire_Lander_45686_faces.ply";
	mapModelsToNames["Rover"] = new cModel(path);

	path = "assets/models/tv_body/RetroTV.edited.bodyonly.ply";
	mapModelsToNames["TV"] = new cModel(path);

	path = "assets/models/tv_screen/RetroTV.obj";
	mapModelsToNames["Screen"] = new cModel(path);

	unsigned int staticTexture;
	glGenTextures(1, &staticTexture);
	glBindTexture(GL_TEXTURE_2D, staticTexture);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	unsigned char *data = SOIL_load_image("assets/textures/static_texture_by_rachaelwrites.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	SOIL_free_image_data(data);

	//Making 3 frame buffers, one for each camera type, and one to draw at the end
	cFrameBuffer mainFrameBuffer(SCR_HEIGHT, SCR_WIDTH);
	cFrameBuffer rotatingFrameBuffer(SCR_HEIGHT, SCR_WIDTH);
	cFrameBuffer staticFrameBuffer(SCR_HEIGHT, SCR_WIDTH);

	//Some simple shapes
	cScreenQuad screenQuad;

	//Positions for some of the point light
	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -8.0f)
	};

	//Position and look vectors that I went and picked out
	glm::vec3 cameraPositions[] = {
		glm::vec3(-0.946220338, -3.62483168, 29.5283394),
		glm::vec3(-3.31653380, -7.49335766, 22.6392479),
		glm::vec3(8.97522545, -6.45418501, 29.3107643),
		glm::vec3(6.70730686, -3.96114349, 12.4173203)
	};

	glm::vec3 cameraFronts[] = {
		glm::vec3(0.620839477, -0.372987628, -0.689520538),
		glm::vec3(0.979664564, 0.177084103, 0.0943324864),
		glm::vec3(-0.613954604, -0.0314112827, -0.788716078),
		glm::vec3(-0.212666675, -0.253757894, 0.943599463)
	};

	//Load the skyboxes
	cSkybox daybox("assets/textures/skybox/");
	cSkybox skybox("assets/textures/spacebox/");

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	mapShaderToName["skyboxProgram"]->useProgram();
	mapShaderToName["skyboxProgram"]->setInt("skybox", 0);

	mapShaderToName["quadProgram"]->useProgram();
	mapShaderToName["quadProgram"]->setInt("screenTexture", 0);

	mapShaderToName["simpleProgram"]->useProgram();
	mapShaderToName["simpleProgram"]->setInt("texture_diffuse1", 0);
	mapShaderToName["simpleProgram"]->setInt("texture_static", 1);

	mapShaderToName["mainProgram"]->useProgram();
	mapShaderToName["mainProgram"]->setInt("skybox", 0);
	mapShaderToName["mainProgram"]->setInt("reflectRefract", 0);
	//Light settings go in here until I've made a class for them
	{
		//http://devernay.free.fr/cours/opengl/materials.html
		mapShaderToName["mainProgram"]->setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		mapShaderToName["mainProgram"]->setVec3("dirLight.ambient", 0.15f, 0.15f, 0.15f);
		mapShaderToName["mainProgram"]->setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
		mapShaderToName["mainProgram"]->setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

		mapShaderToName["mainProgram"]->setVec3("pointLights[0].position", pointLightPositions[0]);
		mapShaderToName["mainProgram"]->setFloat("pointLights[0].constant", 1.0f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[0].linear", 0.09f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[0].quadratic", 0.032f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);

		mapShaderToName["mainProgram"]->setVec3("pointLights[1].position", pointLightPositions[1]);
		mapShaderToName["mainProgram"]->setFloat("pointLights[1].constant", 1.0f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[1].linear", 0.09f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[1].quadratic", 0.032f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);

		mapShaderToName["mainProgram"]->setVec3("pointLights[2].position", pointLightPositions[2]);
		mapShaderToName["mainProgram"]->setFloat("pointLights[2].constant", 1.0f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[2].linear", 0.09f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[2].quadratic", 0.032f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);

		mapShaderToName["mainProgram"]->setVec3("pointLights[3].position", pointLightPositions[3]);
		mapShaderToName["mainProgram"]->setFloat("pointLights[3].constant", 1.0f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[3].linear", 0.09f);
		mapShaderToName["mainProgram"]->setFloat("pointLights[3].quadratic", 0.032f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
		mapShaderToName["mainProgram"]->setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);

		mapShaderToName["mainProgram"]->setVec3("spotLight.position", glm::vec3(0.0f, 6.0f, 0.0f));
		mapShaderToName["mainProgram"]->setVec3("spotLight.direction", glm::vec3(0.0f, -1.0f, 0.0f));
		mapShaderToName["mainProgram"]->setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		mapShaderToName["mainProgram"]->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
		mapShaderToName["mainProgram"]->setFloat("spotLight.constant", 1.0f);
		mapShaderToName["mainProgram"]->setFloat("spotLight.linear", 0.09f);
		mapShaderToName["mainProgram"]->setFloat("spotLight.quadratic", 0.032f);
		mapShaderToName["mainProgram"]->setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		mapShaderToName["mainProgram"]->setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		mapShaderToName["mainProgram"]->setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		//http://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation
	}

	glm::vec3 roverPos = glm::vec3(4.435f, -6.796f, 23.341f);

	float sceneTime = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//If there's static on either TV screen, count down to make it go away
		if (staticTime > 0.0f)
		{
			staticTime -= deltaTime;
		}
		if (staticTime < 0.0f)
		{
			staticTime = 0.0f;
		}

		if (staticTime2 > 0.0f)
		{
			staticTime2 -= deltaTime;
		}
		if (staticTime2 < 0.0f)
		{
			staticTime2 = 0.0f;
		}

		//QUESTION 2 CODE
		cameraRotAngle += 0.25f * deltaTime;

		forwardCamera.x = sin(cameraRotAngle);
		forwardCamera.y = 0.0f;
		forwardCamera.z = cos(cameraRotAngle);
		forwardCamera = glm::normalize(forwardCamera);

		RotatingCamera.position = roverPos + (16.0f * forwardCamera);
		RotatingCamera.position.y = 1.0f;

		RotatingCamera.front = -(forwardCamera);
		RotatingCamera.front.y = -0.6f;
		//END OF CODE

		//QUESTION 3 CODE
		sceneTime += deltaTime;
		if (sceneTime < 3.0f)
		{
			StaticCamera.position = cameraPositions[0];
			StaticCamera.front = cameraFronts[0];
		}
		else if (sceneTime < 6.0f)
		{
			StaticCamera.position = cameraPositions[1];
			StaticCamera.front = cameraFronts[1];
		}
		else if (sceneTime < 9.0f)
		{
			StaticCamera.position = cameraPositions[2];
			StaticCamera.front = cameraFronts[2];
		}
		else if (sceneTime < 12.0f)
		{
			StaticCamera.position = cameraPositions[3];
			StaticCamera.front = cameraFronts[3];
		}
		else
		{
			sceneTime = 0.0f;
		}
		//END OF CODE

		mapShaderToName["mainProgram"]->useProgram();
		mapShaderToName["mainProgram"]->setVec3("cameraPos", RotatingCamera.position);

		glm::mat4 projection = glm::perspective(glm::radians(RotatingCamera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = RotatingCamera.getViewMatrix();
		glm::mat4 skyboxView = glm::mat4(glm::mat3(RotatingCamera.getViewMatrix()));

		//Begin by drawing the mars scene to the first frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, rotatingFrameBuffer.FBO);

		glEnable(GL_DEPTH_TEST);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Draw the mars surface
		mapShaderToName["mainProgram"]->useProgram();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f));
		mapShaderToName["mainProgram"]->setMat4("projection", projection);
		mapShaderToName["mainProgram"]->setMat4("view", view);
		mapShaderToName["mainProgram"]->setMat4("model", model);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
		mapModelsToNames["Surface"]->Draw(*mapShaderToName["mainProgram"]);

		model = glm::mat4(1.0f);
		model = glm::translate(model, roverPos);
		model = glm::scale(model, glm::vec3(0.04f));
		mapShaderToName["mainProgram"]->setMat4("model", model);
		mapModelsToNames["Rover"]->Draw(*mapShaderToName["mainProgram"]);

		//Drawing the space skybox
		mapShaderToName["skyboxProgram"]->useProgram();

		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		mapShaderToName["skyboxProgram"]->setMat4("projection", projection);
		mapShaderToName["skyboxProgram"]->setMat4("view", skyboxView);

		glBindVertexArray(skybox.VAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthFunc(GL_LESS);

		//Draw the static camera image
		mapShaderToName["mainProgram"]->useProgram();
		mapShaderToName["mainProgram"]->setVec3("cameraPos", StaticCamera.position);

		projection = glm::perspective(glm::radians(StaticCamera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		view = StaticCamera.getViewMatrix();
		skyboxView = glm::mat4(glm::mat3(StaticCamera.getViewMatrix()));

		glBindFramebuffer(GL_FRAMEBUFFER, staticFrameBuffer.FBO);

		glEnable(GL_DEPTH_TEST);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Draw the mars surface
		mapShaderToName["mainProgram"]->useProgram();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f));
		mapShaderToName["mainProgram"]->setMat4("projection", projection);
		mapShaderToName["mainProgram"]->setMat4("view", view);
		mapShaderToName["mainProgram"]->setMat4("model", model);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
		mapModelsToNames["Surface"]->Draw(*mapShaderToName["mainProgram"]);

		model = glm::mat4(1.0f);
		model = glm::translate(model, roverPos);
		model = glm::scale(model, glm::vec3(0.04f));
		mapShaderToName["mainProgram"]->setMat4("model", model);
		mapModelsToNames["Rover"]->Draw(*mapShaderToName["mainProgram"]);

		//Drawing the space skybox
		mapShaderToName["skyboxProgram"]->useProgram();

		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		mapShaderToName["skyboxProgram"]->setMat4("projection", projection);
		mapShaderToName["skyboxProgram"]->setMat4("view", skyboxView);

		glBindVertexArray(skybox.VAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthFunc(GL_LESS);

		//Begin writing to the main frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer.FBO);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		projection = glm::perspective(glm::radians(Camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		view = Camera.getViewMatrix();
		skyboxView = glm::mat4(glm::mat3(Camera.getViewMatrix()));

		//Making TV number 1
		mapShaderToName["mainProgram"]->useProgram();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.5f, -0.75, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02f));
		mapShaderToName["mainProgram"]->setMat4("projection", projection);
		mapShaderToName["mainProgram"]->setMat4("view", view);
		mapShaderToName["mainProgram"]->setMat4("model", model);
		mapModelsToNames["TV"]->Draw(*mapShaderToName["mainProgram"]);

		//Generate random numbers for the static this frame
		int intXOffset = rand() % 1001;		//Random number from 0 to 1000
		int intYOffset = rand() % 1001;		//To be divided by 1000, becoming a range from 0.000 to 1.000
		float floatXOffset = (float)intXOffset / 1000.0f;
		float floatYOffset = (float)intYOffset / 1000.0f;

		mapShaderToName["simpleProgram"]->useProgram();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.5f, -0.75, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02f));
		mapShaderToName["simpleProgram"]->setMat4("projection", projection);
		mapShaderToName["simpleProgram"]->setMat4("view", view);
		mapShaderToName["simpleProgram"]->setMat4("model", model);
		mapShaderToName["simpleProgram"]->setFloat("staticTime", staticTime);
		mapShaderToName["simpleProgram"]->setFloat("randOffsetX", floatXOffset);
		mapShaderToName["simpleProgram"]->setFloat("randOffsetY", floatYOffset);
		glActiveTexture(GL_TEXTURE0);
		if (TV1Channel)
		{
			glBindTexture(GL_TEXTURE_2D, rotatingFrameBuffer.textureID);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, staticFrameBuffer.textureID);
		}	
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, staticTexture);
		mapModelsToNames["Screen"]->Draw(*mapShaderToName["simpleProgram"]);

		//And here's TV number 2
		mapShaderToName["mainProgram"]->useProgram();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(1.5f, -0.75, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02f));
		mapShaderToName["mainProgram"]->setMat4("model", model);
		mapModelsToNames["TV"]->Draw(*mapShaderToName["mainProgram"]);

		//Generate random numbers for the static this frame
		intXOffset = rand() % 1001;		//Random number from 0 to 1000
		intYOffset = rand() % 1001;		//To be divided by 1000, becoming a range from 0.000 to 1.000
		floatXOffset = (float)intXOffset / 1000.0f;
		floatYOffset = (float)intYOffset / 1000.0f;

		mapShaderToName["simpleProgram"]->useProgram();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(1.5f, -0.75, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02f));
		mapShaderToName["simpleProgram"]->setMat4("model", model);
		mapShaderToName["simpleProgram"]->setFloat("staticTime", staticTime2);
		mapShaderToName["simpleProgram"]->setFloat("randOffsetX", floatXOffset);
		mapShaderToName["simpleProgram"]->setFloat("randOffsetY", floatYOffset);
		glActiveTexture(GL_TEXTURE0);
		if (TV2Channel)
		{
			glBindTexture(GL_TEXTURE_2D, rotatingFrameBuffer.textureID);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, staticFrameBuffer.textureID);
		}
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, staticTexture);
		mapModelsToNames["Screen"]->Draw(*mapShaderToName["simpleProgram"]);

		mapShaderToName["skyboxProgram"]->useProgram();

		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		mapShaderToName["skyboxProgram"]->setMat4("projection", projection);
		mapShaderToName["skyboxProgram"]->setMat4("view", skyboxView);

		glBindVertexArray(skybox.VAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, daybox.textureID);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthFunc(GL_LESS);

		//Final pass: Render all of the above on one quad
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//Paste the entire scene onto a quad as a single texture
		mapShaderToName["quadProgram"]->useProgram();
		mapShaderToName["quadProgram"]->setInt("drawType", drawType);
		glBindVertexArray(screenQuad.VAO);
		glBindTexture(GL_TEXTURE_2D, mainFrameBuffer.textureID);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		std::string playerHealth = "Rover Pos: " + std::to_string(Camera.position.x) + ", " + std::to_string(Camera.position.y) + ", " + std::to_string(Camera.position.z);
		glfwSetWindowTitle(window, playerHealth.c_str());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	SCR_HEIGHT = height;
	SCR_WIDTH = width;
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	return;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		if (!spaceLock)
		{
			TV1Channel = !TV1Channel;
			staticTime = 1.5f;
			spaceLock = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
	{
		spaceLock = false;
	}

	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
	{
		if (!enterLock)
		{
			TV2Channel = !TV2Channel;
			staticTime2 = 1.5f;
			enterLock = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE)
	{
		enterLock = false;
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		drawType = 1;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		drawType = 2;
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		drawType = 3;
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		drawType = 4;
	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		drawType = 5;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		Camera.processKeyboard(Camera_Movement::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		Camera.processKeyboard(Camera_Movement::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		Camera.processKeyboard(Camera_Movement::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		Camera.processKeyboard(Camera_Movement::RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		roverPos.x += 6 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		roverPos.x -= 6 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		roverPos.z -= 6 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		roverPos.z += 6 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		roverPos.y -= 6 * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		roverPos.y += 6 * deltaTime;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse) // this bool variable is initially set to true
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xOffset = xpos - lastX;
	float yOffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	Camera.processMouseMovement(xOffset, yOffset, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Camera.processMouseScroll(yoffset);
}

unsigned int loadCubeMap(std::string directory, std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		std::string fullPath = directory + faces[i];
		unsigned char *data = SOIL_load_image(fullPath.c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			GLenum format;
			if (nrChannels == 1)
				format = GL_RED;
			else if (nrChannels == 3)
				format = GL_RGB;
			else if (nrChannels == 4)
				format = GL_RGBA;

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data
			);
			SOIL_free_image_data(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			SOIL_free_image_data(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}