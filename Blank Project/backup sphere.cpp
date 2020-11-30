#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include <algorithm>
#include <cmath>

#define SHADOWSIZE 20480

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	quad = Mesh::GenerateQuad();
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");


	camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 10.0f, 8.0f));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

	reflectShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");  //water
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	lightShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	buildingShader = new Shader("shadowSceneVert.glsl", "shadowSceneFrag.glsl");  //building and heightmap

	sceneShader = new Shader("shadowSceneVert2.glsl", "shadowSceneFrag2.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");

	if (!sceneShader->LoadSuccess() || !shadowShader->LoadSuccess()) {
		return;
	}


	buildingTex = SOIL_load_OGL_texture(TEXTUREDIR "bb.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	buildingBump = SOIL_load_OGL_texture(TEXTUREDIR "bb2.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	heightMap = new HeightMap(TEXTUREDIR "smooth2.JPG");
	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR "tarmac.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR "tarmacBump.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR "px.png", TEXTUREDIR "nx.png",
		TEXTUREDIR "py.png", TEXTUREDIR "ny.png",
		TEXTUREDIR "pz.png", TEXTUREDIR "nz.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex || !buildingTex || !buildingBump) {
		//return;
	}


	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(buildingTex, true);
	SetTextureRepeating(buildingBump, true);





	//BuildingShadowFBO();
	SphereShadowFBO();

	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("OffsetCubeY.msh")); ////////
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));

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


	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


	glEnable(GL_POLYGON_OFFSET_FILL);

	waterRotate = 0.0f;
	waterCycle = 0.0f;


	sceneTransforms.resize(5);
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));  //floor quad won't move, similar to water in cube map tut
	sceneTime = 0.0f;
	init = true;
}


Renderer ::~Renderer(void) {

	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete light;

	delete root;
	delete cube;
	delete buildingShader;
	glDeleteTextures(1, &buildingTex);
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);



	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);

	for (auto& i : sceneMeshes) {
		delete i;
	}

	delete camera;
	delete sceneShader;
	delete shadowShader;
}

void Renderer::UpdateScene(float dt) {  //no view matrix as shadow map is rendered first so will use view matrix in drawshadowscene
	camera->UpdateCamera(dt);
	waterRotate += dt * 2.0f; //2 degrees a second
	waterCycle += dt * 0.25f; //10 units a second

	sceneTime += dt;

	for (int i = 1; i < 5; ++i) { // skip the floor !
		Vector3 t = Vector3(-12 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0));
	}

	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);

}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawShadowScene();

	DrawMainScene();
}

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
	glDrawBuffer(GL_NONE);  //none as no colour needs to be attached
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, sphereFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix; // used later

	for (int i = 0; i < 5; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}


	//DrawHeightmap2();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::DrawMainScene() {
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
	glBindTexture(GL_TEXTURE_2D, sphereTex);


	for (int i = 0; i < 5; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	//DrawHeightmap();  ////putting draw calls in main scene
}

