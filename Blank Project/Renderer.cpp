#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include <algorithm>
#include <cmath>


#define SHADOWSIZE 20480
const int LIGHT_NUM = 100; //number of lights
const int BUILDING_NUM = 20; //number of buildings
const int BLUR_PASSES = 20; //blur strength
float changeSpotLightPos = 0;
float changeShadowPos = 0;
float changeRobotPos = 0;
float changeSpherePos = 0;
int keepRobotCount = 0; //to rotate robot walking position


Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	usingBlur = false; 
	partylight = false;
	gammaCorrect = false;
	splitScreen = false;
	afterSplit = false;

	quad = Mesh::GenerateQuad();
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	cylinder = Mesh::LoadFromMeshFile("Cylinder.msh");

	heightMap = new HeightMap(TEXTUREDIR "smooth5.JPG");
	buildingTex = SOIL_load_OGL_texture(TEXTUREDIR "bb.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	buildingBump = SOIL_load_OGL_texture(TEXTUREDIR "bb1.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	floorTex = SOIL_load_OGL_texture(TEXTUREDIR "tarmac.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	floorBump = SOIL_load_OGL_texture(TEXTUREDIR "tarmacBump.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	pinkTex = SOIL_load_OGL_texture(TEXTUREDIR "pink.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	vWaveTex = SOIL_load_OGL_texture(TEXTUREDIR "vwave.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	neonTex = SOIL_load_OGL_texture(TEXTUREDIR "neon.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	stencilTex = SOIL_load_OGL_texture(TEXTUREDIR "vwaveStencil.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	blankTex = SOIL_load_OGL_texture(TEXTUREDIR "blank.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR "px.png", TEXTUREDIR "nx.png",
		TEXTUREDIR "py.png", TEXTUREDIR "ny.png",
		TEXTUREDIR "pz.png", TEXTUREDIR "nz.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);


	if (!buildingTex || !buildingBump|| !waterTex|| !floorTex || !floorBump || !pinkTex || !vWaveTex|| !neonTex|| !stencilTex|| !blankTex|| !cubeMap) {
		return;
	}

	SetTextureRepeating(buildingTex, true);
	SetTextureRepeating(buildingBump, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(floorTex, true);
	SetTextureRepeating(floorBump, true);
	SetTextureRepeating(pinkTex, true);
	SetTextureRepeating(vWaveTex, true);
	SetTextureRepeating(neonTex, true);
	SetTextureRepeating(stencilTex, true);
	SetTextureRepeating(blankTex, true);
	

	reflectShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");  //water
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	buildingShader = new Shader("shadowSceneVert.glsl", "shadowSceneFrag.glsl");  //building and heightmap
	geomShader = new Shader("shadowSceneVert2.glsl", "shadowSceneFrag2.glsl"); //for random geom
	pointlightShader = new Shader("pointLightVert.glsl", "pointLightFrag.glsl");
	combineShader = new Shader("combineVert.glsl", "combineFrag.glsl"); //for scene with lights
	robotShader = new Shader("SkinningVertex.glsl", "TexturedFragmentMain.glsl");
	robotShadowShader = new Shader("TexturedVertex2.glsl", "TexturedFragmentrobot.glsl");
	blurShader = new Shader("TexturedVertex2.glsl", "TexturedFragmentMain.glsl");
	gammaShader = new Shader("TexturedVertex2.glsl", "TexturedFragmentGamma.glsl");
	colourChangeShader = new Shader("TexturedVertex2.glsl", "ProcessFrag.glsl"); //changes colour of scene
	basicShader = new Shader("TexturedVertex2.glsl", "TexturedFragment2.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess()  || !buildingShader->LoadSuccess() || !geomShader->LoadSuccess() 
		|| !pointlightShader->LoadSuccess() || !combineShader->LoadSuccess() || !robotShader->LoadSuccess() || !robotShadowShader->LoadSuccess()
		|| !blurShader->LoadSuccess() || !gammaShader->LoadSuccess() || !colourChangeShader->LoadSuccess() || !basicShader->LoadSuccess() ) {
		return;
	}


	BuildingShadowFBO();
	WholeSceneFBO();
	PointLightsFBO();
	BlurFBOs();

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	//Manually added geometry
	sceneMeshes.emplace_back(cylinder);
	sceneMeshes.emplace_back(cylinder);
	sceneMeshes.emplace_back(quad);

	//Scene Graph
	root = new SceneNode();
	for (int i = 0; i < BUILDING_NUM; ++i) {
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
	sphMesh->SetMesh(sphere);
	sphMesh->SetColour(Vector4(1, 1, 1, 0.5));
	sphMesh->SetTransform(Matrix4::Translation(heightmapSize * Vector3(0.7, 1.2, 0.9)));
	sphMesh->SetModelScale(Vector3(200, 200, 200));
	root->AddChild(sphMesh);
	
	camera = new Camera(0.0f, 55.0f, Vector3(3800, 450, 3500));
	light = new Light(heightmapSize * Vector3(-0.5f, 5.5f, -0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 100.0f);
	ambientLight = new Light(heightmapSize * Vector3(0.95f, 3.5f, 0.95f), Vector4(1, 0.55, 0, 1), heightmapSize.x * 500.0f);

	//spotlights
	pointLights = new Light[LIGHT_NUM];
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		float randx = rand() % ((int)heightmapSize.x+50);
		float randy = rand() % 200 + 100;
		float randz = rand() % ((int)heightmapSize.z+50);
		l.SetPosition(Vector3(randx, randy, randz));  //rand x and z

		float x = 1 + (float)(rand() / (float)RAND_MAX);
		float y = 1 + (float)(rand() / (float)RAND_MAX);
		float z = 1 + (float)(rand() / (float)RAND_MAX);
		l.SetColour(Vector4(x, y, z, 1));
		l.SetRadius((rand() % 300 + 600)); //min size is 50

	}

	//robot animation
	animMesh = Mesh::LoadFromMeshFile("Robot1.msh");
	anim = new MeshAnimation("Robot1.anm");
	material = new MeshMaterial("Robot1.mat");
	for (int i = 0; i < animMesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	waterRotate = 0.0f;
	waterCycle = 0.0f;
	currentFrame = 0;
	frameTime = 0.0f;
	sceneTime = 0.0f;

	sceneTransforms.resize(3);

	init = true;
}

Renderer ::~Renderer(void) {

	delete reflectShader;
	delete skyboxShader;
	delete buildingShader;
	delete geomShader;
	delete pointlightShader;
	delete combineShader;
	delete robotShader;
	delete robotShadowShader;
	delete blurShader;
	delete gammaShader;
	delete colourChangeShader;
	delete basicShader;

	delete camera;
	delete heightMap;

	delete sphere;
	delete quad;
	delete cube;

	delete light;
	delete ambientLight;
	delete[] pointLights;
	delete root;

	delete animMesh;
	delete anim;
	delete material;


	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);

	glDeleteTextures(1, &wholeSceneTex);
	glDeleteFramebuffers(1, &wholeSceneFBO);

	glDeleteFramebuffers(1, &buffBlurFBO);
	glDeleteFramebuffers(1, &processFBO);
	glDeleteTextures(2, buffColTex);
	glDeleteTextures(1, &buffDepthTex);

	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &pointLightFBO);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

}

void Renderer::UpdateScene(float dt) {
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
	sceneTime += dt;

	camera->UpdateCamera(dt);

	changeSpotLightPos = 1000.0f * sin(sceneTime * 2) + 500;
	changeShadowPos = 40.0f * sin(sceneTime * 2);

	float walkSpeed = sceneTime / 5;
	changeRobotPos = (float)0.5 * (1 + sin(walkSpeed));
	changeSpherePos = (float)500 * (1 + sin(sceneTime)) + 420;

	if (partylight == true) {
		float randx = 1.5 * (sin(sceneTime * 1.55) + 2.5);
		float randy = 1.5 * (sin(sceneTime * 1.65) + 2);
		float randz = 1.5 * (sin(sceneTime * 1.75) + 1.5);

		ambientLight->SetColour(Vector4(randx, randy, randz, 1));
	}
	else if (partylight == false) {
		ambientLight->SetColour(Vector4(1, 0.55, 0, 1));
	}

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}

	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawFloorShadow();
	FillFirstFBO();
	DrawHeightmap();
	DrawSkybox();
	FillLightFBO();

	FinalSceneIntoFBO();

	PrintColourChange();

	if (splitScreen == true) {
		PrintSplitScreen();
	}
	if (usingBlur == true) {
		PrintBlurScreen();
	}
	if (gammaCorrect == true) {
		PrintGammaCorrect();
	}

	CombineFirstAndLightBuffers();
	DrawWater();
}



void Renderer::DrawSkybox() {
	glDisable(GL_BLEND);
	glDepthMask(GL_FALSE);
	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
	glEnable(GL_BLEND);
}

void Renderer::DrawHeightmap() {

	BindShader(buildingShader);
	SetShaderLight(*light);

	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorTex);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, floorBump);

	glUniform1f(glGetUniformLocation(buildingShader->GetProgram(), "alpha"), 1);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightmapSize();
	modelMatrix =
		Matrix4::Translation(Vector3(hSize.x * 0.5f, hSize.y * 0.55, hSize.z * 0.5f)) *
		Matrix4::Scale(hSize * 0.5f) *
		Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix =
		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10));
	Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	SetShaderLight(*light);
	quad->Draw();
}

