#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include <algorithm>
#include <cmath>

#define SHADOWSIZE 20480

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);
	sceneShader = new Shader("shadowSceneVert2.glsl", "shadowSceneFrag2.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");

	quad = Mesh::GenerateQuad();
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));

	buildingTex = SOIL_load_OGL_texture(TEXTUREDIR "bb.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	buildingBump = SOIL_load_OGL_texture(TEXTUREDIR "bb1.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	//heightMap = new HeightMap(TEXTUREDIR "smooth2.JPG");
	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR "tarmac.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR "tarmacBump.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR "px.png", TEXTUREDIR "nx.png",
		TEXTUREDIR "py.png", TEXTUREDIR "ny.png",
		TEXTUREDIR "pz.png", TEXTUREDIR "nz.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex || !buildingTex || !buildingBump) {
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(buildingTex, true);
	SetTextureRepeating(buildingBump, true);

	reflectShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");  //water
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	lightShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	buildingShader = new Shader("shadowSceneVert.glsl", "shadowSceneFrag.glsl");  //building and heightmap
	orbShader = new Shader("shadowSceneVert2.glsl", "shadowSceneFrag2.glsl");  //building and heightmap
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess() ||
		!buildingShader->LoadSuccess() || !orbShader->LoadSuccess() || !shadowShader->LoadSuccess()) {
		return;
	}

	//BuildingShadowFBO();
	SphereShadowFBO();


	root = new SceneNode();
	for (int i = 0; i < 10; ++i) {
		SceneNode* s = new SceneNode();
		float bounds = 0;
		s->SetColour(Vector4(1, 1, 1, 1));
		s->SetTransform(Matrix4::Translation(Vector3(rand() % 2001 + 250, 110, 500 + 300 * i)));
		s->SetModelScale(Vector3(rand() % 21 + 100.0f, rand() % 301 + 350.0f, rand() % 21 + 120.0f));

		s->GetModelScale().x >= s->GetModelScale().y ? bounds = s->GetModelScale().x : bounds = s->GetModelScale().y;
		s->SetBoundingRadius(bounds);

		s->SetMesh(cube);
		s->SetTexture(buildingTex);
		root->AddChild(s);
	}

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	//camera = new Camera(0.0f, 220.0f, heightmapSize * Vector3(-0.5f, 5.0f, -0.5f));
	//light = new Light(heightmapSize * Vector3(-0.5f, 5.5f, -0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 100.0f);
	//light2 = new Light(heightmapSize * Vector3(0.5f, 5.5f, 0.5f), Vector4(1, 0.55, 0, 1), heightmapSize.x * 500.0f);
	//orbLight = new Light(Vector3(1000.0f, 1000.0f, 1000.0f), Vector4(1, 1, 1, 1), 250000.0f);


	//camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 10.0f, 8.0f));
	//orbLight = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);


	//light = new Light(heightmapSize * Vector3(0.5, 2.10f, 0.5), Vector4(1, 1, 1, 1), heightmapSize.x * 10.0f);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


	glEnable(GL_POLYGON_OFFSET_FILL);

	waterRotate = 0.0f;
	waterCycle = 0.0f;

	sceneTransforms.resize(4);
	Vector3 t = Vector3(2500, 300, 500);
	//sceneTransforms[0] = Matrix4::Translation(t) * Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(1000, 1000, 1000));
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	sceneTime = 0.0f;
	init = true;
}

Renderer ::~Renderer(void) {
	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	//delete light;

	delete root;
	delete cube;
	delete buildingShader;
	glDeleteTextures(1, &buildingTex);
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteFramebuffers(1, &sphereFBO);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	//viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 2.0f; //2 degrees a second
	waterCycle += dt * 0.25f; //10 units a second
	sceneTime += dt;


	for (int i = 1; i < 4; ++i) {

		//Vector3 t = Vector3(2500, 200.0f * sin(sceneTime * 2) + 500, 500);
		Vector3 t = Vector3(-12 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		//sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(100, 100, 100));
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0));
	}


	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//DrawSkybox();

	//DrawFullScene();
	OrbShadow();
	MakeOrbs();

	//DrawWater();

}

