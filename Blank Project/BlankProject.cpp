//Name: Jane Lee
//Date: 12/2020
//8502 Graphics CW


#include "../NCLGL/window.h"
#include "Renderer.h"

int main()	{
	Window w("JLee 8502 Project", 1280, 720, false);

	if(!w.HasInitialised()) {
		return -1;
	}
	
	Renderer renderer(w);
	if(!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while(w.UpdateWindow()  && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){


		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2)) {
			renderer.usingBlur = !renderer.usingBlur;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3)) {
			renderer.partyLight = !renderer.partyLight;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4)) {
			renderer.gammaCorrect = !renderer.gammaCorrect;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_5)) {
			renderer.splitScreen = !renderer.splitScreen;
		}


		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		renderer.SwapBuffers();
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}
	return 0;
}