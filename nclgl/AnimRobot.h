#pragma once
#include "../nclgl/scenenode.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/OGLRenderer.h"

class AnimRobot : public SceneNode {
public:
	AnimRobot(Vector3 heightmap);
	AnimRobot(Mesh* mesh, MeshAnimation* anim, MeshMaterial* mat, vector<GLuint> matTex, Shader*shader, Vector3 heightmap);
	~AnimRobot(void) {};
	void Update(float dt) override;

protected:
	Mesh* animMesh;
	MeshAnimation* anim;
	MeshMaterial* material;
	vector <GLuint > matTextures;
	Shader* shader;


	Vector3 heightMapSize;
	int currentFrame;
	float frameTime;
	int changeRobotPos = 0;
	int count = 0;
	float sceneTime;
};
