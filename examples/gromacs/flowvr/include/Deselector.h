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

#ifndef DESELECTOR_H
#define DESELECTOR_H

#include <iostream>
#include <sstream>
#include <functional>
#include <map>   
#include <string>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "Vector3D.h"

typedef unsigned int uint;

// VARIABLES ----------------------------------------------------------

enum Button
{
    eNull = -1,     // anything
    eAtom = 0,      // an atom
    eMove,          // move the menu on the left/right
    eOpen, eClose,  // open/close menu
    eValid,         // valid deselection
    eClean,         // de-selection of atoms on the list
    eUp, eDown      // slide list of atoms up/down
};
					
// CLASSES ------------------------------------------------------------

// MyObject
class MyObject 
{
public:
	
	MyObject(): _type(eNull), _position(Vector3D(0.0,0.0,0.0)), _dimension(Vector3D(0.0,0.0,0.0)), _state(false), _currentcolor(1)
	{
		for (int i=0; i<3; i++)
			_colors[i] = Vector3D(0.0,0.0,0.0);
	}
	MyObject(Button t, Vector3D p, Vector3D d, Vector3D colors[3]): _type(t), _position(p), _dimension(d), _state(false), _currentcolor(1)
	{
		for (int i=0; i<3; i++)
			_colors[i] = colors[i];
	 }
	~MyObject() {}
	
	// accesseurs
	Button getType() const            { return _type; }
	Vector3D getPosition() const      { return _position; }
	Vector3D getDimension() const     { return _dimension; }
	bool getState() const             { return _state; }
	virtual Vector3D getButtonColor() const { return _colors[_state]; }
	Vector3D getTextColor() const     { return _colors[2]; }
	
	// mutateurs
	void setType(Button t)        { _type = t; }
	void setPosition(Vector3D p)  { if(p.getX() >= 0 && p.getY() >=0)  _position = p; }
	void setDimension(Vector3D d) { if(d.getX() >= 0 && d.getY() >=0) _dimension = d; }
	void setState(bool state)     { _state = state; }
	void setColor(Vector3D color, unsigned int i) { if(i>=0 and i<=2) _colors[i] = color; }

protected:

	// constantes
	Vector3D _colors[3]; 				// [1] color : not pressed, [2] : pressed, [3] : text
	
	// attributs
	Button _type;        // atom or action
	Vector3D _position;  // (x,y) : top left corner of the button
	Vector3D _dimension; // x:width, y:height of the button
	bool _state;         // pressed or not pressed ?
	unsigned int _currentcolor;  // button color	
	
};

// MyAtoms
class MyAtom : public MyObject 
{
public:
	
	MyAtom():MyObject(),_value(0), _selected(false){}
	MyAtom(unsigned int value, Vector3D p, Vector3D d, Vector3D colors[3]): MyObject(eAtom, p, d, colors), _value(value), _selected(false){} // at min : the value of the atom and its position
	~MyAtom(){}
		
	// Accesseur
	unsigned int getValue() { return _value; }
	bool isSelected() { return _selected; }
	
	// Mutateur
	void setValue(unsigned int val) { if(val >=0) _value = val; }
	void swapSelected () { _selected = !_selected; } // swap variable
	
protected:

	// additional
	unsigned int _value; // identification number of the atom
	bool _selected;
};

// MyAction
class MyAction : public MyObject 
{
public:
	
	MyAction():MyObject(),_value("unknown"){}
	MyAction(std::string value, Button t, Vector3D p, Vector3D d, Vector3D colors[3]):MyObject(t, p, d, colors), _value(value){}
	~MyAction(){}
		
	// Accesseur
	std::string getValue() { return _value; } 
		
	// Mutateur
	void setValue(std::string val) { if(val != "") _value = val; }
	
protected:
	
	// additional
	std::string _value; // identification number of the action
};


// USEFUL OPERATIONS --- ----------------------------------------------

template <typename N> // need include <sstream>
std::string toString (N n)
{
    std::ostringstream oss; // créer un flux de sortie
    oss << n; 				// écrire un nombre dans le flux
    return oss.str(); 		// récupérer une chaîne de caractères
}

// --------------------------------------------------------------------
typedef std::map<unsigned int,MyAtom> AtomButtons;
typedef std::map<Button,MyAction> ActionButtons;
// --------------------------------------------------------------------


// Deselector
class Deselector
{	
protected:
	
	// constantes
	double _spaceBetweenButton; /*!< space between 2 buttons (in pixels) */
	double _widthMenu; /*!< width of menu */
	double _heightMenu; /*!< height of menu */
			
