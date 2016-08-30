// $Id: AtomAndBondGLSL.cpp 18 2011-10-16 22:37:02Z baaden $
// ****************************************************************** //
//                                                                    //
// Copyright (C) 2010-2011 by                                         //
// Laboratoire de Biochimie Theorique (CNRS),                         //
// Laboratoire d'Informatique Fondamentale d'Orleans (Universite      //
// d'Orleans),                                                        //
// (INRIA) and                                                        //
// Departement des Sciences de la Simulation et de l'Information      //
// (CEA).                                                             //
// ALL RIGHTS RESERVED.                                               //
//                                                                    //
// contributors :                                                     //
// Matthieu Chavent,                                                  //
// Antoine Vanel,                                                     //
// Alex Tek,                                                          //
// Marc Piuzzi,                                                       //
// Jean-Denis Lesage,                                                 //
// Bruno Levy,                                                        //
// Sophie Robert,                                                     //
// Sebastien Limet,                                                   //
// Bruno Raffin and                                                   //
// Marc Baaden                                                        //
//                                                                    //
// October 2011                                                       //
//                                                                    //
// Contact: Marc Baaden                                               //
// E-mail: baaden@smplinux.de                                         //
// Webpage: http://hyperballs.sourceforge.net                         //
//                                                                    //
// This software is a computer program whose purpose is to visualize  //
// molecular structures. The source code is part of FlowVRNano, a     //
// general purpose library and toolbox for interactive simulations.   //
//                                                                    //
// This software is governed by the CeCILL-C license under French law //
// and abiding by the rules of distribution of free software. You can //
// use, modify and/or redistribute the software under the terms of    //
// the CeCILL-C license as circulated by CEA, CNRS and INRIA at the   //
// following URL "http://www.cecill.info".                            //
//                                                                    //
// As a counterpart to the access to the source code and  rights to   //
// copy, modify and redistribute granted by the license, users are    //
// provided only with a limited warranty and the software's author,   //
// the holder of the economic rights, and the successive licensors    //
// have only limited liability.                                       // 
//                                                                    //
// In this respect, the user's attention is drawn to the risks        //
// associated with loading, using, modifying and/or developing or     //
// reproducing the software by the user in light of its specific      //
// status of free software, that may mean  that it is complicated to  //
// manipulate, and that also therefore means that it is reserved for  //
// developers and experienced professionals having in-depth computer  //
// knowledge. Users are therefore encouraged to load and test the     //
// software's suitability as regards their requirements in conditions //
// enabling the security of their systems and/or data to be ensured   //
// and, more generally, to use and operate it in the same conditions  //
// as regards security.                                               //
//                                                                    //
// The fact that you are presently reading this means that you have   //
// had knowledge of the CeCILL-C license and that you accept its      //
// terms.                                                             //
// ****************************************************************** //

#include <cstdio>
#include <AtomAndBondGLSL.h>
#include <math.h>
#include <memory.h>
#include <iostream>

using namespace std;

/******************************************************************************/
//Constructor
AtomAndBondGLSL::AtomAndBondGLSL() : AtomAndBondShaders(){

}

//Destructor
AtomAndBondGLSL::~AtomAndBondGLSL() {

}



/******************************************************************************/
//OpenGl - GLSL drawing functions

void AtomAndBondGLSL::drawAtoms(int nbAtomsToShow)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	
	//Buffer binding  
	glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_VERTICE]);
	glVertexPointer(3,GL_FLOAT,0,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_INDICE]);
	
	//Indices in the texture of atoms
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD]);
	glTexCoordPointer(2,GL_FLOAT,0,0);
	
	//Load Shader 
	glUseProgram(program);
	
	//Handles for textures
	Atp = glGetUniformLocation(program,"texturePosition");
	Atc = glGetUniformLocation(program,"textureColors");
	Atsz = glGetUniformLocation(program,"textureSizes");
	Atsc = glGetUniformLocation(program,"textureScale");
	
  	//Bindings between textures handles and textures ids
	glUniform1i(Atp,TEX_POSITIONS);
	glUniform1i(Atc,TEX_COLORS);
	glUniform1i(Atsz,TEX_SIZES);
	glUniform1i(Atsc,TEX_ASCALES);
	
	//Draws the atoms
        glDrawElements(GL_TRIANGLES,nbAtomsToShow*12*3,
				   GL_UNSIGNED_INT,0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,0);

	if (vertex_shader) glDetachShader(program, vertex_shader);
	if (fragment_shader) glDetachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	glUseProgram(0);	
}

