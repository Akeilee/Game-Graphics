#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
class Camera;
class Shader;
class HeightMap;
class SceneNode;
class Mesh;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);
	void RenderScene() override;
	
	void UpdateScene(float dt) override;

protected:

	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNodes2();///////////////
	void DrawNode(SceneNode* n);
	void DrawNode2(SceneNode* n);////////////////

	void DrawHeightmap();
	void DrawHeightmap2();////////////
	void DrawWater();
	void DrawSkybox();
	void DrawShadowScene();

	//tut 7
	SceneNode* root;
	HeightMap* buildingMap;
	Mesh* cube;
	Shader* buildingShader;
	GLuint buildingTex;
	GLuint buildingBump;
	Frustum frameFrustum;
	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	//tut 14
	Shader* shadowShader;
	GLuint shadowTex;
	GLuint shadowFBO;
	void DrawBuilding();
	void DrawFloorShadow();



	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;
	HeightMap* heightMap;
	Mesh* quad;
	Light* light;
	Light* light2;
	Camera* camera;
	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;

	float waterRotate;
	float waterCycle;

};