	// attributes
	Vector3D _positionMenu;       /*!< position 'top left corner' of the menu on the screen */ 
	Vector3D _positionFirstAtom;  /*!< position of the first atom on the screen (x,y) used to know the position of others atoms*/
	double _widthWin, _heightWin; /*!< dimensions of the window */
	bool _move;    				  /*!< menu is moving ? */
    bool _visible; 				  /*!< menu visible ? or hide ? */
	
	//~ typedef std::map<Button,MyAction> ActionButtons;
    ActionButtons _actionButtons; /*!< a map to manage actions on the menu => key:type of action | objet:action */
    
    //~ typedef std::map<unsigned int,MyAtom> AtomButtons;
    AtomButtons _atomButtons;     /*!< a map to manage atoms selected in the simulation => key:atom's number | objet:atom */
    unsigned int _firstAtomDisplayed;     /*!< the identification number of the first atom displayed */

	// operations

	// ACTION BUTTONS
	virtual void onButtonAtom ( MyAtom atom , bool state ); // update state of the button **
	virtual void onButtonMove ( bool state ); // update state of the button + variable "_move" **
	virtual void onButtonValid( bool state ); // delete all selected atoms on the list + update state of the button + update atoms positions on the screen **
	virtual void onButtonClean( bool state ); // de-selection all selected atoms into the list (on the screen) **
	virtual void onButtonUp   ( bool state ); // increment variable _firstAtomDisplayed (if atom != first atom on the list) + update positions of atoms on the screen + update state of the button **
	virtual void onButtonDown ( bool state ); // decrement variable _firstAtomDisplayed (if atom != last atom on the list) + update positions of atoms on the screen + update state of the button **
	virtual void onButtonOpen ( bool state ); // variable _visible = true + update state of the button **
	virtual void onButtonClose( bool state ); // variable _visible = false + update state of the button **

	// OpenGL METHODS
	virtual void drawButton( MyObject button, std::string value ) const; // draw an OpenGL button via its dimensions **
	virtual void drawActionButton( MyAction button ) const; // draw an OpenGL button via its dimensions **
	virtual void drawAtomButton( MyAtom button ) const;     // draw an OpenGL button via its dimensions **
	
	// UPDATE METHODS
	virtual void updateActionsPosition(); // updates "action buttons" position when the menu has been moved **
	virtual void updateAtomsPosition();   // updates "atoms  buttons" position when the menu has been moved OR some atoms have been : deleted / inserted **
	
	// INITIALIZATION OF ACTION BUTTONS
	virtual void initButtonsMenu(); // set the buttons **
	
public:

	Deselector( double w, double h ); // initialize "move","_visible" + map of actions + "_positionMenu","_positionFirstAtom" **
	virtual ~Deselector(); // **
    
    // MAIN METHODS
    virtual MyObject whichButtons( double x, double y ); // obtain button pressed according to the position on the screen (X,Y) in pixels **
 	virtual void doAction( MyObject myObj, bool state , unsigned int x=0, unsigned int y=0); // execute action according to the button pressed **
    virtual void displayMenu(); // display menu (raz, initialization) **
    
	// UPDATE POSITION/STATE OF THE MENU,BUTTONS...
	virtual void updatePositionMenu( double x, double y ); // update the position of the menu on the screen + positions of atoms / actions **
	virtual void updateSizeWindow ( double w, double h );  // update the size of the window and also the positions of menu **
	virtual void updateAtomsList(); // clean the list of atoms
	virtual void releaseAllButtons (); // put all variables _state = false
	
	// FOR ATOMS
	virtual void saveAtom(MyAtom atom);     // save a new selected atom in the list + update position of atoms into the list + update variable "_firstAtomDisplayed"**
	virtual void clearAtoms(); // clean the map of atoms buttons
	virtual void deselectedTemporaryAllAtoms(); // We put all atoms into the map with the type = eNull. It is use with the method "updateListAtom" with will delete all atoms YET with a eNull value (old Atoms)
	virtual bool isEmpty(); // return true if the numbers of atoms is NOT NULL **
	// ---------
	virtual AtomButtons addSelectedAtoms(); // return a map with selected atoms only **
	virtual MyAtom addPressedAtom(unsigned int x, unsigned int y); // obtain the atom which has been pressed **
	
	// ACCESSEURS
	virtual bool isMoving() const;  // indicates if the menu is moving by user or not **
	virtual bool isVisible() const; // indicates if the menu is closed or not ** PROTECTED OR PUBLIC ?!
	virtual double getSpaceBetweenButton() const; // return the variable "_spaceBetweenButton" **
	virtual double getWidthMenu() const; // return the width of the menu **
	
	// MUTATEURS
	virtual void setWidthMenu(double w); // set a new width of the menu **
	virtual void setHeightMenu(double h); // set a new height of the menu **
	
	// STATIC METHODS
	static void drawText(Vector3D position, Vector3D color, std::string text); // draw text on the screen **
};

#endif // DESELECTOR_H
