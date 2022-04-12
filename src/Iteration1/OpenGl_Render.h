#pragma once
#include<iostream>
#include"../src/Octree/Octree.h"
#include"Octree_Iteration1.h"

using namespace std;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 900;

// camera
Mycamera camera = Mycamera(glm::vec3(30.0f, 30.0f, 80.0f));
//Mycamera camera = Mycamera(glm::vec3(70.0f, 50.0f, 100.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos = glm::vec3(12.0f, 10.0f, 20.0f);



void OpenGL_Render(float length, float wild, float heigth, const char* PathFilename)
{
	GLFWwindow* window;
	//Octreecut.Visualizer(Octreecut.C, i,Octreecut.GetCutterBoundBox(Octreecut.C));
	// glfw: initialize and configure
// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	// glad: load all OpenGL function pointers
// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	Octree Head(Point3f(length, wild, heigth), Point3f(length, wild, heigth));//���ù�����λ����ߴ�
	//Octree Head(Point3f(30, 30, 30), Point3f(20, 20, 20));
	//30,30,30,20,20,20
	camera.Right += 100;//��������ӽ�
	//OctreeCut_2 Octreecut(Head, Point3f(100, 50, 20), Point3f(8, 2, 2));//���õ��ߵ�λ����ߴ磻
	OctreeCut_Iteration1 Octreecut(Head, Point3f(30, 30, 50), Point3f(50, 3, 3));
	Octreecut.ReadPathFile(PathFilename);// PathFilename"./res/temp.path"
	//Octreecut.Iteration = 5;
	int itera = 5;
	//Octreecut.Cutter(Octreecut.C, i);



	//BoundBox CutterBox = Octreecut.GetCutterBoundBox(Octreecut.C);
	//Octreecut.Loop(Octreecut.C, itera, CutterBox);


	GLuint VBO[3], VAO[3], EBO;

	glGenVertexArrays(3, VAO);
	glGenBuffers(3, VBO);
	glGenBuffers(1, &EBO);

	cout << "��ʼ��" << endl;

	Shader ourShader("./res/Octree_DrawPoints_Itera2.vs", "./res/Octree_DrawPoints.fs", "./res/Octree_DrawPoints.gs");//������ɫ��
	//Shader ourShader("./res/Octree_OpenGL_Tesseion.vs", "./res/Octree_OpenGL.fs", "./res/Octree_OpenGL_Tesseion.gs");//������ɫ��

	float  T = 1000;
	float delta_t = 2;
	float t = 0;
	int PathIndex = 0;
	//t += delta_t;
	Point3f CutterPos = Octreecut.GetCutterPos(T, t);

	//Point3f CutterPos = Octreecut.GetFileCutterPos(0);

	CudaOctree  cudaoctree = AllocMemoryForCudaOctree(Head.origin, Head.halfDimension);
	BoundBox CutterBox = Octreecut.GetCutterBoundBox(CutterPos);
	//PrepareCut(cudaoctree, CutterPos, Octreecut.CutterSize, CutterBox);
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		//CutterPos = Octreecut.GetCutterStarPos(T, t);
		CutterPos = Octreecut.GetFileCutterPos(PathIndex);
		PathIndex++;
		t += delta_t;
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;


		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		ourShader.use();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);
		ourShader.setMat4("projectionInverse", glm::inverse(projection));
		ourShader.setMat4("viewInverse", glm::inverse(view));
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, lightPos); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	// it's a bit too big for our scene, so scale it down
		ourShader.setMat4("model", model);

		//���ù��ղ���
		//ourShader.setVec3("lightPos", glm::vec3(60.5f, 60.5f, 30.5f));
		ourShader.setVec3("HalfDimension", glm::vec3(Octreecut.Head->halfDimension.x, Octreecut.Head->halfDimension.y, Octreecut.Head->halfDimension.z));
		ourShader.setVec3("lightPos", glm::vec3(-80.0f, -80.0f, -80.0f));
		ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
		ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));


		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//glUseProgram(ourShader.ID);

		CutterBox = Octreecut.GetCutterBoundBox(CutterPos);

		Octreecut.Loop(CutterPos, itera, CutterBox);
		//if (((int)t % 10) == 0) {
		//Octreecut.AddPointsForBrick512(Octreecut.Head, itera);
		//}
		//Octreecut.CudaCuter_Dynamic(cudaoctree, CutterPos, Octreecut.CutterSize, CutterBox);
		//Octreecut.CopyMamery(cudaoctree);
		//Octreecut.CudaAddPoints(cudaoctree);
		//PrepareCut(cudaoctree, CutterPos, Octreecut.CutterSize, CutterBox);
		int VoxelSum = 0;
		int VertexSum = 0;
		int BoolSum = 0;
		int TNum = 1;

		if (TNum == 20) {
			//�ؽ�����Ⱦ�б�
			//Octreecut.testVertex.clear();
			//Octreecut.RebuildRenderList(Octreecut.Head,itera);
			cout << "���β������������" << ((BoolSum += Octreecut.BoolNum) / 10) << "   ��ǰ����������" << ((VoxelSum += Octreecut.VoxelNum) / 10)
				<< "   ��ǰ����������" << ((VertexSum += Octreecut.testVertex.size()) / 10) << endl;
			//cout << "���ӵ�����������" << Octreecut.VoxelNum  - PreVoxelNum << "   ���ӵĶ���������" << Octreecut.testVertex.size() - PreVertexNum << endl;

			cout << endl << endl;
			TNum = 0;
			BoolSum = 0;
			VertexSum = 0;
			VoxelSum = 0;
		}
		else {
			BoolSum += Octreecut.BoolNum;
			VertexSum += Octreecut.testVertex.size();
			VoxelSum += Octreecut.VoxelNum;

		}
		/*      glBindVertexArray(VAO[0]);

			   glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);//�󶨶��㻺�����
			   glBufferData(GL_ARRAY_BUFFER, Octreecut.vertices.size() * sizeof(Point3f), &Octreecut.vertices[0], GL_STATIC_DRAW);//������Ҫ�õ��Ķ�������

			   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			   glBufferData(GL_ELEMENT_ARRAY_BUFFER, Octreecut.faces.size() * sizeof(Point3i), &Octreecut.faces[0], GL_STATIC_DRAW);

			   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point3f), (void*)0);//���ö�������ָ��
			   glEnableVertexAttribArray(0);

			   glBindVertexArray(0);

			   glBindVertexArray(VAO[0]);
			   glDrawElements(GL_TRIANGLES, Octreecut.faces.size() * 3 , GL_UNSIGNED_INT, 0);
			   Octreecut.ClearFaces();
	   */


	   //�������
		glBindVertexArray(VAO[2]);//�󶨵�һ���������Id��������󣬶����������˶����������ã�����������õ�һϵ������
		glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);//�󶨻������
		//���û�������Ҫ�洢�Ķ��㼯�����ں������͸�GPU
		glBufferData(GL_ARRAY_BUFFER, Octreecut.testVertex.size() * sizeof(RenderData), &Octreecut.testVertex[0], GL_STATIC_DRAW);
		//���ö���λ�����ԣ�ָ��locationΪ0
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderData), (void*)0);//���ص�һ������
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(RenderData), (void*)(sizeof(Point3f)));//���صľ���
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);


		glBindVertexArray(VAO[2]);
		glDrawArrays(GL_POINTS, 0, Octreecut.testVertex.size());
		//cout << "��������" << Octreecut.testVertex.size();
		//Octreecut.testVertex.clear();

		//cout << "�������" << endl;
		// glBindVertexArray(0); // no need to unbind it every time 

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();


}


void  processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void  framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