void Renderer::DrawBuilding() {
	glCullFace(GL_BACK);
	BuildNodeLists(root);
	SortNodeLists();
	BindShader(buildingShader);
	SetShaderLight(*ambientLight);

	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, buildingBump);

	glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(buildingShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	UpdateShaderMatrices();

	DrawNodes();
	ClearNodeLists();

}

void Renderer::DrawFloorShadow() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BuildNodeLists(root);
	SortNodeLists();
	BindShader(buildingShader);
	glUniform1f(glGetUniformLocation(buildingShader->GetProgram(), "alpha"), 1);

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

		if (n->GetMesh() == sphere) {

			sphMesh->SetTransform(Matrix4::Translation(Vector3(heightMap->GetHeightmapSize().x * 0.7, changeSpherePos, heightMap->GetHeightmapSize().z * 0.9)));
			glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, waterTex);

			glUniform1f(glGetUniformLocation(buildingShader->GetProgram(), "alpha"), 0.5);
		}
		else {
			modelMatrix.ToIdentity();
			glUniform1i(glGetUniformLocation(buildingShader->GetProgram(), "diffuseTex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, buildingTex);

			glUniform1f(glGetUniformLocation(buildingShader->GetProgram(), "alpha"), 1);
		}

		n->Draw(*this);
	}

}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}


