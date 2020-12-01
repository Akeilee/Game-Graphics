#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include <algorithm>
#include <cmath>

#define SHADOWSIZE 20480
const int LIGHT_NUM = 80; //number of lights in scene
float changelightpos = 0;
float changeShadowPos = 0;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	sceneShader = new Shader("shadowSceneVert2.glsl", "shadowSceneFrag2.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");
	basicShader = new Shader("textureVertex.glsl", "textureFragment2.glsl");

	quad = Mesh::GenerateQuad();
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));

	buildingTex = SOIL_load_OGL_texture(TEXTUREDIR "bb.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	buildingBump = SOIL_load_OGL_texture(TEXTUREDIR "bb1.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	heightMap = new HeightMap(TEXTUREDIR "smooth3.JPG");
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
	textureShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess() ||
		!buildingShader->LoadSuccess() || !orbShader->LoadSuccess() || !shadowShader->LoadSuccess()) {
		return;
	}

	BuildingShadowFBO();
	SphereShadowFBO();
	WholeSceneFBO();

	root = new SceneNode();
	for (int i = 0; i < 20; ++i) {
		SceneNode* s = new SceneNode();
		float bounds = 0;
		s->SetColour(Vector4(1, 1, 1, 1));
		s->SetTransform(Matrix4::Translation(Vector3(rand() % 1501 + 250, 290, 220 + 170 * i)));
		s->SetModelScale(Vector3(rand() % 21 + 60.0f, rand() % 301 + 350.0f, rand() % 21 + 60.0f));

		s->GetModelScale().x >= s->GetModelScale().y ? bounds = s->GetModelScale().x : bounds = s->GetModelScale().y;
		s->SetBoundingRadius(bounds);

		s->SetMesh(cube);
		//s->SetTexture(buildingTex);
		root->AddChild(s);
	}

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(0.0f, 220.0f, heightmapSize * Vector3(-0.5f, 5.0f, -0.5f));
	light = new Light(heightmapSize * Vector3(-0.5f, 5.5f, -0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 100.0f);
	light2 = new Light(heightmapSize * Vector3(0.5f, 5.5f, 0.5f), Vector4(1, 0.55, 0, 1), heightmapSize.x * 500.0f);
	//orbLight = new Light(heightmapSize * Vector3(-0.5f, 5.5f, -0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 100.0f);


	//camera = new Camera(-30.0f, 315.0f, heightmapSize * Vector3(-8.0f, 10.0f, 8.0f));
	orbLight = new Light(Vector3(-200.0f, 1500.0f, -20.0f), Vector4(1, 1, 1, 1), 1000 * 250.0f);

	camera->SetPosition(orbLight->GetPosition());
	//light = new Light(heightmapSize * Vector3(0.5, 2.10f, 0.5), Vector4(1, 1, 1, 1), heightmapSize.x * 10.0f);




	pointLights = new Light[LIGHT_NUM];

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		l.SetPosition(Vector3(rand() % (int)heightmapSize.x, rand() % 200 + 100, rand() % (int)heightmapSize.z));  //rand x and z
		l.SetColour(Vector4(1.5f + (float)(rand() / (float)RAND_MAX), 1.5f + (float)(rand() / (float)RAND_MAX), 1.5f + (float)(rand() / (float)RAND_MAX), 1));
		l.SetRadius((rand() % 200 + 800)); //min size is 50

	}
	sceneShader2 = new Shader("BumpVertex.glsl", "bufferFragment.glsl");
	pointlightShader = new Shader("pointLightVert.glsl", "pointLightFrag.glsl");
	combineShader = new Shader("combineVert.glsl", "combineFrag.glsl");

	PointLightFBO();


	///////////////

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	//glEnable(GL_CULL_FACE);  ///destroys water textuer
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//glEnable(GL_POLYGON_OFFSET_FILL);

	waterRotate = 0.0f;
	waterCycle = 0.0f;

	sceneTransforms.resize(4);
	Vector3 t = Vector3(2500, 300, 500);
	sceneTransforms[0] = Matrix4::Translation(t) * Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(1000, 1000, 1000));
	//sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	sceneTime = 0.0f;
	init = true;
}

void Renderer::PointLightFBO() {

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = {
	 GL_COLOR_ATTACHMENT0 ,
	 GL_COLOR_ATTACHMENT1
	};

	// Generate our scene depth texture ...
	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	// And now attach them to our FBOs
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);  //renders both colour attachments
	glViewport(0, 0, width, height);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}


	//no depth attachment
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



Renderer ::~Renderer(void) {
	delete camera;
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
	glDeleteFramebuffers(1, &sphereFBO);


	delete sceneShader;
	delete combineShader;
	delete pointlightShader;

	delete sphere;
	delete[] pointLights;

	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
}


void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}


void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	//viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 2.0f; //2 degrees a second
	waterCycle += dt * 0.25f; //10 units a second
	sceneTime += dt;

	changelightpos = 1000.0f * sin(sceneTime * 2) + 500;
	changeShadowPos = 40.0f * sin(sceneTime * 2);

	for (int i = 1; i < 4; ++i) {

		Vector3 t = Vector3(2500, 200.0f * sin(sceneTime * 2) + 500, 500);
		//Vector3 t = Vector3(-12 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(100, 100, 100));
		//sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0));
	}


	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//OrbShadow();
	//MakeOrbs();

	DrawBuilding2(); //fbo not work
	DrawFloorShadow(); //shadofbo building shadow
	FillBuffers(); //bufferfbo water, height, building

	////DrawBuilding();  //drawing again might not need
	////DrawHeightmap();


	//DrawFullScene();

	DrawSkybox();

	DrawPointLights(); //lightfbo
	CombineBuffers(); //wholefbo

	Gui();

	//DrawBuilding();  //drawing again might not need
	//DrawHeightmap();


}


