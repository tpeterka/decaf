// $Id$
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

#include <cstdio>
#include <AtomAndBondShaders.h>
#include <math.h>
#include <memory.h>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

//Return power of 2 superior than nb
//Used for setting size of textures
unsigned int nextPower2(const unsigned int nb)
{
    unsigned int power = 2;
    while(power <= nb)
        power = power << 1;
    return power;
}




//Constructor
AtomAndBondShaders::AtomAndBondShaders() {
    // vertex, indices, and 4 for coordinates
    NB_BUFFERS=7;
    // atom position, atom color, atom size, atom scale, bond scale and bond shrink,framebuffer
    NB_TEXTURES=7;

    // GL variables
    buffer_id =new GLuint[NB_BUFFERS];
    texture_id= new GLuint[NB_TEXTURES];
    vertice=NULL;
    texturecoord=NULL;
    texturecoord0=NULL;
    texturecoord1=NULL;
    texturecoord2=NULL;
    indice=NULL;
    colors=NULL;
    positions=NULL;
    atomScales=NULL;
    bondScales=NULL;
    sizes=NULL;
    shrinks=NULL;
    nbAtoms=nbBonds=nbBondsToDisplay=0;
    textureSize=textureSizeBonds=0;
}

//Destructor
AtomAndBondShaders::~AtomAndBondShaders() {
    if(buffer_id) 		delete [] buffer_id;
    if(texture_id) 		delete [] texture_id;
    if(vertice) 		delete [] vertice;
    if(texturecoord) 	delete [] texturecoord;
    if(texturecoord0)	delete [] texturecoord0;
    if(texturecoord1) 	delete [] texturecoord1;
    if(texturecoord2) 	delete [] texturecoord2;
    if(indice)			delete [] indice;
    if(colors) 			delete [] colors;
    if(positions) 		delete [] positions;
    if(atomScales) 		delete [] atomScales;
    if(bondScales) 		delete [] bondScales;
    if(sizes) 			delete [] sizes;
    if(shrinks) 		delete [] shrinks;
}

/******************************************************************************/
//Set parameters

void AtomAndBondShaders::setColors(GLfloat *colors)
{
    memcpy((GLfloat*)this->colors,(GLfloat*) colors,4*nbAtoms*sizeof(GLfloat));
}

void AtomAndBondShaders::setPositions(GLfloat *positions)
{
    this->positions = positions;
}

void AtomAndBondShaders::setAtomScales(GLfloat *scales)
{
    memcpy((GLfloat*)this->atomScales,(GLfloat*) scales,nbAtoms*sizeof(GLfloat));
}

void AtomAndBondShaders::setBondScales(GLfloat *scales)
{
    memcpy((GLfloat*)this->bondScales,(GLfloat*) scales,nbBonds*sizeof(GLfloat));
}


void AtomAndBondShaders::setSizes(GLfloat *sizes)
{
    memcpy((GLfloat*)this->sizes,(GLfloat*) sizes,nbAtoms*sizeof(GLfloat));

}

void AtomAndBondShaders::setShrinks(GLfloat *shrinks)
{
    memcpy((GLfloat*)this->shrinks,(GLfloat*) shrinks,nbAtoms*sizeof(GLfloat));

}

void AtomAndBondShaders::setAtomFragmentShaderProgramName(const string fragmentShaderProgramName)
{
    this->atomFragmentShaderProgramName = fragmentShaderProgramName;
}

void AtomAndBondShaders::setBondFragmentShaderProgramName(const string fragmentShaderProgramName)
{
    this->bondFragmentShaderProgramName = fragmentShaderProgramName;
}

void AtomAndBondShaders::setAtomVertexShaderProgramName(const string vertexShaderProgramName)
{
    this->atomVertexShaderProgramName = vertexShaderProgramName;
}

void AtomAndBondShaders::setBondVertexShaderProgramName(const string vertexShaderProgramName)
{
    this->bondVertexShaderProgramName = vertexShaderProgramName;
}


/******************************************************************************/

//Init atoms parameters
void AtomAndBondShaders::initAtomTextures(spheretype *dataAtoms)
{
    for (int i=0; i<nbAtoms;i++)
    {
        //Decreasing color intensity to improve lightning effect
        colors[4*i]=dataAtoms[i].color.red*.8;
        colors[4*i+1]=dataAtoms[i].color.green*.8;
        colors[4*i+2]=dataAtoms[i].color.blue*.8;
        colors[4*i+3]=dataAtoms[i].color.alpha;

        sizes[i]=dataAtoms[i].radius;
        atomScales[i]=dataAtoms[i].scale;
    }
}

