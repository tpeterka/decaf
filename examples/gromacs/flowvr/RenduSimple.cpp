/******* COPYRIGHT ************************************************
*                                                                 *
*                             Vitamins                            *
*                  Analytics Framework for Molecular Dynamics     *
*                                                                 *
*-----------------------------------------------------------------*
* COPYRIGHT (C) 2014 by                                           *
* INRIA, CNRS, Université d'Orsay, Université d'Orléans           *
* ALL RIGHTS RESERVED.                                            *
*                                                                 *
* This source is covered by the CeCILL-C licence,                 *
* Please refer to the COPYING file for further information.       *
*                                                                 *
*-----------------------------------------------------------------*
*                                                                 *
*  Original Contributors:                                         *
*  Marc Baaden                                                    *
*  Matthieu Dreher                                                *
*  Nicolas Férey                                                  *
*  Yohann Kernoa                                                   *
*  Sébastien Limet                                                *
*  Jessica Prevoteau-Jonquet                                      *
*  Marc Piuzzi                                                    *
*  Bruno Raffin                                                   *
*  Sophie Robert                                                  *
*  Mikaël Trellet                                                 *
*                                                                 *
*******************************************************************
*    Contact :                                                    *
*    Sébastien Limet <sebastien.limet@univ-orleans.fr             *
******************************************************************/
#include <cstdio>
#include <iostream>
#include <math.h>
#include "flowvr/module.h"
#include <GL/glew.h>

#include <QtCore/QDateTime>

#ifdef __APPLE__
// MB use MacOSX GLUT framework
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>
#endif

#include <QtCore/QTime>

#include <ftl/chunkevents.h>
#include <ftl/chunkreader.h>
#include <ftl/quat.h>
#include <ftl/vec.h>
#include <ftl/mat.h>

#include <vitamins_data.h>
#include <flowvr-vrpn/core/forces.h>

//#include <vitamins/core/vrpNanoMsg.h>

#include "Deselector.h"    // classe Deselector + Vector3D
#include "AtomAndBondGLSL.h"


// Détection de la modification des positions
#define TAILLE_FENETRE_TEST_POSITIONS 50

using namespace flowvr;
using namespace std;
using namespace ftl;
// mouse handling
bool leftbutton = false;
bool rightbutton = false;
bool middlebutton = false;
int mouseXOld, mouseYOld;
float xTrans=0,yTrans=0,zTrans=0,xRot=0,yRot=0,zRot=0;
float stepTrans=1.0; // step for translation

float mat[16]={1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1};
//jess
bool first=true;

int width = 800;
int height = 600;
//Pour ecrire des informations sur la sélection

int TypeSelection = 0; 
char text[300];
//Message mSelectionMode;

// Pour le FrameRate
int nbFrames;

// Pour les coefficients liés aux shaders
double newShrink=0, oldShrink=0, newAtomScale=0, oldAtomScale=0, newBondScale=0, oldBondScale=0;


int idSel, idSelPrec=-1;

// Pour limiter le nombre de fps
int fps; // The number of frames per second
int maxElapsedMS; // The maximum elapsed time, in milliseconds, between two frames
QTime fpsTime; // The Qt time handler, to be aware of the elapsed time between two frames
QTime fpsTimeDisplayer; // Qt time to display the ms in the window title

static bool prem = true;

// Partie FlowVR module, ports et message
ModuleAPI * rFlowVRModule = 0;
InputPort * pPos = new InputPort("positions"); // Les positions de GMX
InputPort * InBonds = new InputPort("bonds");  // Les liens de NC
InputPort * InAtoms = new InputPort("atoms");  // Les métadonnées sur les atomes
InputPort * InBBox = new InputPort("bbox"); // La bounding box
InputPort * pSelect = new InputPort("idatomselect"); // Les atomes sélectionnés
InputPort * pSelectable = new InputPort("selectable"); // L'atome sélectionnable
InputPort * InForce = new InputPort("force"); // La force visuelle
InputPort * InShaders = new InputPort("shaders");// 3 réels corresponds aux paramètres scale et shrink des shaders
InputPort * InAvatar = new InputPort("Avatar");// Les commandes des peripheriques
InputPort * InCamera = new InputPort("Camera");
InputPort * InActivation = new InputPort("Activations");
InputPort * InSelectionMode = new InputPort("selectionmode");
InputPort * InPhantomCalibration = new InputPort("PhantomCalibration");
InputPort * InParams3D = new InputPort("Params3D");
InputPort * InTargets = new InputPort("Targets");
InputPort * InSelectionPos = new InputPort("SelectionPos");
InputPort * In3DAnalog = new InputPort("TroisDAnalog");
InputPort * InBBoxBase = new InputPort("InBBoxBase");
InputPort * InBBoxRedistribute = new InputPort("InBBoxRedistribute");
InputPort * InGridGlobal = new InputPort("InGridGlobal");
InputPort * InGridActive = new InputPort("InGridActive");
InputPort * InMesh = new InputPort("InMesh");
InputPort * InBestPos = new InputPort("InBestPos");
InputPort * InMatrix = new InputPort("InMatrix");

OutputPort* OutAvatar = new OutputPort("OutAvatar"); // Position de l'avatar par rapport à la caméra pour le calcul des distances
OutputPort* OutMatrix = new OutputPort("OutMatrix");

Message mBut;
Message mAnalog;


OutputPort *OutGDeselector = new OutputPort("OutGDeselector"); // Les atomes déselectionnés
// les positions des atomes
//Message mPos;
//float* dataPos = NULL;
GLfloat* dataPos = NULL;
unsigned int nbPos = 0;
int change=1; //test si les positions ont change
flowvr::StampInfo stampPositions("TypePositions", flowvr::TypeInt::create());

// les liens (reçus une fois)
Message mBonds;
cylindertype * dataBonds;
int nbBonds;
bool initBonds = false;

// les métadonnées sur les atomes (diamètre, couleur, reçues une fois)
Message mAtoms;
spheretype * dataAtoms;
int nbAtoms;
bool initAtoms = false;
float fenetreTest[TAILLE_FENETRE_TEST_POSITIONS];


// Pour l'avatar
float* mConeGL = (float*)malloc(16*sizeof(float));

// Pour les forces
bool activeforce = false;
bool activecube = false;
float* OldPosTracker = (float*)malloc(3*sizeof(float));
bool initForce = true;

// Pour la bounding box
BBoxtype* BB;
Message MsgBB;
bool initBB = false;

// Pour le bouton du phantom
bool active = false;

// Pour les atomes sélectionnés
flowvr::Message mSelect;
int* selections=NULL;
unsigned nbselections=0;

// Pour l'atome sélectionnable
Message MsgSelectable;
int Selectable = -1;

// Pour la force
Message mForce;
fieldforces* ff = new fieldforces();  
float* force;
double scaleForce = 10.0;

// pour les paramètres des shaders
Message MsgShaders;

Message mParams3D;

// Partie Glut
static int win_id;


// Pour le menu de la sélection/déselection
MessageWrite mGDeselector;
Deselector* gGDeselector;
unsigned nbdeselections = 0;
unsigned int* deselections = NULL;

// pour le prefix pour lire les fichiers de shaders
const char* envpath;

// Pour gerer le shader des atomes
AtomAndBondGLSL shaders;
string vertSuffix = ".vert";
string fragSuffix = ".frag";
string shaderType = "GLSL";

// Pour le DeviceManager :
Message mAvatar;
Message mCamera;
Message mActivation;

// Pour l'affichage des Targets
float* posTargets = NULL;
int nbTargets = 0;
float* posSelection = NULL;



// Pour le Space Navigator


// float mat[16]={1,0,0,0,
//                0,1,0,0,
//                0,0,1,0,
//                0,0,0,1};


float xCenter=0;
float yCenter=0;
float zCenter=0;

// Avatar
float avatarX = 0;
float avatarY = 0;
float avatarZ = 0;

bool avatarOn = true;

// For the selectable atoms
GLUquadricObj *circle;
// --------------------------------------------------------------------
// Camera functions
// --------------------------------------------------------------------

// Pour la caméra
float eyeX = 0;
float eyeY = 0;
float eyeZ = 0;

float atX = 0;
float atY = 0;
float atZ = 0;

float upX = 0;
float upY = 1;
float upZ = 0;

double leftx = 0; 
double lefty = 0;
double leftz = 0;

double pi = 3.141592654;

int displayMode = 0; //0 : normal, 1 : 3D Side by Side, 2 : 3D Up and Down

//Camera3D
float eyeLeftX = eyeX;
float eyeLeftY = eyeY;
float eyeLeftZ = eyeZ;

float eyeRightX = eyeX;
float eyeRightY = eyeY;
float eyeRightZ = eyeZ;

int windowWidth = 0;
int windowHeight = 0;

int imageWidth = 0;
int imageHeight = 0;

int startLeftWidth = 0;
int startLeftHeight = 0;

int startRightWidth = 0;
int startRightHeight = 0;

double eyeDistance = 1.0;

bool fullscreen = false;


float mat1[16]={1,0,0,0,
                0,1,0,0,
                0,0,1,0,
                0,0,0,1};

float MatriceAvatar[16] = {1,0,0,0,
                           0,1,0,0,
                           0,0,1,0,
                           0,0,0,1};

MessageWrite MAvatar;

Message msgBBoxBase;
BBoxtype *BoxBase = NULL;
int nbBoxBase = 0;

Message msgBBoxRedistribute;
BBoxtype *BoxRedistribute = NULL;
int nbBoxRedistribute = 0;
float *color = NULL;

Message msgGridGlobal;
float *gridGlobal = NULL;
int nbCellsGlobal = 0;

