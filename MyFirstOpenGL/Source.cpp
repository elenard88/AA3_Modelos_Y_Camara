#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include "Model.h"
#include <stb_image.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

std::vector<GLuint> compiledPrograms;
std::vector<Model> models;

struct ShaderProgram {
	GLuint vertexShader = 0;
	GLuint geometryShader = 0;
	GLuint fragmentShader = 0;
};

void Resize_Window(GLFWwindow* window, int iFrameBufferWidth, int iFrameBufferHeight) {

	//Definir nuevo tamaño del viewport
	glViewport(0, 0, iFrameBufferWidth, iFrameBufferHeight);
	glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), iFrameBufferWidth, iFrameBufferHeight);
}

//Funcion que leera un .obj y devolvera un modelo para poder ser renderizado
Model LoadOBJModel(const std::string& filePath) {

	//Verifico archivo y si no puedo abrirlo cierro aplicativo
	std::ifstream file(filePath);

	if (!file.is_open()) {
		std::cerr << "No se ha podido abrir el archivo: " << filePath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	//Variables lectura fichero
	std::string line;
	std::stringstream ss;
	std::string prefix;
	glm::vec3 tmpVec3;
	glm::vec2 tmpVec2;

	//Variables elemento modelo
	std::vector<float> vertexs;
	std::vector<float> vertexNormal;
	std::vector<float> textureCoordinates;

	//Variables temporales para algoritmos de sort
	std::vector<float> tmpVertexs;
	std::vector<float> tmpNormals;
	std::vector<float> tmpTextureCoordinates;

	//Recorremos archivo linea por linea
	while (std::getline(file, line)) {

		//Por cada linea reviso el prefijo del archivo que me indica que estoy analizando
		ss.clear();
		ss.str(line);
		ss >> prefix;

		//Estoy leyendo un vertice
		if (prefix == "v") {

			//Asumo que solo trabajo 3D así que almaceno XYZ de forma consecutiva
			ss >> tmpVec3.x >> tmpVec3.y >> tmpVec3.z;

			//Almaceno en mi vector de vertices los valores
			tmpVertexs.push_back(tmpVec3.x);
			tmpVertexs.push_back(tmpVec3.y);
			tmpVertexs.push_back(tmpVec3.z);
		}

		//Estoy leyendo una UV (texture coordinate)
		else if (prefix == "vt") {

			//Las UVs son siempre imagenes 2D asi que uso el tmpvec2 para almacenarlas
			ss >> tmpVec2.x >> tmpVec2.y;

			//Almaceno en mi vector temporal las UVs
			tmpTextureCoordinates.push_back(tmpVec2.x);
			tmpTextureCoordinates.push_back(tmpVec2.y);

		}

		//Estoy leyendo una normal
		else if (prefix == "vn") {

			//Asumo que solo trabajo 3D así que almaceno XYZ de forma consecutiva
			ss >> tmpVec3.x >> tmpVec3.y >> tmpVec3.z;

			//Almaceno en mi vector temporal de normales las normales
			tmpNormals.push_back(tmpVec3.x);
			tmpNormals.push_back(tmpVec3.y);
			tmpNormals.push_back(tmpVec3.z);

		}

		//Estoy leyendo una cara
		else if (prefix == "f") {

			int vertexData;
			short counter = 0;

			//Obtengo todos los valores hasta un espacio
			while (ss >> vertexData) {

				//En orden cada numero sigue el patron de vertice/uv/normal
				switch (counter) {
				case 0:
					//Si es un vertice lo almaceno - 1 por el offset y almaceno dos seguidos al ser un vec3, salto 1 / y aumento el contador en 1
					vertexs.push_back(tmpVertexs[(vertexData - 1) * 3]);
					vertexs.push_back(tmpVertexs[((vertexData - 1) * 3) + 1]);
					vertexs.push_back(tmpVertexs[((vertexData - 1) * 3) + 2]);
					ss.ignore(1, '/');
					counter++;
					break;
				case 1:
					//Si es un uv lo almaceno - 1 por el offset y almaceno dos seguidos al ser un vec2, salto 1 / y aumento el contador en 1
					textureCoordinates.push_back(tmpTextureCoordinates[(vertexData - 1) * 2]);
					textureCoordinates.push_back(tmpTextureCoordinates[((vertexData - 1) * 2) + 1]);
					ss.ignore(1, '/');
					counter++;
					break;
				case 2:
					//Si es una normal la almaceno - 1 por el offset y almaceno tres seguidos al ser un vec3, salto 1 / y reinicio
					vertexNormal.push_back(tmpNormals[(vertexData - 1) * 3]);
					vertexNormal.push_back(tmpNormals[((vertexData - 1) * 3) + 1]);
					vertexNormal.push_back(tmpNormals[((vertexData - 1) * 3) + 2]);
					counter = 0;
					break;
				}
			}
		}
	}
	return Model(vertexs, textureCoordinates, vertexNormal);
}


//Funcion que devolvera una string con todo el archivo leido
std::string Load_File(const std::string& filePath) {

	std::ifstream file(filePath);

	std::string fileContent;
	std::string line;

	//Lanzamos error si el archivo no se ha podido abrir
	if (!file.is_open()) {
		std::cerr << "No se ha podido abrir el archivo: " << filePath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	//Leemos el contenido y lo volcamos a la variable auxiliar
	while (std::getline(file, line)) {
		fileContent += line + "\n";
	}

	//Cerramos stream de datos y devolvemos contenido
	file.close();

	return fileContent;
}

GLuint LoadFragmentShader(const std::string& filePath) {

	// Crear un fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	//Usamos la funcion creada para leer el fragment shader y almacenarlo 
	std::string sShaderCode = Load_File(filePath);
	const char* cShaderSource = sShaderCode.c_str();

	//Vinculamos el fragment shader con su código fuente
	glShaderSource(fragmentShader, 1, &cShaderSource, nullptr);

	// Compilar el fragment shader
	glCompileShader(fragmentShader);

	// Verificar errores de compilación
	GLint success;
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	//Si la compilacion ha sido exitosa devolvemos el fragment shader
	if (success) {

		return fragmentShader;

	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);

		//Obtenemos el log
		std::vector<GLchar> errorLog(logLength);
		glGetShaderInfoLog(fragmentShader, logLength, nullptr, errorLog.data());

		//Mostramos el log y finalizamos programa
		std::cerr << "Se ha producido un error al cargar el fragment shader:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}


GLuint LoadGeometryShader(const std::string& filePath) {

	// Crear un vertex shader
	GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);

	//Usamos la funcion creada para leer el vertex shader y almacenarlo 
	std::string sShaderCode = Load_File(filePath);
	const char* cShaderSource = sShaderCode.c_str();

	//Vinculamos el vertex shader con su código fuente
	glShaderSource(geometryShader, 1, &cShaderSource, nullptr);

	// Compilar el vertex shader
	glCompileShader(geometryShader);

	// Verificar errores de compilación
	GLint success;
	glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);

	//Si la compilacion ha sido exitosa devolvemos el vertex shader
	if (success) {

		return geometryShader;

	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &logLength);

		//Obtenemos el log
		std::vector<GLchar> errorLog(logLength);
		glGetShaderInfoLog(geometryShader, logLength, nullptr, errorLog.data());

		//Mostramos el log y finalizamos programa
		std::cerr << "Se ha producido un error al cargar el vertex shader:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

GLuint LoadVertexShader(const std::string& filePath) {

	// Crear un vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	//Usamos la funcion creada para leer el vertex shader y almacenarlo 
	std::string sShaderCode = Load_File(filePath);
	const char* cShaderSource = sShaderCode.c_str();

	//Vinculamos el vertex shader con su código fuente
	glShaderSource(vertexShader, 1, &cShaderSource, nullptr);

	// Compilar el vertex shader
	glCompileShader(vertexShader);

	// Verificar errores de compilación
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	//Si la compilacion ha sido exitosa devolvemos el vertex shader
	if (success) {

		return vertexShader;

	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);

		//Obtenemos el log
		std::vector<GLchar> errorLog(logLength);
		glGetShaderInfoLog(vertexShader, logLength, nullptr, errorLog.data());

		//Mostramos el log y finalizamos programa
		std::cerr << "Se ha producido un error al cargar el vertex shader:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

//Función que dado un struct que contiene los shaders de un programa generara el programa entero de la GPU
GLuint CreateProgram(const ShaderProgram& shaders) {

	//Crear programa de la GPU
	GLuint program = glCreateProgram();

	//Verificar que existe un vertex shader y adjuntarlo al programa
	if (shaders.vertexShader != 0) {
		glAttachShader(program, shaders.vertexShader);
	}

	if (shaders.geometryShader != 0) {
		glAttachShader(program, shaders.geometryShader);
	}

	if (shaders.fragmentShader != 0) {
		glAttachShader(program, shaders.fragmentShader);
	}

	// Linkear el programa
	glLinkProgram(program);

	//Obtener estado del programa
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	//Devolver programa si todo es correcto o mostrar log en caso de error
	if (success) {

		//Liberamos recursos
		if (shaders.vertexShader != 0) {
			glDetachShader(program, shaders.vertexShader);
		}

		//Liberamos recursos
		if (shaders.geometryShader != 0) {
			glDetachShader(program, shaders.geometryShader);
		}

		//Liberamos recursos
		if (shaders.fragmentShader != 0) {
			glDetachShader(program, shaders.fragmentShader);
		}

		return program;
	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

		//Almacenamos log
		std::vector<GLchar> errorLog(logLength);
		glGetProgramInfoLog(program, logLength, nullptr, errorLog.data());

		std::cerr << "Error al linkar el programa:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void SendTintColor(GLuint program, const glm::vec4& tintColor) {

	glUniform4f(glGetUniformLocation(program, "tintColor"), tintColor.r, tintColor.g, tintColor.b, tintColor.a);
}

void main() {

	//Definir semillas del rand según el tiempo
	srand(static_cast<unsigned int>(time(NULL)));

	//Inicializamos GLFW para gestionar ventanas e inputs
	glfwInit();

	//Configuramos la ventana
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	//Inicializamos la ventana
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "My Engine", NULL, NULL);

	//Asignamos función de callback para cuando el frame buffer es modificado
	glfwSetFramebufferSizeCallback(window, Resize_Window);

	//Definimos espacio de trabajo
	glfwMakeContextCurrent(window);

	//Permitimos a GLEW usar funcionalidades experimentales
	glewExperimental = GL_TRUE;

	//Activamos cull face
	glEnable(GL_CULL_FACE);

	//Indicamos lado del culling
	glCullFace(GL_BACK);

	//Indicamos lado del culling
	glEnable(GL_DEPTH_TEST);

	//Leer textura
	int width, height, nrChannels;
	unsigned char* textureInfo = stbi_load("Assets/Textures/rock.png", &width, &height, &nrChannels, 0);

	//comprobamos si carga la textura
	if (textureInfo == nullptr) {
		std::cout << "No se ha podido cargar la textura rock.png" << std::endl;
		glfwTerminate();
		return;
	}

	//Inicializamos GLEW y controlamos errores
	if (glewInit() == GLEW_OK) {

		//Compilar shaders
		ShaderProgram myFirstProgram;
		myFirstProgram.vertexShader = LoadVertexShader("MyFirstVertexShader.glsl");
		myFirstProgram.geometryShader = LoadGeometryShader("MyFirstGeometryShader.glsl");
		myFirstProgram.fragmentShader = LoadFragmentShader("MyFirstFragmentShader.glsl");

		//Cargo Modelo
		models.push_back(LoadOBJModel("Assets/Models/rock.obj"));

		//Compilar programa
		compiledPrograms.push_back(CreateProgram(myFirstProgram));

		//Definimos canal de textura activo
		glActiveTexture(GL_TEXTURE0);

		//Generamos textura
		GLuint textureID;
		glGenTextures(1, &textureID);

		//Vinculamos textura
		glBindTexture(GL_TEXTURE_2D, textureID);

		//Configuramos textura
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Definimos formato segun canales
		GLenum textureFormat = GL_RGB;

		if (nrChannels == 4) {
			textureFormat = GL_RGBA;
		}
		else if (nrChannels == 3) {
			textureFormat = GL_RGB;
		}

		//Pasamos la imagen cargada a OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, width, height, 0, textureFormat, GL_UNSIGNED_BYTE, textureInfo);

		//Generamos mipmap
		glGenerateMipmap(GL_TEXTURE_2D);

		//Liberamos memoria de CPU
		stbi_image_free(textureInfo);

		//Definimos color para limpiar el buffer de color
		glClearColor(0.f, 0.f, 0.f, 1.f);

		//Definimos modo de dibujo para cada cara
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//Indicar a la tarjeta GPU que programa debe usar
		glUseProgram(compiledPrograms[0]);

		//Definir la matriz de vista
		glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.5f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		//Definir la matriz proyeccion
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

		//Asignar valores iniciales al programa
		glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);

		//Asignar valor variable de textura a usar
		glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 0);

		//Pasar matrices de camara
		glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		//Generamos el game loop
		while (!glfwWindowShouldClose(window)) {

			//Pulleamos los eventos (botones, teclas, mouse...)
			glfwPollEvents();
			
			//Limpiamos los buffers
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			//Activamos la textura de roca
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureID);

			//roca 1
			glm::mat4 rock1TranslationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-2.f, -1.5f, 0.0f));
			glm::mat4 rock1RotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(75.0f), glm::vec3(0.0f, 1.0f, -0.5f));
			glm::mat4 rock1ScaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.45f, 0.45f, 0.45f));

			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(rock1TranslationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rock1RotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(rock1ScaleMatrix));

			SendTintColor(compiledPrograms[0], glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			models[0].Render();


			//roca 2
			glm::mat4 rock2TranslationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, -1.5f, 0.0f));
			glm::mat4 rock2RotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-10.0f), glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 rock2ScaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.55f, 0.55f, 0.55f));

			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(rock2TranslationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rock2RotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(rock2ScaleMatrix));

			SendTintColor(compiledPrograms[0], glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			models[0].Render();

			//roca 3
			glm::mat4 rock3TranslationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(2.f, -1.5f, 0.0f));
			glm::mat4 rock3RotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(35.0f), glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 rock3ScaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.75f, 0.75f, 0.75f));

			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(rock3TranslationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rock3RotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(rock3ScaleMatrix));

			SendTintColor(compiledPrograms[0], glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			models[0].Render();

			//Nube roca
			glm::mat4 cloudTranslationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-1.5f, 1.4f, -1.0f));
			glm::mat4 cloudRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 cloudScaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(1.6f, 0.25f, 0.6f));

			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(cloudTranslationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(cloudRotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(cloudScaleMatrix));

			SendTintColor(compiledPrograms[0], glm::vec4(0.8f, 0.9f, 1.0f, 1.0f));

			models[0].Render();

			//Cambiamos buffers
			glFlush();
			glfwSwapBuffers(window);
		}

		//Desactivar y eliminar programa
		glUseProgram(0);
		glDeleteProgram(compiledPrograms[0]);

	}
	else {
		std::cout << "Ha petao." << std::endl;
		glfwTerminate();
	}

	//Finalizamos GLFW
	glfwTerminate();

}