//Init bond parameters
void AtomAndBondShaders::initBondTextures(cylindertype* dataBonds)
{
    for(int i = 0; i < nbBonds; ++i)
    {
        shrinks[i] = dataBonds[i].shrink;
        bondScales[i] = dataBonds[i].scale;
    }
}

//Init vertex positions and storage in textures
void AtomAndBondShaders::initVertTCoordIndice(){
    unsigned pPosTex=0;
    GLfloat *vdata=vertice;
    GLfloat *vtextcoord=texturecoord;
    GLfloat *vtextcoord0=texturecoord0;
    GLfloat *vtextcoord1=texturecoord1;
    GLfloat *vtextcoord2=texturecoord2;
    GLuint *idata=indice;

    unsigned int pPos1, pPos2;
    int i;
    for (i=0;(i<nbBonds)&&(i<nbAtoms); ++i){

        pPos1 = links[i].atom1;
        pPos2 = links[i].atom2;
        pPosTex = i;

        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        //generation of the indices (2 triangles per face of each cube so 12 triangles)
        *(idata++) = 0+(8*i);
        *(idata++) = 1+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 3+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 4+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 3+(8*i);
        *(idata++) = 7+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 1+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 4+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 7+(8*i);

    }

    // if there are more atoms than bonds
    for (;i<nbAtoms; ++i){
        pPosTex = i;

        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vtextcoord++) = ((float) (pPosTex % textureSize));
        *(vtextcoord++) = ((float) (pPosTex / textureSize));

        //generation of the indices (2 triangles per face of each cube so 12 triangles)
        *(idata++) = 0+(8*i);
        *(idata++) = 1+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 3+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 4+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 3+(8*i);
        *(idata++) = 7+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 1+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 4+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 7+(8*i);
    }

    // if there are more bonds than atoms
    for (;i<nbBonds; ++i){

        pPos1 = links[i].atom1;
        pPos2 = links[i].atom2;
        pPosTex = i;

        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = -1;
        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vdata++) = -1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));

        *(vdata++) = -1;
        *(vdata++) = 1;
        *(vdata++) = 1;
        *(vtextcoord0++) = ((float) (pPos1 % textureSize));
        *(vtextcoord0++) = ((float) (pPos1 / textureSize));
        *(vtextcoord1++) = ((float) (pPos2 % textureSize));
        *(vtextcoord1++) = ((float) (pPos2 / textureSize));
        *(vtextcoord2++) = ((float) (pPosTex % textureSizeBonds));
        *(vtextcoord2++) = ((float) (pPosTex / textureSizeBonds));


        //generation of the indices (2 triangles per face of each cube so 12 triangles)
        *(idata++) = 0+(8*i);
        *(idata++) = 1+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 3+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 4+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 3+(8*i);
        *(idata++) = 7+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 1+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 4+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 7+(8*i);
    }
}

//Init arrays used to fill the textures
void AtomAndBondShaders::setupBuffersAndTextures(int nbAtoms, 
                                                 int nbBonds,
                                                 spheretype *dataAtoms,
                                                 cylindertype* dataBonds)
{
    //initialize nbAtoms
    this->nbAtoms = nbAtoms;
    this->nbBonds = nbBonds;
    nbBondsToDisplay=nbBonds;

    int nbMax=(nbAtoms>nbBonds)?nbAtoms:nbBonds;

    // each atom is represented first by a cube => 8 vertice by atoms
    // each vertex has 3 coordinates
    vertice=new GLfloat[nbMax*3*8];
    // two texture coordinates by atoms
    texturecoord=new GLfloat[nbAtoms*2*8];
    // two texture coordinates by bonds
    texturecoord0=new GLfloat[nbBonds*2*8];
    // two texture coordinates by bonds
    texturecoord1=new GLfloat[nbBonds*2*8];
    // two texture coordinates by bonds
    texturecoord2=new GLfloat[nbBonds*2*8];

    // each atom is drawn with 12 triangle (2 by face of the cube)
    indice=new GLuint[nbMax*12*3];

    // textureSize contains the side size of the  textures for atoms
    textureSize=(int)ceil(sqrt(nbAtoms));
    // textureSizeBonds contains the side size of the  textures for bonds
    textureSizeBonds=(int)ceil(sqrt(nbBonds));

    // the colors are represented by 4 floats
    colors= new GLfloat[textureSize*textureSize*4];
    // the positions are represented by 3 floats
    positions= new GLfloat[textureSize*textureSize*3];
    //	the scales are represented by one float
    atomScales= new GLfloat[textureSize*textureSize*1];
    // the sizes are represented by one float
    sizes= new GLfloat[textureSize*textureSize*1];
    //	the scales are represented by one float
    bondScales= new GLfloat[textureSizeBonds*textureSizeBonds*1];
    // the shrinks are represented by one float
    shrinks= new GLfloat[textureSizeBonds*textureSizeBonds*1];

    links=new LinkAtom[nbBonds];
    for(int i=0; i<nbBonds; i++)
    {
        links[i].atom1 = dataBonds[i].idsrc;
        links[i].atom2 = dataBonds[i].iddest;
    }

    this->initVertTCoordIndice();
    this->initAtomTextures(dataAtoms);
    this->initBondTextures(dataBonds);

}