void AtomAndBondGLSL::drawBonds()
{
	//Activates use of Vertex and Texture_Coord
	glEnableClientState(GL_VERTEX_ARRAY);
	
  	//Buffer binding for vertices
	glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_VERTICE]);
	glVertexPointer(3,GL_FLOAT,0,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_BINDICE]);
	
	//Indices in the texture of atom 1 for each bond
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD0]);
	glTexCoordPointer(2,GL_FLOAT,0,0);
	
	//Indices in the texture of atom 2 for each bond
	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD1]);
	glTexCoordPointer(2,GL_FLOAT,0,0);
	
	//Indices in the texture of bonds
	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD2]);
	glTexCoordPointer(2,GL_FLOAT,0,0);
	
	//Load shader program
	glUseProgram(Bprogram);
	
 	//Handles for textures
	Btsh = glGetUniformLocation(Bprogram,"textureShrink");
	Btp = glGetUniformLocation(Bprogram,"texturePosition");
	Btc = glGetUniformLocation(Bprogram,"textureColors");
	Btsz = glGetUniformLocation(Bprogram,"textureSizes");
	Btsc = glGetUniformLocation(Bprogram,"textureScale");
	
	//Bindings between textures handles and textures ids
	glUniform1i(Btsh,5);
	glUniform1i(Btp,0);
	glUniform1i(Btc,1);
	glUniform1i(Btsz,2);
	glUniform1i(Btsc,4);
	
	//Drawing of bonds
    	//std::cout << "On affiche " << nbBondsToDisplay << " liens " << std::endl;
    	glDrawElements(GL_TRIANGLES,nbBondsToDisplay*12*3,
				   GL_UNSIGNED_INT,0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER,0);

	if (Bvertex_shader) glDetachShader(Bprogram, Bvertex_shader);
	if (Bfragment_shader) glDetachShader(Bprogram, Bfragment_shader);
	glDeleteShader(Bvertex_shader);
	glDeleteShader(Bfragment_shader);
	glUseProgram(0);
}

/******************************************************************************/
//Load shader source code
char *AtomAndBondGLSL::LoadSource(const char *fn){
	
	char *src = NULL;   /* shader source code */
	FILE *fp = NULL;    /* file */
	long size;          /* file size */
	long i;             /* counter */

	fp = fopen(fn, "rb");
	if(fp == NULL)
	{
		fprintf(stderr, "Opening error for file '%s'\n", fn);
		return NULL;
	}

	/* File size */
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	rewind(fp);

	/* Memory allocation to store the source code into a string */
	src = new char[size+1];
	if(src == NULL)
	{
		fclose(fp);
		fprintf(stderr, "Memory allocation error!\n");
		return NULL;
	}

	/* Reading file */
	for(i=0; i<size; i++)
		src[i] = fgetc(fp);

	/* last char to \0 */
	src[size] = '\0';

	fclose(fp);

	return src;
	
}

