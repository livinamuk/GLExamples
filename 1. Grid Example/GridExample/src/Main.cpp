
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

#define NEAR_PLANE 0.005f
#define FAR_PLANE 50.0f
#define HELL_PI 3.141592653589793f
#define DARK_SQUARE glm::vec3(0.3984375f, 0.265625f, 0.2265625f)
#define LIGHT_SQUARE glm::vec3(0.95703125f, 0.8984375f, 0.74609375f)

namespace GL {

	GLFWwindow* _window;

	void Init(int width, int height, std::string title) {
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
		_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		if (_window == NULL) {
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return;
		}
		glfwMakeContextCurrent(_window);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cout << "Failed to initialize GLAD" << std::endl;
			return;
		}
	}

	GLFWwindow* GetWindowPointer() {
		return _window;
	}

	bool WindowIsOpen() {
		return !glfwWindowShouldClose(_window);
	}
	
	void SetWindowShouldClose(bool value) {
		glfwSetWindowShouldClose(_window, value);
	}

	void SwapBuffersPollEvents() {
		glfwSwapBuffers(_window);
		glfwPollEvents();
	}

	void Cleanup() {
		glfwTerminate();
	}
}

namespace Util {

	inline std::string ReadTextFromFile(std::string path) {
		std::ifstream file(path);
		std::string str;
		std::string line;
		while (std::getline(file, line)) {
			str += line + "\n";
		}
		return str;
	}
}

struct Shader {

	int _ID = -1;
	std::unordered_map<std::string, int> _uniformsLocations;

	int CheckErrors(unsigned int shader, std::string type) {
		int success;
		char infoLog[1024];
		if (type != "PROGRAM") {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "Shader compilation error: " << type << "\n" << infoLog << "\n";
			}
		}
		else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "Shader linking error: " << type << "\n" << infoLog << "\n";
			}
		}
		return success;
	}

	void Load(std::string vertexPath, std::string fragmentPath) {
		_uniformsLocations.clear();
		std::string vertexSource = Util::ReadTextFromFile("res/shaders/" + vertexPath);
		std::string fragmentSource = Util::ReadTextFromFile("res/shaders/" + fragmentPath);
		const char* vShaderCode = vertexSource.c_str();
		const char* fShaderCode = fragmentSource.c_str();
		unsigned int vertex, fragment;
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		CheckErrors(vertex, "VERTEX");
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		int tempID = glCreateProgram();
		glAttachShader(tempID, vertex);
		glAttachShader(tempID, fragment);
		glLinkProgram(tempID);
		if (CheckErrors(tempID, "PROGRAM")) {
			_ID = tempID;
		}
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void Bind() {
		glUseProgram(_ID);
	}

	void SetMat4(const std::string& name, glm::mat4 value) {
		if (_uniformsLocations.find(name) == _uniformsLocations.end()) {
			_uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
		}
		glUniformMatrix4fv(_uniformsLocations[name], 1, GL_FALSE, &value[0][0]);
	}

	void SetVec3(const std::string& name, const glm::vec3& value) {
		if (_uniformsLocations.find(name) == _uniformsLocations.end()) {
			_uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
		}
		glUniform3fv(_uniformsLocations[name], 1, &value[0]);
	}
};

struct Transform {
	glm::vec3 position = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);
	glm::vec3 scale = glm::vec3(1);
	glm::mat4 to_mat4() {
		glm::mat4 m = glm::translate(glm::mat4(1), position);
		m *= glm::mat4_cast(glm::quat(rotation));
		m = glm::scale(m, scale);
		return m;
	};
};

struct Vertex {
	glm::vec3 position = glm::vec3(0);
	glm::vec3 normal = glm::vec3(0);
	glm::vec2 uv = glm::vec2(0);
};

namespace Input {

	bool _keyPressed[372];
	bool _keyDown[372];
	bool _keyDownLastFrame[372];

