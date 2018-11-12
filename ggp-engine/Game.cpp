#include "stdafx.h"

#include "Game.h"
#include "Vertex.h"
#include "PBRDemoScene.h"
#include "DebugScene.h"
// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	// Initialize fields
	vertexBuffer = 0;
	indexBuffer = 0;

	//Is the cursor locked (Default yes)
	//TODO: Make this toggle-able and have more functionality in the System singleton
	mouseLocked = true;
	ShowCursor(false);

	//Get Singleton Instances
	resourceManager = ResourceManager::GetInstance();
	renderManager = RenderManager::GetInstance();
	inputManager = InputManager::GetInstance();
	lightManager = LightManager::GetInstance();
    physicsManager = Physics::PhysicsManager::GetInstance();

	#if defined(DEBUG) || defined(_DEBUG)
		// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.");
	#endif

}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game() {
	// Release any (and all!) DirectX objects
	// we've made in the Game class
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }

	//Release all singletons
	resourceManager->ReleaseInstance();
	renderManager->ReleaseInstance();
	inputManager->ReleaseInstance();
	lightManager->ReleaseInstance();
    physicsManager->ReleaseInstance();

	// Clean up ImGui
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete activeScene;
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init() {
	//Give the device and context to the resource manager
	ResourceManager::SetDevicePointer(dxDevice);
	ResourceManager::SetContextPointer(dxContext);

	//Create and init the active scene
	activeScene = new PBRDemoScene("PBR_Demo");
    //activeScene = new DebugScene( "Debug Scene" );
	activeScene->Init();

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	dxContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Call start on the singletons that need it
	renderManager->Start();
	//Call start on the active scene
	activeScene->Start();

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(this->dxDevice, this->dxContext);
	ImGui::StyleColorsDark();
}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize() {
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	//Pass resize event to scene
	activeScene->OnResize((float)width, (float)height);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime) {
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();

	//Handle mouse movement
	HandleMouseMove();

	inputManager->Update();

	activeScene->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime) {
	// Background color (Black in this case) for clearing
	const float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	dxContext->ClearRenderTargetView(backBufferRTV, color);
	dxContext->ClearDepthStencilView(
		depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	//Call render on the renderManager
	renderManager->Render(dxContext);

	// ImGui Draw
	// Start the ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Create ImGui window
	ImGui::Begin("Test Boi");
	ImGui::End();
	// Assemble together draw data
	ImGui::Render();
	// Render draw data
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);
}

void Game::HandleMouseMove() {
	if (mouseLocked) {
		//Get the new mouse position
		GetCursorPos(&mousePos);
		//Store window rect
		RECT windowRect;
		GetWindowRect(hWnd, &windowRect);
		//Previous position is always the window's center when the cursor is locked
		prevMousePos.x = windowRect.left + width / 2;
		prevMousePos.y = windowRect.top + height / 2;
		//Get mouse displacement
		int displaceX = mousePos.x - prevMousePos.x;
		int displaceY = mousePos.y - prevMousePos.y;
		//Pass data to input manager if mouse moved
		if (displaceX != 0 || displaceY != 0) {
			inputManager->_OnMouseMove(prevMousePos.x, prevMousePos.y, mousePos.x, mousePos.y);
		}
		//Reset cursor position
		SetCursorPos(prevMousePos.x, prevMousePos.y);
	} else {
		//Store previous mouse position
		prevMousePos.x = mousePos.x;
		prevMousePos.y = mousePos.y;
		//Get new mouse position
		GetCursorPos(&mousePos);
		//Pass to input manager if there was movement
		if (prevMousePos.x != mousePos.x || prevMousePos.y != mousePos.y) {
			inputManager->_OnMouseMove(prevMousePos.x, prevMousePos.y, mousePos.x, mousePos.y);
		}
	}
}


#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y) {
	//Pass event to the input manager
	inputManager->_OnMouseDown(buttonState, x, y);

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y) {
	//Pass event to the input manager
	inputManager->_OnMouseUp(buttonState, x, y);

	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y) {
	//Currently does nothing
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y) {
	//Pass event to the input manager
	inputManager->_OnMouseWheel(wheelDelta);
}
#pragma endregion