//void Renderer::DrawFullScene() {
//	DrawFloorShadow();
//	DrawBuilding();
//	DrawHeightmap();
//}
//
//
//void Renderer::BuildingShadowFBO() {
//	glGenTextures(1, &shadowTex);
//	glBindTexture(GL_TEXTURE_2D, shadowTex);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//
//	glBindTexture(GL_TEXTURE_2D, 0);
//
//	glGenFramebuffers(1, &shadowFBO);
//	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
//	glDrawBuffer(GL_NONE);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}
//
void Renderer::SphereShadowFBO() {
	glGenTextures(1, &sphereTex);
	glBindTexture(GL_TEXTURE_2D, sphereTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &sphereFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, sphereFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sphereTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void Renderer::MakeOrbs() {
	BindShader(sceneShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buildingTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, buildingBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);


	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
}

void Renderer::OrbShadow() {
	glBindFramebuffer(GL_FRAMEBUFFER, sphereFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


////tut12
//void Renderer::DrawBuilding() {
//	glEnable(GL_CULL_FACE);
//	glCullFace(GL_BACK);
//	BuildNodeLists(root);
//	SortNodeLists();
//	BindShader(buildingShader);
//	//SetShaderLight(*light);
//	SetShaderLight(*light2);
//
//	viewMatrix = camera->BuildViewMatrix();
//	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
//
//	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, buildingTex);
//
//	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, buildingBump);
//
//	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "shadowTex"), 2);
//	glActiveTexture(GL_TEXTURE2);
//	glBindTexture(GL_TEXTURE_2D, shadowTex);
//
//	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition() + 10);
//	UpdateShaderMatrices();
//
//	DrawNodes();
//	ClearNodeLists();
//
//	glDisable(GL_CULL_FACE);
//}
//
//
//void Renderer::DrawFloorShadow() {
//
//	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
//	
//	glClear(GL_DEPTH_BUFFER_BIT);
//	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//
//	BuildNodeLists(root);
//	SortNodeLists();
//	BindShader(buildingShader);
//
//
//	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0,0,0));
//	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
//	shadowMatrix = projMatrix * viewMatrix; 
//	UpdateShaderMatrices();
//
//	DrawNodes();
//	ClearNodeLists();
//	//HeightMapShadow();  //poss dont need
//
//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//	glViewport(0, 0, width, height);
//	
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	
//}
//
//
///////tut 7
//void Renderer::BuildNodeLists(SceneNode* from) {
//	if (frameFrustum.InsideFrustum(*from)) {
//		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
//		from->SetCameraDistance(Vector3::Dot(dir, dir));
//
//		if (from->GetColour().w < 1.0f) {
//			transparentNodeList.push_back(from);
//		}
//		else {
//			nodeList.push_back(from);
//		}
//	}
//
//	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
//		BuildNodeLists((*i));
//	}
//}
//
//void Renderer::SortNodeLists() {
//	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);  //r = reverse iterator
//
//	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
//}
//
//void Renderer::DrawNodes() {
//	for (const auto& i : nodeList) {
//		DrawNode(i);
//	}
//	for (const auto& i : transparentNodeList) {
//		DrawNode(i);
//	}
//}
//
//void Renderer::DrawNode(SceneNode* n) {
//	if (n->GetMesh()) {
//
//		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
//		glUniformMatrix4fv(glGetUniformLocation(buildingShader->GetProgram(), "modelMatrix"), 1, false, model.values);
//
//		//buildingTex = n->GetTexture();
//		//glActiveTexture(GL_TEXTURE0);
//		//glBindTexture(GL_TEXTURE_2D, buildingTex);
//
//		//glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "useTexture"), buildingTex);
//
//		n->Draw(*this);
//	}
//}
//
//void Renderer::ClearNodeLists() {
//	transparentNodeList.clear();
//	nodeList.clear();
//}
//
//
//
////////tut 13
//void Renderer::DrawSkybox() {
//	glDepthMask(GL_FALSE);
//	BindShader(skyboxShader);
//	UpdateShaderMatrices();
//
//	quad->Draw();
//	glDepthMask(GL_TRUE);
//}
//
//void Renderer::DrawHeightmap() {
//	BindShader(buildingShader);
//	SetShaderLight(*light);
//	//SetShaderLight(*light2);
//	viewMatrix = camera->BuildViewMatrix();
//	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
//
//	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
//
//	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, earthTex);
//
//	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, earthBump);
//
//	modelMatrix.ToIdentity(); 
//	textureMatrix.ToIdentity(); 
//
//	UpdateShaderMatrices();
//
//	heightMap->Draw();
//}
//
//void Renderer::HeightMapShadow() {
//
//	BindShader(buildingShader);
//
//	viewMatrix = Matrix4::Orthographic(-1.0f, 1500.0f, 1000 , -1000 , 1000 , -1000); ///////////////
//
//	modelMatrix.ToIdentity();
//	textureMatrix.ToIdentity();
//
//	UpdateShaderMatrices();
//
//	heightMap->Draw();
//}
//
//void Renderer::DrawWater() {
//	BindShader(reflectShader);
//
//	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
//
//	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
//	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);
//
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, waterTex);
//
//	glActiveTexture(GL_TEXTURE2);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
//
//	Vector3 hSize = heightMap->GetHeightmapSize();
//
//	//scale water quad
//	modelMatrix =
//		Matrix4::Translation(hSize * 0.5f) *
//		Matrix4::Scale(hSize * 0.5f) *
//		Matrix4::Rotation(90, Vector3(1, 0, 0));
//
//	//making water move
//	textureMatrix =
//		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
//		Matrix4::Scale(Vector3(10, 10, 10)) *
//		Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
//
//	UpdateShaderMatrices();
//	SetShaderLight(*light);
//	quad->Draw();
//}

