//
//
//
#include <fbxsdk.h>
#include "OpenGLcore.h"
#include <GLFW/glfw3.h>


// inclus la definition de DWORD
#include <IntSafe.h>

// les repertoires d'includes sont:
// ../libs/glfw-3.3/include			fenetrage
// ../libs/glew-2.1.0/include		extensions OpenGL
// ../libs/stb						gestion des images (entre autre)

// les repertoires des libs sont (en 64-bit):
// ../libs/glfw-3.3/lib-vc2015
// ../libs/glew-2.1.0/lib/Release/x64

// Pensez a copier les dll dans le repertoire x64/Debug, cad:
// glfw-3.3/lib-vc2015/glfw3.dll
// glew-2.1.0/bin/Release/x64/glew32.dll		si pas GLEW_STATIC

// _WIN32 indique un programme Windows
// _MSC_VER indique la version du compilateur VC++
#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "glew32s.lib")			// glew32.lib si pas GLEW_STATIC
#pragma comment(lib, "opengl32.lib")
#elif defined(__APPLE__)
#elif defined(__linux__)
#endif

#include <iostream>
#include <fstream>

FbxManager* m_fbxManager;
FbxScene* m_scene;

#include "../common/GLShader.h"
#include "mat4.h"
#include "Texture.h"
#include "Mesh.h"
#include "FrameBuffer.h"

std::vector<glm::vec3> positions[1];
std::vector<bool> normals[1];
int* verts;

GLuint FBXVAO;	// la structure d'attributs stockee en VRAM
GLuint FBXVBO;	// les vertices de l'objet stockees en VRAM
GLuint FBXIBO;	// les indices de l'objet stockees en VRAM


void AddMesh(FbxNode* node, FbxNode* parent)
{

	int controlPointIndex = 0;

	int nbControlPoint;

	std::cout << "mesh" << std::endl;

	FbxMesh* mesh = node->GetMesh();

	verts = mesh->GetPolygonVertices();

	int nbVertex = 0;
	int nbPoly = 0;
	nbPoly = mesh->GetPolygonCount();
	nbVertex = mesh->GetPolygonVertexCount();
	normals->resize(nbVertex);
	positions->resize(nbVertex);

	for (int i = 0; i < nbPoly; i++) {
		int p = mesh->GetPolygonSize(i);
		for (int j = 0; j < p; j++) {
			FbxVector4 position = mesh->GetControlPointAt(mesh->GetPolygonVertex(i, j));
			positions->emplace_back(glm::vec3((float)position[0], (float)position[1], (float)position[2]));

			FbxVector4 normal;
			normals->push_back(mesh->GetPolygonVertexNormal(i, j, normal));

		}
	}

}

void Terminate()
{
	m_scene->Destroy();
	m_fbxManager->Destroy();
}

void LoadFBX() {
	m_scene = FbxScene::Create(m_fbxManager, "Ma Scene");
	FbxImporter* importer = FbxImporter::Create(m_fbxManager, "");
	bool status = importer->Initialize("../data/ironman/ironman.fbx", -1, m_fbxManager->GetIOSettings());
	status = importer->Import(m_scene);
	importer->Destroy();
}

void ProcessNode(FbxNode* node, FbxNode* parent) {
	//
	// insérer TRAITEMENT DU NODE, cf plus bas
	//

	FbxNodeAttribute* att = node->GetNodeAttribute();

	if (att != NULL)
	{
		FbxNodeAttribute::EType type = att->GetAttributeType();
		switch (type)
		{
		case FbxNodeAttribute::eMesh:
			AddMesh(node, parent);
			break;

		case FbxNodeAttribute::eSkeleton:
			// illustratif, nous traiterons du cas des squelettes 
			// dans une fonction spécifique
			//AddJoint(node, parent);
			break;
			//…
		}
	}




	int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; i++)
	{
		FbxNode* child = node->GetChild(i);
		ProcessNode(child, node);
	}
}

struct Application
{
	Mesh* object;

	GLShader opaqueShader;
	GLShader copyShader;			// remplace glBlitFrameBuffer

	uint32_t MatricesUBO;
	uint32_t MaterialUBO;

	Framebuffer offscreenBuffer;	// rendu hors ecran
	
	// dimensions du back buffer / Fenetre
	int32_t width;
	int32_t height;