Message msgGridActive;
float *gridActive = NULL;
int nbCellsActive = 0;

Message msgMesh;
float *mesh = NULL;
int nbTriangles = 0;

Message msgBestPos;
double *camera_position = NULL;

MessageWrite mOutMatrix;


// Pour le cube d'amplitude de l'avatar
float cubeX = 0, cubeY = 0, cubeZ = 0;
float xPhantom = 0, yPhantom = 0, zPhantom = 0;
Message mPhantomCalibration;

bool displayBox = false;
bool displayFullGrid = false;
bool displayDensityGrid = false;
bool displayQuickSurf = false;

float positionXOld = 0, positionX = 0, positionYOld = 0, positionY = 0, positionZOld = 0, positionZ = 0;

float angle360(float angle){
	while (angle<0)
		angle+=360;
	while (angle>=360)
		angle-=360;
	
	return angle;
}


void Compute3DCamera(float ecart)
{
    float cx, cy, cz;

    // Produit vectoriel target.x.up

    cx=upY*(eyeZ-atZ)-upZ*(eyeY-atY);
    cy=upZ*(eyeX-atX)-upX*(eyeZ-atZ);
    cz=upX*(eyeY-atY)-upY*(eyeX-atX);

    float norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);

    cx=cx/norm;
    cy=cy/norm;
    cz=cz/norm;
    cx=ecart*cx;
    cy=ecart*cy;
    cz=ecart*cz;

    eyeLeftX = eyeX + cx;
    eyeLeftY = eyeY + cy;
    eyeLeftZ = eyeZ + cz;

    eyeRightX = eyeX - cx;
    eyeRightY = eyeY - cy;
    eyeRightZ = eyeZ - cz;
}

void CameraTranslationX(float CameraTransX)
{
    float cx, cy, cz;

    // Produit vectoriel target.x.up

    cx=upY*(eyeZ-atZ)-upZ*(eyeY-atY);
    cy=upZ*(eyeX-atX)-upX*(eyeZ-atZ);
    cz=upX*(eyeY-atY)-upY*(eyeX-atX);

    float norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);

    cx=cx/norm;
    cy=cy/norm;
    cz=cz/norm;
    cx=CameraTransX*cx;
    cy=CameraTransX*cy;
    cz=CameraTransX*cz;

    atX+=cx;
    atY+=cy;
    atZ+=cz;

    eyeX+=cx;
    eyeY+=cy;
    eyeZ+=cz;

    avatarX+=cx;
    avatarY+=cy;
    avatarZ+=cz;
    
    cubeX+=cx;
    cubeY+=cy;
    cubeZ+=cz;
}

void CameraTranslationY(float CameraTransY)
{

    float cx = CameraTransY*upX;
    float cy = CameraTransY*upY;
    float cz = CameraTransY*upZ;
    
    eyeX+=cx;
    eyeY+=cy;
    eyeZ+=cz;

    avatarX+=cx;
    avatarY+=cy;
    avatarZ+=cz;
    
    cubeX+=cx;
    cubeY+=cy;
    cubeZ+=cz;

    atX+=cx;
    atY+=cy;
    atZ+=cz;
}

void CameraRotationY(float phi)
{

    //   cout << "CameraRotationY" << endl;

    float cx = upX;
    float cy = upY;
    float cz = upZ;
    float norm = 0;
    norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);
    cx = cx/norm;
    cy = cy/norm;
    cz = cz/norm;
    float mat[9];

    mat[0] = cx*cx+(1-cx*cx)*cos(-phi);
    mat[1] = cx*cy*(1-cos(-phi))-cz*sin(-phi);
    mat[2] = cx*cz*(1-cos(-phi))+cy*sin(-phi);

    mat[3] = cx*cy*(1-cos(-phi))+cz*sin(-phi);
    mat[4] = cy*cy+(1-cy*cy)*cos(-phi);
    mat[5] = cy*cz*(1-cos(-phi))-cx*sin(-phi);

    mat[6] = cx*cz*(1-cos(-phi))-cy*sin(-phi);
    mat[7] = cy*cz*(1-cos(-phi))+cx*sin(-phi);
    mat[8] = cz*cz+(1-cz*cz)*cos(-phi);

    float matbis[9];
    matbis[0]=mat[0]*MatriceAvatar[0]+mat[1]*MatriceAvatar[1]+mat[2]*MatriceAvatar[2];
    matbis[1]=mat[3]*MatriceAvatar[0]+mat[4]*MatriceAvatar[1]+mat[5]*MatriceAvatar[2];
    matbis[2]=mat[6]*MatriceAvatar[0]+mat[7]*MatriceAvatar[1]+mat[8]*MatriceAvatar[2];

    matbis[3]=mat[0]*MatriceAvatar[4]+mat[1]*MatriceAvatar[5]+mat[2]*MatriceAvatar[6];
    matbis[4]=mat[3]*MatriceAvatar[4]+mat[4]*MatriceAvatar[5]+mat[5]*MatriceAvatar[6];
    matbis[5]=mat[6]*MatriceAvatar[4]+mat[7]*MatriceAvatar[5]+mat[8]*MatriceAvatar[6];

    matbis[6]=mat[0]*MatriceAvatar[8]+mat[1]*MatriceAvatar[9]+mat[2]*MatriceAvatar[10];
    matbis[7]=mat[3]*MatriceAvatar[8]+mat[4]*MatriceAvatar[9]+mat[5]*MatriceAvatar[10];
    matbis[8]=mat[6]*MatriceAvatar[8]+mat[7]*MatriceAvatar[9]+mat[8]*MatriceAvatar[10];

    MatriceAvatar[0]  = matbis[0];
    MatriceAvatar[1]  = matbis[1];
    MatriceAvatar[2]  = matbis[2];
    MatriceAvatar[4]  = matbis[3];
    MatriceAvatar[5]  = matbis[4];
    MatriceAvatar[6]  = matbis[5];
    MatriceAvatar[8]  = matbis[6];
    MatriceAvatar[9]  = matbis[7];
    MatriceAvatar[10] = matbis[8];

    cx = mat[0]*eyeX+mat[1]*eyeY+mat[2]*eyeZ;
    cy = mat[3]*eyeX+mat[4]*eyeY+mat[5]*eyeZ;
    cz = mat[6]*eyeX+mat[7]*eyeY+mat[8]*eyeZ;

    eyeX = cx;
    eyeY = cy;
    eyeZ = cz;

    cx = mat[0]*atX+mat[1]*atY+mat[2]*atZ;
    cy = mat[3]*atX+mat[4]*atY+mat[5]*atZ;
    cz = mat[6]*atX+mat[7]*atY+mat[8]*atZ;

    atX = cx;
    atY = cy;
    atZ = cz;

    cx = mat[0]*avatarX+mat[1]*avatarY+mat[2]*avatarZ;
    cy = mat[3]*avatarX+mat[4]*avatarY+mat[5]*avatarZ;
    cz = mat[6]*avatarX+mat[7]*avatarY+mat[8]*avatarZ;

    avatarX = cx;
    avatarY = cy;
    avatarZ = cz;
    
    
    cx = mat[0]*cubeX+mat[1]*cubeY+mat[2]*cubeZ;
    cy = mat[3]*cubeX+mat[4]*cubeY+mat[5]*cubeZ;
    cz = mat[6]*cubeX+mat[7]*cubeY+mat[8]*cubeZ;
    
    cubeX = cx;
    cubeY = cy;
    cubeZ = cz;
    
}

