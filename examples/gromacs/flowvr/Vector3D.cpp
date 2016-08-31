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
*    Sébastien Limet <sebastien.limet@univ-orleans.fr                 *
******************************************************************/

#include <Vector3D.h>


/**
* @brief Constructor
*/
Vector3D::Vector3D() {
    X = 0;
    Y = 0;
    Z = 0;
}

/**
* @brief Destructor
* @param x is the new X coordinate
* @param y is the new Y coordinate
* @param z is the new Z coordinate
*/
Vector3D::Vector3D(double x,double y,double z) {
    X = x;
    Y = y;
    Z = z;
}

/**
* @brief Copy constructor
* @param v is the new value of the vector3D
*/
Vector3D::Vector3D(const Vector3D & v) {
    X = v.X;
    Y = v.Y;
    Z = v.Z;
}

/**
* @brief assign a new vector which is the difference between two vectors
* @param from is the beginning vector
* @param to is the ending vector
*/
Vector3D::Vector3D(const Vector3D & from,const Vector3D & to) {
    X = to.X - from.X;
    Y = to.Y - from.Y;
    Z = to.Z - from.Z;
}

// ********************************************************************

/**
* @brief overload the "=" operator
* @param v is assigned in the vector3D
* @return a vector with the calculated value in the method
*/
Vector3D & Vector3D::operator= (const Vector3D & v) {
    X = v.X;
    Y = v.Y;
    Z = v.Z;
    return *this;
}

// ********************************************************************

/**
* @brief overload the "+=" operator
* @param v is added and assign to the vector
* @return a vector with the calculated value in the method
*/
Vector3D & Vector3D::operator+= (const Vector3D & v) {
    X += v.X;
    Y += v.Y;
    Z += v.Z;
    return *this;
}

/**
* @brief overload the "+" operator
* @param v is added to the vector
* @return a vector with the calculated value in the method
*/
Vector3D Vector3D::operator+ (const Vector3D & v) const {
    Vector3D t = *this;
    t += v; // surchage ci-dessus appellée
    return t;
}

// ********************************************************************

/**
* @brief overload the "-=" operator
* @param v is substracted and assigned to the vector
* @return a vector with the calculated value in the method
*/
Vector3D & Vector3D::operator-= (const Vector3D & v) {
    X -= v.X;
    Y -= v.Y;
    Z -= v.Z;
    return *this;
}

/**
* @brief overload the "-" operator
* @param v is substracted to the vector
* @return a vector with the calculated value in the method
*/
Vector3D Vector3D::operator- (const Vector3D & v) const {
    Vector3D t = *this;
    t -= v; // surchage ci-dessus appellée
    return t;
}

// ********************************************************************

/**
* @brief overload the "*=" operator
* @param v is multiplicated and assigned to the vector
* @return a vector with the calculated value in the method
*/
Vector3D & Vector3D::operator*= (const double a) {
    X *= a;
    Y *= a;
    Z *= a;
    return *this;
}

/**
* @brief overload the "*=" operator
* @param v is multiplicated to the vector
* @return a vector with the calculated value in the method
*/
Vector3D Vector3D::operator* (const double a) const {
    Vector3D t = *this;
    t *= a; // surchage ci-dessus appellée
    return t;
}

/**
* @brief overload the "*=" operator
* @param a is a constant multiplicated to the vector v
* @param v is multiplicated to the vector
* @return a vector with the calculated value in the method
*/
Vector3D operator* (const double a,const Vector3D & v) {
    return Vector3D(v.X*a,v.Y*a,v.Z*a);
}

// ********************************************************************

/**
* @brief overload the "/=" operator
* @param a is the quotient which divides the vector
* @return a vector with the calculated value in the method
*/
Vector3D & Vector3D::operator/= (const double a) {
    X /= a;
    Y /= a;
    Z /= a;
    return *this;
}

/**
* @brief overload the "/" operator
* @param a is the quotient which divides the vector
* @return a vector with the calculated value in the method
*/
Vector3D Vector3D::operator/ (const double a) const {
    Vector3D t = *this;
    t /= a;
    return t;
}

// ********************************************************************

/**
* @brief to calculate the cross product between two vectors
* @param v is the second vector used to process the cross product
* @return a vector with is the cross product
*/
Vector3D Vector3D::crossProduct(const Vector3D & v) const {
    Vector3D t;
    t.X = Y*v.Z - Z*v.Y;
    t.Y = Z*v.X - X*v.Z;
    t.Z = X*v.Y - Y*v.X;
    return t;
}