	void Initialize()
	{
		GLenum error = glewInit();
		if (error != GLEW_OK) {
			std::cout << "erreur d'initialisation de GLEW!"
				<< std::endl;
		}

		m_fbxManager = FbxManager::Create();
		FbxIOSettings* ioSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
		m_fbxManager->SetIOSettings(ioSettings);

		LoadFBX();

		FbxNode* root_node = m_scene->GetRootNode();

		ProcessNode(root_node, NULL);

		std::cout << "Version : " << glGetString(GL_VERSION) << std::endl;
		std::cout << "Vendor : " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "Renderer : " << glGetString(GL_RENDERER) << std::endl;

		// on utilise un texture manager afin de ne pas recharger une texture deja en memoire
		// de meme on va definir une ou plusieurs textures par defaut
		Texture::SetupManager();

		opaqueShader.LoadVertexShader("opaque.gl3.vs.glsl");
		opaqueShader.LoadFragmentShader("opaque.gl3.fs.glsl");
		opaqueShader.Create();
		copyShader.LoadVertexShader("copy.gl3.vs.glsl");
		copyShader.LoadFragmentShader("copy.gl3.fs.glsl");
		copyShader.Create();

		object = new Mesh;

		//Mesh::ParseObj(object, "../data/ironman/ironman.fbx");
		Mesh::ParseObj(object, "../data/lightning/lightning_obj.obj");
		//Mesh::ParseObj(object, "../data/suzanne.obj");

		int32_t program = opaqueShader.GetProgram();
		glUseProgram(program);
		// on connait deja les attributs que l'on doit assigner dans le shader
		int32_t positionLocation = glGetAttribLocation(program, "a_Position");
		int32_t normalLocation = glGetAttribLocation(program, "a_Normal");
		int32_t texcoordsLocation = glGetAttribLocation(program, "a_TexCoords");

		//
		// creation et attribution d'un binding index a chaque ubo
		//

		// notre convention
		const int MatricesBindingIndex = 0;
		const int MaterialBindingIndex = 1;

		glGenBuffers(1, &MaterialUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, MaterialUBO);
		// Le materiau va etre mis a jour plusieurs fois par frame -> DYNAMIC
		glBufferData(GL_UNIFORM_BUFFER, sizeof(Material), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, MaterialBindingIndex, MaterialUBO);
		// malheureusement en OpenGL3 il faut faire les appels suivants pour chaque shader program ...
		int materialBlockIndex = glGetUniformBlockIndex(program, "Materials");
		glUniformBlockBinding(program, materialBlockIndex, MaterialBindingIndex);

		glGenBuffers(1, &MatricesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, MatricesUBO);
		// Les matrices vont etre mis a jour une fois par frame -> STREAM
		glBufferData(GL_UNIFORM_BUFFER, 2*sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, MatricesBindingIndex, MatricesUBO);
		// malheureusement en OpenGL3 il faut faire les appels suivants pour chaque shader program ...
		int matricesBlockIndex = glGetUniformBlockIndex(program, "Matrices");
		glUniformBlockBinding(program, matricesBlockIndex, MatricesBindingIndex);

		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		//
		// on cree un VAO par submesh auquel on rattache un VBO et un IBO
		// Avant OpenGL4. il n'est pas possible de decoupler la signature d'un shader (VAO)
		// des donnees qui lui sont rattachees (VBO), c'est pour cette raison qu'il nous
		// faut ici creer plusieurs fois un VAO aux proprietes similaires
		//
		/*for (uint32_t i = 0; i < object->meshCount; i++)
		{
			SubMesh& mesh = object->meshes[i];

			glGenVertexArrays(1, &mesh.VAO);
			glBindVertexArray(mesh.VAO);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IBO);

			glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
			// Specifie la structure des donnees envoyees au GPU
			glVertexAttribPointer(positionLocation, 3, GL_FLOAT, false, sizeof(Vertex), 0);
			glVertexAttribPointer(normalLocation, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
			glVertexAttribPointer(texcoordsLocation, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));
			// indique que les donnees sont sous forme de tableau
			glEnableVertexAttribArray(positionLocation);
			glEnableVertexAttribArray(normalLocation);
			glEnableVertexAttribArray(texcoordsLocation);
			
			// ATTENTION, les instructions suivantes ne detruisent pas immediatement les VBO/IBO
			// Ceci parcequ'ils sont référencés par le VAO. Ils ne seront détruit qu'au moment
			// de la destruction du VAO
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			//DeleteBufferObject(mesh.VBO);
			//DeleteBufferObject(mesh.IBO);
		}*/

		glGenVertexArrays(1, &FBXVAO);
		glBindVertexArray(FBXVAO);

		glGenBuffers(1, &FBXVBO);
		glBindBuffer(GL_ARRAY_BUFFER, FBXVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions	, GL_STATIC_DRAW);

		glGenBuffers(1, &FBXIBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FBXIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		glVertexAttribPointer(positionLocation, 3, GL_FLOAT, false, sizeof(Vertex), 0);
		glVertexAttribPointer(normalLocation, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		glVertexAttribPointer(texcoordsLocation, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));
		// indique que les donnees sont sous forme de tableau
		glEnableVertexAttribArray(positionLocation);
		glEnableVertexAttribArray(normalLocation);
		glEnableVertexAttribArray(texcoordsLocation);
		 
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// force TOUS les framebuffers a effectuer une conversion automatique lineaire -> sRGB en ecriture
		glEnable(GL_FRAMEBUFFER_SRGB);

		offscreenBuffer.CreateFramebuffer(width, height, true);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);
	}