	void Update() {
		GLFWwindow* window = GL::GetWindowPointer();
		for (int i = 30; i < 350; i++) {
			_keyDown[i] = (glfwGetKey(window, i) == GLFW_PRESS);
			_keyPressed[i] = (_keyDown[i] && !_keyDownLastFrame[i]);
			_keyDownLastFrame[i] = _keyDown[i];
		}
	}

	bool KeyPressed(unsigned int keycode) {
		return _keyPressed[keycode];
	}

	bool KeyDown(unsigned int keycode) {
		return _keyDown[keycode];
	}
}

namespace Game {

	Transform _camera;
	float _camSpeed = 0.05f;

	void Init() {
		_camera.position = glm::vec3(3.75f, 5.0f, 10.3f);
		_camera.rotation = glm::vec3(-0.75f, 0.0f, 0.0f);
	}

	void Update() {

		if (Input::KeyPressed(GLFW_KEY_ESCAPE)) {
			GL::SetWindowShouldClose(true);
		}

		if (Input::KeyDown(GLFW_KEY_A)) {
			_camera.position.x -= _camSpeed;
		}
		if (Input::KeyDown(GLFW_KEY_D)) {
			_camera.position.x += _camSpeed;
		}
		if (Input::KeyDown(GLFW_KEY_W)) {
			_camera.position.z -= _camSpeed;
		}
		if (Input::KeyDown(GLFW_KEY_S)) {
			_camera.position.z += _camSpeed;
		}
	}

	glm::mat4 GetViewMatrix() {
		return glm::inverse(_camera.to_mat4());
	}
}

namespace Renderer {

	Shader _solidColorshader;
	GLuint _quadVao = 0;

	void Init() {
		_solidColorshader.Load("solidcolor.vert", "solidcolor.frag");
	}

	void DrawQuad() {
		if (_quadVao == 0) {
			std::vector<Vertex> vertices;
			Vertex vert0 = { glm::vec3(-0.5f,  0.5f, 0.0f) };
			Vertex vert1 = { glm::vec3( 0.5f,  0.5f, 0.0f) };
			Vertex vert2 = { glm::vec3( 0.5f, -0.5f, 0.0f) };
			Vertex vert3 = { glm::vec3(-0.5f, -0.5f, 0.0f) };
			vertices.push_back(vert0);
			vertices.push_back(vert1);
			vertices.push_back(vert2);
			vertices.push_back(vert3);
			std::vector<unsigned int> indices = { 2, 1, 0, 0, 3, 2 };
			unsigned int VBO;
			unsigned int EBO;
			glGenVertexArrays(1, &_quadVao);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			glBindVertexArray(_quadVao);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
		glBindVertexArray(_quadVao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	void RenderFrame() {

		glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = glm::perspective(1.0f, 1920.0f / 1080.0f, NEAR_PLANE, FAR_PLANE);
		glm::mat4 view = Game::GetViewMatrix();

		_solidColorshader.Bind();
		_solidColorshader.SetMat4("projection", projection);
		_solidColorshader.SetMat4("view", view);

		bool lightSquare = true;

		for (int x = 0; x < 8; x++) {
			for (int z = 0; z < 8; z++) {

				Transform gridSquare;
				gridSquare.position = glm::vec3(x, 0, z);
				gridSquare.rotation.x = HELL_PI * -0.5f;

				_solidColorshader.SetMat4("model", gridSquare.to_mat4());

				if (lightSquare) {
					_solidColorshader.SetVec3("color", LIGHT_SQUARE);
				}
				else {
					_solidColorshader.SetVec3("color", DARK_SQUARE);
				}
				DrawQuad();
				lightSquare = !lightSquare;
			}
			lightSquare = !lightSquare;
		}

		// Shader hotloading
		if (Input::KeyPressed(GLFW_KEY_H)) {
			_solidColorshader.Load("solidcolor.vert", "solidcolor.frag");
		}
	}
}

void main() {

	GL::Init(1920, 1080, "Grid Example");
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
