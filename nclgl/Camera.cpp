#include "Camera.h"
#include "Window.h"

bool changecam = false;
float addtime;

void Camera::UpdateCamera(float dt) {
	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);


	//float speed = 30.0f * dt;
	float speed = 5000.0f * dt;

	if (changecam == true) {
		pitch -= (Window::GetMouse()->GetRelativePosition().y);
		yaw -= (Window::GetMouse()->GetRelativePosition().x);
	}
	addtime += dt;
	if (changecam == false) {
		//yaw = (30*sin(addtime)+50);
		//yaw = (30*sin(addtime)+20);
		//pitch = 0;

		//position = Vector3(3800,450,3500);
		position += forward * (sin(addtime)*12);
		position -= right * (cos(addtime)*4);
		position.y += (sin(addtime)*2);

		//yaw -= 0.25*sin(addtime)/2;
		pitch += (0.25* sin(addtime)/4);

	}

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;

	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}



	if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) { //forward
		changecam = !changecam;
	}

	if (changecam == true) {
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) { //forward
			position += forward * speed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) { //backward
			position -= forward * speed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) { //left
			position -= right * speed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) { //right
			position += right * speed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_Q)) { //up
			position.y += speed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_E)) { //down
			position.y -= speed;
		}
	}

}

Matrix4 Camera::BuildViewMatrix() {
	 return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
};