	void Shutdown()
	{
		glUseProgram(0);
		glBindVertexArray(0);

		glDeleteBuffers(1, &MaterialUBO);
		glDeleteBuffers(1, &MatricesUBO);

		object->Destroy();
		delete object;

		// On n'oublie pas de détruire les objets OpenGL

		Texture::PurgeTextures();

		copyShader.Destroy();
		opaqueShader.Destroy();
	}

	// la scene est rendue hors ecran
	void RenderOffscreen()
	{
		offscreenBuffer.EnableRender();

		glClearColor(0.973f, 0.514f, 0.475f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Defini le viewport en pleine fenetre
		glViewport(0, 0, width, height);

		// En 3D il est usuel d'activer le depth test pour trier les faces, et cacher les faces arrières
		// Par défaut OpenGL considère que les faces anti-horaires sont visibles (Counter Clockwise, CCW)
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		uint32_t program = opaqueShader.GetProgram();
		glUseProgram(program);

		// ces matrices sont communes à tous les SubMesh
		glm::mat4 worldMatrix, viewMatrix, projectionMatrix;
		worldMatrix = glm::mat4(1.0f);
		glm::mat4 rotateCam(1.f);
		rotateCam = glm::rotate(rotateCam, (float)glfwGetTime(), glm::vec3(0.f, 1.f, 0.f));
		worldMatrix = rotateCam;
		glm::vec3 position = { 0.f, 0.f, 200.f };
		viewMatrix = glm::lookAt(position, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));// *rotateCam;
		projectionMatrix = glm::perspective(glm::radians(45.f), (float)width / (float)height, 0.1f, 1000.f);

		int32_t worldLocation = glGetUniformLocation(program, "u_WorldMatrix");
		glUniformMatrix4fv(worldLocation, 1, false, glm::value_ptr(worldMatrix));
		
		// position de la camera
		int32_t camPosLocation = glGetUniformLocation(program, "u_CameraPosition");
		// calcul de la position de la camera, a partir de la view matrix
		// 1. il faut se rappeler, que la view matrix en  fait une matrice inverse
		// 2. la position de la camera se trouve dans la 4eme colonne de la matrice...
		// 3. ...mais cette position est également une position "inverse"
		//
		// technique A : inverse + extraction de la colonne qui nous interesse
		glm::mat4 cameraTransformMatrix = glm::inverse(viewMatrix);
		position = glm::vec3(cameraTransformMatrix[3]);		
		// technique B : pseudo-inverse
		// extraire la sous-matrice (mineure) 3x3 car pour une matrice orthogonale
		// M^-1 = M^T
		glm::mat3 minor = glm::transpose(glm::mat3(viewMatrix));
		// pour un translation T(v), T^-1(v) = T(-v)
		position = -glm::vec3(viewMatrix[3]);
		position = minor * position;

		glUniform3fv(camPosLocation, 1, &position.x);

		glBindBuffer(GL_UNIFORM_BUFFER, MatricesUBO);
		// en OpenGL moderne on prefere glMapBufferRange a glMapBuffer, cela dit les deux fonctionnent
		// glMapBuffer(GL_UNIFORM_BUFFER, 0, GL_WRITE_ONLY);
		void* mappedBuffer = glMapBufferRange(GL_UNIFORM_BUFFER, 0, 2*sizeof(glm::mat4), GL_MAP_WRITE_BIT);
		glm::mat4* matrixArray = reinterpret_cast<glm::mat4*>(mappedBuffer);
		matrixArray[0] = viewMatrix;
		matrixArray[1] = projectionMatrix;
		// ne pas oublier de "unmap" le buffer lorsqu'il n'est plus utile
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		// On va maintenant affecter les valeurs du matériau à chaque SubMesh
		// On peut éventuellement optimiser cette boucle en triant les SubMesh par materialID
		// On ne devra modifier les uniformes que lorsque le materialID change

		// alternativement on pourrait creer un tableau de Material dans l'UBO du shader
		glBindBuffer(GL_UNIFORM_BUFFER, MaterialUBO);		

		for (uint32_t i = 0; i < object->meshCount; i++)
		{
			SubMesh& submesh = object->meshes[i];
			Material& mat = submesh.materialId > -1 ? object->materials[submesh.materialId] : Material::defaultMaterial;
			
			void* mappedBuffer = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(Material), GL_MAP_WRITE_BIT);
			Material* ubo = reinterpret_cast<Material*>(mappedBuffer);
			*ubo = mat;
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			// glActiveTexture() n'est pas strictement requis ici car nous n'avons qu'une texture à la fois
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);

