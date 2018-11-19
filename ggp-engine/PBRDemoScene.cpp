#include "stdafx.h"

#include "PBRDemoScene.h"
#include <DirectXMath.h>
#include "RenderManager.h"
#include "ResourceManager.h"
#include "FlyingCamera.h"
#include "Material.h"
#include "PBRMaterial.h"
#include "Mesh.h"
#include "MeshRenderer.h"

using namespace DirectX;
using namespace std;

void PBRDemoScene::Init() {
	Scene::Init();



	Material* blueMatte = resourceManager->AddMaterial("blueMatte", XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), 0.0f);
	PBRMaterial* pbrMats[7];
	//Create a PBR material for testing
	pbrMats[0] = resourceManager->GetPBRMaterial("icePBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/ice_a.jpg", L"assets/textures/PBR/ice_n.jpg", L"assets/textures/PBR/ice_r.jpg", nullptr);
	pbrMats[1] = resourceManager->GetPBRMaterial("goldPBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/gold_a.jpg", L"assets/textures/PBR/gold_n.jpg", L"assets/textures/PBR/gold_r.jpg", L"assets/textures/PBR/gold_m.jpg");
	pbrMats[2] = resourceManager->GetPBRMaterial("bronzePBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/bronze_a.png", L"assets/textures/PBR/bronze_n.png", L"assets/textures/PBR/bronze_r.png", L"assets/textures/PBR/bronze_m.png");
	pbrMats[3] = resourceManager->GetPBRMaterial("woodFloorPBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/woodFloor_a.jpg", L"assets/textures/PBR/woodFloor_n.jpg", L"assets/textures/PBR/woodFloor_r.jpg", nullptr);
	pbrMats[4] = resourceManager->GetPBRMaterial("marblePBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/marble_a.jpg", L"assets/textures/PBR/marble_n.jpg", L"assets/textures/PBR/marble_r.jpg", nullptr);
	pbrMats[5] = resourceManager->GetPBRMaterial("tactilePavingPBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/tactilePaving_a.jpg", L"assets/textures/PBR/tactilePaving_n.jpg", L"assets/textures/PBR/tactilePaving_r.jpg", nullptr);
	pbrMats[6] = resourceManager->GetPBRMaterial("gravelPBR", L"VertexShader.cso", L"PBRPShader.cso", L"assets/textures/PBR/gravel_a.jpg", L"assets/textures/PBR/gravel_n.jpg", L"assets/textures/PBR/gravel_r.jpg", nullptr);
	//Load a sphere mesh
	Mesh* sphereMesh = resourceManager->GetMesh("assets/meshes/sphere.obj");
	//Mesh* sphereMesh = resourceManager->GenerateCube(1.0f, 2.0f);
	//Create a line of white spheres.
	for (UINT i = 0; i < 7; i++) {
		GameObject* newSphere = new GameObject("Sphere");
		AddChild(newSphere);
        MeshRenderer* aMeshRend = newSphere->AddComponent<MeshRenderer>( newSphere );
        if ( aMeshRend )
        {
            aMeshRend->SetMesh( sphereMesh );
            aMeshRend->SetMaterial( pbrMats[ i ] );
        }

		newSphere->transform.position.x = (float)i * 1.2f;
	}

	//Create a single, white directional light
	GameObject* dirLight = new GameObject("dirLight1");
	AddChild(dirLight);
	dirLight->AddDirLight(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), 0.8f, 0.00f);
	GameObject* dirLight2 = new GameObject("dirLight2");
	AddChild(dirLight2);
	dirLight2->AddDirLight(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -0.25f, 0.0f), 0.2f, 0.00f);
	GameObject* dirLight3 = new GameObject("dirLight3");
	AddChild(dirLight3);
	dirLight3->AddDirLight(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 1.0f), 0.2f, 0.00f);

	GameObject* pointLight1 = new GameObject("pointLight1");
	AddChild(pointLight1);
	pointLight1->AddPointLight(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	pointLight1->GetComponent<PointLight>(CompType::POINT_LIGHT)->SetRange(10.0f);
	pointLight1->transform.position.x += 2.0f;
	pointLight1->transform.position.y += 4.0f;

	GameObject* pointLight2 = new GameObject("pointLight2");
	AddChild(pointLight2);
	pointLight2->AddPointLight(XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
	pointLight2->GetComponent<PointLight>(CompType::POINT_LIGHT)->SetIntensity(1.0f);
	pointLight2->transform.position.x += 6.0f;
	pointLight2->transform.position.y += 3.0f;

	GameObject* spotLight1 = new GameObject("spotLight1");
	AddChild(spotLight1);
	spotLight1->AddSpotLight(XMFLOAT4(0.901f, 0.239f, 0.337f, 1.0f));
	spotLight1->GetComponent<SpotLight>(CompType::SPOT_LIGHT)->SetCone(15.0f);
	spotLight1->GetComponent<SpotLight>(CompType::SPOT_LIGHT)->SetRange(80.0f);
	spotLight1->GetComponent<SpotLight>(CompType::SPOT_LIGHT)->SetDirection(XMFLOAT3(0.0f, -1.0f, 0.0f));
	spotLight1->transform.position.y += 15.0f;

	//Create a camera
	activeCamera = new FlyingCamera("MainCamera");
	AddChild(activeCamera);
	activeCamera->Start();
	activeCamera->transform.position.z = -5.0f;
	activeCamera->CalculateViewMatrix();

	//Load terrain
	GameObject* terrain = new GameObject("testTerrain");
	Mesh* terrainMesh = resourceManager->GetTerrain("assets/terrain/testTerrain.raw", 513, 100.0f);
	AddChild(terrain);
	terrain->AddComponent<MeshRenderer>(terrain);
	terrain->GetComponentType<MeshRenderer>()->SetMesh(terrainMesh);
	terrain->GetComponentType<MeshRenderer>()->SetMaterial(pbrMats[6]);
	terrain->transform.position.y = -3.0f;
}

void PBRDemoScene::Start() {
	Scene::Start();
	renderManager->SetActiveCamera(activeCamera);
}

void PBRDemoScene::Update(float _deltaTime) {
	Scene::Update(_deltaTime);

	GameObject* pointLight2 = GetGameObject<GameObject>("pointLight2");
	pointLight2->transform.position.x = 3 + 3 * cos(totalTime);
	pointLight2->transform.position.z = 3 * sin(totalTime);
}