void Renderer::DrawGeom() {
	BindShader(geomShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buildingTex);

	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, buildingBump);

	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "thirdTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, pinkTex);

	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "fourthTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, neonTex);

	glUniform3fv(glGetUniformLocation(geomShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());


	sceneTransforms[0] = Matrix4::Translation(Vector3(2500, 400, 300)) * Matrix4::Rotation(0, Vector3(0, 0, 0)) * Matrix4::Scale(Vector3(300, 150, 300));
	sceneTransforms[1] = Matrix4::Translation(Vector3(2500, 400 + 150, 300)) * Matrix4::Rotation(0, Vector3(0, 0, 0)) * Matrix4::Scale(Vector3(150, 50, 150));
	sceneTransforms[2] = Matrix4::Translation(Vector3(2800, 450, 2700)) * Matrix4::Rotation(180, Vector3(1, 0, 0)) * Matrix4::Rotation(70, Vector3(0, 1, 0)) * Matrix4::Scale(Vector3(150, 150, 0));

	for (int i = 0; i < 3; ++i) {
		if (i == 2) {
			glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "diffuseTex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, vWaveTex);

			glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "bumpTex"), 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, buildingBump);

			glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "shadowTex"), 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, shadowTex);

			glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "thirdTex"), 3);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, stencilTex);

			glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "fourthTex"), 4);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, pinkTex);

			glUniform3fv(glGetUniformLocation(geomShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
		}

		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();

	}

}

void Renderer::DrawRobot() {
	//robot shadow
	BindShader(robotShadowShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glUniform1i(glGetUniformLocation(robotShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorTex);

	modelMatrix = Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.55, 1.001, changeRobotPos)) * Matrix4::Scale(Vector3(100, 0, 120));
	UpdateShaderMatrices();
	cylinder->Draw();

	//robot
	modelMatrix.ToZero();
	glEnable(GL_CULL_FACE);
	viewMatrix = camera->BuildViewMatrix();
	Vector3 heightMapSize = heightMap->GetHeightmapSize();

	//change robot position
	if (keepRobotCount == 1) {
		modelMatrix = Matrix4::Translation(heightMapSize * Vector3(0.55, 1.01, changeRobotPos)) * Matrix4::Scale(Vector3(100, 100, 100)) * Matrix4::Rotation(0, Vector3(0, 1, 0));
	}
	if (keepRobotCount == 0) {
		modelMatrix = Matrix4::Translation(heightMapSize * Vector3(0.55, 1.01, changeRobotPos)) * Matrix4::Scale(Vector3(100, 100, 100)) * Matrix4::Rotation(180, Vector3(0, 1, 0));
	}

	if (changeRobotPos >= 0.99) {
		keepRobotCount = 0;
	}
	if (changeRobotPos <= 0.01) {
		keepRobotCount = 1;
	}

	BindShader(robotShader);
	glUniform1i(glGetUniformLocation(robotShader->GetProgram(), "diffuseTex"), 0);
	UpdateShaderMatrices();

	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = animMesh->GetInverseBindPose();
	const Matrix4* frameData = anim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < animMesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(robotShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());


	for (int i = 0; i < animMesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, matTextures[i]);
		animMesh->DrawSubMesh(i);
	}
	glDisable(GL_CULL_FACE);

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