			// bind implicitement les VBO et IBO rattaches, ainsi que les definitions d'attributs
			glBindVertexArray(submesh.VAO);
			// dessine les triangles

			if (submesh.indicesCount)
				glDrawElements(GL_TRIANGLES, submesh.indicesCount, GL_UNSIGNED_INT, 0);
			else
				glDrawArrays(GL_TRIANGLES, 0, submesh.verticesCount);
		}		
	}

	void Render()
	{
		//
		// Rendu de la scene 3D hors ecran (via un FBO)
		//

		RenderOffscreen();

		//
		// desactive le test de profondeur (passage en 2D)
		//

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		Framebuffer::RenderToBackBuffer(width, height);

		// on va maintenant dessiner un quadrilatere plein ecran
		// pour copier (sampler et inscrire dans le backbuffer) le color buffer du FBO

		uint32_t program = copyShader.GetProgram();
		glUseProgram(program);
		
		// on indique au shader que l'on va bind la texture sur le sampler 0 (TEXTURE0)
		// pas necessaire techniquement car c'est le sampler par defaut
		int32_t samplerLocation = glGetUniformLocation(program, "u_Texture");
		glUniform1i(samplerLocation, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, offscreenBuffer.colorBuffer);

		// Pas de VAO ni VBO ici, la definition des points est en dur dans le shader
		glBindVertexArray(0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	void Resize(int w, int h)
	{
		if (width != w || height != h)
		{
			width = w;
			height = h;
			// le plus simple est de detruire et recreer tout
			offscreenBuffer.DestroyFramebuffer();
			offscreenBuffer.CreateFramebuffer(width, height, true);
		}
	}
};


void ResizeCallback(GLFWwindow* window, int w, int h)
{
	Application* app = (Application *)glfwGetWindowUserPointer(window);
	app->Resize(w, h);
}

void MouseCallback(GLFWwindow* , int, int, int) {

}


static void error_callback(int error, const char* description)
{
	std::cout << "Error GFLW " << error << " : " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(int argc, const char* argv[])
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(960, 720, "Projet FBX", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	Application app;

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Passe l'adresse de notre application a la fenetre
	glfwSetWindowUserPointer(window, &app);

	// inputs
	glfwSetMouseButtonCallback(window, MouseCallback);

	// recuperation de la taille de la fenetre
	glfwGetWindowSize(window, &app.width, &app.height);

	// definition de la fonction a appeler lorsque la fenetre est redimensionnee
	// c'est necessaire afin de redimensionner egalement notre FBO
	glfwSetWindowSizeCallback(window, &ResizeCallback);

	// toutes nos initialisations vont ici
	app.Initialize();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glfwGetWindowSize(window, &app.width, &app.height);

		app.Render();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	// ne pas oublier de liberer la memoire etc...
	app.Shutdown();

	glfwTerminate();
	return 0;
}