## 8502 - Game Graphics - 2020/2021
<br />
This repo shows rendering techniques using C++ and OpenGL for the 8502 coursework at Newcastle University. The coursework was based upon a retrowave/cyberpunk theme.
<br />

### Acknowledgements
The main framework was provided by Dr Rich Davison and he has given permission to use it for learning purposes.
<br /><br />

### Short Demonstration Video
[Video Link](https://youtu.be/5CmZtc3gN7A)
<br /><br />

### Screenshots
#### Shadows:
_Buildings are created with a scene graph and then shadows are created using shadow mapping._<br />

<a name = "shadow"><img src="https://github.com/Akeilee/Game-Graphics/blob/master/Screenshots/shadows.PNG" width = "500"></a> <br /><br />

#### Deferred Rendering:
_Deferred rendering allows the multiple lighting calculations to be done on the pixels shown on the screen rather than doing the expensive calclations per fragment which could be hidden behind other objects._ <br />

<a name = "lighting"><img src="https://github.com/Akeilee/Game-Graphics/blob/master/Screenshots/lighting.gif" width = "500"></a> <br /><br />

#### Post Processing:
_Post processing is when a scene is rendered as a texture and is put into a Frame Buffer Object which then after a shader is applied. The result will look as if the scene was rendered straight to the back buffer. This technique is used so different shaders can be applied to the same texture._<br />

_Blur_<br />
<a name = "blur"><img src="https://github.com/Akeilee/Game-Graphics/blob/master/Screenshots/blur.gif" width = "500"></a> <br />

_Gamma Correction_<br />
<a name = "gamma"><img src="https://github.com/Akeilee/Game-Graphics/blob/master/Screenshots/gamma.gif" width = "500"></a> <br />

_Split Screen_<br />
<a name = "split"><img src="https://github.com/Akeilee/Game-Graphics/blob/master/Screenshots/splitscreen.gif" width = "500"></a> <br /><br />

#### Textures:
_Bump maps used on buildings and heightmap_ <br />

<a name = "texture"><img src="https://github.com/Akeilee/Game-Graphics/blob/master/Screenshots/textures.PNG" width = "500"></a> <br /><br />
<br />
### Keybinds
| | |
| :---: | :---: |
|**W,S,A,D**| Move camera |
|**Q,E**| Move camera up and down |
| | |
|**1**| Toggle camera to auto or manual |
|**2**| Toggle blur effect |
|**3**| Toggle lighting effect |
|**4**| Toggle gamma correction |
|**5**| Toggle split screen view (able to use post processing effects during split screen) |
| | |
|**F5**| Reload shaders |
<br />

#### Asset references

[Robot](https://assetstore.unity.com/packages/3d/characters/robots/robot-1-65726)

[Skybox](https://www.artstation.com/artwork/9QDkL)