void Renderer::FinalSceneIntoFBO() {

	glBindFramebuffer(GL_FRAMEBUFFER, wholeSceneFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);
	CombineFirstAndLightBuffers();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, buffBlurFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	CombineFirstAndLightBuffers();
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

void Renderer::PointLightsFBO() {
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = {
	 GL_COLOR_ATTACHMENT0 ,
	 GL_COLOR_ATTACHMENT1
	};

	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);


	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);  
	glViewport(0, 0, width, height);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BlurFBOs() {
	glGenTextures(1, &buffDepthTex);
	glBindTexture(GL_TEXTURE_2D, buffDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &buffColTex[i]);
		glBindTexture(GL_TEXTURE_2D, buffColTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &buffBlurFBO); 
	glGenFramebuffers(1, &processFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, buffBlurFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, buffDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, buffDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffColTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !buffDepthTex || !buffColTex[0]) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	glDrawBuffers(2, buffers);
	glViewport(0, 0, width, height);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Renderer::FillFirstFBO() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	DrawRobot();
	DrawHeightmap();
	DrawWater();
	DrawGeom();

	DrawBuilding();

	DrawRobot();
	DrawHeightmap();
	DrawWater();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::FillLightFBO() {
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

	//changing spec light positions
	Vector3 lightPos = light->GetPosition();
	lightPos.z = -changeSpotLightPos;
	lightPos.y = changeSpotLightPos;
	lightPos.x = -changeSpotLightPos;

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&lightPos);

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineFirstAndLightBuffers() {
	glClearColor(0, 0, 0, 0);
	BindShader(combineShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToZero();
	projMatrix.ToZero();

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	UpdateShaderMatrices();
	quad->Draw();
}



void Renderer::PrintSplitScreen() {
	glViewport(0, 0, width / 2, height);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	BindShader(basicShader);

	glUniform1i(glGetUniformLocation(basicShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, wholeSceneTex);

	UpdateShaderMatrices();
	quad->Draw();


	glViewport(width / 2, 0, width / 2, height);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	BindShader(basicShader);

	glUniform1i(glGetUniformLocation(basicShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	UpdateShaderMatrices();
	quad->Draw();

	afterSplit = true;

}

void Renderer::PrintBlurScreen() {

	if (afterSplit == true) {
		glViewport(0, 0, width / 2, height);
	}

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	BindShader(blurShader);

	glUniform1i(glGetUniformLocation(blurShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buffColTex[0]);

	UpdateShaderMatrices();
	quad->Draw();

	//prevent showing main screen in background with split
	if (afterSplit == true) {
		glViewport(width / 2, 0, width / 2, height);
		afterSplit = false;
	}
	else {
		glViewport(width / 2, 0, 0, 0);
	}


	//other view to render properly
	BindShader(blurShader);
	glUniform1i(glGetUniformLocation(blurShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buffColTex[0]);
	UpdateShaderMatrices();
	quad->Draw();

}

void Renderer::PrintGammaCorrect() {
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	BindShader(gammaShader);

	glUniform1i(glGetUniformLocation(gammaShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, wholeSceneTex);

	UpdateShaderMatrices();
	quad->Draw();

	//other view to render properly
	glViewport(width / 2, 0, 0, 0);
	BindShader(gammaShader);
	glUniform1i(glGetUniformLocation(gammaShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, wholeSceneTex);
	UpdateShaderMatrices();
	quad->Draw();

}

void Renderer::PrintColourChange() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffColTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(colourChangeShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(colourChangeShader->GetProgram(), "sceneTex"), 0);


	for (int i = 0; i < BLUR_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffColTex[1], 0);
		glUniform1i(glGetUniformLocation(colourChangeShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, buffColTex[0]);
		quad->Draw();

		glUniform1i(glGetUniformLocation(colourChangeShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffColTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, buffColTex[1]);
		quad->Draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}