void Renderer::FillBuffers() {  // 1
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);


	DrawBuilding();
	DrawHeightmap();
	DrawWater();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


void Renderer::DrawPointLights() { // 2 
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);  //additive blending
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	//for spec light
	Vector3 lightPos = light->GetPosition();
	lightPos.z = -changelightpos;
	lightPos.y = changelightpos;
	lightPos.x = -changelightpos;

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&lightPos);

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	//drawing lights
	UpdateShaderMatrices();
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineBuffers() { // 3

	glClearColor(0, 0, 0, 0);
	BindShader(combineShader);
	modelMatrix.ToIdentity();

	viewMatrix.ToZero();
	//viewMatrix.ToIdentity();
	//viewMatrix = viewMatrix* Matrix4::Translation(Vector3(0,10,0));

	//projMatrix.ToIdentity();
	projMatrix.ToZero();
	//projMatrix = projMatrix * Matrix4::Translation(Vector3(0, 10, 0));

	//glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);

	UpdateShaderMatrices();


	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex); ////////


	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	quad->Draw();

}


void Renderer::DrawFullScene() {
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, width, height);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(textureShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(textureShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex); ////////
	glUniform1i(glGetUniformLocation(textureShader->GetProgram(), "BuildingShadowTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowTex); ////////
	quad->Draw();


}


void Renderer::WholeSceneFBO() {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &wholeSceneFBO);
	GenerateScreenTexture(wholeSceneTex);

	GLenum buffers[2] = {
	 GL_COLOR_ATTACHMENT0 ,
	 GL_COLOR_ATTACHMENT1
	};

	glBindFramebuffer(GL_FRAMEBUFFER, wholeSceneFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, wholeSceneTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, wholeSceneTex, 0);
	glDrawBuffers(2, buffers);  //renders both colour attachments
	glViewport(0, 0, width, height);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Renderer::BuildingShadowFBO() {
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void Renderer::forReflect() {
	glBindFramebuffer(GL_FRAMEBUFFER, wholeSceneFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	DrawSkybox();
	DrawFullScene();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void Renderer::MakeOrbs() {
	BindShader(sceneShader);
	SetShaderLight(*orbLight);
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

	viewMatrix = Matrix4::BuildViewMatrix(orbLight->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix; // used later

	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Renderer::Gui() {
	//glClear(GL_COLOR_BUFFER_BIT);
	Vector3 hSize = heightMap->GetHeightmapSize();
	glViewport(50, height - 250, hSize.x / 10, hSize.y / 1.5);
	BindShader(basicShader);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	UpdateShaderMatrices();
	quad->Draw();
}

////tut12
void Renderer::DrawBuilding() {
	//glCullFace(GL_BACK);
	BuildNodeLists(root);
	SortNodeLists();
	BindShader(buildingShader);
	SetShaderLight(*light2);

	modelMatrix.ToIdentity();
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buildingTex);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, buildingBump);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition() + 10);
	UpdateShaderMatrices();

	DrawNodes();
	ClearNodeLists();
}



void Renderer::DrawBuilding2() {
	glBindFramebuffer(GL_FRAMEBUFFER, wholeSceneFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);



	//glCullFace(GL_BACK);
	BuildNodeLists(root);
	SortNodeLists();
	BindShader(buildingShader);
	SetShaderLight(*light2);

	modelMatrix.ToIdentity();
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buildingTex);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, buildingBump);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition() + 10);
	UpdateShaderMatrices();

	DrawNodes();
	ClearNodeLists();




	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}





void Renderer::DrawFloorShadow() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BuildNodeLists(root);
	SortNodeLists();
	BindShader(buildingShader);

	modelMatrix.ToIdentity();
	Vector3 lightPos = light->GetPosition();
	lightPos.y = changeShadowPos;

	viewMatrix = Matrix4::BuildViewMatrix(lightPos, Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	shadowMatrix = projMatrix * viewMatrix;
	UpdateShaderMatrices();

	DrawNodes();
	ClearNodeLists();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


/////tut 7
void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);  //r = reverse iterator

	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {

		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		modelMatrix = model;
		glUniformMatrix4fv(glGetUniformLocation(buildingShader->GetProgram(), "modelMatrix"), 1, false, model.values);


		//buildingTex = n->GetTexture();
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, buildingTex);

		//glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "useTexture"), buildingTex);

		n->Draw(*this);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}



//////tut 13
void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);
	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap() {
	BindShader(buildingShader);  //changed from build shader
	SetShaderLight(*light);

	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}

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

void Renderer::DrawWater() {

	//glClearColor(0, 0, 0, 0);
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);   ///reflt whole fbo

	Vector3 hSize = heightMap->GetHeightmapSize();

	//scale water quad
	modelMatrix =
		Matrix4::Translation(Vector3(hSize.x * 0.5f, hSize.y * 0.55, hSize.z * 0.5f)) *
		Matrix4::Scale(hSize * 0.5f) *
		Matrix4::Rotation(90, Vector3(1, 0, 0));

	//making water move
	textureMatrix =
		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10)) *
		Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	SetShaderLight(*light);
	quad->Draw();
}

