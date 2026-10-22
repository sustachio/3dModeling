/*
NOTE TO FUTURE ME:

The purpose of this was to extend the raymarcher I built
on ShaderToy into a full 3D renderer.

This was my first time using c++, and I just learned how
pointers work about halfway through this so some of it is
a bit wierd.

Also im using pointers instead of references because they
look cooler
*/

/*
TODO (short-term):
 - Use dockspace
 - Implement object propreties window

TODO (long-term):
 - Implement heichry
*/

#include <iostream>
#include <list>
#include <variant>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "utils.h"
#include "shader.h"
#include "renderingquad.h"
#include "objectInputs.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main()
{
	glm::ivec2 screenSize(1600, 1200);
	GLFWwindow* window;
	unsigned int VAO; // for triangles
	unsigned int sceneFBO, sceneRBO, sceneTexture; // for scene imgui window

	util_glfw_init();
	util_create_glfw_window(screenSize, "Seth's 3D renderer", &window);
	util_glad_setup();
	util_imgui_setup(window);

	// create a viewport
	glViewport(0, 0, screenSize.x, screenSize.y);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback); 

	util_create_triangles(&VAO, new unsigned int, new unsigned int);
	Shader renderer("basic.vert", "3drenderer.frag");
	util_create_framebuffer(glm::uvec2(100,100), &sceneFBO, &sceneRBO, &sceneTexture);

	/////////// BUFFERS ////////

	// shader storage buffer object
	unsigned int SSBO;
	float old[] = {
		Operator, Union,
		Object, Point_End, Plane, Color, 0, 1, 1, SDF_End,
		Object, Position, -2, 1, 15, Point_End, Sphere, Color, 1, 0, 0, SDF_End,
		Object, Position, 0, 1, 15, Point_End, Sphere, Color, 0, 1, 0, SDF_End,
		Object, Position, 2, 1, 15, Point_End, Sphere, Color, 0, 0, 1, SDF_End,
		Operator, Down,
	};
	item scene = item(std::string("Scene"), group(Union, std::vector<item*>{
		new item(std::string("Green"), object(
			Cube,
			std::vector<float>({1., 1., 1.}),
			std::vector<point_parameter>({
				point_parameter(Position, std::vector<float>{0, 1, 15}),
				point_parameter(Rotation, std::vector<float>{0, 0, 0}),
			}),
			std::vector<post_sdf_parameter>({
				post_sdf_parameter(Color, std::vector<float>{0, 1, 0}),
			})
		)),
		new item(std::string("Blue"), object(
			Sphere,
			std::vector<float>({2.}),
			std::vector<point_parameter>({
				point_parameter(Position, std::vector<float>{2, 1, 15}),
				point_parameter(Rotation, std::vector<float>{0, 0, 0}),
			}),
			std::vector<post_sdf_parameter>({
				post_sdf_parameter(Color, std::vector<float>{0, 0, 1}),
			})
		)),
		new item(std::string("Smooth"), group(Smooth_Union, std::vector<item*>{
			new item(std::string("Ground"), object(
				Plane,
				std::vector<float>(),
				std::vector<point_parameter>({
					point_parameter(Position, std::vector<float>{0, 0, 0}),
					point_parameter(Rotation, std::vector<float>{0, 0, 0}),
				}),
				std::vector<post_sdf_parameter>({
					post_sdf_parameter(Color, std::vector<float>{0, 1, 1}),
				})
			)),
			new item(std::string("Red"), object(
				Sphere,
				std::vector<float>({2.}),
				std::vector<point_parameter>({
					point_parameter(Position, std::vector<float>{-2, 1, 15}),
					point_parameter(Rotation, std::vector<float>{0, 0, 0}),
				}),
				std::vector<post_sdf_parameter>({
					post_sdf_parameter(Color, std::vector<float>{1, 0, 0}),
				})
			))
		})),
	}));
	printf(scene.to_string().c_str());

	//childrens.erase(std::find(childrens.begin(), childrens.end(), smooth));
		
	{
		myArray<float> objects = vector_to_array(group_to_float_inputs(&scene));

		glGenBuffers(1, &SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, objects.size, objects.arr, GL_DYNAMIC_READ);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	};

	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);

	///////// STATE ////////

	int renderHeight = 550;
	item* activeObject = nullptr;

	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

	/////////////// RENDER LOOP //////////////////
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	float lastTime = 0;
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();


		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		/////// SCENE WINDOW /////

		ImGui::Begin("My Scene");

		// rescale
		const glm::uvec2 sceneWindowSize(
			ImGui::GetContentRegionAvail().x,
			ImGui::GetContentRegionAvail().y);
		const float aspectRatio = (float)sceneWindowSize.y / (float)sceneWindowSize.x;
		const glm::uvec2 renderWindowSize(renderHeight / aspectRatio, renderHeight);

		util_rescale_framebuffer(renderWindowSize, &sceneRBO, &sceneTexture);
		glViewport(0, 0, renderWindowSize.x, renderWindowSize.y);


		ImGui::Image(
			(void*)sceneTexture,
			ImVec2(sceneWindowSize.x, sceneWindowSize.y),
			ImVec2(0, 1),
			ImVec2(1, 0)
		);

		ImGui::End();

		//////// DEBUG WINDOW //////

		ImGui::Begin("Debug");

		double fps = 1.0 / (glfwGetTime() - lastTime);
		lastTime = glfwGetTime();
		ImGui::Text("FPS: %.0f", fps);

		std::string error = renderer.glslCheckError();
		ImGui::Text("Errors:");
		ImGui::BulletText(error.c_str());

		ImGui::SliderInt("Render Height", &renderHeight, 0, sceneWindowSize.y);

		ImGui::End();

		ImGui::Begin("Propreties");

		//////// HEIRARCHY //////

		ImGui::SeparatorText("Hiechry");

		imgui_active_object_buttons(activeObject, &scene); // heirarchy items

		// change name
		if (activeObject != nullptr) // check again incase deleted
		{
			ImGui::SeparatorText("Hiechry Object Options");

			ImGui::InputText("Name", &activeObject->name);
		}

		// delete button (for all but root)
		if (activeObject != nullptr && activeObject->parent != nullptr)
		{
			{
				std::vector<item*>& siblings = activeObject->parent->getGroup().children;
				if (ImGui::Button("Delete"))
				{
					// find object in vector
					int index = std::find(siblings.begin(), siblings.end(), activeObject) - siblings.begin();
					
					// remove from vector
					activeObject->freeMemory();
					siblings.erase(siblings.begin() + index);

					activeObject = nullptr;
				}
			}
		}

		// duplicate object
		if (activeObject != nullptr && activeObject->isObject())
		{
			if (ImGui::Button("Duplicate"))
			{
				std::vector<item*>& siblings = activeObject->parent->getGroup().children;
				item* duplicate = new item(*activeObject);
				duplicate->id = items++;

				siblings.push_back(duplicate);
				activeObject = duplicate;
			}
		}

		ImGui::NewLine();

		// add children to group
		if (activeObject != nullptr && activeObject->isGroup())
		{
			if (ImGui::Button("Add Child Object"))
			{
				item* newobj = new item(std::string("New Object"), object(
					Sphere,
					std::vector<float>({ 1 }),
					std::vector<point_parameter>({
						point_parameter(Position, std::vector<float>{0, 0, 10}),
						point_parameter(Rotation, std::vector<float>{0, 0, 0}),
					}),
					std::vector<post_sdf_parameter>({
						post_sdf_parameter(Color, std::vector<float>{1, 0, 0}),
					})));
				newobj->parent = activeObject;

				activeObject->getGroup().children.push_back(newobj);

				activeObject = newobj;
			}

			if (ImGui::Button("Add Child Group"))
			{
				item* newobj = new item(std::string("New Group"), group(
					Union,
					std::vector<item*>{}));
				newobj->parent = activeObject;

				activeObject->getGroup().children.push_back(newobj);

				activeObject = newobj;
			}
		}


		//////////////// PROPRETIES //////////

		// if an object is selected
		if (activeObject != nullptr && activeObject->isObject()) // check again incase deleted
		{
			object* obj = &activeObject->getObject();

			ImGui::SeparatorText("Properties");


			/////// POINT PARAMS

			ImGui::Text("Point Parameters");
			float* pos = &obj->pointParameters.at(0).args.at(0);
			ImGui::DragFloat3("position", pos, .01, 10);

			float* rot = &obj->pointParameters.at(1).args.at(0);
			ImGui::DragFloat3("rotation", rot, .1, 10);

			ImGui::NewLine();

			//////// POST SDF PARAMS

			ImGui::Text("Post SDF Parameters");
			float* col = &obj->postSDFParameters.at(0).args.at(0);
			ImGui::SliderFloat3("color", col, 0, 1);

			ImGui::NewLine();

			/////// SDF
			ImGui::Text("SDF");

			int* sdf = &obj->sdf;
			ImGui::SliderInt("sdf", sdf, 0, SDF_TYPES_COUNT-1);
			ImGui::BulletText("0 - plane");
			ImGui::BulletText("1 - cube");
			ImGui::BulletText("2 - shpere");

			ImGui::NewLine();

			/////// SDF PARAMS
			ImGui::Text("SDF Parameters");

			if (obj->sdf == Sphere)
			{
				obj->sdfParameters.resize(1, 1);
				float* radius = &obj->sdfParameters.at(0);
				ImGui::DragFloat("radius", radius, .1);
			}

			else if (obj->sdf == Cube)
			{
				obj->sdfParameters.resize(3, 1);
				float* bounds = &obj->sdfParameters.at(0);
				ImGui::DragFloat3("bounds", bounds, .1);
			}

			else if (obj->sdf == Plane)
			{
				obj->sdfParameters.resize(0);
			}
		}

		// change group's operator
		if (activeObject != nullptr && activeObject->isGroup())
		{
			ImGui::SeparatorText("Operator Propreties");

			int* op = &activeObject->getGroup().operator_;
			ImGui::SliderInt("Operator", op, 1, UNION_OP_COUNT-1); // start at 1 to avoid down
			ImGui::BulletText("1 - Union");
			ImGui::BulletText("2 - Smooth Union");
			ImGui::BulletText("3 - Intersection");
			ImGui::BulletText("4 - Difference (subtract all from first)");
		}

		if (activeObject == nullptr)
			ImGui::Text("Click an object or group to view its propreties");

		ImGui::End();


        ImGui::Render();

		/////// SCENE RENDERING //////

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		myArray<float> objects = vector_to_array(group_to_float_inputs(&scene));
		glBufferData(GL_SHADER_STORAGE_BUFFER, objects.size, objects.arr, GL_DYNAMIC_READ);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
			glUseProgram(renderer.ID);

			glfwGetWindowSize(window, &screenSize.x, &screenSize.y);
			glUniform2f(glGetUniformLocation(renderer.ID, "uResolution"), renderWindowSize.x, renderWindowSize.y);
			glUniform1f(glGetUniformLocation(renderer.ID, "uTime"), glfwGetTime());

			glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			glUseProgram(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//////////////////

		glfwGetWindowSize(window, &screenSize.x, &screenSize.y);


        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window); // swap color buffer

		glfwPollEvents();    
	}

	glfwTerminate();
	return 0;
}

/////////////// SETUP TOOLS ////////////////

// used to resize viewport when screen is resized
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}  

/////////////// MAIN LOOP ////////////

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

	/*
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		blockPos += glm::vec3(-0.1f, 0.0f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		blockPos += glm::vec3(0.1f, 0.0f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		blockPos += glm::vec3(0.0f, 0.1f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		blockPos += glm::vec3(0.0f, -0.1f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		blockPos += glm::vec3(0.0f, 0.0f, 0.1f);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		blockPos += glm::vec3(0.0f, 0.0f, -0.1f);
		*/
}

		//glUniform3f(glGetUniformLocation(shaderProgram.ID, "blockPos"), blockPos.x, blockPos.y, blockPos.z);
		//float data[] = { 1, blockPos.x, blockPos.y, blockPos.z, 1, 0, 0, 0 };
		//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 8*sizeof(float), 8*sizeof(float), data);
