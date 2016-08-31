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

#ifndef _VECTOR3D_H_
#define _VECTOR3D_H_

#include <math.h>
#include <iostream>

#ifdef __APPLE__
#include <GLUT/glut.h>
//#include <GLKit/GLKMath.h>
#else
#include <GL/glut.h>
#endif



/*! \class Vector3D
 *  \brief class which represents a vector (X,Y,Z)
 *
 * This class permit to manipulate vector in 3 dimensions
 */
class Vector3D {
    
    double X; /*!< X coordinate */
    double Y; /*!< Y coordinate */
    double Z; /*!< Z coordinate */

  public:
  
    Vector3D(); // constructor (default)
    Vector3D(double x,double y,double z);
    Vector3D(const Vector3D & v); // constructor copy
    Vector3D(const Vector3D & from,const Vector3D & to);

    Vector3D & operator= (const Vector3D & v); 

    Vector3D & operator+= (const Vector3D & v);
    Vector3D operator+ (const Vector3D & v) const;

    Vector3D & operator-= (const Vector3D & v);
    Vector3D operator- (const Vector3D & v) const;

    Vector3D & operator*= (const double a);
    Vector3D operator* (const double a)const;
    friend Vector3D operator* (const double a,const Vector3D & v);

    Vector3D & operator/= (const double a);
    Vector3D operator/ (const double a)const;

    Vector3D crossProduct(const Vector3D & v)const;
    double scalaireProduct(const Vector3D & v) const; 
    double length()const;
    Vector3D & normalize();
    
    double getX() const;
    double getY() const;
    double getZ() const;
    
    double setX(double x);
    double setY(double y);
    double setZ(double z);
    
    void displayVector3D(std::ostream &out);// display vector
    void readVector3D(std::istream &in);// read vector

    void round(int nbAc);// nbAc : numbers After comma
            
    static Vector3D getOGLPos(int x, int y); // give the position of the move in OpenGL coordinates
    static Vector3D quat2rvector(double w, double x, double y, double z); // based on "Rodrigues' rotation formula" to obtain a rotated vector from a quaternion
};

// to read/write a Vector3D thanks to iostream
/*!
* \fn std::ostream &operator<<( std::ostream &out, Vector3D &v );
* \brief display a vector3D in standart output
*/
std::ostream &operator<<( std::ostream &out, Vector3D &v );
/*!
* \fn std::istream &operator>>( std::istream &in, Vector3D &v );
* \brief write in a vector3D via standart input (keyboard)
*/
std::istream &operator>>( std::istream &in, Vector3D &v );

#endif // VECTOR3D_H
