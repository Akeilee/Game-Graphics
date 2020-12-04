//Name: Jane Lee
//Date: 12/2020
//8502 Graphics CW


#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
class Camera;
class Shader;
class HeightMap;
class SceneNode;
class Mesh;
class MeshAnimation;
class MeshMaterial;


class Renderer : public OGLRenderer {
public:

	Renderer(Window& parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;

	bool usingBlur;
	bool partyLight;
	bool autoCamera;
	bool gammaCorrect;
	bool splitScreen;

protected:

	void DrawSkybox();
	void DrawHeightmap();
	void DrawWater();

	void DrawBuilding();
	void DrawFloorShadow();

	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);
	void ClearNodeLists();

	void DrawGeom();
	void DrawRobot();

	void GenerateScreenTexture(GLuint& into, bool depth =false);

	void FinalSceneIntoFBO();
	void BuildingShadowFBO();
	void PointLightsFBO();
	void BlurFBOs();
	void WholeSceneFBO();

	void FillFirstFBO();
	void FillLightFBO(); // Lighting Render Pass
	void CombineFirstAndLightBuffers(); // Combination Render Pass

	//post processes
	void PrintSplitScreen();
	void PrintBlurScreen();
	void PrintGammaCorrect();
	void PrintColourChange();
	

	SceneNode* sphMesh = new SceneNode();
	Mesh* sphere;
	Mesh* cylinder;
	Mesh* quad;
	Mesh* cube;

	Light* light;
	Light* ambientLight;
	Light* pointLights; // Array of lights

	Camera* camera;

	HeightMap* heightMap;

	//textures
	GLuint pinkTex;
	GLuint neonTex;
	GLuint stencilTex;
	GLuint vWaveTex;
	GLuint blankTex;
	GLuint buildingTex;
	GLuint buildingBump;
	GLuint cubeMap;
	GLuint waterTex;
	GLuint floorTex;
	GLuint floorBump;

	//shaders
	Shader* reflectShader;
	Shader* skyboxShader;
	Shader* buildingShader;
	Shader* geomShader;
	Shader* pointlightShader; 
	Shader* combineShader; 
	Shader* robotShader;
	Shader* robotShadowShader;
	Shader* blurShader;
	Shader* gammaShader;
	Shader* colourChangeShader;
	Shader* basicShader;


	//scene graph
	SceneNode* root;
	Frustum frameFrustum;
	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	//shadow map
	GLuint shadowTex;
	GLuint shadowFBO;

	//manual add geom
	vector<Mesh*> sceneMeshes;
	vector<Matrix4> sceneTransforms;

	//whole scene FBO
	GLuint wholeSceneTex;
	GLuint wholeSceneFBO;

	//animation
	Mesh* animMesh;
	MeshAnimation* anim;
	MeshMaterial* material;
	vector <GLuint > matTextures;

	//blur
	GLuint buffBlurFBO;
	GLuint processFBO;
	GLuint buffColTex[2];
	GLuint buffDepthTex;

	//point lights
	GLuint bufferFBO; 
	GLuint bufferColourTex; 
	GLuint bufferNormalTex; 
	GLuint bufferDepthTex; 
	GLuint pointLightFBO; // FBO for our lighting pass
	GLuint lightDiffuseTex; // Store diffuse lighting
	GLuint lightSpecularTex; // Store specular lighting


	float waterRotate;
	float waterCycle;
	float sceneTime;
	float frameTime;
	int currentFrame;
	bool afterSplit;
};