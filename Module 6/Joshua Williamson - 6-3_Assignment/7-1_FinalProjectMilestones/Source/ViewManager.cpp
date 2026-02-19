///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;

	// the following variable is true when the cursor is DISABLED (i.e., 
	// it is hidden and bound to the window); otherwise it is NORMAL 
	// (i.e., unconstrained to the window). 
	bool bCursorDisabled = true;

	// a boolean to note that the TAB key has been pressed (cleared 
	// after a release)
	bool bTabPressed = false;

	// key press latches for O/P toggles
	bool bOPressed = false;
	bool bPPressed = false;

	// store perspective camera settings
	glm::vec3 gSavedPerspectivePos(0.0f);
	glm::vec3 gSavedPerspectiveFront(0.0f);
	glm::vec3 gSavedPerspectiveUp(0.0f);
	float gSavedPerspectiveZoom = 80.0f;

	// orthographic zoom factor
	float gOrthoZoom = 1.0f;

	// movement speed limits
	const float MIN_MOVE_SPEED = 2.0f;
	const float MAX_MOVE_SPEED = 60.0f;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();

	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 20;

	// save defaults for restoring perspective mode
	gSavedPerspectivePos = g_pCamera->Position;
	gSavedPerspectiveFront = g_pCamera->Front;
	gSavedPerspectiveUp = g_pCamera->Up;
	gSavedPerspectiveZoom = g_pCamera->Zoom;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// mouse callbacks
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting transparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos;

	gLastX = xMousePos;
	gLastY = yMousePos;

	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (NULL == g_pCamera)
	{
		return;
	}

	// Orthographic zoom
	if (bOrthographicProjection)
	{
		gOrthoZoom -= static_cast<float>(yOffset) * 0.1f;

		if (gOrthoZoom < 0.3f)
		{
			gOrthoZoom = 0.3f;
		}
		if (gOrthoZoom > 3.0f)
		{
			gOrthoZoom = 3.0f;
		}
	}
	// Perspective: adjust movement speed
	else
	{
		g_pCamera->MovementSpeed += static_cast<float>(yOffset) * 2.0f;

		if (g_pCamera->MovementSpeed < MIN_MOVE_SPEED)
		{
			g_pCamera->MovementSpeed = MIN_MOVE_SPEED;
		}
		if (g_pCamera->MovementSpeed > MAX_MOVE_SPEED)
		{
			g_pCamera->MovementSpeed = MAX_MOVE_SPEED;
		}
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// toggle cursor with TAB
	if (glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_PRESS)
	{
		bTabPressed = true;
	}
	if (bTabPressed && (glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_RELEASE))
	{
		bTabPressed = false;
		glfwSetInputMode(
			m_pWindow,
			GLFW_CURSOR,
			bCursorDisabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		bCursorDisabled = !bCursorDisabled;
	}

	// O = orthographic
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		bOPressed = true;
	}
	if (bOPressed && glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_RELEASE)
	{
		bOPressed = false;

		if (!bOrthographicProjection)
		{
			gSavedPerspectivePos = g_pCamera->Position;
			gSavedPerspectiveFront = g_pCamera->Front;
			gSavedPerspectiveUp = g_pCamera->Up;
			gSavedPerspectiveZoom = g_pCamera->Zoom;
		}

		bOrthographicProjection = true;
		gOrthoZoom = 0.6f;

		g_pCamera->Position = glm::vec3(0.0f, 5.0f, 14.0f);
		g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	// P = perspective
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		bPPressed = true;
	}
	if (bPPressed && glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_RELEASE)
	{
		bPPressed = false;

		bOrthographicProjection = false;

		g_pCamera->Position = gSavedPerspectivePos;
		g_pCamera->Front = gSavedPerspectiveFront;
		g_pCamera->Up = gSavedPerspectiveUp;
		g_pCamera->Zoom = gSavedPerspectiveZoom;
	}

	// WASD movement
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);

	// Q / E vertical movement
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
		g_pCamera->Position += g_pCamera->Up * (g_pCamera->MovementSpeed * gDeltaTime);
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
		g_pCamera->Position -= g_pCamera->Up * (g_pCamera->MovementSpeed * gDeltaTime);
}

/***********************************************************
 *  PrepareSceneView()
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	ProcessKeyboardEvents();

	view = g_pCamera->GetViewMatrix();

	// default perspective projection
	projection = glm::perspective(
		glm::radians(g_pCamera->Zoom),
		(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
		0.1f,
		100.0f);

	// orthographic projection (uses zoom factor)
	if (bOrthographicProjection)
	{
		float orthoWidth = 10.0f * gOrthoZoom;
		float orthoHeight = 8.0f * gOrthoZoom;

		projection = glm::ortho(
			-orthoWidth, orthoWidth,
			-orthoHeight, orthoHeight,
			0.1f, 100.0f);
	}

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