void CameraRotationZ(float psi)
{
    float cx, cy, cz;
    cx=eyeX-atX;
    cy=eyeY-atY;
    cz=eyeZ-atZ;
    
    float norm = 0;
    norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);
    cx = cx/norm;
    cy = cy/norm;
    cz = cz/norm;
    
    float mat[9];

    mat[0] = cx*cx+(1-cx*cx)*cos(-psi);
    mat[1] = cx*cy*(1-cos(-psi))-cz*sin(-psi);
    mat[2] = cx*cz*(1-cos(-psi))+cy*sin(-psi);

    mat[3] = cx*cy*(1-cos(-psi))+cz*sin(-psi);
    mat[4] = cy*cy+(1-cy*cy)*cos(-psi);
    mat[5] = cy*cz*(1-cos(-psi))-cx*sin(-psi);

    mat[6] = cx*cz*(1-cos(-psi))-cy*sin(-psi);
    mat[7] = cy*cz*(1-cos(-psi))+cx*sin(-psi);
    mat[8] = cz*cz+(1-cz*cz)*cos(-psi);

    float matbis[9];
    matbis[0]=mat[0]*MatriceAvatar[0]+mat[1]*MatriceAvatar[1]+mat[2]*MatriceAvatar[2];
    matbis[1]=mat[3]*MatriceAvatar[0]+mat[4]*MatriceAvatar[1]+mat[5]*MatriceAvatar[2];
    matbis[2]=mat[6]*MatriceAvatar[0]+mat[7]*MatriceAvatar[1]+mat[8]*MatriceAvatar[2];

    matbis[3]=mat[0]*MatriceAvatar[4]+mat[1]*MatriceAvatar[5]+mat[2]*MatriceAvatar[6];
    matbis[4]=mat[3]*MatriceAvatar[4]+mat[4]*MatriceAvatar[5]+mat[5]*MatriceAvatar[6];
    matbis[5]=mat[6]*MatriceAvatar[4]+mat[7]*MatriceAvatar[5]+mat[8]*MatriceAvatar[6];

    matbis[6]=mat[0]*MatriceAvatar[8]+mat[1]*MatriceAvatar[9]+mat[2]*MatriceAvatar[10];
    matbis[7]=mat[3]*MatriceAvatar[8]+mat[4]*MatriceAvatar[9]+mat[5]*MatriceAvatar[10];
    matbis[8]=mat[6]*MatriceAvatar[8]+mat[7]*MatriceAvatar[9]+mat[8]*MatriceAvatar[10];

    MatriceAvatar[0]  = matbis[0];
    MatriceAvatar[1]  = matbis[1];
    MatriceAvatar[2]  = matbis[2];
    MatriceAvatar[4]  = matbis[3];
    MatriceAvatar[5]  = matbis[4];
    MatriceAvatar[6]  = matbis[5];
    MatriceAvatar[8]  = matbis[6];
    MatriceAvatar[9]  = matbis[7];
    MatriceAvatar[10] = matbis[8];

    cx = mat[0]*upX+mat[1]*upY+mat[2]*upZ;
    cy = mat[3]*upX+mat[4]*upY+mat[5]*upZ;
    cz = mat[6]*upX+mat[7]*upY+mat[8]*upZ;
    
    upX = cx;
    upY = cy;
    upZ = cz;
    
    //  cout << "avant la rotation : " << eyeX << " " << eyeY << " " << eyeZ << endl;
    
    cx = mat[0]*eyeX+mat[1]*eyeY+mat[2]*eyeZ;
    cy = mat[3]*eyeX+mat[4]*eyeY+mat[5]*eyeZ;
    cz = mat[6]*eyeX+mat[7]*eyeY+mat[8]*eyeZ;

    eyeX = cx;
    eyeY = cy;
    eyeZ = cz;
    //   cout << "après la rotation : " << eyeX << " " << eyeY << " " << eyeZ << endl;
    
    cx = mat[0]*atX+mat[1]*atY+mat[2]*atZ;
    cy = mat[3]*atX+mat[4]*atY+mat[5]*atZ;
    cz = mat[6]*atX+mat[7]*atY+mat[8]*atZ;
    // cout << "avant la rotation : " << atX << " " << atY << " " << atZ << endl;
    atX = cx;
    atY = cy;
    atZ = cz;
    // cout << "après la rotation : " << atX << " " << atY << " " << atZ << endl;
    cx = mat[0]*avatarX+mat[1]*avatarY+mat[2]*avatarZ;
    cy = mat[3]*avatarX+mat[4]*avatarY+mat[5]*avatarZ;
    cz = mat[6]*avatarX+mat[7]*avatarY+mat[8]*avatarZ;

    avatarX = cx;
    avatarY = cy;
    avatarZ = cz;

    
    cx = mat[0]*cubeX+mat[1]*cubeY+mat[2]*cubeZ;
    cy = mat[3]*cubeX+mat[4]*cubeY+mat[5]*cubeZ;
    cz = mat[6]*cubeX+mat[7]*cubeY+mat[8]*cubeZ;
    
    cubeX = cx;
    cubeY = cy;
    cubeZ = cz;
    

}

void CameraRotationX(float theta)
{
    float cx, cy, cz;
    cx=upY*(eyeZ-atZ)-upZ*(eyeY-atY);
    cy=upZ*(eyeX-atX)-upX*(eyeZ-atZ);
    cz=upX*(eyeY-atY)-upY*(eyeX-atX);

    float norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);
    cx=cx/norm;
    cy=cy/norm;
    cz=cz/norm;

    //     leftx=cx;
    //     lefty=cy;
    //     leftz=cz;

    //mat : matrice de rotation sur l'axe left.

    float mat[9];

    mat[0] = cx*cx+(1-cx*cx)*cos(-theta);
    mat[1] = cx*cy*(1-cos(-theta))-cz*sin(-theta);
    mat[2] = cx*cz*(1-cos(-theta))+cy*sin(-theta);

    mat[3] = cx*cy*(1-cos(-theta))+cz*sin(-theta);
    mat[4] = cy*cy+(1-cy*cy)*cos(-theta);
    mat[5] = cy*cz*(1-cos(-theta))-cx*sin(-theta);

    mat[6] = cx*cz*(1-cos(-theta))-cy*sin(-theta);
    mat[7] = cy*cz*(1-cos(-theta))+cx*sin(-theta);
    mat[8] = cz*cz+(1-cz*cz)*cos(-theta);

    float matbis[9];
    matbis[0]=mat[0]*MatriceAvatar[0]+mat[1]*MatriceAvatar[1]+mat[2]*MatriceAvatar[2];
    matbis[1]=mat[3]*MatriceAvatar[0]+mat[4]*MatriceAvatar[1]+mat[5]*MatriceAvatar[2];
    matbis[2]=mat[6]*MatriceAvatar[0]+mat[7]*MatriceAvatar[1]+mat[8]*MatriceAvatar[2];

    matbis[3]=mat[0]*MatriceAvatar[4]+mat[1]*MatriceAvatar[5]+mat[2]*MatriceAvatar[6];
    matbis[4]=mat[3]*MatriceAvatar[4]+mat[4]*MatriceAvatar[5]+mat[5]*MatriceAvatar[6];
    matbis[5]=mat[6]*MatriceAvatar[4]+mat[7]*MatriceAvatar[5]+mat[8]*MatriceAvatar[6];

    matbis[6]=mat[0]*MatriceAvatar[8]+mat[1]*MatriceAvatar[9]+mat[2]*MatriceAvatar[10];
    matbis[7]=mat[3]*MatriceAvatar[8]+mat[4]*MatriceAvatar[9]+mat[5]*MatriceAvatar[10];
    matbis[8]=mat[6]*MatriceAvatar[8]+mat[7]*MatriceAvatar[9]+mat[8]*MatriceAvatar[10];

    MatriceAvatar[0]  = matbis[0];
    MatriceAvatar[1]  = matbis[1];
    MatriceAvatar[2]  = matbis[2];
    MatriceAvatar[4]  = matbis[3];
    MatriceAvatar[5]  = matbis[4];
    MatriceAvatar[6]  = matbis[5];
    MatriceAvatar[8]  = matbis[6];
    MatriceAvatar[9]  = matbis[7];
    MatriceAvatar[10] = matbis[8];
    
    cx = mat[0]*eyeX+mat[1]*eyeY+mat[2]*eyeZ;
    cy = mat[3]*eyeX+mat[4]*eyeY+mat[5]*eyeZ;
    cz = mat[6]*eyeX+mat[7]*eyeY+mat[8]*eyeZ;

    eyeX = cx;
    eyeY = cy;
    eyeZ = cz;

    cx = mat[0]*upX+mat[1]*upY+mat[2]*upZ;
    cy = mat[3]*upX+mat[4]*upY+mat[5]*upZ;
    cz = mat[6]*upX+mat[7]*upY+mat[8]*upZ;

    upX = cx;
    upY = cy;
    upZ = cz;

    cx = mat[0]*atX+mat[1]*atY+mat[2]*atZ;
    cy = mat[3]*atX+mat[4]*atY+mat[5]*atZ;
    cz = mat[6]*atX+mat[7]*atY+mat[8]*atZ;

    atX = cx;
    atY = cy;
    atZ = cz;

    cx = mat[0]*avatarX+mat[1]*avatarY+mat[2]*avatarZ;
    cy = mat[3]*avatarX+mat[4]*avatarY+mat[5]*avatarZ;
    cz = mat[6]*avatarX+mat[7]*avatarY+mat[8]*avatarZ;

    avatarX = cx;
    avatarY = cy;
    avatarZ = cz;
    
    cx = mat[0]*cubeX+mat[1]*cubeY+mat[2]*cubeZ;
    cy = mat[3]*cubeX+mat[4]*cubeY+mat[5]*cubeZ;
    cz = mat[6]*cubeX+mat[7]*cubeY+mat[8]*cubeZ;
    
    cubeX = cx;
    cubeY = cy;
    cubeZ = cz;
}

void CameraZoom(float zoom)
{
    float cx, cy, cz;
    cx = eyeX-atX;
    cy = eyeY-atY;
    cz = eyeZ-atZ;
    float norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);
    cx=zoom*cx/norm;
    cy=zoom*cy/norm;
    cz=zoom*cz/norm;

    eyeX += cx;
    eyeY += cy;
    eyeZ += cz;

    avatarX+=cx;
    avatarY+=cy;
    avatarZ+=cz;
    
    cubeX+=cx;
    cubeY+=cy;
    cubeZ+=cz;
    
    
}

void AvatarTranslationX(float dep)
{
    //   cx=upY*(eyeZ-atZ)-upZ*(eyeY-atY);
    //     cy=upZ*(eyeX-atX)-upX*(eyeZ-atZ);
    //     cz=upX*(eyeY-atY)-upY*(eyeX-atX);
    //
    //     float norm = cx*cx+cy*cy+cz*cz;
    //     norm = sqrt(norm);
    //
    //     cx=cx/norm;
    //     cy=cy/norm;
    //     cz=cz/norm;
    //     cx=CameraTransX*cx;
    //     cy=CameraTransX*cy;
    //     cz=CameraTransX*cz;
    //   avatarX+=cx;
    //     avatarY+=cy;
    //     avatarZ+=cz;
    //



    float cx, cy, cz;
    cx=upY*(eyeZ-atZ)-upZ*(eyeY-atY);
    cy=upZ*(eyeX-atX)-upX*(eyeZ-atZ);
    cz=upX*(eyeY-atY)-upY*(eyeX-atX);
    float norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);

    cx=cx/norm;
    cy=cy/norm;
    cz=cz/norm;
    avatarX=avatarX+dep*cx;
    avatarY=avatarY+dep*cy;
    avatarZ=avatarZ+dep*cz;
    
}

