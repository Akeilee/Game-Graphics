#include "Camera.h"
#include "Window.h"

void Camera::UpdateCamera(float dt) {
	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw -= (Window::GetMouse()->GetRelativePosition().x);

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;

	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);


	//float speed = 30.0f * dt;
	float speed = 5000.0f * dt;

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

Matrix4 Camera::BuildViewMatrix() {
	 return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
};

