#include "AnimRobot.h"

AnimRobot::AnimRobot(Vector3 heightMap) {
	heightMap = heightMapSize;
}
AnimRobot::AnimRobot(Mesh* mesh, MeshAnimation* anim1, MeshMaterial* mat, vector<GLuint> matTex, Shader* shader1, Vector3 heightMap) {
	heightMap = heightMapSize;

	animMesh = mesh;
	anim = anim1;
	material = mat;
	matTextures = matTex;
	shader = shader1;





}


void AnimRobot::Update(float dt) {
	sceneTime += dt;
	float walkSpeed = sceneTime / 5;
	changeRobotPos = (float)0.5 * (1 + sin(walkSpeed));

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}

	if (count == 1) {
		transform = Matrix4::Translation(heightMapSize * Vector3(0.55, 1.01, changeRobotPos)) * Matrix4::Scale(Vector3(100, 100, 100)) * Matrix4::Rotation(0, Vector3(0, 1, 0));
	}
	if (count == 0) {
		transform = Matrix4::Translation(heightMapSize * Vector3(0.55, 1.01, changeRobotPos)) * Matrix4::Scale(Vector3(100, 100, 100)) * Matrix4::Rotation(180, Vector3(0, 1, 0));
	}

	if (changeRobotPos >= 0.99) {
		count = 0;
	}
	if (changeRobotPos <= 0.01) {
		count = 1;
	}



	SceneNode::Update(dt);
}