void AvatarTranslationY(float dep)
{
    avatarX+=dep*upX;
    avatarY+=dep*upY;
    avatarZ+=dep*upZ;
    

}
void AvatarTranslationZ(float dep)
{
    float cx, cy, cz;
    cx = eyeX-atX;
    cy = eyeY-atY;
    cz = eyeZ-atZ;

    float norm = cx*cx+cy*cy+cz*cz;
    norm = sqrt(norm);

    cx=cx/norm;
    cy=cy/norm;
    cz=cz/norm;

    avatarX+=dep*cx;
    avatarY+=dep*cy;
    avatarZ+=dep*cz;
    
}

// --------------------------------------------------------------------
// FlowVR functions
// --------------------------------------------------------------------

void CleanFlowVR()
{
    // Release FlowVR module handler :
    if (rFlowVRModule)
        rFlowVRModule->close();
}

// MB: for OSX compatibility -- implement gluBitmapString function missing in SL GLUT framework
void glutBitmapString(void *font, char *string)
{
    size_t len = strlen(string);

    for (size_t i = 0; i < len; i++)
        glutBitmapCharacter(font, string[i]);
}



void InitAvatar()
{

    for (int i=0; i<16; i++)
        mConeGL[i] = 0.0;

    for (int i=0; i<4; i++)
        mConeGL[4*i+i] = 1.0;
}

void InitBB()
{
    BB = (BBoxtype*)malloc(sizeof(BBoxtype));
    BB->Xmin = 0; BB->Xmax = 0;
    BB->Ymin = 0; BB->Ymax = 0;
    BB->Zmin = 0; BB->Zmax = 0;
}

void InitGDeselector (double w, double h)
{
    gGDeselector = new Deselector(w,h);
}


// --------------------------------------------------------------------
// OpenGL specific drawing routines
// --------------------------------------------------------------------

void update3DWindow(){
    switch(displayMode){
    case 1 : //3D Side by Side
        imageWidth = windowWidth / 2;
        imageHeight = windowHeight;

        startLeftWidth = 0;
        startLeftHeight = 0;
        startRightWidth = imageWidth;
        startRightHeight = 0;
        break;

    case 2: //3D Up and Down
        imageWidth = windowWidth;
        imageHeight = windowHeight / 2;
        startLeftWidth = 0;
        startLeftHeight = 0;
        startRightWidth = 0;
        startRightHeight = imageHeight;
        break;
    }
}

void InitDisplay(int width, int height)
{
  //  std::cout << "InitDisplay" << std::endl;
    glewInit();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0);
    
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); // remise a zéro
    
    nbFrames = 0;

    circle = gluNewQuadric();
    gluQuadricDrawStyle(circle,GLU_FILL);
}

void displayInfo()
{
  //  std::cout << "DisplayInfo" << std::endl;
    char text[300];

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, glutGet(GLUT_WINDOW_WIDTH), 0.0, glutGet(GLUT_WINDOW_HEIGHT), -1.0, 1.0);

    // draw text
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos3d(0,50,0); // specify the raster position for pixel operations

    switch(TypeSelection)
    {
    case 0:
        sprintf(text,"Atom Selection");
        break;
    case 1:
        sprintf(text,"Residu Selection");
        break;
    case 2:
        sprintf(text,"Chain Selection");
        break;
    }

    glutBitmapString(GLUT_BITMAP_HELVETICA_12,(char*)text);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void PreDisplay()
{
    if (prem && initAtoms && initBonds)
    {
        shaders.setAtomFragmentShaderProgramName(string(envpath)+"/shaders/ballimproved"+fragSuffix);
        shaders.setAtomVertexShaderProgramName(string(envpath)+"/shaders/ballimproved"+vertSuffix);
        shaders.setBondFragmentShaderProgramName(string(envpath)+"/shaders/stickimproved"+fragSuffix);
        shaders.setBondVertexShaderProgramName(string(envpath)+"/shaders/stickimproved"+vertSuffix);
        shaders.setupBuffersAndTextures(nbAtoms,nbBonds,dataAtoms,dataBonds);

        shaders.initGL();

        prem=false;
    }

    //shaders.updatePositions((GLfloat *)dataPos);
    if(dataPos){
        //change=0;
        shaders.updatePositions(dataPos);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    xRot=angle360(xRot);
    yRot=angle360(yRot);
    zRot=angle360(zRot);

}

void MakeSquare(float x1, float y1, float z1, float x2, float y2, float z2)
{
    glBegin(GL_QUADS);
    glVertex3f(x1,y1,z1);
    glVertex3f(x2,y1,z1);
    glVertex3f(x2,y2,z1);
    glVertex3f(x1,y2,z1);

    glVertex3f(x1,y1,z2);
    glVertex3f(x2,y1,z2);
    glVertex3f(x2,y2,z2);
    glVertex3f(x1,y2,z2);

    glVertex3f(x1,y1,z1);
    glVertex3f(x1,y2,z1);
    glVertex3f(x1,y2,z2);
    glVertex3f(x1,y1,z2);

    glVertex3f(x2,y1,z1);
    glVertex3f(x2,y2,z1);
    glVertex3f(x2,y2,z2);
    glVertex3f(x2,y1,z2);

    glEnd();
}

void doMainDisplay()
{
  //  std::cout << "doMainDisplay" << std::endl;
    // The avatar as an arrow
    if (active)
        glColor3f(1.0,0.0,0.0);
    else
        glColor3f(0.0,1.0,1.0);

    glPushMatrix();

    glTranslatef(avatarX,avatarY,avatarZ);
    glMultMatrixf(MatriceAvatar);
    glMultMatrixf(mConeGL);

    // Pour déplacer le cone afin que la pointe soit initialement en 0,0,0.
    glTranslatef(0.0,2.5,0);
    glRotatef(90,1,0,0);
    if (avatarOn)
      glutSolidCone(0.5,2.5,10,10);
    glPopMatrix();

    glPushMatrix();
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    //glTranslatef(avatarX,avatarY,avatarZ);
    glTranslatef(cubeX,cubeY,cubeZ);

//    cout << "Avatar : " << avatarX << " " << avatarY << " " << avatarZ << endl;
//    cout << "Cube : " << cubeX << " " << cubeY << " " << cubeZ << endl;
    glMultMatrixf(MatriceAvatar);
    //    MakeSquare(-xPhantom/6.3,-yPhantom/6.3,-zPhantom/6.3,xPhantom/6.3,yPhantom/6.3,zPhantom/6.3);
    //MakeSquare(avatarX-xPhantom/6.3,avatarY-yPhantom/6.3,avatarZ-zPhantom/6.3,avatarX+xPhantom/6.3,avatarY+yPhantom/6.3,avatarZ+zPhantom/6.3);
    glPopMatrix();

    glTranslatef(-xCenter,-yCenter,-zCenter);
    float xmin = BB->Xmin;
    float xmax = BB->Xmax;
    float ymin = BB->Ymin;
    float ymax = BB->Ymax;
    float zmin = BB->Zmin;
    float zmax = BB->Zmax;

    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

    //MakeSquare(xmin,ymin,zmin,xmax,ymax,zmax);
    //         glBegin(GL_QUADS);
    //         glVertex3f(xmin,ymin,zmin);
    //         glVertex3f(xmax,ymin,zmin);
    //         glVertex3f(xmax,ymax,zmin);
    //         glVertex3f(xmin,ymax,zmin);
    //
    //         glVertex3f(xmin,ymin,zmax);
    //         glVertex3f(xmax,ymin,zmax);
    //         glVertex3f(xmax,ymax,zmax);
    //         glVertex3f(xmin,ymax,zmax);
    //
    //         glVertex3f(xmin,ymin,zmin);
    //         glVertex3f(xmin,ymax,zmin);
    //         glVertex3f(xmin,ymax,zmax);
    //         glVertex3f(xmin,ymin,zmax);
    //
    //         glVertex3f(xmax,ymin,zmin);
    //         glVertex3f(xmax,ymax,zmin);
    //         glVertex3f(xmax,ymax,zmax);
    //         glVertex3f(xmax,ymin,zmax);
    //
    //         glEnd();

    /*if(BoxBase)
    {
        glPushMatrix();
        glColor4f(1.f,0.5f,0.f, 1);
        for(int i = 0; i < nbBoxBase; i++)
            MakeSquare(BoxBase[i].Xmin, BoxBase[i].Ymin, BoxBase[i].Zmin, BoxBase[i].Xmax, BoxBase[i].Ymax, BoxBase[i].Zmax);
        glPopMatrix();
    }*/

    if(BoxRedistribute && displayBox)
    {
        glPushMatrix();
        glLineWidth(3);
        for(int i = 0; i < nbBoxRedistribute; i++){
            glColor4f(color[3*i]  ,color[3*i+1],color[3*i+2],1.f);
            MakeSquare(BoxRedistribute[i].Xmin, BoxRedistribute[i].Ymin, BoxRedistribute[i].Zmin, BoxRedistribute[i].Xmax, BoxRedistribute[i].Ymax, BoxRedistribute[i].Zmax);
        }
        glLineWidth(1);
        glPopMatrix();
    }

    if(gridGlobal && displayFullGrid)
    {
        glPushMatrix();
        glColor3f(0.8f,1.f,0.f);
        for(int i = 0; i < nbCellsGlobal; i++)
            MakeSquare(gridGlobal[i*6],gridGlobal[i*6+1],gridGlobal[i*6+2],gridGlobal[i*6+3],gridGlobal[i*6+4],gridGlobal[i*6+5]);
        glPopMatrix();
    }

    if(gridActive && displayDensityGrid)
    {
        glPushMatrix();
        glColor3f(0.8f,1.f,0.f);
        for(int i = 0; i < nbCellsActive; i++)
            MakeSquare(gridActive[i*6],gridActive[i*6+1],gridActive[i*6+2],gridActive[i*6+3],gridActive[i*6+4],gridActive[i*6+5]);
        glPopMatrix();
    }

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);



    //std::cout<<"Display des forces..."<<std::endl;
    if (activeforce && nbselections != 0)
    {

        //float x = avatarX - OldPosTracker[0];
        //float y = avatarY - OldPosTracker[1];
        //float z = avatarZ - OldPosTracker[2];

        //    cout << "norme de la force" << x << " " << y << " " << z << endl;
      //        std::cout<<"Dessin des forces("<<force[0]<<","<<force[1]<<","<<force[2]<<")"<<std::endl;
        glLineWidth(5);

        for (unsigned int i = 0; i < nbselections; i++)
        {
            glBegin(GL_LINES);
            glVertex3f(dataPos[3*selections[i]],dataPos[3*selections[i]+1],dataPos[3*selections[i]+2]);
            //glVertex3f(dataPos[3*selections[i]]+x,dataPos[3*selections[i]+1]+y,dataPos[3*selections[i]+2]+z);
            //Y is multiplied by -1 because the ForceField has been saved with the opposite coordinate. Don't know why
            glVertex3f(dataPos[3*selections[i]]+force[0]*scaleForce,dataPos[3*selections[i]+1]-force[1]*scaleForce,dataPos[3*selections[i]+2]+force[2]*scaleForce);
            glEnd();
        }
        glLineWidth(1);
    }

    if (Selectable!=-1)
    {
        glPushMatrix();
        glTranslatef(dataPos[3*Selectable],dataPos[3*Selectable+1],dataPos[3*Selectable+2]);
        glColor4f(dataAtoms[Selectable].color.red, dataAtoms[Selectable].color.green, dataAtoms[Selectable].color.blue,  1);
        glutWireSphere(dataAtoms[Selectable].radius*(newAtomScale*15),10,10);

        glPopMatrix();
    }

    if(nbTargets > 0)
    {
        for(int i = 0; i < nbTargets; i++)
        {
            glPushMatrix();
            glTranslatef(posTargets[3*i],posTargets[3*i+1],posTargets[3*i+2]);
            glColor4f(1.f, 1.f, 1.f,  1);
            glutWireSphere(2.0f,10,10);
            glPopMatrix();
        }
        glLineWidth(3.f);
        glBegin(GL_LINE_STRIP);
        for(int i = 0; i < nbTargets; i++)
        {
            glVertex3f(posTargets[3*i],posTargets[3*i+1],posTargets[3*i+2]);
        }
        glEnd();
        glLineWidth(1.f);
    }

    if(posSelection)
    {
        glPushMatrix();
        glTranslatef(posSelection[0],posSelection[1],posSelection[2]);
        glColor4f(0.f, 1.f, 0.f,  1);
        glutWireSphere(1.0f,10,10);
        glPopMatrix();
    }




    //    An atom renders as a sphere and the selected atom(s) renders with the radius times 5
    if (initAtoms && initBonds && nbPos!=0)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawElements(GL_TRIANGLES,0,
                       GL_UNSIGNED_INT,0);
        glDisableClientState(GL_VERTEX_ARRAY);
        shaders.drawAtoms(nbPos);
        shaders.drawBonds();
    }

    if(mesh && displayQuickSurf)
    {
        glColor3f(0.f,1.f,0.5f);
	//        std::cout<<"Affichage de "<<nbTriangles <<" triangles"<<std::endl;
        for(int i = 0; i < nbTriangles; i++)
        {
            glBegin(GL_LINE_STRIP);
            glVertex3f(mesh[i*9],mesh[i*9+1],mesh[i*9+2]);
            glVertex3f(mesh[i*9+3],mesh[i*9+4],mesh[i*9+5]);
            glVertex3f(mesh[i*9+6],mesh[i*9+7],mesh[i*9+8]);
            glEnd();
        }
    }

    gGDeselector->displayMenu();
}

