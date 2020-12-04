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
	void PointLightFBO(); /////////
	~Renderer(void);
	void DrawRobot();
	void RenderScene() override;

	void robotintoFBO();
	
	void UpdateScene(float dt) override;
	void Gui();
	void splitscreen();
	bool usingdepth;
	bool partylight;
	bool autoCamera;

protected:
	
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();

	void DrawNode(SceneNode* n);


	void DrawHeightmap();
	void HeightMapShadow();////////////
	void DrawWater();
	void DrawSkybox();
	void DrawShadowScene();

	void DrawFullScene();

	void BuildingShadowFBO();

	void SphereShadowFBO();

	void MakeOrbs();

	void DrawStencilObject();

	void OrbShadow();

	

	//tut 7
	SceneNode* root;
	HeightMap* buildingMap;
	Mesh* cube;
	Shader* buildingShader;
	GLuint buildingTex;
	GLuint buildingBump;


	GLuint thirdTex;
	GLuint fourthTex;
	GLuint stencilTex;
	GLuint vWaveTex;
	GLuint blankTex;
	Frustum frameFrustum;
	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	//tut 14
	Shader* shadowShader;
	GLuint shadowTex;
	GLuint shadowFBO;
	void DrawBuilding();
	void DrawBuilding2();
	void DrawMainScene();
	void DrawFloorShadow();

	void robotShadow();

	vector<Mesh*> sceneMeshes;
	vector <Matrix4 > sceneTransforms;
	GLuint sphereTex;
	GLuint sphereFBO;
	Mesh* sphere;
	Mesh* cylinder;
	Matrix4 buildingTransform;
	float sceneTime;

	Shader* orbShader;
	Shader* blendShader;

	Shader* tut1Shader;
	Shader* basicShader;
	Shader* sceneShader;
	Shader* sceneShader2; ////////
	Shader* textureShader; ////////
	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;
	HeightMap* heightMap;
	Mesh* quad;
	Mesh* waterQuad;
	Light* light;
	Light* light2;
	Light* orbLight;
	Camera* camera;
	Camera* camera2;
	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;
	GLuint robotshadow;



	float waterRotate;
	float waterCycle;

	GLuint wholeSceneTex;
	GLuint wholeSceneFBO;
	void WholeSceneFBO();
	void forReflect();

	

	//tut 15
	void FillBuffers(); //G- Buffer Fill Render Pass
	void DrawPointLights(); // Lighting Render Pass
	void CombineBuffers(); // Combination Render Pass
	// Make a new texture ...
	void GenerateScreenTexture(GLuint& into, bool depth = false);
	Shader* pointlightShader; // Shader to calculate lighting
	Shader* combineShader; // shader to stick it all together

	GLuint bufferFBO; // FBO for our G- Buffer pass
	GLuint bufferColourTex; // Albedo goes here
	GLuint bufferNormalTex; // Normals go here
	GLuint bufferDepthTex; // Depth goes here

	GLuint pointLightFBO; // FBO for our lighting pass
	GLuint lightDiffuseTex; // Store diffuse lighting
	GLuint lightSpecularTex; // Store specular lighting

	Light* pointLights; // Array of lighting data


	//tut 9
	Mesh* animMesh;
	MeshAnimation* anim;
	MeshMaterial* material;
	vector <GLuint > matTextures;
	Shader* tut9shader;

	int currentFrame;
	float frameTime;



	//tut10
	Shader* helpshader;
	Shader* processShader;

	void Tut10FBO();
	void PrintBlurScreen();
	void DrawPostProcess();
	GLuint buffBlurFBO;
	GLuint processFBO;
	GLuint buffColTex[2];
	GLuint buffDepthTex;
};