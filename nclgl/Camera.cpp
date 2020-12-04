#include "Camera.h"
#include "Window.h"

float addTime;
float timer;

void Camera::UpdateCamera(float dt) {
	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);


	//float speed = 30.0f * dt;
	float speed = 2500.0f * dt;

	if (changeCam == true) {
		pitch -= (Window::GetMouse()->GetRelativePosition().y);
		yaw -= (Window::GetMouse()->GetRelativePosition().x);
	}

	addTime += dt;
	timer += dt;

	if (changeCam == false) {

		position += forward * (0.5*(1+sin(addTime)*12));
		position -= right * (1+sin(addTime)*4);
		position.y += (1+sin(addTime)/2);

		pitch += (0.25* (sin(addTime))/5);
		yaw += (sin(addTime)/20);
	}
	if (changeCam == false && timer >= 15 && revert == false) {
		position = Vector3(200, 700, 200);
		yaw = 250;
		/*position.x -= 100 * (sin(addTime) * 12);
		position.z += 100 * (cos(addTime) * 4);
		position.y += (sin(addTime) * 2);

		pitch += (0.25 * sin(addTime) / 4);*/
		timer = 0;
		timer += dt;
		revert = true;
	}

	if (changeCam == false && timer >= 10 && revert == true) {
		position = Vector3(3700, 600, 1600);
		yaw = 75;
		/*position.x += -120 * (sin(addTime) * 12);
		position.z -= -80 * (cos(addTime) * 4);
		position.y += (sin(addTime) * 2);*/

		//pitch += (0.25 * sin(addTime) / 4);
		timer = 0;
		timer += dt;
		revert = false;
	}





	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;

	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}


	if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) { //auto cam
		changeCam = !changeCam;
	}

	if (changeCam == true) {
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