void CrossProduct()
{
    upX = -eyeZ * atY + eyeY * atZ;
    upY = eyeZ * atX - eyeX * atZ;
    upZ = -eyeY * atX + eyeX * atY;
}

void DisplayFunc()
{
  //  std::cout << "DisplayFunc" << std::endl;
    //std::cout<<"Nouvel affichage"<<std::endl;
  if (fpsTime.elapsed() > maxElapsedMS) {
    fpsTime.restart();
    PreDisplay();
    
    if(displayMode == 0){
      
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      
      displayInfo();
      if (camera_position){
	//	std::cout << "Camera_position: " << camera_position[0] << std::endl;
	eyeX = camera_position[0];
	eyeY = camera_position[1];
	eyeZ = camera_position[2];
	atX = camera_position[3];
	atY = camera_position[4];
	atZ = camera_position[5];
	//                CrossProduct();
	//                eyeX = 0.0;
	//                eyeY = 0.0;
	//                eyeZ = 0.0;
	//                atX = 0.0;
	//                atY = 0.0;
	//                atZ = 100.0;
      }
      //      std::cout << "GluLookAt parameters: EYE " << eyeX << " " << eyeY << " " << eyeZ << " AT " << atX << " " << atY << " " << atZ << " UP " << upX << " " << upY << " " << upZ << std::endl;
      gluLookAt(eyeX,eyeY,eyeZ,atX,atY,atZ,upX,upY,upZ);
      //            gluLookAt(0.0,0.0,200.0,0.0,0.0,300.0,0.0,1.0,0.0);
      GLdouble model[16];
      glGetDoublev(GL_MODELVIEW_MATRIX, model);
      // printf("-------- MODELVIEW MATRIX ---\n");
      // for(int i=0; i<4; i++){
      // 	printf("%f\t%f\t%f\t%f", model[i], model[i+4], model[i+8], model[i+12]);
      // 	printf("\n");
      // }
      /**************************************************************/
      /*         Rajout pour la gestion de la souris                */
      /*         VERSION INCOMPATIBLE avec le space3D en même temps */
      /*         et surtout avec la sélection !!!!!                 */
      /**************************************************************/
      /*glTranslatef(xTrans,yTrans,zTrans);
      
      // compute the rotations    
      glPushMatrix();
      glLoadIdentity();
      glRotatef(zRot,0,0,1);
      glRotatef(yRot,0,1,0);
      glRotatef(xRot,1,0,0);
      // apply former rotation
      glMultMatrixf(mat);
      // get the new rotation
      glGetFloatv(GL_MODELVIEW_MATRIX,mat);
      // reset the angles to 0
      xRot=yRot=zRot=0;
      glPopMatrix();*/
      
      // apply the new rotation to the model
      //      glMultMatrixf(mat);
      
      // place the center of the molecule in (0,0,0) 
      //      glTranslatef(-xCenter,-yCenter,-zCenter);

      /***********************************************/
      /*     FIN Rajout pour la gestion de la souris */
      /***********************************************/

      

      //            MessageWrite moutMatrix;
      //            mOutMatrix.data = rFlowVRModule->alloc(16 * sizeof(double));
      //            memcpy(mOutMatrix.data.writeAccess(), model, 16 * sizeof(double));
      //            rFlowVRModule->put(OutMatrix, mOutMatrix);
      //std::cout<<"Main display..."<<std::endl;
      doMainDisplay();
      //std::cout<<"Main display ok"<<std::endl;
    }
    else {
      Compute3DCamera(eyeDistance);
      
      glViewport(startLeftWidth, startLeftHeight, imageWidth, imageHeight);
      
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      
      displayInfo();
      gluLookAt(eyeLeftX,eyeLeftY,eyeLeftZ,atX,atY,atZ,upX,upY,upZ);
      doMainDisplay();
      
      
      glViewport(startRightWidth, startRightHeight, imageWidth, imageHeight);
      
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      
      displayInfo();
      gluLookAt(eyeRightX,eyeRightY,eyeRightZ,atX,atY,atZ,upX,upY,upZ);
      doMainDisplay();
    }
    
    
    
    glutSwapBuffers ();
    glutPostRedisplay();
    
    nbFrames++;
    
    if (fpsTimeDisplayer.elapsed() > 1000)
      {
	glutSetWindowTitle(QString("VITAMINS      %1 fps").arg((nbFrames*1000.0)/fpsTimeDisplayer.elapsed()).toAscii().data());
	
	fpsTimeDisplayer.restart();
	
	nbFrames = 0;
      }
  }
  //std::cout<<"Fin affichage."<<std::endl;
}

