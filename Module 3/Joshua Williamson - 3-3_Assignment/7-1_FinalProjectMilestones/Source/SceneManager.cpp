///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

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
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  SetTransformations()
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
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	glm::vec3 offsetXYZ = glm::vec3(0.0f);
	float XrotationDegrees = 0.0f;

	// ===============================
	// Environment
	// ===============================

	scaleXYZ = glm::vec3(28.0f, 1.0f, 18.0f);
	positionXYZ = glm::vec3(0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(28.0f, 1.0f, 18.0f);
	positionXYZ = glm::vec3(0.0f, 12.0f, -16.0f);
	SetTransformations(scaleXYZ, 90, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.90f, 0.80f, 0.62f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// ===============================
	// Display stand
	// ===============================

	scaleXYZ = glm::vec3(7.5f, 0.35f, 4.2f);
	positionXYZ = glm::vec3(0.0f, 0.20f, 1.2f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.75f, 0.78f, 0.82f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.55f, 1.20f, 0.70f);

	positionXYZ = glm::vec3(-2.4f, 1.05f, 1.85f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	m_basicMeshes->DrawPrismMesh();

	positionXYZ = glm::vec3(2.4f, 1.05f, 1.85f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	m_basicMeshes->DrawPrismMesh();

	scaleXYZ = glm::vec3(6.2f, 0.55f, 0.55f);
	positionXYZ = glm::vec3(0.0f, 1.55f, 0.15f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ, offsetXYZ);
	m_basicMeshes->DrawBoxMesh();


	// ===============================
	// Game Boy cartridge
	// ===============================

	const float CART_THICKNESS = 1.10f;
	const float CART_BACK_Z = 0.0f;
	const float CART_Z_CENTER = CART_BACK_Z + (CART_THICKNESS * 0.5f);
	const float CART_FRONT_Z = CART_BACK_Z + CART_THICKNESS;

	// push header + notch further back INTO the body to match thickness of body
	const float HEADER_PUSH_BACK = 0.48f;   

	XrotationDegrees = -8.0f;

	// Main body
	scaleXYZ = glm::vec3(6.2f, 7.0f, CART_THICKNESS);
	positionXYZ = glm::vec3(0.0f, 4.60f, CART_Z_CENTER);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.78f, 0.10f, 0.10f, 1.0f);
	m_basicMeshes->DrawBoxMesh();


	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.82f, 0.14f, 0.14f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Top notch cap
	scaleXYZ = glm::vec3(5.45f, 0.28f, CART_THICKNESS);
	positionXYZ = glm::vec3(
		-0.28f,
		8.24f,
		CART_Z_CENTER - HEADER_PUSH_BACK
	);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.78f, 0.10f, 0.10f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// White label
	scaleXYZ = glm::vec3(4.6f, 3.6f, 0.05f);
	positionXYZ = glm::vec3(
		0.0f,
		5.20f,
		CART_FRONT_Z - (scaleXYZ.z * 0.5f)
	);
	SetTransformations(scaleXYZ, XrotationDegrees, 0, 0, positionXYZ, offsetXYZ);
	SetShaderColor(0.92f, 0.92f, 0.90f, 1.0f);
	m_basicMeshes->DrawBoxMesh();


}