//Read and compile the Shader source code 
GLuint AtomAndBondGLSL::LoadShader(GLenum type, const char *filename)
{
	GLuint shader = 0;
	GLsizei logsize = 0;
	GLint compile_status = GL_TRUE;
	char *log = NULL;
	char *src = NULL;

	shader = glCreateShader(type);
	if(shader == 0)
	{
		fprintf(stderr, "Cannot create shader\n");
		return 0;
	}

	/* Source code loading */
	src = LoadSource(filename);
	if(src == NULL)
	{      
		glDeleteShader(shader);
		return 0;
	}

	glShaderSource(shader, 1, (const GLchar**)&src, NULL);

	/* Compilation */
	glCompileShader(shader);

	/* Memory deallocation */
	free(src);
	src = NULL;

	/* Checking */
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if(compile_status != GL_TRUE)
	{
		/* Compilation error: generation of error log */
		
		/* Size of error message */
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logsize);
		
		/* Memory allocation for the message */
		log = new char[logsize + 1];
		if(log == NULL)
		{
		    fprintf(stderr, "Cannot allocate memory !\n");
		    return 0;
		}
		/* Init */
		memset(log, '\0', logsize + 1);
		
		glGetShaderInfoLog(shader, logsize, &logsize, log);
		fprintf(stderr, "cannot compile the shader '%s' :\n%s",
		        filename, log);
		
		/* Memory deallocation */
		free(log);
		glDeleteShader(shader);
		
		return 0;
	}
	return shader;
}

//OpenGL init for Buffers and Textures and shaders
void AtomAndBondGLSL::initGL(){
	AtomAndBondShaders::initGL();
	initGLSL();
}

//OpengGl init for GLSL
void AtomAndBondGLSL::initGLSL(){

	// Setup Atom GLSL -------------------------------------------------------------------------------------
	vertex_shader = LoadShader(GL_VERTEX_SHADER, atomVertexShaderProgramName.c_str());
	if(vertex_shader == 0)
		fprintf(stderr, "Atoms vertex shader error\n");
	
	fragment_shader = LoadShader(GL_FRAGMENT_SHADER, atomFragmentShaderProgramName.c_str());
	if(fragment_shader == 0)
		fprintf(stderr, "Atoms fragment shader error\n");
	
	program = glCreateProgram();
	if (vertex_shader) glAttachShader(program, vertex_shader);  
	if (fragment_shader) glAttachShader(program, fragment_shader);
	
	glLinkProgram(program);
	
	int isValid;
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &isValid);
	
	if(isValid == GL_FALSE)
	{
		fprintf(stderr, "Invalid atoms shader program\n");
		exit(0);
	}
	
	const unsigned int BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	GLsizei length = 0;
	
	memset(buffer, 0, BUFFER_SIZE);
	glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);
	if (length > 0)
		printf("Link info: %s\n", buffer);

	// Setup Bond GLSL ---------------------------------------------------------------------------------
	Bvertex_shader = LoadShader(GL_VERTEX_SHADER, bondVertexShaderProgramName.c_str());
	if(Bvertex_shader == 0)
		fprintf(stderr, "Bonds vertex shader error\n");
	
	Bfragment_shader = LoadShader(GL_FRAGMENT_SHADER, bondFragmentShaderProgramName.c_str());
	if(Bfragment_shader == 0)
		fprintf(stderr, "Bonds fragment shader error\n");
	
	Bprogram = glCreateProgram();
	if (Bvertex_shader) glAttachShader(Bprogram, Bvertex_shader);  
	if (Bfragment_shader) glAttachShader(Bprogram, Bfragment_shader);
	
	glLinkProgram(Bprogram);
	
	glValidateProgram(Bprogram);
	glGetProgramiv(Bprogram, GL_VALIDATE_STATUS, &isValid);
	
	if(isValid == GL_FALSE)
	{
		fprintf(stderr, "Invalid bonds shader program\n");
		exit(0);
	}
	
	memset(buffer, 0, BUFFER_SIZE);
	length = 0;
	
	memset(buffer, 0, BUFFER_SIZE);
	glGetProgramInfoLog(Bprogram, BUFFER_SIZE, &length, buffer);
	if (length > 0)
		printf("link info bonds: %s\n", buffer);

	printf("GLSL Shaders initialized.\n");
}