void IdleFunc()
{
    Message mPos;
    Message mSelectionMode;
    //    std::cout << "Idle Func" << std::endl;

    if (rFlowVRModule->wait())
    {

      //      std::cout << "sortie du wait" << std::endl;
        // Réception (1 fois) des liens entre atomes
        if (!initBonds)
        {
            rFlowVRModule->get(InBonds,mBonds);
            if (mBonds.data.getSize() > 0)
            {
                nbBonds = mBonds.data.getSize() / (sizeof(cylindertype)) ;
                dataBonds=(cylindertype * ) malloc(nbBonds*sizeof(cylindertype));
                memcpy((cylindertype *) dataBonds, (cylindertype *) mBonds.data.readAccess(),nbBonds*sizeof(cylindertype));

                initBonds=true;
                //std::cout << "Bonds have been received (" << nbBonds << " bonds)" << std::endl;
            }
        }

        // Réception (1 fois) des métadonnées sur les atomes
        if (!initAtoms)
        {
            rFlowVRModule->get(InAtoms,mAtoms);
            if (mAtoms.data.getSize()>0)
            {
                nbAtoms = mAtoms.data.getSize() / (sizeof(spheretype)) ;
                dataAtoms=(spheretype * ) malloc(nbAtoms*sizeof(spheretype));
                memcpy((spheretype *) dataAtoms, (spheretype *) mAtoms.data.readAccess(),nbAtoms*sizeof(spheretype));
                //dataPos = new GLfloat[nbAtoms*3];
                initAtoms = true;
                //std::cout << "At	oms have been received (" << nbAtoms << " atoms)" << std::endl;
            }
        }

        //std::cout<<"Nouvelle iteration"<<std::endl;
        rFlowVRModule->get(pPos, mPos);
        int it, identifiedAtoms;

        mPos.stamps.read(pPos->stamps->it,it);
        mPos.stamps.read(stampPositions, identifiedAtoms);

        if (mPos.data.getSize() > 1) // If new positions have been received
        {
	  //            std::cout<<"Reception de "<<mPos.data.getSize() / (3 * 3 * sizeof(float))<<" atomes"<<std::endl;
            if (identifiedAtoms != idSelPrec) // on est dans le cas où la selection a changé
            {
                if (identifiedAtoms == 0) // on a toute la sélection (fvnanosimple)
                {
                    nbPos = mPos.data.getSize() / (3*sizeof(float));
                    //jess
                    if(first == true)
                    {
                      //dataPos=new GLfloat[mPos.data.getSize()];
                      dataPos=(GLfloat*)malloc(nbPos * 3 * sizeof(GLfloat));
                      first = false;
                    }
                    
                    memcpy((void *)dataPos, (void *)mPos.data.readAccess(),mPos.data.getSize());


                    shaders.resetIndices();
                    //                    shaders.updatePositions(dataPos);
                    //std::cout<<"Reception de nouvelles positions("<<nbPos<<")"<<std::endl;
                }
                else  // une selection d'atomes
                {
                    nbPos = mPos.data.getSize() / (sizeof(idPositionType));
		    //                    std::cout<<"Reception d'un nouveau jeu d'atomes ("<<nbPos<<")"<<std::endl;
                    idPositionType * idPositionTab = (idPositionType*) mPos.data.readAccess();
                    vector<int>indiceDraw(nbPos);

                    // on suppose que le nombre d'atomes ne change pas pendant une exécution
                    for (unsigned int i = 0; i < nbPos; ++i)
                    {
                        //		      cout<<"id="<<idPositionTab[i].atomID<<" "<<idPositionTab[i].posX<<" "<<idPositionTab[i].posY<<" "<<idPositionTab[i].posZ<<endl;
                        unsigned atomId=indiceDraw[i] = idPositionTab[i].atomID;
                        dataPos[atomId * 3] = idPositionTab[i].posX;
                        dataPos[atomId * 3 + 1] = idPositionTab[i].posY;
                        dataPos[atomId * 3 + 2] = idPositionTab[i].posZ;
                    }
                    shaders.generateIndices(indiceDraw);
                    //                    shaders.updatePositions(dataPos);
                }
                idSelPrec = identifiedAtoms;
            }
            else // la selection n'a pas changé donc on ne met à jour que les positions
            {
                	        //std::cout<<"Reception de nouvelles positions sans chgt de selection("<<nbPos<<")"<<std::endl;
                change=memcmp((void *)fenetreTest,(void *)mPos.data.readAccess(),TAILLE_FENETRE_TEST_POSITIONS*sizeof(float));

                //	      cout<<"it= "<<it<<" change="<<change<<endl;
                if (change!=0){
                    		  //std::cout<<"A priori les positions ont changé"<<endl;
                    memcpy((void *)fenetreTest,(void *)mPos.data.readAccess(),TAILLE_FENETRE_TEST_POSITIONS*sizeof(float));

                    if (identifiedAtoms == 0) // tous les atomes
                    {
                        memcpy((void *)dataPos,(void*) mPos.data.readAccess(),nbAtoms*3*sizeof(GLfloat));
                        //		      shaders.updatePositions(dataPos);
                    }
                    else // filtrage
                    {
                        //	std::cout<<"Reception d'atomes filtres avec filtrage deja connu"<<std::endl;
                        idPositionType * idPositionTab = (idPositionType*) mPos.data.readAccess();
                        for (unsigned int i = 0; i < nbPos; ++i)
                        {
                            //			  cout<<"Id="<<idPositionTab[i].atomID<<" "<<idPositionTab[i].posX<<" "<<idPositionTab[i].posY<<" "<<idPositionTab[i].posZ<<endl;
                            unsigned atomId=idPositionTab[i].atomID;
                            dataPos[atomId * 3] = idPositionTab[i].posX;
                            dataPos[atomId * 3 + 1] = idPositionTab[i].posY;
                            dataPos[atomId * 3 + 2] = idPositionTab[i].posZ;
                        }
                        //			shaders.updatePositions(dataPos);
                    }
                }
            }
        }

        // Réception de la bounding box
        if (!initBB)
        {
            rFlowVRModule->get(InBBox, MsgBB);

            if (MsgBB.data.getSize() > 0)
            {
                BB = (BBoxtype*) MsgBB.data.readAccess();

                xCenter=(BB->Xmin+BB->Xmax)/2;
                yCenter=(BB->Ymin+BB->Ymax)/2;
                zCenter=(BB->Zmin+BB->Zmax)/2;

                eyeX = 0;
                eyeY = 0;
                eyeZ = 0-(BB->Zmax-BB->Zmin)-10;

                atX = 0;
                atY = 0;
                atZ = 0;

                upX = 0;
                upY = 1;
                upZ = 0;

                initBB = true;
                //std::cout << "Bounding box has been received" << std::endl;
            }
        }

        if (InSelectionMode->isConnected())
        {
            rFlowVRModule->get(InSelectionMode, mSelectionMode);

            if (mSelectionMode.data.getSize()>0)
            {
                int * mode = (int*)mSelectionMode.data.readAccess();
                TypeSelection = mode[0];
            }
        }
        
        if(InParams3D->isConnected())
        {
            rFlowVRModule->get(InParams3D, mParams3D);

            if(mParams3D.data.getSize()>0)
            {
                float *params = (float*)mParams3D.data.readAccess();
                displayMode = (int) (params[0]);
                eyeDistance = params[1];
                update3DWindow();
            }
        }

        // Reception des activations
        if (InActivation->isConnected()) {
	  rFlowVRModule->get(InActivation, mActivation);
	  if (mActivation.data.getSize()>0) {
	    bool * act = (bool*)mActivation.data.readAccess();
	    activeforce = act[3];
	    
	    //cout << "IEIEIEIEIEIEIEIEIEIEIEI " << essai << endl;
	    
	    if (activeforce) {
	      if (initForce) {
		OldPosTracker[0] = avatarX;
		OldPosTracker[1] = avatarY;
		OldPosTracker[2] = avatarZ;
		
		initForce = false;
	      }
	    }
	    else
	      initForce = true;
	    /*	    if (act[1])
		    activecube = !activecube;*/
	    activecube = act[1];
	  }
        }

        // Reception de l'avatar
        if (InAvatar->isConnected())
        {
            rFlowVRModule->get(InAvatar, mAvatar);
            if (mAvatar.data.getSize() > 0)
            {
                positionXOld = positionX;
                positionYOld = positionY;
                positionZOld = positionZ;

                Mat4x4f aux = *(Mat4x4f *)mAvatar.data.readAccess();
                for (int i=0; i<3; i++)
                    for (int j=0; j<3; j++)
                        mConeGL[i*4+j] = aux[j][i];

                positionX = aux[0][3];
                positionY = aux[1][3];
                positionZ = aux[2][3];

                aux[0][3] = aux[0][3] - positionXOld;
                aux[1][3] = aux[1][3] - positionYOld;
                aux[2][3] = aux[2][3] - positionZOld;

                AvatarTranslationX(xPhantom*aux[0][3]);
                AvatarTranslationY(yPhantom*aux[1][3]);
                AvatarTranslationZ(zPhantom*aux[2][3]);
            }
        }
	else
	  avatarOn=false;

        //std::cout<<"Avatar ok."<<std::endl;
        // Reception des mouvements de camera
        if (InCamera->isConnected())
        {
            rFlowVRModule->get(InCamera, mCamera);



            if (mCamera.data.getSize() > 0)
            {

                float * aux = (float*)mCamera.data.readAccess();

                if (aux[0] > 0) {
		  if (activecube) {
		    AvatarTranslationX(0.5);
		  }
		  else {
                    CameraTranslationX(-0.1);
		  }
		}
                else if (aux[0] < 0) {
		  if (activecube)
		    AvatarTranslationX(-0.5);
		  else
                    CameraTranslationX(0.1);
		}
		
                if (aux[1] > 0) {
		  if (activecube)
		    AvatarTranslationY(-0.5);
		  else
                    CameraTranslationY(0.1);
		}
		else if (aux[1] < 0) {
		  if (activecube)
		    AvatarTranslationY(0.5);
		  else
		    CameraTranslationY(-0.1);
		}
		
                if (aux[2] > 0) {
		  if (!activecube)
                    CameraZoom(-0.1);
		  else
		    AvatarTranslationZ(0.5);
		}
                else if (aux[2] < 0) {
		  if (!activecube)
                    CameraZoom(0.1);
		  else
		    AvatarTranslationZ(-0.5);
		}
		
                if (aux[3] > 0) {
		  if (!activecube)
		    CameraRotationY(pi/100);
		}
		else if (aux[3] < 0) {
		  if (!activecube)
		    CameraRotationY(-pi/100);
		}
		
                if (aux[4] > 0) {
		  if (!activecube)
                    CameraRotationX(-pi/100);
		}
                else if (aux[4] < 0) {
		  if (!activecube)
                    CameraRotationX(pi/100);
		}
                if (aux[5] > 0) {
		  if (!activecube)
                    CameraRotationZ(-pi/100);
		}
                else if (aux[5] < 0) {
		  if (!activecube)
                    CameraRotationZ(pi/100);
		}
            }
        }
        //std::cout<<"Camera ok."<<std::endl;

        // Réception des atomes sélectionnés
        rFlowVRModule->get(pSelect, mSelect);
        nbselections = mSelect.data.getSize() / sizeof(unsigned);

        if (nbselections != 0)
            selections = (int*) mSelect.data.readAccess();

        shaders.updateSelection(selections,nbselections,dataAtoms);
        //std::cout<<"Shaders ok."<<std::endl;

        // Réception de l'atome sélectionnable
        rFlowVRModule->get(pSelectable,MsgSelectable);

        if (MsgSelectable.data.getSize() > 0)
            Selectable = *((int*)MsgSelectable.data.readAccess());
        //std::cout<<"Atome selectionne ok ("<<Selectable<<")"<<std::endl;

        // Mise à jour du menu
        if (nbselections != 0)
        {
            gGDeselector->deselectedTemporaryAllAtoms();
            for(unsigned int i = 0; i < nbselections; i++)
            {
                Vector3D colors[3]={Vector3D(0.89,0.91,0.92),
                                    Vector3D(0.43,0.43,0.42),
                                    Vector3D(0.0,0.0,0.0)};

                MyAtom newAtom = MyAtom(selections[i],
                                        Vector3D(0.0,0.0,0.0),
                                        Vector3D(gGDeselector->getWidthMenu()+gGDeselector->getSpaceBetweenButton(),
                                                 20.0,0.0),colors);
                gGDeselector->saveAtom (newAtom);// save the new atom into the menu
            }

            gGDeselector->updateAtomsList(); // update the list of atoms into the menu (delete old atoms...)
        }
        else if (!gGDeselector->isEmpty()) // nbSelection == 0, so ...
            gGDeselector->clearAtoms();

        // Réception de la force
        rFlowVRModule->get(InForce,mForce);

        /*if (mForce.data.getSize()>0) {
            ff->ForceMsg(mForce);
            force = ff->getFFForce();
            activeforce = true;
        }
        else*/
            activeforce = false;
        //std::cout<<"Reception force ok."<<std::endl;
        
        if (InShaders->isConnected())
        {
            rFlowVRModule->get(InShaders, MsgShaders);
            if (MsgShaders.data.getSize() > 0)
            {
                oldShrink = newShrink;
                oldAtomScale = newAtomScale;
                oldBondScale = newBondScale;

                double* t = (double*) MsgShaders.data.readAccess();

                newShrink = t[0];
                newAtomScale = t[1];
                newBondScale = t[2];

                if (oldShrink != newShrink)
                    shaders.updateShrinks(newShrink);
                if (oldAtomScale != newAtomScale)
                    shaders.updateAtomScales(newAtomScale);
                if ( oldBondScale != newBondScale)
                    shaders.updateBondScales(newBondScale);
            }
        }
        
        //std::cout<<"Reception shader ok"<<std::endl;

        // Emission des atomes déselectionnés par le menu
        if (nbdeselections == 0)
        {
	  //	  cout << "déselection : envoi d'un message vide" << endl;
	  mGDeselector.data=rFlowVRModule->alloc(0);
	  rFlowVRModule->put(OutGDeselector,mGDeselector);
        }
        else
        {
	  //	  cout << "déselection : envoi d'un message avec " << nbdeselections << " atomes" << endl;
            mGDeselector.data = rFlowVRModule->alloc(sizeof(unsigned)*nbdeselections);    // allocate space for mGDeselector
            memcpy((void*)mGDeselector.data.writeAccess(),deselections, sizeof(unsigned)*nbdeselections); // copy data into the message

	    //	    cout << "envoi de la déselection (n): " << nbdeselections << endl;

	    // for (int i=0; i<nbdeselections; i++)
	    //   cout << deselections[i] << " ";
	    // cout << endl;

            rFlowVRModule->put(OutGDeselector,mGDeselector);
            nbdeselections=0;

        }
        //std::cout<<"Emission deselection."<<std::endl;
        
        if (InPhantomCalibration->isConnected())
        {
            rFlowVRModule->get(InPhantomCalibration, mPhantomCalibration);

            if (mPhantomCalibration.data.getSize() > 0){
                float * ptr = (float*)mPhantomCalibration.data.readAccess();

                xPhantom = ptr[0];
                yPhantom = ptr[1];
                zPhantom = ptr[2];

            }
        }
        //std::cout<<"Phantom Calibration ok."<<std::endl;

        if(InTargets->isConnected())
        {
            flowvr::Message mTargets;
            rFlowVRModule->get(InTargets, mTargets);

            if(mTargets.data.getSize() > 0){
                nbTargets = mTargets.data.getSize() / (3 * sizeof(float));
                if(!posTargets)
                    posTargets = (float*)malloc(nbTargets*3*sizeof(float));
                memcpy(posTargets,mTargets.data.readAccess(),nbTargets*3*sizeof(float));

            }
        }

        if(InSelectionPos->isConnected())
        {
            flowvr::Message mSelectionPos;
            rFlowVRModule->get(InSelectionPos, mSelectionPos);

            if(mSelectionPos.data.getSize() > 0){
                if(!posSelection)
                    posSelection = (float*)malloc(3*sizeof(float));
                memcpy(posSelection, mSelectionPos.data.readAccess(), 3*sizeof(float));
            }
        }

        if(InBBoxBase->isConnected())
        {
            rFlowVRModule->get(InBBoxBase, msgBBoxBase);

            if(msgBBoxBase.data.getSize() > 0)
            {
                if(BoxBase == NULL) BoxBase = (BBoxtype*)(malloc(msgBBoxBase.data.getSize()));
                memcpy(BoxBase, msgBBoxBase.data.readAccess(), msgBBoxBase.data.getSize());
                nbBoxBase = msgBBoxBase.data.getSize() / sizeof(BBoxtype);
            }
        }

        if(InBBoxRedistribute->isConnected())
        {
            rFlowVRModule->get(InBBoxRedistribute, msgBBoxRedistribute);

            if(msgBBoxRedistribute.data.getSize() > 0)
            {
                if(BoxRedistribute == NULL) BoxRedistribute = (BBoxtype*)(malloc(msgBBoxRedistribute.data.getSize()));
                memcpy(BoxRedistribute, msgBBoxRedistribute.data.readAccess(), msgBBoxRedistribute.data.getSize());
                nbBoxRedistribute = msgBBoxRedistribute.data.getSize() / sizeof(BBoxtype);
                if(color == NULL){
                    color = (float*)malloc(nbBoxRedistribute * 3 * sizeof(float));
                    for(int i = 0; i < nbBoxRedistribute*3; i++)
                        color[i] = (float)rand() / (float)RAND_MAX;
                }
            }
        }

        if(InGridGlobal->isConnected())
        {
            rFlowVRModule->get(InGridGlobal, msgGridGlobal);
            if(msgGridGlobal.data.getSize() > 0)
            {
                if(gridGlobal) free(gridGlobal);
                gridGlobal = (float*)(malloc(msgGridGlobal.data.getSize()));
                memcpy(gridGlobal, msgGridGlobal.data.readAccess(), msgGridGlobal.data.getSize());
                nbCellsGlobal = msgGridGlobal.data.getSize() / (6*sizeof(float));
            }
        }

        if(InGridActive->isConnected())
        {
            rFlowVRModule->get(InGridActive, msgGridActive);
            if(msgGridActive.data.getSize() > 0)
            {
                if(gridActive) free(gridActive);
                gridActive = (float*)(malloc(msgGridActive.data.getSize()));
                memcpy(gridActive, msgGridActive.data.readAccess(), msgGridActive.data.getSize());
                nbCellsActive = msgGridActive.data.getSize() / (6*sizeof(float));
            }
        }

        if(InMesh->isConnected())
        {
            rFlowVRModule->get(InMesh, msgMesh);
            if(msgMesh.data.getSize() > 0)
            {
                if(mesh) free(mesh);
                mesh = (float*)(malloc(msgMesh.data.getSize()));
                memcpy(mesh, msgMesh.data.readAccess(), msgMesh.data.getSize());
                nbTriangles = msgMesh.data.getSize() / (9 * sizeof(float));
            }
        }
        
        if(InBestPos->isConnected())
        {
	  //            std::cout << "ENTERED IN BEST POV" << std::endl;
            rFlowVRModule->get(InBestPos, msgBestPos);
            if(msgBestPos.data.getSize() > 0)
            {
                camera_position = (double*)malloc(6*sizeof(double));
                memcpy(camera_position, msgBestPos.data.readAccess(), 6*sizeof(double));
                camera_position[0] -= xCenter;
                camera_position[1] -= yCenter;
                camera_position[2] -= zCenter;
                camera_position[3] -= xCenter;
                camera_position[4] -= yCenter;
                camera_position[5] -= zCenter;

                // cout << "Recenter : " << xCenter << " " << yCenter << " " << zCenter << endl;
                // std::cout << "Camera best position: " << camera_position[0] << " " << camera_position[1] << " " << camera_position[2] << std::endl;
                // std::cout << "Target position: " << camera_position[3] << " " << camera_position[4] << " " << camera_position[5] << std::endl;
            }
        }
        
        
        float tab[3];
        tab[0] = avatarX+xCenter;
        tab[1] = avatarY+yCenter;
        tab[2] = avatarZ+zCenter;

        MAvatar.data=rFlowVRModule->alloc(3*sizeof(float));
        memcpy((void*)MAvatar.data.writeAccess(),tab,3*sizeof(float));
        rFlowVRModule->put(OutAvatar,MAvatar);
        //std::cout<<"position de l'avatar ("<<tab[0]<<","<<tab[1]<<","<<tab[2]<<")"<<std::endl;
        //std::cout<<"Fin de l'iteration."<<std::endl;
        
    }
    else
    {
        CleanFlowVR();
        delete gGDeselector; gGDeselector=NULL;
        exit(0);
    }
    glutSetWindow(win_id);
}

