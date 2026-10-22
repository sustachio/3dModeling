#pragma once

#ifndef RENDERING_QUAD
#define RENDERING_QUAD

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

class RenderingQuad
{
public:
	unsigned int VBO, VAO, EBO;

	RenderingQuad()
	{
		// vertex buffer
		float vertices[] = {
			// positions          // texture cordinates
			 1.0f,  1.0f, 0.0f,   1.0,  1.0, // top right
			 1.0f, -1.0f, 0.0f,   1.0,  0.0, // bottom right
			-1.0f, -1.0f, 0.0f,   0.0,  0.0, // bottom left
			-1.0f,  1.0f, 0.0f,   0.0,  1.0 // top left 
		};
		unsigned int indices[] = {  
			0, 1, 3,   // first triangle
			1, 2, 3    // second triangle
		};

		// vertex array object
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		// vertex buffer object
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// element buffer object
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// layouts
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0); 
		glBindVertexArray(0); 
	};
};

#endif