double Vector3D::scalaireProduct(const Vector3D & v) const 
{
  return (double) (X*v.X + Y*v.Y + Z*v.Z);
}

/**
* @brief to calculate the length of a vector
* @return the length of the vector
*/
double Vector3D::length() const {
    return sqrt( X*X + Y*Y + Z*Z);
}

/**
* @brief to normalize a vector (length = 1)
* @return a normalized vector
*/
Vector3D & Vector3D::normalize() { // pour normalizer; on divise par sa norme
    (*this) /= length();
    return (*this);
}

// ********************************************************************

/**
* @brief to obtain X coordinate
* @return X coordinate
*/
double Vector3D::getX() const
{
	return X;
}

/**
* @brief to obtain Y coordinate
* @return Y coordinate
*/
double Vector3D::getY() const 
{
	return Y;
}

/**
* @brief to obtain Z coordinate
* @return Z coordinate
*/
double Vector3D::getZ() const 
{
	return Z;
}
    
// ********************************************************************

/**
* @brief to set X coordinate
* @param X is the new coordinate
*/
double Vector3D::setX(double x)
{
	X=x;
}

/**
* @brief to set Y coordinate
* @param Y is the new coordinate
*/
double Vector3D::setY(double y)
{
	Y = y;
}

/**
* @brief to set Z coordinate
* @param Z is the new coordinate
*/
double Vector3D::setZ(double z)
{
	Z=z;
}

void Vector3D::round(int nbAc)
{
    int val=1;
    for (int i=0; i<nbAc; i++)
        val *=10;

    X = (int)(X*val);
    Y = (int)(Y*val);
    Z = (int)(Z*val);

    X = (double)(X/val);
    Y = (double)(Y/val);
    Z = (double)(Z/val);
}
    
// ********************************************************************

/**
* @brief to display a vector
* @param out is an ostream
*/
void Vector3D::displayVector3D(std::ostream &out)
{
	out <<"( " << X << "," << Y << "," << Z << " )";
}

/**
* @brief overloading of the << method to write a vector on default output
* @param out is an ostream 
* @param v is the vector to display
*/
std::ostream &operator<<( std::ostream &out, Vector3D &v )
{
    v.displayVector3D(out);
    return out;
}

/**
* @brief to read a vector
* @param in is an istream
*/
void Vector3D::readVector3D(std::istream &in)
{
	in >> X >> Y >> Z;	
}

/**
* @brief overloading of the >> method to read a vector on default input
* @param in is an istream 
* @param v is the vector to read
*/
std::istream &operator>>( std::istream &in, Vector3D &v )
{
    v.readVector3D(in);
    return in;
}

// ********************************************************************

/**
* @brief useful method which permit to obtain OpenGL coordinates of the mouse (X,Y coordinates)
* @param x absciss coordinate of the mouse
* @param y ordinate coordinate of the mouse
* @return a vector with the OpenGL position of the mouse
*/
Vector3D Vector3D::getOGLPos(int x, int y)
{
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	winX = (float)x;
	// the window's coordinates are the inverse of the viewport's coordinates
	winY = (float)viewport[3] - (float)y;
	// returns pixel data from the frame buffer -> used here to grab winZ
	glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );

	// converts Windows screen coordinates to OpenGL coordinates
	gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);


        return Vector3D(posX, posY, posZ); // coordinate X ????
}


/**
  * @brief useful method to convert a quaternion into a rotated vector (based on "Rodrigues' rotation formula")
  * @param w angle (given by the quaternion)
  * @param x,y,z axis (given by the quaternion)
  * @return a Vector3D correctly rotated
  */
#include <iostream>
using namespace std;
Vector3D Vector3D::quat2rvector(double w, double x, double y, double z)
{
    double norm = sqrt(x*x + y*y + z*z);
    if(norm > 0.002) // if "norm" doesn't close to zero then direction of axis not important
    {
        x /= norm;
        y /= norm;
        z /= norm;
    }

    Vector3D V(0.,0.,1.);
    Vector3D W(x,y,z);
    double angle = 2*acos(w); // w radian    

    return V*cos(angle) + (W.crossProduct(V))*sin(angle) + W*(W.scalaireProduct(V))*(1-cos(angle));
}