void makeFullScreen()
{
    if(!fullscreen){
        glutFullScreen();
    }
    else {
        glutReshapeWindow(windowWidth,windowHeight);
        glutPositionWindow(0,0);
    }

    fullscreen = !fullscreen;
}

void TouchesAppui(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_RIGHT:
        CameraRotationY(pi/10);
        break;
    case GLUT_KEY_LEFT:
        CameraRotationY(-pi/10);
        break;
    case GLUT_KEY_UP:
        CameraRotationX(pi/10);
        break;
    case GLUT_KEY_DOWN:
        CameraRotationX(-pi/10);
        break;
    case GLUT_KEY_PAGE_UP:
        CameraZoom(-0.5);
        break;
    case GLUT_KEY_PAGE_DOWN:
        CameraZoom(0.5);
        break;
    case GLUT_KEY_F1:
        CameraTranslationX(-0.5);
        break;
    case GLUT_KEY_F2:
        CameraTranslationX(0.5);
        break;
    case GLUT_KEY_F3:
        CameraTranslationY(0.5);
        break;
    case GLUT_KEY_F4:
        CameraTranslationY(-0.5);
        break;
    case GLUT_KEY_F5:
        AvatarTranslationX(0.5);
        break;
    case GLUT_KEY_F6:
        AvatarTranslationX(-0.5);
        break;
    case GLUT_KEY_F7:
        AvatarTranslationY(0.5);
        break;
    case GLUT_KEY_F8:
        AvatarTranslationY(-0.5);
        break;
    case GLUT_KEY_F9:
        AvatarTranslationZ(-0.5);
        break;
    case GLUT_KEY_F10:
        AvatarTranslationZ(0.5);
        break;
    case GLUT_KEY_F11:
        makeFullScreen();
        break;
    }
}

