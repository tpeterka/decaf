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
#include <Deselector.h>

#include <stdexcept>

using namespace std;


// PUBLIC -------------------------------------------------------------

/**
* @brief Constructor
* @param w Width of the window
* @param h Height of the window
*/
Deselector::Deselector(double w, double h): _widthWin(w), _heightWin(h)
{
	// at the beginning ...
	_spaceBetweenButton = 4;
	_widthMenu =  200 + _spaceBetweenButton;
	_heightMenu = 240;
	
	_visible = true;//menu is displayed
	_move = false;  // we don't move the menu
	_firstAtomDisplayed = 0; // we don't have atom
		
	// menu position
	_positionMenu.setX ( _widthWin - _widthMenu - _spaceBetweenButton );
	_positionMenu.setY ( _spaceBetweenButton );
	
	// initialization of map "_actionButtons"
	initButtonsMenu();		
}

/**
 * @brief Destructor - destruction of maps Actions and Atoms
 */
Deselector::~Deselector()
{
	if ( !_atomButtons.empty() )
		_atomButtons.clear();
	
	if ( !_actionButtons.empty() )
		_actionButtons.clear();
}

// --------------------------------------------------------------------

/**
 * @brief set the buttons
 */
void Deselector::initButtonsMenu()
{
	// "move" button
		Vector3D p = _positionMenu;
		Vector3D d = Vector3D(_widthMenu/4.0 ,_heightMenu/8.0,0.0);
		Vector3D colors[3] = { Vector3D(0.85,0.85,0.85),Vector3D(0.68,0.68,0.68),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction move ( "<=>",eMove, p, d, colors );
		_actionButtons.insert( pair<Button,MyAction>( move.getType(), move ) );
		
		// "open" button
		p = Vector3D(move.getPosition().getX()+move.getDimension().getX()+_spaceBetweenButton, move.getPosition().getY(), 0.0);
		d = Vector3D(_widthMenu/(4.0/3.0) ,_heightMenu/8.0,0.0);
		Vector3D colors2[3] = { Vector3D(0.50,0.87,0.49),Vector3D(0.35,0.65,0.34),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction open ( "OPEN",eOpen, p, d, colors2 );
		_actionButtons.insert( pair<Button,MyAction>( open.getType(), open ) );
		
		// "close" button - same position as "open" button (according to _visible, we will see "open" or "close" button on the screen)
		p = Vector3D(move.getPosition().getX()+move.getDimension().getX()+_spaceBetweenButton, move.getPosition().getY(), 0.0);
		d = Vector3D(_widthMenu/(4.0/3.0) ,_heightMenu/8.0,0.0);
		Vector3D colors3[3] = { Vector3D(0.82,0.33,0.43),Vector3D(0.80,0.09,0.25),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction close ( "CLOSE",eClose, p, d, colors3 );
		_actionButtons.insert( pair<Button,MyAction>( close.getType(), close ) );
	
		// "Valid" button
		p = Vector3D(move.getPosition().getX(), move.getPosition().getY()+move.getDimension().getY()+_spaceBetweenButton, 0.0);
		d = Vector3D(_widthMenu/2.0,_heightMenu/8.0,0.0);
		Vector3D colors4[3] = { Vector3D(0.61,0.92,0.39),Vector3D(0.51,0.74,0.34),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction valid ( "VALID",eValid, p, d, colors4 );
		_actionButtons.insert( pair<Button,MyAction>( valid.getType(), valid ) );
		
		// "clean" button
		p = Vector3D(valid.getPosition().getX()+valid.getDimension().getX()+_spaceBetweenButton, valid.getPosition().getY(), 0.0);
		d = Vector3D(_widthMenu/2.0 ,_heightMenu/8.0,0.0);
		Vector3D colors5[3] = { Vector3D(0.96,0.92,0.40),Vector3D(0.89,0.83,0.14),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction clean ( "CLEAN",eClean, p, d, colors5 );
		_actionButtons.insert( pair<Button,MyAction>( clean.getType(), clean ) );
	
		// "up" button
		p = Vector3D(valid.getPosition().getX(), valid.getPosition().getY()+valid.getDimension().getY()+_spaceBetweenButton, 0.0);
		d = Vector3D(_widthMenu/2.0 ,_heightMenu/8.0,0.0);
		Vector3D colors6[3] = { Vector3D(0.48,0.83,0.96),Vector3D(0.29,0.65,0.79),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction up ( "UP",eUp, p, d, colors6 );
		_actionButtons.insert( pair<Button,MyAction>( up.getType(), up ) );

		// "down" button
		p = Vector3D(up.getPosition().getX()+up.getDimension().getX()+_spaceBetweenButton, up.getPosition().getY(), 0.0);
		d = Vector3D(_widthMenu/2.0 ,_heightMenu/8.0,0.0);
		Vector3D colors7[3] = { Vector3D(0.89,0.61,0.94),Vector3D(0.82,0.43,0.88),Vector3D(0.0,0.0,0.0) }; // not pressed, pressed, text
		MyAction down ( "DOWN",eDown, p, d, colors7 );
		_actionButtons.insert( pair<Button,MyAction>( down.getType(), down ) );	
	
	// initialization of variable "positionFirstAtom"
	_positionFirstAtom.setX(up.getPosition().getX());
	_positionFirstAtom.setY(up.getPosition().getY()+up.getDimension().getY()+(2*_spaceBetweenButton));
} 

/**
 * @brief obtain the button pressed according to the position on the screen (X,Y) in pixels
 * @param x the x coordinate on the screen (in pixel)
 * @param y the y coordinate on the screen (in pixel)
 * @return MyObject which can be an action or an atom
 */
MyObject Deselector::whichButtons( double x, double y )
{
  // search if we press an action button ?
  for ( ActionButtons::iterator it = _actionButtons.begin(); it != _actionButtons.end(); it++ ) {
    if ( ( x >= it->second.getPosition().getX() and x <= it->second.getPosition().getX()+it->second.getDimension().getX() ) // X coordinate
	 and
	 ( y >= it->second.getPosition().getY() and y <= it->second.getPosition().getY()+it->second.getDimension().getY() ) ) // Y coordinate
      {
	if ( isVisible() ) // to avoid tested "eOpen" when menu visible
	  {
	    if ( it->first != eOpen)
	      return it->second;
	  }
	else if ( it->first == eMove or it->first == eOpen ) // ONLY eMove && eOpen
	  return it->second;		
      }
  }
	
	// if not, browse the list of atoms in order to find which is pressed on the screen !
  for ( AtomButtons::iterator it = _atomButtons.begin(); it != _atomButtons.end(); it++ )
    {
      if ( ( x >= it->second.getPosition().getX() and x <= it->second.getPosition().getX()+it->second.getDimension().getX() ) // X coordinate
	   and
	   ( y >= it->second.getPosition().getY() and y <= it->second.getPosition().getY()+it->second.getDimension().getY() ) ) // Y coordinate
	{
	  cout << "atome sélectionné : " << (it->second).getValue() << endl;
	  return it->second;
	  
	}
    }
  MyObject Vide;
  return Vide;
    throw std::runtime_error("Deselector::whichButtons did not found a button");
}

/**
 * @brief execute an action according to the button pressed
 * @param myObj is the event on the menu (action or atom)
 * @param state is the state of the button (pressed or released)
 */
void Deselector::doAction( MyObject myObj, bool state , uint x, uint y)
{
	// if the menu is not visible => only the action "Open" is authorized
	if ( isVisible() == false )
	{
		switch ( myObj.getType() ) // according to the button pressed
		{
			case eOpen :
				onButtonOpen ( state );
				break;		
			case eMove :
				onButtonMove ( state );
				break;				
			default:
				break;
		}
	}
	else // if the menu is visible => all actions are authorized ( except "eOpen" because the menu is already open ) 
	{
		MyAtom atom;
		
		switch ( myObj.getType() ) // according to the button pressed
		{
			case eAtom :
				atom = addPressedAtom(x,y);
				onButtonAtom  ( atom , state );
				break;		
			case eMove :
				onButtonMove  ( state );
				break;				
			case eClose :
				onButtonClose ( state );
				break;
			case eValid :
				onButtonValid ( state );
				break;
			case eClean:
				onButtonClean ( state );
				break;
			case eUp :
				onButtonUp    ( state );
				break;
			case eDown :
				onButtonDown  ( state );
				break;
			default:
				break;	
		}
	}
	// UPDATE 'MENU DISPLAY'
	//displayMenu();
}

/**
 * @brief display menu (raz, initialization)
 */
void Deselector::displayMenu()
{
	// display map : _actionButtons
	if ( isVisible() == false ) // only display bouton open/move
	{
		drawActionButton ( _actionButtons[eMove] );
		drawActionButton ( _actionButtons[eOpen] );
	}
	else
	{
		for ( ActionButtons::iterator it = _actionButtons.begin(); it != _actionButtons.end(); it++ )
		{
			if ( it->first != eOpen )
				drawActionButton ( it->second );
		}
		
		// display map : _atomButtons FROM _firstAtomDisplayed -> end of the map !!!!!
		for ( AtomButtons::iterator it = _atomButtons.find(_firstAtomDisplayed); it != _atomButtons.end(); it++ )
		{
			drawAtomButton ( it->second );
		}
	}
}

// --------------------------------------------------------------------

/**
 * @brief browse into the map and return selected atoms into a new map
 * @return return a map with selected atoms only
 */
AtomButtons Deselector::addSelectedAtoms()
{
	AtomButtons res;
	
	for ( AtomButtons::iterator it = _atomButtons.begin(); it != _atomButtons.end(); it++ )
    {
    	if ( it->second.isSelected() == true ) // if atom selected ?
    		res.insert( pair<uint,MyAtom>( it->first,it->second ) ); // insert it into the map
	}
	
	return res;
}

/**
 * @brief size of the map _atomButtons
 * @return return true if the numbers of atoms is not null, else return false
 */
bool Deselector::isEmpty()
{
	return _atomButtons.empty();
}

/**
 * @brief We put all atoms into the map with the type = eNull. It is use with the method "updateListAtom" with will delete all atoms YET with a eNull value (old Atoms NOT SELECTED)
 */	
void Deselector::deselectedTemporaryAllAtoms()
{
	for ( AtomButtons::iterator it=_atomButtons.begin(); it!=_atomButtons.end(); it++)
		it->second.setType(eNull);
}
	
/**
 * @brief save a new selected atom into the list + update position of atom into the screen + update variable "_firstAtomDisplayed"
 * @param atom is the element inserted into the list
 */	
void Deselector::saveAtom(MyAtom atom)
{
	if ( _atomButtons.find(atom.getValue() ) == _atomButtons.end() ) // new atom not into the list ?
	{
		_atomButtons.insert( pair<uint,MyAtom>( atom.getValue(),atom ) ); // inserts the atom into the list
		
		//~ if ( _firstAtomDisplayed == 0 )
		_firstAtomDisplayed = _atomButtons.begin()->first;		
		
		updateAtomsPosition(); // update position of atoms on the screen
	}
	else // if atom already present, we put variable "type" of the atom at "eAtom" because before, the value "type" of each atoms into the list was at "eNull"
		_atomButtons[atom.getValue()].setType(eAtom);
}

/**
 * @brief obtain the atom which has been pressed
 * @param x is the abscissa where the user has clicked
 * @param y is the ordinate where the user has clicked
 */
MyAtom Deselector::addPressedAtom(uint x, uint y)
{
	for ( AtomButtons::iterator it = _atomButtons.begin(); it != _atomButtons.end(); it++ )
    {
    	if ( ( x >= it->second.getPosition().getX() and x <= it->second.getPosition().getX()+it->second.getDimension().getX() ) // X coordinate
    		 and
    		 ( y >= it->second.getPosition().getY() and y <= it->second.getPosition().getY()+it->second.getDimension().getY() ) ) // Y coordinate
    	   {
    	   		return it->second;		
		   }
	}
	cout << "Deselection::addPressedAtom did not found anything" << endl;

    throw runtime_error("Deselection::addPressedAtom did not found anything");
}

/**
 * @brief update the position of the menu on the screen IF variable _move = true
 * @param x is the new abscissa of the menu
 * @param y is the new ordinate of the menu
 */ 
void Deselector::updatePositionMenu( double x, double y )
{
	// update of _positionMenu
	_positionMenu.setX(x);
	_positionMenu.setY(y);
	
	// update of position of each buttons on the screen
	updateActionsPosition(); // map "ActionButtons" FIRSTLY !!!
	updateAtomsPosition();   // map "AtomButtons" SECONDLY !!!
}

/**
 * @brief update the size of the window and also the position of the menu
 * @param w is the new width of the window
 * @param h is the new height of the window
 */
void Deselector::updateSizeWindow ( double w, double h )
{
	_widthWin = w;
	_heightWin = h;
	
	updatePositionMenu ( _widthWin - _widthMenu - (3*_spaceBetweenButton), (2*_spaceBetweenButton) ); // updates also the position of atoms/actions	
}

/**
 * @brief put actionButtons with _state=false (buttons released)
 */
void Deselector::releaseAllButtons ()
{
	for ( ActionButtons::iterator it=_actionButtons.begin(); it!=_actionButtons.end(); it++ )
	{
		if ( it->second.getState() == true )
			it->second.setState(false);	
	}
	
	for ( AtomButtons::iterator it=_atomButtons.begin(); it!=_atomButtons.end(); it++ )
	{
		if ( it->second.getState() == true )
			onButtonAtom(it->second , false);
	}
	
	_move = false;
}

/**
 * @brief clean the list of atoms when old atoms were not selected
 */
void Deselector::updateAtomsList()
{
	// iterator not valid after the operation "erase" into a "For" loop
	
	// only 1 atom
	if ( (int)_atomButtons.size() == 1 )
	{
		if ( _atomButtons.begin()->second.getType() == eNull )
		{
			clearAtoms();
			_firstAtomDisplayed = 0;
		}
	}
	else // many atoms into the list ?
	{
		//~AtomButtons::iterator it = std::find_if ( _atomButtons.begin(), _atomButtons.end(), std::bind2nd(first_eNull() , eNull) );
		// AtomButtons::iterator it = std::find_if ( _atomButtons.begin(), _atomButtons.end(), comparator(eNull) );
		
		//~ while ( it != _atomButtons.end() )
		//~ {
			//~ _atomButtons.erase ( it ); // after that, the iterator is invalid
			//~ it = std::find_if ( _atomButtons.begin(), _atomButtons.end(), std::bind2nd(first_eNull(),eNull) ); // we search the new position between it and the end of the map with an atom with its type = eNull
			//it = std::find_if ( _atomButtons.begin(), _atomButtons.end(), comparator(eNull) );
		//~ }
		
		AtomButtons::iterator it = _atomButtons.begin();
		AtomButtons::iterator succ = it++;
		
		while ( it!=_atomButtons.end() )
		{	
			if ( it->second.getType() == eNull )
			{
				// if we erase the first atom displayed -> we must choice another one
				if ( _firstAtomDisplayed == it->first ) 
				{
					_atomButtons.erase ( it );
					_firstAtomDisplayed = _atomButtons.begin()->first;
				}
				else // esle, erase it only
				_atomButtons.erase ( it );
			}
			
			it = succ; // move iterator
			
			if ( succ != _atomButtons.end() ) // update position "succ" iterator if we are not at the end of the map
			{
				succ++;
			}
		}
	}

	//~ _firstAtomDisplayed = _atomButtons.begin()->first;
	updateAtomsPosition(); // update the map "atomButtons" if some of them have been erased, inserted...
}

/**
 * @brief clean the map of atoms
 */
void Deselector::clearAtoms()
{
	_atomButtons.clear();
	_firstAtomDisplayed = 0;
	//~ updateAtomsPosition();
}

// --------------------------------------------------------------------

/**
 * @brief indicates if the menu is moving by user or not
 * @return true if we are moving the menu, else -> false
 */
bool Deselector::isMoving() const
{
	return _move;
}

/**
 * @brief indicates if the menu is closed or not
 * @return true if the menu is visibled ( we have clicked on the button "close" ), else -> false
 */
bool Deselector::isVisible() const
{
	return _visible;
}

/**
 * @brief obtain the space between button
 * @return space between button via "_spaceBetweenButton"
 */
double Deselector::getSpaceBetweenButton() const
{
	return _spaceBetweenButton;
}

/**
 * @brief width of the menu
 * @return the width of the menu
 */
double Deselector::getWidthMenu() const
{
	return _widthMenu;
}
	
/**
 * @brief set a new width of the menu
 * @param w is the new width
 */
void Deselector::setWidthMenu(double w)
{
	if ( w > 0 )
	{
		_widthMenu = w;
	
		// recalculate the dimension of the menu, buttons...
		initButtonsMenu();	
		updatePositionMenu( _widthWin - _widthMenu - (3*_spaceBetweenButton), (2*_spaceBetweenButton));
	}	
}

/**
 * @brief set a new height of the menu
 * @param h is the new height
 */
void Deselector::setHeightMenu(double h)
{
	if ( h > 0 )
	{
		_heightMenu = h;
	
		// recalculate the dimension of the menu, buttons...
		initButtonsMenu();	
		updatePositionMenu( _widthWin - _widthMenu - (3*_spaceBetweenButton), (2*_spaceBetweenButton));
	}	
}


// --------------------------------------------------------------------

/**
 *  @brief draw text in an OpenGL context
 *  @param position is the specific position of the text on the screen (pixels)
 *  @param color is the color of the text
 *  @param text is the text written
 */
void Deselector::drawText(Vector3D position, Vector3D color, std::string text)
{

 	// set a convenient projection
  	glMatrixMode(GL_PROJECTION);
	  	glPushMatrix();
	  	glLoadIdentity();
  		glOrtho(0.0, glutGet(GLUT_WINDOW_WIDTH), 0.0, glutGet(GLUT_WINDOW_HEIGHT), -1.0, 1.0);
  
  	// draw text
  	glMatrixMode(GL_MODELVIEW);
	  	glPushMatrix();
  		glPushAttrib(GL_CURRENT_BIT); // save current color | size
  		glLoadIdentity();
  
  	glColor3d(color.getX(), color.getY(), color.getZ()); // color of text
  	glRasterPos3d(position.getX(), glutGet(GLUT_WINDOW_HEIGHT) - position.getY(), position.getZ()); // specify the raster position for pixel operations
	//~ glScaled (0.15,0.15,0.0);
	//~ glTranslatef (position.getX(), position.getY(), position.getZ()); // specify the raster position for pixel operations

    for (unsigned int i = 0; i <= text.length(); i++)
    	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);

    //~ for (int i=0; i<=text.length(); i++)
    	//~ glutStrokeCharacter(GLUT_STROKE_ROMAN , text[i]);

		glPopAttrib();
  		glPopMatrix();
  	glMatrixMode(GL_PROJECTION);
  		glPopMatrix();
  	glMatrixMode(GL_MODELVIEW);
}


// PROTECTED ----------------------------------------------------------

// action buttons
/**
 * @brief only update state of the button
 * @param atom is the atom button into the list
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonAtom ( MyAtom atom , bool state )
{
	_atomButtons[atom.getValue()].setState( state );
	
	if ( state == false )
		_atomButtons[atom.getValue()].swapSelected(); // swap variable _selected
}

/**
 * @brief update state of the button + variable "_move"
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonMove ( bool state )
{
	_actionButtons[eMove].setState( state );
	_move = state; // true : button pressed, false : button not pressed
}

/**
 * @brief delete all selected atoms on the list + update state of the button + update atoms positions on the screen + variable _firstAtomDisplayed
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonValid( bool state )
{
	_actionButtons[eValid].setState( state );
		
	if ( state == true )
	{
		/// PROBLEME ICI CAR APRES ERASE -> "IT" PLUS CORRECTE !!!!!!!!!!!!!!!!!!!!!!! A CHANGER -> voir updateListAtom
		//~ for ( AtomButtons::iterator it = _atomButtons.begin(); it != _atomButtons.end(); it++ )
		//~ {
			//~ if ( it->second.isSelected() == true ) // atom selected into the list ? YES -> delete it, NO -> do nothing
				//~ _atomButtons.erase( it->first ); 
		//~ }
		
		/// Autre méthode
		if ( (int)_atomButtons.size() == 1 )
		{
			if ( _atomButtons.begin()->second.isSelected() )
			{
				clearAtoms();
				_firstAtomDisplayed = 0;
			}
		}
		else // many atoms into the list ?
		{
			AtomButtons::iterator it = _atomButtons.begin();
			AtomButtons::iterator succ = it++;
		
			while ( it!=_atomButtons.end() )
			{	
				if ( it->second.isSelected() )
				{
					if ( _firstAtomDisplayed == it->first )
					{
						_atomButtons.erase ( it );
						if ( _atomButtons.size()!=1 )
							_firstAtomDisplayed = _atomButtons.begin()->first;
						else
							_firstAtomDisplayed = 0;
					}
					else
						_atomButtons.erase ( it );
				}
				
				it = succ;
				
				if ( succ != _atomButtons.end() )
				{
					succ++;
				}
			}
		}
		
		updateAtomsPosition(); // recalculate the position for each atoms into the list ( depending on "_firstPositionAtom")
	}
}

/**
 * @brief selection all atoms into the list (on the screen) to have an aptitude for deselected all atoms + update state of button
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonClean( bool state )
{
	_actionButtons[eClean].setState( state );
	
	if ( state == true )
	{
		for ( AtomButtons::iterator it = _atomButtons.begin(); it != _atomButtons.end(); it++ )
		{
			if ( it->second.isSelected() == false ) // atom selected into the list ? NO -> deselect it, YES -> do nothing
				it->second.swapSelected();
		}
	}
}

/**
 * @brief update variable _firstAtomDisplayed (if atom != first atom on the list) + update positions of atoms on the screen + update state of the button
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonUp   ( bool state )
{
	_actionButtons[eUp].setState( state );
	
	if ( state == true )
	{
		if ( !isEmpty() )
		{
			AtomButtons::iterator rit;
			if ( _firstAtomDisplayed != _atomButtons.begin()->first) // if we aren't at the top of the list
			{
				rit = _atomButtons.find(_firstAtomDisplayed); // pointer on the first atom displayed on the screen
				rit--; // move up a notch the list
				_firstAtomDisplayed = rit->first; // update variable "_firstAtomDiplayed"
				updateAtomsPosition(); // update new atoms position on the screen
			}
		}
	}
}

/**
 * @brief decrement variable _firstAtomDisplayed (if atom != last atom on the list) + update positions of atoms on the screen + update state of the button
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonDown ( bool state )
{
	_actionButtons[eDown].setState( state );
	
	if ( state == true )
	{
		if ( !isEmpty() )
		{
			AtomButtons::iterator it; // iterator
			if ( _firstAtomDisplayed != _atomButtons.rbegin()->first) // if we aren't at the end of the list
			{
				it = _atomButtons.find(_firstAtomDisplayed); // pointer on the first atom displayed on the screen
				it++; // move down a notch the list
				_firstAtomDisplayed = it->first; // update variable "_firstAtomDiplayed"
				updateAtomsPosition(); // update new atoms position on the screen
			}
		}
	}
}

/**
 * @brief update variable _visible + update state of the button
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonOpen ( bool state )
{
	_actionButtons[eOpen].setState ( state );
	
	if ( state == true )
	{
		_visible = true;
	}
}

/**
 * @brief update variable _visible + update state of the button
 * @param state is the state of the button (pressed or not pressed)
 */
void Deselector::onButtonClose( bool state )
{
	_actionButtons[eClose].setState ( state );
	
	if ( state == true )
	{
		_visible = false;
	}
}


// OpenGL operation

/**
 * @brief draw an OpenGL action button via its dimensions 
 * @param button contains position,dimension and color
 */
void Deselector::drawActionButton( MyAction button ) const
{
	drawButton ( button, button.getValue() );
}

/**
 * @brief draw an OpenGL atom button via its dimensions 
 * @param button contains position,dimension and color
 */
void Deselector::drawAtomButton( MyAtom button ) const
{
	if ( button.isSelected() )
		button.setColor(Vector3D(0.52,0.50,0.41), 0); // change color of button "not pressed" to show this atom is selected
		
	drawButton ( button, toString<uint>(button.getValue()) );
}

/**
 * @brief draw an OpenGL button via its dimensions 
 * @param button contains position,dimension and color
 */
void Deselector::drawButton( MyObject button, std::string value ) const
{
	// set a convenient projection matrix
	
   	glMatrixMode(GL_PROJECTION);
  		glPushMatrix();
	  	glLoadIdentity();
  		glOrtho(0.0, _widthWin, 0.0, _heightWin, -1.0, 1.0);
  
   	glMatrixMode(GL_MODELVIEW);
	  	glPushMatrix();
	  	glPushAttrib( GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT );
	  	glLoadIdentity();
  	
  	// set parameters
  	glEnable(GL_BLEND); // Enabling transparency
  	glDisable(GL_DEPTH_TEST);
  	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // mode plein
  	glColor4d ( button.getButtonColor().getX(), button.getButtonColor().getY(), button.getButtonColor().getZ(), 0.75 ); // activation de la transparence : glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  	
	// draw rectangle
	Vector3D v = button.getDimension();
   	glRectd ( button.getPosition().getX(), _heightWin - button.getPosition().getY(), 
    		  button.getPosition().getX() + button.getDimension().getX(), _heightWin - (button.getPosition().getY() + button.getDimension().getY()) );
	
	// draw Text
	Vector3D positionText = Vector3D(button.getPosition().getX()+(button.getDimension().getX()/2.0) - (((float)glutBitmapWidth(GLUT_BITMAP_HELVETICA_12,value[0])*value.length())/2.0), button.getPosition().getY()+(button.getDimension().getY()/2.0)+((float)glutBitmapWidth(GLUT_BITMAP_HELVETICA_12,value[0])/2), 0.0);
	drawText( positionText, button.getTextColor(), value );

	glDisable(GL_BLEND); // Disabling transparency
	
		glPopAttrib();
  		glPopMatrix();
  	glMatrixMode(GL_PROJECTION);
	  	glPopMatrix();
  	glMatrixMode(GL_MODELVIEW);
}
	
	
// operations of updating


/**
 * @brief update action position when the menu has been moved + update variable "positionFirstAtom"
 */
void Deselector::updateActionsPosition()
{
	// update action buttons position
		// "move" button
		_actionButtons[eMove].setPosition ( _positionMenu );
		_actionButtons[eOpen].setPosition ( Vector3D( _actionButtons[eMove].getPosition().getX()+_actionButtons[eMove].getDimension().getX()+_spaceBetweenButton, _actionButtons[eMove].getPosition().getY(), 0.0 ) );
		_actionButtons[eClose].setPosition( Vector3D( _actionButtons[eMove].getPosition().getX()+_actionButtons[eMove].getDimension().getX()+_spaceBetweenButton, _actionButtons[eMove].getPosition().getY(), 0.0) );
		_actionButtons[eValid].setPosition( Vector3D( _actionButtons[eMove].getPosition().getX(), _actionButtons[eMove].getPosition().getY()+_actionButtons[eMove].getDimension().getY()+_spaceBetweenButton, 0.0) );		
		_actionButtons[eClean].setPosition( Vector3D( _actionButtons[eValid].getPosition().getX()+_actionButtons[eValid].getDimension().getX()+_spaceBetweenButton, _actionButtons[eValid].getPosition().getY(), 0.0) );	
		_actionButtons[eUp].setPosition   ( Vector3D( _actionButtons[eValid].getPosition().getX(), _actionButtons[eValid].getPosition().getY()+_actionButtons[eValid].getDimension().getY()+_spaceBetweenButton, 0.0) );
		_actionButtons[eDown].setPosition ( Vector3D( _actionButtons[eUp].getPosition().getX()+_actionButtons[eUp].getDimension().getX()+_spaceBetweenButton, _actionButtons[eUp].getPosition().getY(), 0.0) );

	// update variable "positionFirstAtom"
	_positionFirstAtom.setX(_actionButtons[eUp].getPosition().getX());
	_positionFirstAtom.setY(_actionButtons[eUp].getPosition().getY()+_actionButtons[eUp].getDimension().getY()+(2*_spaceBetweenButton));
}

/**
 * @brief update atoms position when the menu has been moved or some atoms have been deleted or atom inserted
 */
void Deselector::updateAtomsPosition()
{
	// the position of the first atom (_firstAtomDisplayed) DISPLAYED is known by the variable "_positionFirstAtom"
	int i=0;
	for (AtomButtons::iterator it = _atomButtons.find(_firstAtomDisplayed); it != _atomButtons.end(); it++)
	{
		it->second.setPosition( Vector3D( _positionFirstAtom.getX(), _positionFirstAtom.getY() + i*(it->second.getDimension().getY()+_spaceBetweenButton), 0.0 ) );
		i++;
	}
	
	// we put atoms between "begin" <-> "firstAtomDisplayed" at (0.0,0.0,0.0) to avoid method "addPressedAtom(x,y)" deceives when check the atom pressed
	// because, when the map of atoms is up/down by the buttons "onButtonUp/Down", several atoms could have the same position ( the atom designated by the variable "_firstAtomDisplayed" and the old atom not displayed on the screen )
	for (AtomButtons::iterator it = _atomButtons.begin(); it != _atomButtons.find(_firstAtomDisplayed); it++)
	{
		it->second.setPosition( Vector3D( 0.0, 0.0, 0.0 ) );
	}
}