void AtomAndBondShaders::updatePositions(GLfloat *positions)
{
    this->positions = positions;

    //    std::cout<<"update pos"<<std::endl;
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, texture_id[TEX_POSITIONS]);

    glTexImage2D (GL_TEXTURE_RECTANGLE_NV,
                  0,
                  GL_RGB32F_ARB,
                  textureSize,
                  textureSize,
                  0,
                  GL_RGB,
                  GL_FLOAT,
                  positions);

    glDisable(GL_TEXTURE_RECTANGLE_NV);

}

void AtomAndBondShaders::updateColors(GLfloat *colors)
{
    memcpy((GLfloat*)this->colors,(GLfloat*) colors,4*nbAtoms*sizeof(GLfloat));
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_COLORS]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, textureSize, textureSize, 0, GL_RGBA, GL_FLOAT, colors);
    glDisable(GL_TEXTURE_RECTANGLE_NV);
}

void AtomAndBondShaders::updateAtomScales(double newScale){
    for(int i = 0; i < nbAtoms; ++i){
        atomScales[i] = newScale;
    }
    updateAtomScales(atomScales);
}

void AtomAndBondShaders::updateAtomScales(GLfloat *scales){
    // MB: is this memcpy line really needed?
    memcpy((GLfloat*)this->atomScales,(GLfloat*) scales,nbAtoms*sizeof(GLfloat));
    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_ASCALES]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSize, textureSize, 0, GL_LUMINANCE, GL_FLOAT, scales);
    glDisable(GL_TEXTURE_RECTANGLE_NV);
}

void AtomAndBondShaders::updateBondScales(double newScale){
    for(int i = 0; i < nbBonds; ++i){
        bondScales[i] = newScale;
    }
    updateBondScales(bondScales);
}


void AtomAndBondShaders::updateBondScales(GLfloat *scales){
    // MB: is this memcpy line really needed?
    memcpy((GLfloat*)this->bondScales,(GLfloat*) scales,nbBonds*sizeof(GLfloat));
    glActiveTexture(GL_TEXTURE4);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_BSCALES]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSizeBonds, textureSizeBonds, 0, GL_LUMINANCE, GL_FLOAT, scales);
    glDisable(GL_TEXTURE_RECTANGLE_NV);
}

void AtomAndBondShaders::updateShrinks(float newShrink){
    for(int i = 0; i < nbBonds; ++i){
        shrinks[i] = newShrink;
    }
    updateShrinks(shrinks);
}

void AtomAndBondShaders::updateShrinks(GLfloat *shrinks){
    memcpy((GLfloat*)this->shrinks,(GLfloat*) shrinks,nbBonds*sizeof(GLfloat));
    glActiveTexture(GL_TEXTURE5);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_SHRINKS]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSizeBonds, textureSizeBonds, 0, GL_LUMINANCE, GL_FLOAT, shrinks);
    glDisable(GL_TEXTURE_RECTANGLE_NV);
}

void AtomAndBondShaders::updateSizes(GLfloat *sizes){
    memcpy((GLfloat*)this->sizes,(GLfloat*) sizes,nbAtoms*sizeof(GLfloat));
    // les tailles
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_SIZES]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSize, textureSize, 0, GL_LUMINANCE, GL_FLOAT, sizes);}

