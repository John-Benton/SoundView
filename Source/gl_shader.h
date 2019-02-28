#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:

	//The structure and functions of this class are based on that given by Joey de Vries in "LearnOpenGL"

	/*====================================================================================================
	
	Step one is to load the shaders from file and place them in c-strings so they can be compiled
	
	====================================================================================================*/

	unsigned int ID; //the ID of the shader program

	Shader(const GLchar* vertexPath, const GLchar* fragmentPath) { //this constructor reads and builds the shader
	
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit); //allows ifstream to throw exception
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream; 
			
			//read file buffers into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();

			//close file handles
			vShaderFile.close();
			fShaderFile.close();

			//convert streams into strings
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}

		catch (std::ifstream::failure e)
		{
			OutputDebugStringA("ERROR::SHADER::FILE_NOT_READ_SUCCESFULLY");
		}
		//put the shader code into c-strings
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		/*====================================================================================================

		Step two is compile and link the shaders together into a shader program

		====================================================================================================*/

		unsigned int vertex, fragment;
		int success;
		char infoLog[512];

		//create vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success) 
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			LPCSTR str(infoLog);
			OutputDebugStringA("ERROR:SHADER::VERTEX::COMPILATION_FAILED");
			OutputDebugStringA(str);
		}

		//create fragment shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			LPCSTR str(infoLog);
			OutputDebugStringA("ERROR:SHADER::FRAGMENT::COMPILATION_FAILED");
			OutputDebugStringA(str);
		}

		//create shader program

		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);

		//check for linking errors
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			LPCSTR str(infoLog);
			OutputDebugStringA("ERROR:SHADER::PROGRAM::LINKING_FAILED");
			OutputDebugStringA(str);
		}

		//delete original shaders as they've now be linked into a program.
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void use() { //this activates the shader for use
		glUseProgram(ID);
	};

	//the following three functions are utility functions
	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	};
	void setInt(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	};
	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	};

};