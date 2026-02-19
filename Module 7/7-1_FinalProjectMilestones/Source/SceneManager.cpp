///////////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
/////////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <iostream>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the number of loaded textures
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename
			<< ", width:" << width
			<< ", height:" << height
			<< ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
				width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with "
				<< colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}


/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return textureID;
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return textureSlot;
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return false;
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return bFound;
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	glm::vec3 offset)
{
	glm::mat4 model;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1));
	translation = glm::translate(positionXYZ + offset);

	model = translation * rotationZ * rotationY * rotationX * scale;

	if (m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, model);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor(redColorValue, greenColorValue, blueColorValue, alphaValue);

	if (m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		int textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		if (FindMaterial(materialTag, material))
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// clear out any previous materials
	m_objectMaterials.clear();

	// wood floor/base material
	OBJECT_MATERIAL woodPanel;
	woodPanel.diffuseColor = glm::vec3(1.00f, 1.00f, 1.00f);
	woodPanel.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	woodPanel.shininess = 20.0f;
	woodPanel.tag = "woodPanel";
	m_objectMaterials.push_back(woodPanel);

	// painted wall
	OBJECT_MATERIAL paintWall;
	paintWall.diffuseColor = glm::vec3(1.00f, 1.00f, 1.00f);
	paintWall.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	paintWall.shininess = 12.0f;
	paintWall.tag = "paintWall";
	m_objectMaterials.push_back(paintWall);

	// clear plastic stand
	OBJECT_MATERIAL clearPlastic;
	clearPlastic.diffuseColor = glm::vec3(1.00f, 1.00f, 1.00f);
	clearPlastic.specularColor = glm::vec3(0.85f, 0.85f, 0.85f);
	clearPlastic.shininess = 72.0f;
	clearPlastic.tag = "clearPlastic";
	m_objectMaterials.push_back(clearPlastic);

	// red plastic cartridge
	OBJECT_MATERIAL redPlastic;
	redPlastic.diffuseColor = glm::vec3(1.00f, 1.00f, 1.00f);
	redPlastic.specularColor = glm::vec3(0.75f, 0.75f, 0.75f);
	redPlastic.shininess = 64.0f;
	redPlastic.tag = "redPlastic";
	m_objectMaterials.push_back(redPlastic);

	// paper/label material
	OBJECT_MATERIAL paperLabel;
	paperLabel.diffuseColor = glm::vec3(1.00f, 1.00f, 1.00f);
	paperLabel.specularColor = glm::vec3(0.08f, 0.08f, 0.08f);
	paperLabel.shininess = 6.0f;
	paperLabel.tag = "paperLabel";
	m_objectMaterials.push_back(paperLabel);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// enable custom lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// ============================================================
	// LIGHT #1 (REDDISH) - Point Light for Red + Blue
	// ============================================================
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(5.0f, 7.0f, -6.5f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.010f, 0.004f, 0.004f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.55f, 0.14f, 0.14f));
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.65f, 0.20f, 0.20f));
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// ============================================================
	// LIGHT #2 (WARM WHITE) - Point Light ABOVE top shelf
	// ============================================================
	m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(10.0f, 15.0f, -6.5f));
	m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.030f, 0.028f, 0.024f));
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(1.00f, 0.93f, 0.82f));
	m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(1.00f, 0.98f, 0.92f));
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Turn off unused point lights
	m_pShaderManager->setBoolValue("pointLights[2].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", false);

	// ============================================================
	// LIGHT #3 (WARM WHITE) - SPOTLIGHT
	// ============================================================
	const glm::vec3 spotPos = glm::vec3(-2.0f, 14.0f, 2.0f);
	const glm::vec3 spotTarget = glm::vec3(8.9f, 12.0f, -6.5f);

	// Precomputed normalized direction from spotPos -> spotTarget
	// (11, -2, -8.5) normalized ≈ (0.783, -0.142, -0.605)
	const glm::vec3 spotDir = glm::vec3(0.783f, -0.142f, -0.605f);

	m_pShaderManager->setVec3Value("spotLight.position", spotPos);
	m_pShaderManager->setVec3Value("spotLight.direction", spotDir);

	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(28.0f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(48.0f)));

	// Warm white Phong components
	m_pShaderManager->setVec3Value("spotLight.ambient", glm::vec3(0.020f, 0.019f, 0.017f));
	m_pShaderManager->setVec3Value("spotLight.diffuse", glm::vec3(1.05f, 0.98f, 0.88f));
	m_pShaderManager->setVec3Value("spotLight.specular", glm::vec3(1.00f, 0.98f, 0.92f));

	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.040f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.0080f);

	m_pShaderManager->setBoolValue("spotLight.bActive", true);

	m_pShaderManager->setBoolValue("directionalLight.bActive", false);
}



