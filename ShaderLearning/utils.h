#pragma once

#include <iostream>
#include <vector>
#include <variant>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "objectInputs.h"

/*
float old[] = {
	Operator, Union,
	Object, Point_End, Plane, Color, 0, 1, 1, SDF_End,
	Object, Position, -2, 1, 15, Point_End, Sphere, Color, 1, 0, 0, SDF_End,
	Object, Position, 0, 1, 15, Point_End, Sphere, Color, 0, 1, 0, SDF_End,
	Object, Position, 2, 1, 15, Point_End, Sphere, Color, 0, 0, 1, SDF_End,
	Operator, Down,
};
*/

void imgui_active_object_buttons(item*& activeObject, item* rootGroup)
{
	group* node = &rootGroup->getGroup();
	std::string id = std::string("##") + std::to_string(rootGroup->id);

	//ImGui::Text(tree.name.c_str());
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0,.5,0,1});
	if (ImGui::Button((rootGroup->name + id).c_str()))
		activeObject = rootGroup;
	ImGui::PopStyleColor(1);

	ImGui::Indent();
	for (item* child : node->children)
	{
		std::string id = std::string("##") + std::to_string(child->id);
		if (child->isObject())
		{
			object obj = child->getObject();
			if (ImGui::Button((child->name + id).c_str()))
				activeObject = child;
		}
		else if (child->isGroup())
		{
			imgui_active_object_buttons(activeObject, child);
		}
	}
	ImGui::Unindent();
}

void util_glfw_init()
{
	if (!glfwInit())
	{
		std::cout << "Failed to initizized glfw";
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // needed for mac
}

void util_create_glfw_window(glm::ivec2 screenSize, const char* name, GLFWwindow** window)
{
	*window = glfwCreateWindow(screenSize.x, screenSize.y, name, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(*window);
}

void util_glad_setup()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(-1);
	}    
}

void util_imgui_setup(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void util_create_triangles(unsigned int* VAO,
								   unsigned int* VBO,
								   unsigned int* EBO)
{
	//unsigned int VAO, VBO, EBO;

	// vertex buffer
	float vertices[] = {
		 1.0f,  1.0f, 0.0f,  // top right
		 1.0f, -1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f,  // bottom left
		-1.0f,  1.0f, 0.0f   // top left 
	};
	unsigned int indices[] = {  
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	// vertex array object
	glGenVertexArrays(1, VAO);
	glBindVertexArray(*VAO);

	// vertex buffer object
	glGenBuffers(1, VBO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// element buffer object
	glGenBuffers(1, EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindVertexArray(0); 
}

///////////////// FRAME BUFFERS FROM https://www.codingwiththomas.com/blog/rendering-an-opengl-framebuffer-into-a-dear-imgui-window /////////

void util_create_framebuffer(glm::uvec2 size,
							 unsigned int* FBO, 
						     unsigned int* RBO,
							 unsigned int* textureID)
{
	glGenFramebuffers(1, FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

	glGenTextures(1, textureID);
	glBindTexture(GL_TEXTURE_2D, *textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *textureID, 0);

	glGenRenderbuffers(1, RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, *RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *RBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void util_rescale_framebuffer(glm::uvec2 size,
						     unsigned int* RBO,
							 unsigned int* textureID)
{
	glBindTexture(GL_TEXTURE_2D, *textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *textureID, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, *RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *RBO);
}
