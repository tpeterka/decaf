// $Id: AtomAndBondGLSL.h 18 2011-10-16 22:37:02Z baaden $
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

#ifndef ATOMANDBONDGLSL_H_
#define ATOMANDBONDGLSL_H_
#include <GL/glew.h>
#ifdef __APPLE__
	// MB use MacOSX GLUT framework
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
	#include <GL/freeglut_ext.h>
#endif
#include <string>
#include <AtomAndBondShaders.h>

// MB: preparation for including shaders as header files
//#include "stickimproved_frag.h"
//#include "stickimproved_vert.h"
//#include "ballimproved_frag.h"
//#include "ballimproved_vert.h"

// This class manage the GLSL shaders to represent molecules with hyperballs
class AtomAndBondGLSL : public AtomAndBondShaders
{

protected:
	// GLSL variables
	GLuint vertex_shader, fragment_shader, program;
	GLint Atp,Atc,Atsz,Atsc;
	GLuint Bvertex_shader, Bfragment_shader, Bprogram;
	GLint Btp,Btc,Btsz,Btsc,Btsh;
	GLuint Dvertex_shader, Dprogram;
	GLuint Ivertex_shader, Ifragment_shader, Iprogram;
	GLuint framebufferID[2];                // FBO names
	GLuint renderbufferID;                  // renderbuffer object name
	GLint maxRenderbufferSize;              // maximum allowed size for FBO renderbuffer
	GLfloat texCoordOffsets[18];	
	GLint windowWidth, windowHeight, textureWidth, textureHeight;
	GLint maxDrawBuffers;                   // maximum number of drawbuffers supported
	GLuint renderTextureID[2];              // 1 in 1st pass and 1 in 2nd pass

    bool _shaderActivated;
    int _atomsToDraw;

public:
	AtomAndBondGLSL();
	virtual ~AtomAndBondGLSL();
	
	// To read shader files (GLSL)
	char *LoadSource(const char *fn);
	GLuint LoadShader(GLenum type, const char *filename);
	// MB: preparation for including shaders as header files
	//	GLuint LoadShader2(GLenum type, const char *stringdef);
	
	void initGLSL();
	virtual void initGL();
	
	// draw the atoms
    virtual void drawAtoms(int nbAtomsToShow);
	// draw the bonds
    virtual void drawBonds();
	
    void activateShader();
    void setAtomsToDraw(int atomsToDraw);

};

#endif