void AtomAndBondShaders::updateSelection(int *selection, int nbSelectiones,spheretype *dataAtoms){
    for (int i=0;i<nbSelectiones;i++){
        sizes[selection[i]]*=2;
        // colors[4*selection[i]+0]=dataAtoms[selection[i]].color.red;
        // colors[4*selection[i]+1]=dataAtoms[selection[i]].color.green;
        // colors[4*selection[i]+2]=dataAtoms[selection[i]].color.blue;
        //    colors[4*selection[i]+3]=0;

    }
    updateSizes(sizes);
    for (int i=0;i<nbSelectiones;i++){
        sizes[selection[i]]/=2;
        // colors[4*selection[i]+0]*=0.8;
        // colors[4*selection[i]+1]*=0.8;
        // colors[4*selection[i]+2]*=0.8;
        //colors[4*selection[i]+3]=dataAtoms[selection[i]].color.alpha;
    }
}

void AtomAndBondShaders::generateIndices(std::vector<int> &atomsToDisplay)
{

    if (atomsToDisplay.size()*12*3 != indiceShown.size())
    {
        indiceShown.resize(atomsToDisplay.size()*12*3);

	// std::cout << "On redimensionne le tableau d'indices : "
        //           << indiceShown.size() << " entiers (" << atomsToDisplay.size()
        //           << " atomes)" << std::endl;
    }

    GLuint* idata = &indiceShown[0];
    nbBondsToDisplay = 0;
    int nbAt[nbBonds];

    for (int i = 0; i < nbBonds; ++i)
        nbAt[i] = 0;

    for (int j = 0; j < atomsToDisplay.size(); ++j)
    {
        int i = atomsToDisplay[j];
        for (int k = 0; k < nbBonds; k++)
        {
            if (nbAt[k] < 2)
            {
                if (links[k].atom1 == i)
                    nbAt[k]++;
                if (links[k].atom2 == i)
                    nbAt[k]++;
                if(nbAt[k] == 2)
                    nbBondsToDisplay++;
            }
        }

 

        *(idata++) = 0+(8*i);
        *(idata++) = 1+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 3+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 1+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 2+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 4+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 7+(8*i);
        *(idata++) = 4+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 3+(8*i);
        *(idata++) = 7+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 5+(8*i);
        *(idata++) = 1+(8*i);

        *(idata++) = 0+(8*i);
        *(idata++) = 4+(8*i);
        *(idata++) = 5+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 2+(8*i);
        *(idata++) = 6+(8*i);

        *(idata++) = 3+(8*i);
        *(idata++) = 6+(8*i);
        *(idata++) = 7+(8*i);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_INDICE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indiceShown.size()*sizeof(GLuint),&indiceShown[0],GL_STREAM_DRAW);

    if (nbBondsToDisplay*12*3 != indiceShown.size())
    {
        indiceShown.resize(nbBondsToDisplay*12*3);

        // std::cout << "On redimensionne le tableau d'indices : "
        //           << indiceShown.size() << " entiers (" << atomsToDisplay.size()
        //           << " atomes) pour afficher " << nbBondsToDisplay
        //           << "/" << nbBonds << " liens" << std::endl;
    }

    // Vérification de la perfection des liens sélectionnés
    //    std::cout << "Debut de la verification des liens..." << std::endl;
    for (int i = 0; i < nbBonds; ++i)
    {
        if (nbAt[i] > 1)
        {
            if (!binary_search(atomsToDisplay.begin(), atomsToDisplay.end(), links[i].atom1) or
                !binary_search(atomsToDisplay.begin(), atomsToDisplay.end(), links[i].atom2))
                std::cout << "ERREUR : Le lien " << i << " est affiche mais ne devrait pas l'etre ("
                          << links[i].atom1 << "->" << links[i].atom2 << ")" << std::endl;
        }
        else
        {
            if (binary_search(atomsToDisplay.begin(), atomsToDisplay.end(), links[i].atom1) and
            binary_search(atomsToDisplay.begin(), atomsToDisplay.end(), links[i].atom2))
                std::cout << "ERREUR : Le lien " << i << " n'est pas affiche mais devrait l'etre ("
                          << links[i].atom1 << "->" << links[i].atom2 << ")" << std::endl;
        }
    }

    //    std::cout << "Verification des liens finie" << std::endl;

    idata = &indiceShown[0];
    for (int i = 0; i < nbBonds; ++i)
    {
        if(nbAt[i] > 1)
        {
            *(idata++) = 0+(8*i);
            *(idata++) = 1+(8*i);
            *(idata++) = 2+(8*i);

            *(idata++) = 0+(8*i);
            *(idata++) = 2+(8*i);
            *(idata++) = 3+(8*i);

            *(idata++) = 1+(8*i);
            *(idata++) = 5+(8*i);
            *(idata++) = 6+(8*i);

            *(idata++) = 1+(8*i);
            *(idata++) = 6+(8*i);
            *(idata++) = 2+(8*i);

            *(idata++) = 4+(8*i);
            *(idata++) = 6+(8*i);
            *(idata++) = 5+(8*i);

            *(idata++) = 4+(8*i);
            *(idata++) = 7+(8*i);
            *(idata++) = 6+(8*i);

            *(idata++) = 0+(8*i);
            *(idata++) = 7+(8*i);
            *(idata++) = 4+(8*i);

            *(idata++) = 0+(8*i);
            *(idata++) = 3+(8*i);
            *(idata++) = 7+(8*i);

            *(idata++) = 0+(8*i);
            *(idata++) = 5+(8*i);
            *(idata++) = 1+(8*i);

            *(idata++) = 0+(8*i);
            *(idata++) = 4+(8*i);
            *(idata++) = 5+(8*i);

            *(idata++) = 3+(8*i);
            *(idata++) = 2+(8*i);
            *(idata++) = 6+(8*i);

            *(idata++) = 3+(8*i);
            *(idata++) = 6+(8*i);
            *(idata++) = 7+(8*i);
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_BINDICE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indiceShown.size()*sizeof(GLuint),&indiceShown[0],GL_STREAM_DRAW);
}

/******************************************************************************/
//Opengl Drawing methods
void AtomAndBondShaders::drawAtoms() {

}

void AtomAndBondShaders::drawBonds() {

}

void AtomAndBondShaders::initGL() {
    // Generation of the buffers
    glGenBuffers(NB_BUFFERS,buffer_id);

    // loading the buffers
    int nbMax=nbAtoms>nbBonds?nbAtoms:nbBonds;
    glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_VERTICE]);
    glBufferData(GL_ARRAY_BUFFER,nbMax*3*8*sizeof(GLfloat),vertice,GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_INDICE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,nbAtoms*3*12*sizeof(GLuint),indice,GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_BINDICE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,nbBonds*3*12*sizeof(GLuint),indice,GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD]);
    glBufferData(GL_ARRAY_BUFFER,nbAtoms*2*8*sizeof(GLfloat),texturecoord,GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD0]);
    glBufferData(GL_ARRAY_BUFFER,nbBonds*2*8*sizeof(GLfloat),texturecoord0,GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD1]);
    glBufferData(GL_ARRAY_BUFFER,nbBonds*2*8*sizeof(GLfloat),texturecoord1,GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,buffer_id[BUF_TCOORD2]);
    glBufferData(GL_ARRAY_BUFFER,nbBonds*2*8*sizeof(GLfloat),texturecoord2,GL_STREAM_DRAW);

    //Generation of the textures used by the shaders
    glGenTextures(NB_TEXTURES,texture_id);

    // atom positions
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_POSITIONS]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB32F_ARB , textureSize, textureSize, 0, GL_RGB, GL_FLOAT, positions);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glDisable(GL_TEXTURE_RECTANGLE_NV);

    // atom colors
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_COLORS]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA32F_ARB, textureSize, textureSize, 0, GL_RGBA, GL_FLOAT, colors);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glDisable(GL_TEXTURE_RECTANGLE_NV);

    // atom sizes
    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_SIZES]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSize, textureSize, 0, GL_LUMINANCE, GL_FLOAT, sizes);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glDisable(GL_TEXTURE_RECTANGLE_NV);

    // atom scales
    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_ASCALES]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSize, textureSize, 0, GL_LUMINANCE, GL_FLOAT, atomScales);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glDisable(GL_TEXTURE_RECTANGLE_NV);

    //   bond scales
    glActiveTexture(GL_TEXTURE4);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_BSCALES]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSizeBonds, textureSizeBonds, 0, GL_LUMINANCE, GL_FLOAT, bondScales);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glDisable(GL_TEXTURE_RECTANGLE_NV);

    //   bond shrinks
    glActiveTexture(GL_TEXTURE5);
    glEnable(GL_TEXTURE_RECTANGLE_NV);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,texture_id[TEX_SHRINKS]);
    glTexImage2D (GL_TEXTURE_RECTANGLE_NV, 0, GL_INTENSITY, textureSizeBonds, textureSizeBonds, 0, GL_LUMINANCE, GL_FLOAT, shrinks);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glDisable(GL_TEXTURE_RECTANGLE_NV);

}

void AtomAndBondShaders::resetIndices()
{
  nbBondsToDisplay=nbBonds;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_INDICE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,nbAtoms*3*12*sizeof(GLuint),indice,GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer_id[BUF_BINDICE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,nbBonds*3*12*sizeof(GLuint),indice,GL_STREAM_DRAW);
}