/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadTorusMesh(); 

	// load textures from file and bind to texture units
	CreateGLTexture("textures/red-plastic.jpg", "red-plastic");
	CreateGLTexture("textures/blue-plastic.jpg", "blue-plastic");
	CreateGLTexture("textures/yellow-plastic.jpg", "yellow-plastic");
	CreateGLTexture("textures/green-plastic.jpg", "green-plastic");
	CreateGLTexture("textures/black-wood.jpg", "black-wood");
	CreateGLTexture("textures/brown-paint.jpg", "copper-paint");
	CreateGLTexture("textures/glass.png", "clear-plastic");
	CreateGLTexture("textures/label.png", "label");
	CreateGLTexture("textures/label-blue.png", "label-blue");
	CreateGLTexture("textures/label-yellow.png", "label-yellow");
	CreateGLTexture("textures/label-green.png", "label-green");
	CreateGLTexture("textures/logo.png", "logo");
	BindGLTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	glm::vec3 offsetXYZ = glm::vec3(0.0f);

	// rotate a local position around Y axis (in degrees)
	auto RotateLocalY = [](const glm::vec3& localPos, float yawDeg) -> glm::vec3
		{
			glm::mat4 r = glm::rotate(glm::radians(yawDeg), glm::vec3(0, 1, 0));
			return glm::vec3(r * glm::vec4(localPos, 1.0f));
		};

	// common helper that "locks" an object to a group yaw by rotating its local position
	auto SetLocked = [&](glm::vec3 scale, float rx, float ryGroup, float rz, glm::vec3 localPos, glm::vec3 groupOffset)
		{
			glm::vec3 rotatedLocal = RotateLocalY(localPos, ryGroup);
			SetTransformations(scale, rx, ryGroup, rz, rotatedLocal, groupOffset);
		};

	// ===============================
	// Environment
	// ===============================

	// Wall panel (textured: copper-paint.jpg)
	scaleXYZ = glm::vec3(28.0f, 1.0f, 18.0f);
	positionXYZ = glm::vec3(0.0f, 12.0f, -16.0f);
	SetTransformations(scaleXYZ, 90, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("copper-paint");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paintWall");
	m_basicMeshes->DrawPlaneMesh();

	// Second wall panel (textured: copper-paint.jpg)
	scaleXYZ = glm::vec3(28.0f, 1.0f, 18.0f);
	positionXYZ = glm::vec3(14.0f, 12.0f, -2.0f);
	SetTransformations(scaleXYZ, 90, -90, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("copper-paint");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paintWall");
	m_basicMeshes->DrawPlaneMesh();

	// ===============================
	// Floating shelves (black-wood) on wall
	// ===============================

	// ------------------------------------------------------------
	// TOP shelf on RIGHT WALL
	// ------------------------------------------------------------
	const float TOP_RIGHT_WALL_YAW = -90.0f; // shelf yaw only
	// center shelf so its BACK face sits on the right wall at x=14.0f
	// and shorten shelf so it does not clip through the back wall.
	const float RIGHT_WALL_X = 14.0f;
	const float TOP_SHELF_DEPTH = 10.2f;         
	const float TOP_SHELF_LENGTH = 18.0f;       
	const glm::vec3 TOP_RIGHT_WALL_SHELF_POS =
		glm::vec3(RIGHT_WALL_X - (TOP_SHELF_DEPTH * 0.5f), 11.6f, -6.5f);

	// Shelf on right wall (same style, rotated to mount on right wall)
	// scaleX = length along the wall (becomes world Z after yaw)
	//       scaleZ = depth out of wall (becomes world X after yaw)
	scaleXYZ = glm::vec3(TOP_SHELF_LENGTH, 0.75f, TOP_SHELF_DEPTH);
	positionXYZ = TOP_RIGHT_WALL_SHELF_POS;
	SetTransformations(scaleXYZ, 0, TOP_RIGHT_WALL_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("black-wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("woodPanel");
	m_basicMeshes->DrawBoxMesh();

	// ------------------------------------------------------------
	// Lower shelf
	// ------------------------------------------------------------
	const float BACK_WALL_Z = -16.0f;

	const float LOWER_SHELF_DEPTH = TOP_SHELF_DEPTH;     
	const float LOWER_SHELF_LENGTH = TOP_SHELF_LENGTH; 
	const float LOWER_SHELF_CENTER_X = RIGHT_WALL_X - (LOWER_SHELF_LENGTH * 0.5f);

	scaleXYZ = glm::vec3(LOWER_SHELF_LENGTH, 0.75f, LOWER_SHELF_DEPTH);
	positionXYZ = glm::vec3(
		LOWER_SHELF_CENTER_X,
		0.6f,
		BACK_WALL_Z + (LOWER_SHELF_DEPTH * 0.5f) + 0.02f
	);

	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("black-wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("woodPanel");
	m_basicMeshes->DrawBoxMesh();

	// ===============================
	// Shelf placement offsets
	// ===============================

	const float TOP_SHELF_SURFACE_Y = 11.95f;
	const float TOP_SHELF_X = TOP_RIGHT_WALL_SHELF_POS.x;

	// place two games along the wall, both facing forward toward camera
	const glm::vec3 TOP_RIGHT_WALL_LEFT_OFFSET = glm::vec3(TOP_SHELF_X, TOP_SHELF_SURFACE_Y, -12.0f);
	const glm::vec3 TOP_RIGHT_WALL_RIGHT_OFFSET = glm::vec3(TOP_SHELF_X, TOP_SHELF_SURFACE_Y, -4.0f);

	const glm::vec3 LOWER_RIGHT_SHELF_OFFSET = glm::vec3(5.2f + LOWER_SHELF_CENTER_X, 0.95f, -11.70f);
	const glm::vec3 LOWER_LEFT_SHELF_OFFSET = glm::vec3(-5.2f + LOWER_SHELF_CENTER_X, 0.95f, -11.70f);

	// ============================================================
	// Pokemon Red cartridge
	// ============================================================

	offsetXYZ = LOWER_LEFT_SHELF_OFFSET;

	// ===============================
	// Display stand
	// ===============================

	// Stand base 
	scaleXYZ = glm::vec3(7.5f, 0.35f, 4.2f);
	positionXYZ = glm::vec3(0.0f, 0.20f, 1.2f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Stand legs/prisms
	scaleXYZ = glm::vec3(0.55f, 1.20f, 0.70f);

	positionXYZ = glm::vec3(-2.4f, 1.05f, 1.85f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	positionXYZ = glm::vec3(2.4f, 1.05f, 1.85f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	// Stand top bar
	scaleXYZ = glm::vec3(0.55f, 6.2f, 0.55f);
	positionXYZ = glm::vec3(3.10f, 1.55f, -0.35f);
	SetTransformations(scaleXYZ, 0, 0, 90, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawCylinderMesh();

	// Torus brace (upright ring brace)
	scaleXYZ = glm::vec3(0.85f, 0.85f, 0.22f);
	positionXYZ = glm::vec3(0.0f, 1.92f, -0.35f);
	SetTransformations(scaleXYZ, 0, 90, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawTorusMesh();

	// ===============================
	// Game Boy cartridge - Pokemon Red
	// ===============================

	const float CART_THICKNESS = 1.10f;
	const float CART_BACK_Z = 0.0f;
	const float CART_Z_CENTER = CART_BACK_Z + (CART_THICKNESS * 0.5f);
	const float CART_FRONT_Z = CART_BACK_Z + CART_THICKNESS;
	const float HEADER_PUSH_BACK = 0.48f;

	XrotationDegrees = -8.0f;

	// Main body
	scaleXYZ = glm::vec3(6.2f, 7.0f, CART_THICKNESS);
	positionXYZ = glm::vec3(0.0f, 4.60f, CART_Z_CENTER);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("red-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Logo decal
	scaleXYZ = glm::vec3(2.4f, 1.5f, 0.55f);
	positionXYZ = glm::vec3(0.0f, 7.30f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetTransformations(scaleXYZ, XrotationDegrees + 90.0f, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("logo");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawPlaneMesh();

	// Top notch cap
	scaleXYZ = glm::vec3(5.45f, 0.28f, CART_THICKNESS);
	positionXYZ = glm::vec3(-0.28f, 8.24f, CART_Z_CENTER - HEADER_PUSH_BACK);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("red-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	// White label
	scaleXYZ = glm::vec3(4.6f, 3.6f, 0.05f);
	positionXYZ = glm::vec3(0.0f, 4.80f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("label");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawBoxMesh();

	// ============================================================
	// Pokemon Blue cartridge
	// ============================================================

	offsetXYZ = LOWER_RIGHT_SHELF_OFFSET;  

	// Stand base
	scaleXYZ = glm::vec3(7.5f, 0.35f, 4.2f);
	positionXYZ = glm::vec3(0.0f, 0.20f, 1.2f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Stand legs/prisms
	scaleXYZ = glm::vec3(0.55f, 1.20f, 0.70f);

	positionXYZ = glm::vec3(-2.4f, 1.05f, 1.85f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	positionXYZ = glm::vec3(2.4f, 1.05f, 1.85f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	// Stand top bar
	scaleXYZ = glm::vec3(0.55f, 6.2f, 0.55f);
	positionXYZ = glm::vec3(3.10f, 1.55f, -0.35f);
	SetTransformations(scaleXYZ, 0, 0, 90, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawCylinderMesh();

	// Torus brace
	scaleXYZ = glm::vec3(0.85f, 0.85f, 0.22f);
	positionXYZ = glm::vec3(0.0f, 1.92f, -0.35f);
	SetTransformations(scaleXYZ, 0, 90, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawTorusMesh();

	// Cartridge - Blue
	XrotationDegrees = -8.0f;

	scaleXYZ = glm::vec3(6.2f, 7.0f, CART_THICKNESS);
	positionXYZ = glm::vec3(0.0f, 4.60f, CART_Z_CENTER);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("blue-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.4f, 1.5f, 0.55f);
	positionXYZ = glm::vec3(0.0f, 7.30f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetTransformations(scaleXYZ, XrotationDegrees + 90.0f, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("logo");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(5.45f, 0.28f, CART_THICKNESS);
	positionXYZ = glm::vec3(-0.28f, 8.24f, CART_Z_CENTER - HEADER_PUSH_BACK);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("blue-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.6f, 3.6f, 0.05f);
	positionXYZ = glm::vec3(0.0f, 4.80f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("label-blue");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawBoxMesh();

	// ============================================================
	// Pokemon Yellow cartridge
	// ============================================================

	offsetXYZ = TOP_RIGHT_WALL_LEFT_OFFSET + glm::vec3(0.0f, 0.0f, 10.5f);

	const float YELLOW_YAW = -90.0f; 

	// Stand base
	scaleXYZ = glm::vec3(7.5f, 0.35f, 4.2f);
	positionXYZ = glm::vec3(0.0f, 0.20f, 1.2f);
	SetLocked(scaleXYZ, 0, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Stand legs/prisms
	scaleXYZ = glm::vec3(0.55f, 1.20f, 0.70f);

	positionXYZ = glm::vec3(-2.4f, 1.05f, 1.85f);
	SetLocked(scaleXYZ, 0, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	positionXYZ = glm::vec3(2.4f, 1.05f, 1.85f);
	SetLocked(scaleXYZ, 0, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	// Torus + brace alignment
	const float YELLOW_BRACE_YAW = YELLOW_YAW + 90.0f;

	const glm::vec3 YELLOW_TORUS_LOCAL = glm::vec3(0.0f, 1.92f, -0.35f + 0.05f);
	const glm::vec3 YELLOW_BAR_LOCAL = glm::vec3(0.10f, 1.92f, -3.10f + 0.05f);

	// Stand top bar (brace behind cartridge)
	scaleXYZ = glm::vec3(0.55f, 6.2f, 0.55f);
	positionXYZ = YELLOW_BAR_LOCAL;
	SetLocked(scaleXYZ, 90, YELLOW_YAW + 90.0f, 90, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawCylinderMesh();

	// Torus brace (upright ring)
	scaleXYZ = glm::vec3(0.85f, 0.85f, 0.22f);
	positionXYZ = YELLOW_TORUS_LOCAL;
	SetLocked(scaleXYZ, 0.0f, YELLOW_BRACE_YAW, 0.0f, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawTorusMesh();

	// Cartridge - Yellow
	XrotationDegrees = -8.0f;

	// Main body
	scaleXYZ = glm::vec3(6.2f, 7.0f, CART_THICKNESS);
	positionXYZ = glm::vec3(0.0f, 4.60f, CART_Z_CENTER);
	SetLocked(scaleXYZ, XrotationDegrees, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("yellow-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Logo decal
	scaleXYZ = glm::vec3(2.4f, 1.5f, 0.55f);
	positionXYZ = glm::vec3(0.0f, 7.30f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetLocked(scaleXYZ, XrotationDegrees + 90.0f, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("logo");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawPlaneMesh();

	// Top notch cap
	scaleXYZ = glm::vec3(5.45f, 0.28f, CART_THICKNESS);
	positionXYZ = glm::vec3(-0.28f, 8.24f, CART_Z_CENTER - HEADER_PUSH_BACK);
	SetLocked(scaleXYZ, XrotationDegrees, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("yellow-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Label
	scaleXYZ = glm::vec3(4.6f, 3.6f, 0.05f);
	positionXYZ = glm::vec3(0.0f, 4.80f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetLocked(scaleXYZ, XrotationDegrees, YELLOW_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("label-yellow");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawBoxMesh();


	// ============================================================
	// Pokemon Green cartridge
	// ============================================================
	offsetXYZ = TOP_RIGHT_WALL_LEFT_OFFSET + glm::vec3(0.0f, 0.0f, 0.5f);

	const float GREEN_YAW = -90.0f;

	// Stand base
	scaleXYZ = glm::vec3(7.5f, 0.35f, 4.2f);
	positionXYZ = glm::vec3(0.0f, 0.20f, 1.2f);
	SetLocked(scaleXYZ, 0, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Stand legs/prisms
	scaleXYZ = glm::vec3(0.55f, 1.20f, 0.70f);

	positionXYZ = glm::vec3(-2.4f, 1.05f, 1.85f);
	SetLocked(scaleXYZ, 0, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	positionXYZ = glm::vec3(2.4f, 1.05f, 1.85f);
	SetLocked(scaleXYZ, 0, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawPrismMesh();

	// --- Torus + brace alignment ---
	const float GREEN_BRACE_YAW = GREEN_YAW + 90.0f;

	const glm::vec3 GREEN_TORUS_LOCAL = glm::vec3(0.0f, 1.92f, -0.35f + 0.05f);
	const glm::vec3 GREEN_BAR_LOCAL = glm::vec3(0.10f, 1.92f, -3.10f + 0.05f);

	// Stand top bar (brace behind cartridge)
	scaleXYZ = glm::vec3(0.55f, 6.2f, 0.55f);
	positionXYZ = GREEN_BAR_LOCAL;
	SetLocked(scaleXYZ, 90, GREEN_YAW + 90.0f, 90, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawCylinderMesh();

	// Torus brace (upright ring)
	scaleXYZ = glm::vec3(0.85f, 0.85f, 0.22f);
	positionXYZ = GREEN_TORUS_LOCAL;
	SetLocked(scaleXYZ, 0.0f, GREEN_BRACE_YAW, 0.0f, positionXYZ, offsetXYZ);
	SetShaderTexture("clear-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("clearPlastic");
	m_basicMeshes->DrawTorusMesh();

	// Cartridge - Green
	XrotationDegrees = -8.0f;

	// Main body
	scaleXYZ = glm::vec3(6.2f, 7.0f, CART_THICKNESS);
	positionXYZ = glm::vec3(0.0f, 4.60f, CART_Z_CENTER);
	SetLocked(scaleXYZ, XrotationDegrees, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("green-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Logo decal
	scaleXYZ = glm::vec3(2.4f, 1.5f, 0.55f);
	positionXYZ = glm::vec3(0.0f, 7.30f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetLocked(scaleXYZ, XrotationDegrees + 90.0f, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("logo");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawPlaneMesh();

	// Top notch cap
	scaleXYZ = glm::vec3(5.45f, 0.28f, CART_THICKNESS);
	positionXYZ = glm::vec3(-0.28f, 8.24f, CART_Z_CENTER - HEADER_PUSH_BACK);
	SetLocked(scaleXYZ, XrotationDegrees, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("green-plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("redPlastic");
	m_basicMeshes->DrawBoxMesh();

	// Label
	scaleXYZ = glm::vec3(4.6f, 3.6f, 0.05f);
	positionXYZ = glm::vec3(0.0f, 4.80f, CART_FRONT_Z - (scaleXYZ.z * 0.5f));
	SetLocked(scaleXYZ, XrotationDegrees, GREEN_YAW, 0, positionXYZ, offsetXYZ);
	SetShaderTexture("label-green");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("paperLabel");
	m_basicMeshes->DrawBoxMesh();

}