void MotionFunc (int x, int y)
{
  if (middlebutton) {
    if (abs(y-mouseYOld) > abs(x-mouseXOld)) { 
      zTrans=(y-mouseYOld)*stepTrans/50;
    }
    else { 
      zTrans=(x-mouseXOld)*stepTrans/50;
    }
    CameraZoom(zTrans);    
    mouseXOld=x;
    mouseYOld=y;   
  }
  else if (rightbutton) {
    xTrans=(x-mouseXOld)*stepTrans/50;
    yTrans=(y-mouseYOld)*stepTrans/50;
    CameraTranslationX(xTrans);       
    CameraTranslationY(yTrans);       
    mouseXOld=x;
    mouseYOld=y;
    
  }
  else if (leftbutton){
    yRot=(x-mouseXOld)*stepTrans/50;
    xRot=(y-mouseYOld)*stepTrans/50;                
    CameraRotationY(yRot);
    CameraRotationX(xRot);
    mouseXOld=x;
    mouseYOld=y;
  }
}


void MouseFunc (int button, int state, int xMouse, int yMouse) 
{
  MyObject object;

  switch (button) {

  case GLUT_RIGHT_BUTTON:
    if (state==GLUT_DOWN) {
      rightbutton=true;
      mouseXOld = xMouse;
      mouseYOld = yMouse;
    }
    else
      rightbutton=false;
    break;
    
  case GLUT_MIDDLE_BUTTON :
    if (state==GLUT_DOWN) {
      middlebutton=true;
      mouseXOld = xMouse;
      mouseYOld = yMouse;
    }
    else
      middlebutton=false;
    break;    
  case GLUT_LEFT_BUTTON:
    object = gGDeselector->whichButtons( (double)xMouse, (double)yMouse );
    
    if (state==GLUT_DOWN) {
      leftbutton=true;
      mouseXOld = xMouse;
      mouseYOld = yMouse;
      if ( object.getType() == eValid ) {
	AtomButtons atomsDeselected = gGDeselector->addSelectedAtoms();
	if ( !atomsDeselected.empty() ) {
	  nbdeselections = atomsDeselected.size();
	  deselections = new unsigned [nbdeselections];
	  int k=0;
	  for (AtomButtons::iterator it=atomsDeselected.begin(); it!=atomsDeselected.end(); it++) 
	    deselections[k++] = it->first;
	  gGDeselector->doAction ( object, true );
	}	
      }
      else if (object.getType() != eNull)
	gGDeselector->doAction ( object, true, xMouse, yMouse );
    }
    else {
      leftbutton=false;
      //      if (object.getType() != eNull)
      gGDeselector->releaseAllButtons();
    }
    break;
  }
}


void ReshapeFunc(int width, int height)
{
    glutSetWindow(win_id);
    
    windowWidth = width;
    windowHeight = height;
    
    update3DWindow();
    
    //   glutReshapeWindow(width, height);
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70,(GLfloat)width/(GLfloat)height,0.1,5000);
    
    InitGDeselector(width,height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ToucheKeyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'a':
        displayBox = !displayBox;
        break;
    case 'z':
        displayDensityGrid = !displayDensityGrid;
        break;
    case 'e':
        displayQuickSurf = !displayQuickSurf;
        break;
    }
}


void OpenGlutWindow(int posx, int posy, int width, int height)
{
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    
    glutInitWindowPosition(posx, posy);
    glutInitWindowSize(width, height);
    win_id = glutCreateWindow("Vitamins");
    
    InitDisplay(width, height);
    
    glutReshapeFunc(ReshapeFunc);
    glutIdleFunc(IdleFunc);
    glutDisplayFunc(DisplayFunc);
    glutSpecialFunc(&TouchesAppui);
    glutKeyboardFunc(&ToucheKeyboard);
    glutMouseFunc(&MouseFunc);
    glutMotionFunc(&MotionFunc);
}

// --------------------------------------------------------------------
// Main routine
// --------------------------------------------------------------------

int main(int argc, char** argv) {
  //  std::cout << "main" << std::endl;

    srand(23432423);
    InShaders->setNonBlockingFlag(true);
    //pPos->setNonBlockingFlag(true);
    InAtoms->setNonBlockingFlag(true);
    InBonds->setNonBlockingFlag(true);
    InBBox->setNonBlockingFlag(true);
    InSelectionMode->setNonBlockingFlag(true);
    InParams3D->setNonBlockingFlag(true);
    InPhantomCalibration->setNonBlockingFlag(true);
    InBestPos->setNonBlockingFlag(true);
    InMatrix->setNonBlockingFlag(true);

    envpath = getenv("VITAMINS_DATA_PATH");
    if(envpath == NULL)
    {
        std::cout << "VITAMINS_DATA_PATH has not been set, please add vitamins-config.sh to your bashrc and bash_profile" << std::endl;
        return 1;
    }

    if (argc > 1) // Main parameters handling, to limit fps
    {
        fps = atoi(argv[1]);
        if (fps <= 0)
            maxElapsedMS = 0;
        else
            maxElapsedMS = 1000 / fps;
    }
    else
        maxElapsedMS = 0;

    srand ( time(NULL) );
    std::vector <flowvr::Port*> ports;
    ports.push_back(pPos);
    ports.push_back(InBonds);
    ports.push_back(InAtoms);
    ports.push_back(InBBox);
    ports.push_back(pSelect);
    ports.push_back(pSelectable);
    ports.push_back(InForce);
    ports.push_back(InShaders);
    ports.push_back(InCamera);
    ports.push_back(InAvatar);
    ports.push_back(InActivation);
    ports.push_back(InSelectionMode);
    ports.push_back(InPhantomCalibration);
    ports.push_back(InParams3D);
    ports.push_back(InTargets);
    ports.push_back(InSelectionPos);
    ports.push_back(InBBoxBase);
    ports.push_back(InBBoxRedistribute);
    ports.push_back(InGridGlobal);
    ports.push_back(InGridActive);
    ports.push_back(InMesh);
    ports.push_back(InMatrix);
    ports.push_back(OutGDeselector);
    ports.push_back(OutAvatar);
    ports.push_back(OutMatrix);
    ports.push_back(InBestPos);

    //ports.push_back(In3DButton);
    ports.push_back(In3DAnalog);

    pPos->stamps->add(&stampPositions);

    InActivation->setNonBlockingFlag(true);
    InCamera->setNonBlockingFlag(true);

    if (!(rFlowVRModule = initModule(ports)))
        return -1;

    fpsTime.start();
    fpsTimeDisplayer.start();

    glutInit(&argc, argv);
    InitAvatar();
    InitBB();
    InitGDeselector(width,height);
    OpenGlutWindow(150,10,width,height);
    //glutSetCursor(GLUT_CURSOR_NONE);
    glutIgnoreKeyRepeat(0);
    //    std::cout << "glut" << std::endl;
    glutMainLoop();
    
    return 0;
}
