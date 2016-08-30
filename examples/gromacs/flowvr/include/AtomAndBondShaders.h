// $Id: AtomAndBondShaders.h 18 2011-10-16 22:37:02Z baaden $
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
// May 2011                                                           //
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



#ifndef ATOMANDBONDSHADERS_H_
#define ATOMANDBONDSHADERS_H_
#include <GL/glew.h>
#ifdef __APPLE__
	// MB use MacOSX GLUT framework
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
	#include <GL/freeglut_ext.h>
#endif
#include <string>
#include <vector>

#include <vitamins_data.h>

// Identifier of the different buffers  used by the shaders
#define BUF_VERTICE 0
#define BUF_INDICE 1
#define BUF_TCOORD 2
#define BUF_TCOORD0 3
#define BUF_TCOORD1 4
#define BUF_TCOORD2 5
#define BUF_BINDICE 6

// idenfifier of the different textures used by the shaders
#define TEX_POSITIONS 0
#define TEX_COLORS 1
#define TEX_SIZES 2
#define TEX_ASCALES 3
#define TEX_BSCALES 4
#define TEX_SHRINKS 5

// ad hoc stuctures to stores the kinks between atoms
struct LinkAtom{
  int atom1;
  int atom2;
};


// compute the smallest power of 2 strictly greater than nb
unsigned int nextPower2(const unsigned int nb);

// This class manage the shaders to represent molecules with hyperballs
class AtomAndBondShaders {

protected:
	int NB_BUFFERS;
	int NB_TEXTURES;

	// GL variables
	GLuint *buffer_id;
	GLuint *texture_id;

	// All in the name
	LinkAtom *links;

	// variables for textures and geometry
	// Vertices
	GLfloat *vertice;
	// the four texture coordinates
	GLfloat *texturecoord;
	GLfloat *texturecoord0;
	GLfloat *texturecoord1;
	GLfloat *texturecoord2;
	// The indices to draw the boxes
	GLuint *indice;
    std::vector<GLuint> indiceShown;
	// For the color texture
	GLfloat *colors;
	// For the position texture
	GLfloat *positions;
	// For the scale of the atoms texture
	GLfloat *atomScales;
	// For the scale of the bonds texture
	GLfloat *bondScales;
	// For the sizes of the atoms texture
	GLfloat *sizes;
	// For the shrinks of the bonds texture
	GLfloat *shrinks;
	// Parameter passed to the shaders??
	GLfloat normMax;

	std::string atomVertexShaderProgramName,bondVertexShaderProgramName;
	std::string atomFragmentShaderProgramName,bondFragmentShaderProgramName;

	// number of atoms
	int nbAtoms;
	// number of Bonds
	int nbBonds;
    int nbBondsToDisplay;
	// Size of the side of the square textures for the atom properties
	int textureSize;
	// Size of the side of the square textures for the bond properties
	int textureSizeBonds;

	
public:
	AtomAndBondShaders();
	virtual ~AtomAndBondShaders();

	// initialize all the OpenGL context for the shaders
	virtual void initGL();

	//void setCgContext(CGcontext cgContext);
	void setAtomFragmentShaderProgramName(const std::string fragmentShaderProgramName);
	void setAtomVertexShaderProgramName(const std::string vertexShaderProgramName);
	void setBondFragmentShaderProgramName(const std::string fragmentShaderProgramName);
	void setBondVertexShaderProgramName(const std::string vertexShaderProgramName);

	void setNB_BUFFERS(int NB_BUFFERS);
	void setNB_TEXTURES(int NB_TEXTURES);

	// set up the buffers and the textures related to the rendering of the molecule
	void setupBuffersAndTextures(int nbAtoms, int nbBonds, spheretype *dataAtoms,cylindertype* dataBonds);

	void setPositions(GLfloat *positions);
	void setAtomScales(GLfloat *scales);
	void setBondScales(GLfloat *scales);
	void setSizes(GLfloat *sizes);
	void setColors(GLfloat *colors);
	void setShrinks(GLfloat *shrinks);

	// update the position of the atoms
	void updatePositions(idPositionType *positions, int size);
	void updatePositions(GLfloat *positions);
	// update the number of atoms
	void updateNbAtoms(int nbAtoms);
	// update the number of bonds
	void updateNbBonds(int nbBonds);
	// update the colors of atoms colors must contain contiguous rgba values for the nbAtoms atoms
	void updateColors(GLfloat *colors);
	// update the atom scales. scales must contain contiguous values for the nbAtoms atoms
	void updateAtomScales(GLfloat *scales);
	// update the bond scales. scales must contain contiguous values for the nbBonds bonds
	void updateBondScales(GLfloat *scales);
	// update the bond shrinks. shrinks must contain contiguous values for the nbBonds bonds
	void updateShrinks(GLfloat *shrinks);
	// set the atom scale to newScale value for all atoms
	void updateAtomScales(double newScale);
	// set the bond scale to newScale value for all bonds
	void updateBondScales(double newScale);
	// set the bond shrink to newShrink value for all bonds
	void updateShrinks(float newShrink);
	// draw the atoms
	virtual void drawAtoms();
	// draw the bonds
	virtual void drawBonds();
	// initialize the buffers of vertice, indice
	void initVertTCoordIndice();
	// initialize the textures related to the atoms
	void initAtomTextures(spheretype *dataAtoms);
	// initialize the textures related to the bonds
	void initBondTextures(cylindertype *dataBonds);
	void updateSizes(GLfloat *sizes);
	void updateSelection(int *selection, int nbSelectiones, spheretype *dataAtoms);

    void generateIndices(std::vector<int> & atomsToDisplay);
    void resetIndices();
};

#endif /* ATOMANDBONDSHADERS_H_ */
