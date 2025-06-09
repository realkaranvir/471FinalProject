/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 * 
 * Code modified by Karan Sandhu as part of CPE 471 course final project.
 */

#include <iostream>
#include <chrono>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "Spline.h"
#include "Bezier.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:
	WindowManager *windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong
	std::shared_ptr<Program> prog;

	// Our shader program for textures
	std::shared_ptr<Program> texProg;

	vector<shared_ptr<Shape>> cactus;
	shared_ptr<Shape> sphere;
	shared_ptr<Shape> skybox;
	shared_ptr<Shape> cube;
	shared_ptr<Shape> cubeWithTex;
	shared_ptr<Shape> blockySphere;
	shared_ptr<Shape> spike;
	shared_ptr<Shape> pillar;
	shared_ptr<Shape> plank;
	shared_ptr<Shape> axe;
	vector<shared_ptr<Shape>> rockScene;

	// global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	// ground VAO
	GLuint GroundVertexArrayID;

	// the image to use as a texture (ground)
	shared_ptr<Texture> texture0;
	shared_ptr<Texture> texture1;
	shared_ptr<Texture> texture2;
	shared_ptr<Texture> texture3;
	shared_ptr<Texture> stoneTex;
	shared_ptr<Texture> marbleTex;
	shared_ptr<Texture> woodTex;
	shared_ptr<Texture> metalTex;

	// global data (larger program should be encapsulated)
	vec3 gMin;


	// Character
	vec3 characterPos = vec3(-18, 6.5, -12);
	bool characterAnimated = false; // toggle for character animation
	float velocityY = 0.0f;
	float gravity = 9.8f;
	float jumpForce = 7.0f;
	float floorY = 6.5f;
	bool jumpKeyHeld = false;
	float velocityX = 0.0f;
	const float maxSpeed = 7.0f;
	const float acceleration = 14.0f;
	const float deceleration = 6.0f;

	vec3 gCamPos = characterPos + vec3(0, 1, -5); // camera position in world space
	bool thirdPerson = true; // toggle for third person camera
	float theta = M_PI / 2;
	float phi = M_PI / 12;
	float radius = 10.0f;
	// Translations in camera space
	float gCamH = 2;
	float gCamX = 0;
	float gCamZ = 0;

	// game state
	bool lost = false;
	bool won = false;
	int sphereNumber = 8;

	// Spline
	Spline splinepath[2];
	bool goCamera = false;

	bool materialSwitch = false;
	// animation data
	float lightTrans = 0;
	float sTheta = 0;
	float eTheta = 0;
	float hTheta = 0;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// Camera freecam controls
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			gCamX += 0.75;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			gCamZ += 1.25;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			gCamZ -= 1.25;
		}
		// update camera height
		if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
		{
			gCamH += 1;
		}

		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			gCamX -= 0.75;
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS)
		{
			gCamH -= 1;
		}
		// camera spline animation
		if (key == GLFW_KEY_G && action == GLFW_PRESS)
		{
			goCamera = !goCamera;
		}
		// toggle third person camera
		if (key == GLFW_KEY_T && action == GLFW_PRESS)
		{
			if (thirdPerson)
			{

				vec3 dir = normalize(characterPos - gCamPos);
				phi = asin(dir.y);
				theta = atan2(dir.z, dir.x);
			}
			else
			{
				// Reset camera position to character position
				gCamPos = characterPos + vec3(0, 1, -5);
				theta = M_PI / 2;
				phi = M_PI / 12;
			}
			thirdPerson = !thirdPerson;
		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			// Reset character position and camera
			characterPos = vec3(-18, 6.5, -12);
			gCamPos = characterPos + vec3(0, 1, -5);
			thirdPerson = true;
			phi = M_PI / 12;
			theta = M_PI / 2;
			splinepath[0] = Spline(glm::vec3(-6, 0, 5), glm::vec3(-1, -5, 5), glm::vec3(1, 5, 5), glm::vec3(2, 0, 5), 5);
			splinepath[1] = Spline(glm::vec3(2, 0, 5), glm::vec3(3, -2, 5), glm::vec3(-0.25, 0.25, 5), glm::vec3(0, 0, 5), 5);
			velocityX = 0.0f; // Reset horizontal velocity
			velocityY = 0.0f; // Reset vertical velocity
			lost = false;
			won = false;
		}
		// light controls
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		{
			lightTrans += 0.5;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			lightTrans -= 0.5;
		}
		// switch starting block material
		if (key == GLFW_KEY_M && action == GLFW_PRESS)
		{
			materialSwitch = !materialSwitch;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX << " Pos Y " << posY << endl;
		}
	}

	void scrollCallback(GLFWwindow *window, double deltaX, double deltaY){
		float sensitivityX = 0.2f;
		float sensitivityY = 0.2f;
		theta += deltaX * sensitivityX;
		phi -= deltaY * sensitivityY;

		// Clamp pitch to prevent gimbal lock
		float minPitch = glm::radians(-80.0f);
		float maxPitch = glm::radians(80.0f);
		phi = glm::clamp(phi, minPitch, maxPitch);
	}

	void resizeCallback(GLFWwindow * window, int width, int height){
		glViewport(0, 0, width, height);
	}

	void init(const std::string &resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addUniform("MatAmb");
		texProg->addUniform("MatDif");
		texProg->addUniform("MatSpec");
		texProg->addUniform("MatShine");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		// read in and load the texture
		texture0 = make_shared<Texture>();
		texture0->setFilename(resourceDirectory + "/rubberball.jpg");
		texture0->init();
		texture0->setUnit(0);
		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		texture2 = make_shared<Texture>();
		texture2->setFilename(resourceDirectory + "/cactus.jpg");
		texture2->init();
		texture2->setUnit(1);
		texture2->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// read in and load the texture
		texture1 = make_shared<Texture>();
		texture1->setFilename(resourceDirectory + "/whitesand.jpeg");
		texture1->init();
		texture1->setUnit(2);
		texture1->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// read in and load the texture
		texture3 = make_shared<Texture>();
		texture3->setFilename(resourceDirectory + "/skybox.jpeg");
		texture3->init();
		texture3->setUnit(3);
		texture3->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		stoneTex = make_shared<Texture>();
		stoneTex->setFilename(resourceDirectory + "/stone.jpg");
		stoneTex->init();
		stoneTex->setUnit(4);
		stoneTex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		marbleTex = make_shared<Texture>();
		marbleTex->setFilename(resourceDirectory + "/marble.jpg");
		marbleTex->init();
		marbleTex->setUnit(5);
		marbleTex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		woodTex = make_shared<Texture>();
		woodTex->setFilename(resourceDirectory + "/wood.jpg");
		woodTex->init();
		woodTex->setUnit(6);
		woodTex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		metalTex = make_shared<Texture>();
		metalTex->setFilename(resourceDirectory + "/metal.jpeg");
		metalTex->init();
		metalTex->setUnit(7);
		metalTex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// init splines up and down
		splinepath[0] = Spline(glm::vec3(-6, 0, 5), glm::vec3(-1, -5, 5), glm::vec3(1, 5, 5), glm::vec3(2, 0, 5), 5);
		splinepath[1] = Spline(glm::vec3(2, 0, 5), glm::vec3(3, -2, 5), glm::vec3(-0.25, 0.25, 5), glm::vec3(0, 0, 5), 5);
	}

	void initGeom(const std::string &resourceDirectory)
	{

		// Initialize cactus mesh.
		vector<tinyobj::shape_t> TOshapesB;
		vector<tinyobj::material_t> objMaterialsB;
		// load in the mesh and make the shape(s)
		string errStr;
		bool rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr, (resourceDirectory + "/cactus.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			for (size_t i = 0; i < TOshapesB.size(); ++i)
			{
				shared_ptr<Shape> shape = make_shared<Shape>();
				shape->createShape(TOshapesB[i]);
				shape->measure();
				shape->init();
				cactus.push_back(shape);
			}
		}

		// Initialize sphere mesh.
		vector<tinyobj::shape_t> TOshapesS;
		vector<tinyobj::material_t> objMaterialsS;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesS, objMaterialsS, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			sphere = make_shared<Shape>();
			sphere->createShape(TOshapesS[0]);
			sphere->measure();
			sphere->init();
		}

		// Initialize skybox mesh.
		vector<tinyobj::shape_t> TOshapesSky;
		vector<tinyobj::material_t> objMaterialsSky;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesSky, objMaterialsSky, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			skybox = make_shared<Shape>();
			skybox->createShape(TOshapesSky[0]);
			skybox->measure();
			skybox->init();
		}

		// Initialize spike mesh.
		vector<tinyobj::shape_t> TOshapesSpike;
		vector<tinyobj::material_t> objMaterialsSpike;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesSpike, objMaterialsSpike, errStr, (resourceDirectory + "/spike.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			spike = make_shared<Shape>();
			spike->createShape(TOshapesSpike[0]);
			spike->measure();
			spike->init();
		}

		// Initialize pillar mesh.
		vector<tinyobj::shape_t> TOshapesPillar;
		vector<tinyobj::material_t> objMaterialsPillar;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesPillar, objMaterialsPillar, errStr, (resourceDirectory + "/column.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			pillar = make_shared<Shape>();
			pillar->createShape(TOshapesPillar[0]);
			pillar->measure();
			pillar->init();
		}

		// Initialize axe mesh.
		vector<tinyobj::shape_t> TOshapesAxe;
		vector<tinyobj::material_t> objMaterialsAxe;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesAxe, objMaterialsAxe, errStr, (resourceDirectory + "/axe.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			axe = make_shared<Shape>();
			axe->createShape(TOshapesAxe[0]);
			axe->measure();
			axe->init();
		}

		// Initialize plank mesh.
		vector<tinyobj::shape_t> TOshapesPlank;
		vector<tinyobj::material_t> objMaterialsPlank;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesPlank, objMaterialsPlank, errStr, (resourceDirectory + "/plank.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			plank = make_shared<Shape>();
			plank->createShape(TOshapesPlank[0]);
			plank->measure();
			plank->init();
		}

		// Initialize blocky sphere mesh.
		vector<tinyobj::shape_t> TOshapesBS;
		vector<tinyobj::material_t> objMaterialsBS;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesBS, objMaterialsBS, errStr, (resourceDirectory + "/icoNoNormals.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			TOshapesBS[0].mesh.normals = TOshapesBS[0].mesh.positions;
			blockySphere = make_shared<Shape>();
			blockySphere->createShape(TOshapesBS[0]);
			blockySphere->measure();
			blockySphere->init();
		}

		// Initialize cube mesh.
		vector<tinyobj::shape_t> TOshapesC;
		vector<tinyobj::material_t> objMaterialsC;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesC, objMaterialsC, errStr, (resourceDirectory + "/cube.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			cube = make_shared<Shape>();
			cube->createShape(TOshapesC[0]);
			cube->measure();
			cube->init();
		}
		// Initialize cube with texture mesh.
		vector<tinyobj::shape_t> TOshapesCT;
		vector<tinyobj::material_t> objMaterialsCT;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesCT, objMaterialsCT, errStr, (resourceDirectory + "/cubeWithTex.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			cubeWithTex = make_shared<Shape>();
			cubeWithTex->createShape(TOshapesCT[0]);
			cubeWithTex->measure();
			cubeWithTex->init();
		}

		// Initialize rock scene mesh.
		vector<tinyobj::shape_t> TOshapesA;
		vector<tinyobj::material_t> objMaterialsA;
		// load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapesA, objMaterialsA, errStr, (resourceDirectory + "/desert.obj").c_str());
		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			for (size_t i = 0; i < TOshapesA.size(); ++i)
			{
				shared_ptr<Shape> shape = make_shared<Shape>();
				shape->createShape(TOshapesA[i]);
				shape->measure();
				shape->init();
				rockScene.push_back(shape);
			}
		}

	}

	// helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i)
	{

		switch (i)
		{
		case 0:
			glUniform3f(curS->getUniform("MatAmb"), 0.3f, 0.05f, 0.05f);
			glUniform3f(curS->getUniform("MatDif"), 0.9f, 0.1f, 0.1f);
			glUniform3f(curS->getUniform("MatSpec"), 0.5f, 0.3f, 0.3f);
			glUniform1f(curS->getUniform("MatShine"), 60.0f);
			break;

		case 1:
			glUniform3f(curS->getUniform("MatAmb"), 0.18f, 0.22f, 0.18f);
			glUniform3f(curS->getUniform("MatDif"), 0.3f, 0.35f, 0.3f);
			glUniform3f(curS->getUniform("MatSpec"), 0.12f, 0.15f, 0.12f);
			glUniform1f(curS->getUniform("MatShine"), 20.0f);
			break;

		case 2: // blue plastic
			if (materialSwitch)
			{
				glUniform3f(curS->getUniform("MatAmb"), 0.1f, 0.1f, 0.3f);
				glUniform3f(curS->getUniform("MatDif"), 0.1f, 0.3f, 0.8f);
				glUniform3f(curS->getUniform("MatSpec"), 0.4f, 0.4f, 0.45f);
				glUniform1f(curS->getUniform("MatShine"), 130.0f);
			}
			else
			{
				glUniform3f(curS->getUniform("MatAmb"), 0.2f, 0.1f, 0.2f);
				glUniform3f(curS->getUniform("MatDif"), 0.5f, 0.2f, 0.5f);
				glUniform3f(curS->getUniform("MatSpec"), 0.7f, 0.4f, 0.7f);
				glUniform1f(curS->getUniform("MatShine"), 100.0f);
			}
			break;

		case 3:
			glUniform3f(curS->getUniform("MatAmb"), 0.8f, 0.6f, 0.55f);
			glUniform3f(curS->getUniform("MatDif"), 0.85f, 0.85f, 0.8f);
			glUniform3f(curS->getUniform("MatSpec"), 0.3f, 0.1f, 0.1f);
			glUniform1f(curS->getUniform("MatShine"), 40.0f);
			break;
		case 4: // skybox material (bright)
			glUniform3f(curS->getUniform("MatAmb"), 1.0f, 1.0f, 1.0f);
			glUniform3f(curS->getUniform("MatDif"), 0.8f, 0.8f, 0.8f);
			glUniform3f(curS->getUniform("MatSpec"), 0.5f, 0.5f, 0.5f);
			glUniform1f(curS->getUniform("MatShine"), 20.0f);
			break;
		case 5: // titanium
			glUniform3f(curS->getUniform("MatAmb"), 0.4f, 0.4f, 0.4f);
			glUniform3f(curS->getUniform("MatDif"), 0.5f, 0.5f, 0.5f);
			glUniform3f(curS->getUniform("MatSpec"), 0.9f, 0.9f, 0.9f);
			glUniform1f(curS->getUniform("MatShine"), 140.0f);
			break;
		}
	}

	/* helper function to set model trasnforms */
	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS)
	{
		mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
		mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
		mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
		mat4 ctm = Trans * RotX * RotY * ScaleS;
		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack> M)
	{
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	}

	void handleAxeCollision(int axeIndex, float eTheta)
	{
		if (eTheta <= 0.4f && eTheta >= -0.4f && characterPos.y >= 4.5f && characterPos.x <= (-3.8f + (axeIndex)*10) && characterPos.x >= (-4.2f + (axeIndex)*10))
		{
			lost = true; // Player is hit by the axe
		}
	}

	void handleBlockCollision(vec3 blockPos, vec3 blockScale, float blockHeight, float &newFloorY)
	{
		vec3 blockMin = blockPos - blockScale * 0.5f;
		vec3 blockMax = blockPos + blockScale * 0.5f;

		if (characterPos.x >= blockMin.x && characterPos.x <= blockMax.x &&
			characterPos.z >= blockMin.z && characterPos.z <= blockMax.z)
		{
			if (characterPos.y < blockHeight)
			{
				float distToMinX = characterPos.x - blockMin.x;
				float distToMaxX = blockMax.x - characterPos.x;
				float distToMinZ = characterPos.z - blockMin.z;
				float distToMaxZ = blockMax.z - characterPos.z;

				float minDist = min({distToMinX, distToMaxX, distToMinZ, distToMaxZ});

				if (minDist == distToMinX)
				{
					characterPos.x = blockMin.x - 0.1f;
				}
				else if (minDist == distToMaxX)
				{
					characterPos.x = blockMax.x + 0.1f;
				}
				else if (minDist == distToMinZ)
				{
					characterPos.z = blockMin.z - 0.1f;
				}
				else if (minDist == distToMaxZ)
				{
					characterPos.z = blockMax.z + 0.1f;
				}
			}
			else
			{
				newFloorY = blockMax.y + 1.0f;
			}
		}
	}

	void handleSphereCollision(vec3 spherePos, float sphereRadius, float sphereHeight, float &newFloorY)
	{
		float sphereDistanceXZ = sqrt(pow(characterPos.x - spherePos.x, 2) +
									  pow(characterPos.z - spherePos.z, 2));

		if (sphereDistanceXZ <= sphereRadius)
		{
			if (characterPos.y < sphereHeight)
			{
				vec3 pushDirection = normalize(vec3(characterPos.x - spherePos.x, 0, characterPos.z - spherePos.z));
				vec3 edgePosition = spherePos + pushDirection * (sphereRadius + 0.1f);
				characterPos.x = edgePosition.x;
				characterPos.z = edgePosition.z;
			}
			else
			{
				float heightAboveCenter = sqrt(std::max(0.0f, sphereRadius * sphereRadius - sphereDistanceXZ * sphereDistanceXZ));
				float sphereTopY = spherePos.y + heightAboveCenter;
				newFloorY = std::max(newFloorY, sphereTopY);
			}
		}
	}

	void updatePlayerBasedOnCollisions()
	{
		float newFloorY = 0.0f;

		handleBlockCollision(vec3(-14, 1, -12), vec3(8, 5, 4), 6.5f, newFloorY);
		for (int i = 0; i < sphereNumber; i++)
		{
			handleSphereCollision(vec3(i*10, 4, -12), 2.0f, 6.7f, newFloorY);
		}

		for (int i = 0; i < 10; i++)
		{
			handleAxeCollision(i, eTheta);
			if (lost)
				break; // Exit early if player was hit
		}

		floorY = newFloorY + 2.0f;
		if (characterPos.y <= 2.0f)
		{
			lost = true;
		}

		if (characterPos.x >= 59.0f && characterPos.y >= 5.5f)
		{
			won = true; // Player reached the end
		}
	}

	void drawYouLost(shared_ptr<Program> prog, shared_ptr<MatrixStack> Model)
	{
		prog->bind();
		glUniform3f(prog->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		SetMaterial(prog, 0);

		Model->pushMatrix();
		Model->translate(vec3(-15, -20, 0)); // out of the world view

		// Y
		Model->pushMatrix();
		Model->translate(vec3(0, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(0, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(1, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(2, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(2, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(3, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(4, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(4, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// O
		Model->pushMatrix();
		Model->translate(vec3(6, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(7, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(7, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// U
		Model->pushMatrix();
		Model->translate(vec3(10, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(11, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// L
		Model->pushMatrix();
		Model->translate(vec3(15, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(16, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(17, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(18, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// O
		Model->pushMatrix();
		Model->translate(vec3(20, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(21, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(20, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(20, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(20, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(20, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(21, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// S
		Model->pushMatrix();
		Model->translate(vec3(24, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(26, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(24, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(24, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(26, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(26, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(24, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(26, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// T
		Model->pushMatrix();
		Model->translate(vec3(28, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(29, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(30, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(31, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(32, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(30, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(30, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(30, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(30, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		Model->popMatrix();
	}

	void drawYouWin(shared_ptr<Program> prog, shared_ptr<MatrixStack> Model)
	{
		prog->bind();
		glUniform3f(prog->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		SetMaterial(prog, 0);

		Model->pushMatrix();
		Model->translate(vec3(-15, -20, 0)); // out of the world view

		// Y
		Model->pushMatrix();
		Model->translate(vec3(0, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(0, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(1, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(2, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(2, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(3, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(4, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(4, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// O
		Model->pushMatrix();
		Model->translate(vec3(6, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(7, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(6, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(7, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(8, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// U
		Model->pushMatrix();
		Model->translate(vec3(10, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(10, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(11, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(12, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// W
		Model->pushMatrix();
		Model->translate(vec3(15, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(15, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(16, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(17, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(18, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(19, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(19, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(19, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(19, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(19, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// I
		Model->pushMatrix();
		Model->translate(vec3(21, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(23, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(21, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(22, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(23, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// N
		Model->pushMatrix();
		Model->translate(vec3(25, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(25, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(26, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(27, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(28, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(29, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(29, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(29, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(29, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(29, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		// !
		Model->pushMatrix();
		Model->translate(vec3(31, 4, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(31, 3, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(31, 2, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();
		Model->pushMatrix();
		Model->translate(vec3(31, 0, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cube->draw(prog);
		Model->popMatrix();

		Model->popMatrix();
	}

	void render(float frametime, GLFWwindow *window)
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use the matrix stack for Lab 6
		float aspect = width / (float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		
		View->pushMatrix();
		View->loadIdentity();
		vec3 lookAt = vec3(radius * cos(phi) * cos(theta), radius * sin(phi), radius * cos(phi) * cos((M_PI / 2.0) - theta));
		vec3 up = vec3(0, 1, 0);
		if (thirdPerson)
		{
			lookAt = characterPos;
			gCamPos = characterPos + vec3(radius * cos(theta), radius * sin(phi), radius * sin(theta));
		}
		else 
		{
			vec3 u = normalize(cross(up, lookAt));

			gCamPos += lookAt * gCamX;
			gCamPos += (gCamH * up);
			gCamPos += (gCamZ * u);
			lookAt = gCamPos + lookAt;
			if (goCamera)
			{
				if (!splinepath[0].isDone())
				{
					splinepath[0].update(frametime);
					gCamPos = splinepath[0].getPosition() + vec3(0, 5, 0);
				}
				else
				{
					splinepath[1].update(frametime);
					gCamPos = splinepath[1].getPosition() + vec3(0, 5, 0);
				}
				if (splinepath[1].isDone() && splinepath[0].isDone())
				{
					goCamera = false;
					splinepath[0] = Spline(glm::vec3(-6, 0, 5), glm::vec3(-1, -5, 5), glm::vec3(1, 5, 5), glm::vec3(2, 0, 5), 5);
					splinepath[1] = Spline(glm::vec3(2, 0, 5), glm::vec3(3, -2, 5), glm::vec3(-0.25, 0.25, 5), glm::vec3(0, 0, 5), 5);
				}
			}
		}
		gCamX = 0;
		gCamH = 0;
		gCamZ = 0;

		if (lost || won)
		{
			gCamPos = vec3(0, -16, 20);
			lookAt = vec3(0, -20, 0);
		}

		View->lookAt(gCamPos, lookAt, up);

		// Draw the scene
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		glUniform1i(texProg->getUniform("flip"), 1);
		texture2->bind(texProg->getUniform("Texture0"));
		// draw the cacti
		Model->pushMatrix();
		std::srand(1234);
		for (int i = -2; i < 5; i++)
		{
			for (int j = -2; j < 5; j++)
			{
				float dScale = 1.0 / (cactus[0]->max.x - cactus[0]->min.x);
				float sp = 15.0;
				float off = -3.5;
				Model->pushMatrix();
				float jitterX = -5 + (rand() % 10);
				float jitterZ = -5 + (rand() % 10);
				for (int k = 0; k < cactus.size(); k++)
				{
					Model->pushMatrix();
					Model->translate(vec3(off + sp * i, 0, off + sp * j));
					Model->translate(vec3(jitterX, 0, jitterZ));
					Model->scale(vec3(dScale / 1.5));
					Model->translate(vec3(0, -4, 0));
					SetMaterial(texProg, 1);
					glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
					cactus[k]->draw(texProg);
					Model->popMatrix();
				}
				Model->popMatrix();
			}
		}
		Model->popMatrix();
		texProg->unbind();

		// Drawing the background
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		texture1->bind(texProg->getUniform("Texture0"));
		// Draw the ground obj multiple times to make it bigger
		for (int j = -7; j < 9; j++)
		{
			for (int i = -9; i < 9; i++)
			{
				Model->pushMatrix();
				Model->scale(vec3(0.5));
				SetMaterial(texProg, 3);
				if (i % 2 != 0)
				{
					Model->scale(vec3(1, 1, -1));
					Model->translate(vec3(0, 0, i * (rockScene[0]->max.z - rockScene[0]->min.z) - 0.5 * i));
					if (j % 2 != 0)
					{
						Model->scale(vec3(-1, 1, 1));
						Model->translate(vec3(-j * (rockScene[0]->max.x - rockScene[0]->min.x) + 0.5 * j, 0, 0));
					}
					else
					{
						Model->translate(vec3(j * (rockScene[0]->max.x - rockScene[0]->min.x) - 0.5 * j, 0, 0));
					}
				}
				else
				{
					Model->translate(vec3(0, 0, -i * (rockScene[0]->max.z - rockScene[0]->min.z) + (0.5 * i)));
					if (j % 2 != 0)
					{
						Model->scale(vec3(-1, 1, 1));
						Model->translate(vec3(-j * (rockScene[0]->max.x - rockScene[0]->min.x) + 0.5 * j, 0, 0));
					}
					else
					{
						Model->translate(vec3(j * (rockScene[0]->max.x - rockScene[0]->min.x) - 0.5 * j, 0, 0));
					}
				}

				glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
				rockScene[0]->draw(texProg);
				Model->popMatrix();
			}
		}

		texProg->unbind();

		for (int i = 0; i < sphereNumber; i++)
		{
			texProg->bind();
			glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
			glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
			glUniform3f(texProg->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
			Model->pushMatrix();
			Model->translate(vec3(i*10, 4, -12));
			SetMaterial(texProg, 3);
			texture1->bind(texProg->getUniform("Texture0"));
			Model->pushMatrix();
			Model->translate(vec3(0, -4, 0));
			Model->pushMatrix();
			Model->scale(vec3(0.5, 4, 0.5));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			sphere->draw(texProg);
			Model->popMatrix();
			Model->scale(vec3(0.5, 4, 0.5));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			sphere->draw(texProg);
			Model->popMatrix();
			texture0->bind(texProg->getUniform("Texture0"));
			Model->scale(vec3(2));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			sphere->draw(texProg);
			Model->popMatrix();
			texProg->unbind();
		}

		// platform
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		texture0->bind(texProg->getUniform("Texture0"));
		SetMaterial(texProg, 3);
		Model->pushMatrix();
		Model->translate(vec3(-14, 1, -12));
		Model->pushMatrix();
		Model->scale(vec3(8, 5, 4));
		Model->pushMatrix();
		Model->translate(vec3(0, 0.2, 0));
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		cubeWithTex->draw(texProg);
		Model->popMatrix();
		Model->popMatrix();
		Model->popMatrix();
		texProg->unbind();

		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(prog->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		if (lost)
		{
			drawYouLost(prog, Model);
		}
		else if (won) {
			drawYouWin(prog, Model);
		}
		prog->unbind();

		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		// draw the spikes

		for (int i = 0; i < 20; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				Model->pushMatrix();
				Model->translate(vec3(i*4 - 9 + (j%3), 0, -9.5 - j));
				Model->pushMatrix();
				Model->scale(vec3(0.001));
				SetMaterial(texProg, 3);
				stoneTex->bind(texProg->getUniform("Texture0"));
				glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
				spike->draw(texProg);
				Model->popMatrix();
				Model->popMatrix();
			}
		}

		SetMaterial(texProg, 3);
		woodTex->bind(texProg->getUniform("Texture0"));
		Model->pushMatrix();
		Model->translate(vec3(32, 14.4, -12.9));
		Model->scale(vec3(5, 1, 1));
		Model->rotate(glm::radians(90.0f), vec3(0, 1, 0));
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		plank->draw(texProg);
		Model->popMatrix();

		// draw the pillars, planks, and axes
		for (int i = 0; i < 10; i++)
		{
			SetMaterial(texProg, 3);
			woodTex->bind(texProg->getUniform("Texture0"));
			Model->pushMatrix();
			Model->pushMatrix();
			Model->translate(vec3(i * 10 - 9, 14.4, -12.5));
			Model->pushMatrix();
			Model->scale(vec3(1, 2.4, 1.01));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			plank->draw(texProg);
			Model->popMatrix();
			Model->popMatrix();
			Model->popMatrix();
			marbleTex->bind(texProg->getUniform("Texture0"));
			Model->pushMatrix();
			Model->translate(vec3(i * 10 - 9, 0, -20));
			Model->scale(vec3(1, 2, 1));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			pillar->draw(texProg);
			Model->translate(vec3(0, 0, 15));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			pillar->draw(texProg);
			Model->popMatrix();

			Model->pushMatrix();
			metalTex->bind(texProg->getUniform("Texture0"));
			SetMaterial(texProg, 5); // titanium material
			Model->translate(vec3(i * 10 - 4, 10, -13));
			Model->pushMatrix();
			Model->translate(vec3(0, 4.5, 0));
			Model->rotate(glm::radians(90.0f), vec3(0, 1, 0));
			if (i % 2 == 0)
			{
				Model->rotate(-eTheta + glm::radians(180.0f), vec3(0, 0, 1));
			}
			else
			{
				Model->rotate(eTheta + glm::radians(180.0f), vec3(0, 0, 1));
			}
			Model->translate(vec3(0, 2, 0));
			Model->scale(vec3(1.5));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			axe->draw(texProg);
			Model->popMatrix();
			Model->popMatrix();
		}
		texProg->unbind();

		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		characterAnimated = false;

		// Movement input
		bool movingLeft = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
		bool movingRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
		bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

		// Horizontal movement (same as before)
		if (movingLeft && !movingRight)
		{
			velocityX -= acceleration * frametime;
			if (velocityX < -maxSpeed)
				velocityX = -maxSpeed;
			characterAnimated = true;
		}
		else if (movingRight && !movingLeft)
		{
			velocityX += acceleration * frametime;
			if (velocityX > maxSpeed)
				velocityX = maxSpeed;
			characterAnimated = true;
		}
		else
		{
			// No input
			if (velocityX > 0.0f)
			{
				velocityX -= deceleration * frametime;
				if (velocityX < 0.0f)
					velocityX = 0.0f;
			}
			else if (velocityX < 0.0f)
			{
				velocityX += deceleration * frametime;
				if (velocityX > 0.0f)
					velocityX = 0.0f;
			}
		}

		updatePlayerBasedOnCollisions();

		// Jumping logic
		bool onGround = (characterPos.y <= floorY + 0.1f);
		if (jumpPressed && onGround && !jumpKeyHeld)
		{
			velocityY = jumpForce;
			jumpKeyHeld = true; // Prevent continuous jumping while holding space
		}
		if (!jumpPressed)
		{
			jumpKeyHeld = false; // Reset when space is released
		}

		// Apply gravity
		velocityY -= gravity * frametime;

		// Update positions
		characterPos.x += velocityX * frametime;
		characterPos.y += velocityY * frametime;

		// Ground collision
		if (characterPos.y <= floorY)
		{
			characterPos.y = floorY;
			velocityY = 0.0f; // Stop downward velocity when hitting ground
		}

		// draw the character
		Model->pushMatrix();
		SetMaterial(prog, 1);
		Model->translate(characterPos);
		Model->pushMatrix();
		Model->scale(vec3(0.5, 0.7, 0.5));
		Model->pushMatrix();
		Model->scale(vec3(2));
		Model->rotate(glm::radians(90.0f), vec3(0, 1, 0));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		blockySphere->draw(prog);

		// right leg
		Model->pushMatrix();
		Model->translate(vec3(-0.3, -0.8, 0));
		Model->pushMatrix();
		Model->rotate(sTheta * 0.8, vec3(1, 0, 0));
		Model->translate(vec3(0, -0.3, 0));
		Model->pushMatrix();
		Model->scale(vec3(0.2, 0.4, 0.2));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		blockySphere->draw(prog);
		Model->popMatrix();
		Model->popMatrix();
		Model->popMatrix();
		// left leg
		Model->pushMatrix();
		Model->translate(vec3(0.3, -0.8, 0));
		Model->pushMatrix();
		Model->rotate(-sTheta * 0.8, vec3(1, 0, 0));
		Model->translate(vec3(0, -0.3, 0));
		Model->pushMatrix();
		Model->scale(vec3(0.2, 0.4, 0.2));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		blockySphere->draw(prog);
		Model->popMatrix();
		Model->popMatrix();
		Model->popMatrix();

		// right arm
		Model->pushMatrix();
		Model->translate(vec3(1, 0, 0));
		Model->pushMatrix();
		Model->rotate(sTheta, vec3(1, 0, 0));
		Model->translate(vec3(0, -0.3, 0));
		Model->pushMatrix();
		Model->scale(vec3(0.2, 0.4, 0.2));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		blockySphere->draw(prog);
		Model->popMatrix();
		Model->popMatrix();
		Model->popMatrix();
		// left arm
		Model->pushMatrix();
		Model->translate(vec3(-1, 0, 0));
		Model->pushMatrix();
		Model->rotate(-sTheta, vec3(1, 0, 0));
		Model->translate(vec3(0, -0.3, 0));
		Model->pushMatrix();
		Model->scale(vec3(0.2, 0.4, 0.2));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		blockySphere->draw(prog);
		Model->popMatrix();
		Model->popMatrix();
		Model->popMatrix();

		Model->popMatrix();
		Model->popMatrix();
		Model->popMatrix();
		prog->unbind();

		// Draw the skybox
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), -2.0 + lightTrans, 2.0, 2.0);
		SetMaterial(texProg, 4);
		texture3->bind(texProg->getUniform("Texture0"));
		Model->pushMatrix();
		Model->translate(vec3(0, 0, 0));
		Model->scale(vec3(60));
		Model->translate(vec3(0, 0.1, 0)); // move skybox up slightly for aesthetics
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		skybox->draw(texProg);
		Model->popMatrix();
		texProg->unbind();

		if (characterAnimated)
		{
			sTheta = sin(glfwGetTime() * 4);
		}
		eTheta = cos(glfwGetTime() *1.5);
		hTheta = std::max(0.0f, (float)cos(glfwGetTime()));

		Projection->popMatrix();
		View->popMatrix();
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	WindowManager *windowManager = new WindowManager();
	windowManager->init(WINDOW_WIDTH, WINDOW_HEIGHT);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime, application->windowManager->getHandle());

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
