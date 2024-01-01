# 1. Grid Example

This very bare bones OpenGL project doubles as a demonstration of my general design approach for organizing gl initialization, shaders, user input, camera, and the game loop. It's 325 lines and all in main.cpp which is obviously not what you want to do and was done for readability.

```
void main() {

	GL::Init(1920, 1080, "GridExample");
	Game::Init();
	Renderer::Init();

	while (GL::WindowIsOpen()) {
		Input::Update();
		Game::Update();
		Renderer::RenderFrame();
		GL::SwapBuffersPollEvents();
	}

	GL::Cleanup();
	return;
}
```

![Image](https://www.principiaprogrammatica.com/dump/gridexample.jpg)
