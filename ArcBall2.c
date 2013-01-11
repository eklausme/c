/** KempoApi: The Turloc Toolkit ******************************
    *    *
    **  **
      **
      **

     Arcball class for mouse manipulation.

                                 (C) 1999-2003 Tatewake.com
      History:
      08/17/2003 - (TJG) - Creation
      09/23/2003 - (TJG) - Bug fix and optimization
      09/25/2003 - (TJG) - Version for NeHe Basecode users
      10/23/2012 - Elmar Klausmeier, C version

**************************************************************/

#ifndef _ArcBall_h
#define _ArcBall_h
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

// 8<--Snip here if you have your own math types/funcs-->8 


//Math types derived from the KempoApi tMath library
    typedef union Tuple2f_t {
        GLfloat T[2];
        struct { GLfloat X, Y; } s;
    } Tuple2fT;      //A generic 2-element tuple that is represented by single-precision floating point x,y coordinates. 

    typedef union Tuple3f_t {
        GLfloat T[3];
        struct { GLfloat X, Y, Z; } s;
    } Tuple3fT;      //A generic 3-element tuple that is represented by single precision-floating point x,y,z coordinates. 

    typedef union Tuple4f_t {
        GLfloat T[4];
        struct { GLfloat X, Y, Z, W; } s;
    } Tuple4fT;      //A 4-element tuple represented by single-precision floating point x,y,z,w coordinates. 

enum {
	D3M00=0, D3XX=0, D3SX=0, D3M10, D3M20,
	D3M01, D3M11=4, D3YY=4, D3SY=4, D3M21,
	D3M02, D3M12, D3M22=8, D3ZZ=8, D3SZ=0
};
enum {
	D4M00=0, D4XX=0, D4SX=0, D4M10, D4M20, D4M30,
	D4M01, D4M11=5, D4YY=5, D4SY=5, D4M21, D4M31,
	D4M02, D4M12, D4M22=10, D4ZZ=10, D4SZ=10, D4M32,
	D4M03, D4M13, D4M23, D4M33=15, D4TW=15, D4SW=15
};
    typedef union Matrix3f_t {
            GLfloat M[9];
            struct {
                //column major
                union { GLfloat M00; GLfloat XX; GLfloat SX; };  //XAxis.X and Scale X
                union { GLfloat M10; GLfloat XY;             };  //XAxis.Y
                union { GLfloat M20; GLfloat XZ;             };  //XAxis.Z
                union { GLfloat M01; GLfloat YX;             };  //YAxis.X
                union { GLfloat M11; GLfloat YY; GLfloat SY; };  //YAxis.Y and Scale Y
                union { GLfloat M21; GLfloat YZ;             };  //YAxis.Z
                union { GLfloat M02; GLfloat ZX;             };  //ZAxis.X
                union { GLfloat M12; GLfloat ZY;             };  //ZAxis.Y
                union { GLfloat M22; GLfloat ZZ; GLfloat SZ; };  //ZAxis.Z and Scale Z
            } s;
    } Matrix3fT;     //A single precision floating point 3 by 3 matrix. 

    typedef union Matrix4f_t {
            GLfloat M[16];
            struct {
                //column major
                union { GLfloat M00; GLfloat XX; GLfloat SX; };  //XAxis.X and Scale X
                union { GLfloat M10; GLfloat XY;             };  //XAxis.Y
                union { GLfloat M20; GLfloat XZ;             };  //XAxis.Z
                union { GLfloat M30; GLfloat XW;             };  //XAxis.W
                union { GLfloat M01; GLfloat YX;             };  //YAxis.X
                union { GLfloat M11; GLfloat YY; GLfloat SY; };  //YAxis.Y and Scale Y
                union { GLfloat M21; GLfloat YZ;             };  //YAxis.Z
                union { GLfloat M31; GLfloat YW;             };  //YAxis.W
                union { GLfloat M02; GLfloat ZX;             };  //ZAxis.X
                union { GLfloat M12; GLfloat ZY;             };  //ZAxis.Y
                union { GLfloat M22; GLfloat ZZ; GLfloat SZ; };  //ZAxis.Z and Scale Z
                union { GLfloat M32; GLfloat ZW;             };  //ZAxis.W
                union { GLfloat M03; GLfloat TX;             };  //Trans.X
                union { GLfloat M13; GLfloat TY;             };  //Trans.Y
                union { GLfloat M23; GLfloat TZ;             };  //Trans.Z
                union { GLfloat M33; GLfloat TW; GLfloat SW; };  //Trans.W and Scale W
            } s;
    } Matrix4fT;     //A single precision floating point 4 by 4 matrix. 


#define Quat4fT     Tuple4fT   //A 4 element unit quaternion represented by single precision floating point x,y,z,w coordinates. 

#define Vector2fT   Tuple2fT   //A 2-element vector that is represented by single-precision floating point x,y coordinates. 
#define Vector3fT   Tuple3fT   //A 3-element vector that is represented by single-precision floating point x,y,z coordinates. 

//utility macros
//assuming IEEE-754(GLfloat), which i believe has max precision of 7 bits
#define ArcBallEpsilon 1.0e-5


//Math functions
#ifdef DDT
//"Inherited" types
#define Point2fT    Tuple2fT   //A 2 element point that is represented by single precision floating point x,y coordinates. 
    /**
     * Sets the value of this tuple to the vector sum of itself and tuple t1.
     * @param t1  the other tuple
     */
    inline static void Point2fAdd(Point2fT* NewObj, const Tuple2fT* t1) {
        NewObj->s.X += t1->s.X;
        NewObj->s.Y += t1->s.Y;
    }

    /**
      * Sets the value of this tuple to the vector difference of itself and tuple t1 (this = this - t1).
      * @param t1 the other tuple
      */
    inline static void Point2fSub(Point2fT* NewObj, const Tuple2fT* t1) {
        NewObj->s.X -= t1->s.X;
        NewObj->s.Y -= t1->s.Y;
    }
#endif

    /**
      * Returns the squared length of this vector.
      * @return the squared length of this vector
      */
    inline static GLfloat Vector3fLengthSquared(const Vector3fT* NewObj) {
        return  (NewObj->s.X * NewObj->s.X) +
                (NewObj->s.Y * NewObj->s.Y) +
                (NewObj->s.Z * NewObj->s.Z);
    }

    /**
      * Returns the length of this vector.
      * @return the length of this vector
      */
    inline static GLfloat Vector3fLength(const Vector3fT* NewObj) {
        return sqrtf(Vector3fLengthSquared(NewObj));
    }

    /**
      * Sets this vector to be the vector cross product of vectors v1 and v2.
      * @param v1 the first vector
      * @param v2 the second vector
      */
    inline static void Vector3fCross(Vector3fT* NewObj, const Vector3fT* v1, const Vector3fT* v2) {
        Vector3fT Result; //safe not to initialize

        Result.s.X = (v1->s.Y * v2->s.Z) - (v1->s.Z * v2->s.Y);
        Result.s.Y = (v1->s.Z * v2->s.X) - (v1->s.X * v2->s.Z);
        Result.s.Z = (v1->s.X * v2->s.Y) - (v1->s.Y * v2->s.X);

        //copy result back
        *NewObj = Result;
    }

    /**
      * Computes the dot product of the this vector and vector v1.
      * @param  v1 the other vector
      */
    inline static GLfloat Vector3fDot(const Vector3fT* NewObj, const Vector3fT* v1) {
        return  (NewObj->s.X * v1->s.X) +
                (NewObj->s.Y * v1->s.Y) +
                (NewObj->s.Z * v1->s.Z);
    }

    inline static void Matrix3fSetZero(Matrix3fT* NewObj) {
        NewObj->s.M00 = 0, NewObj->s.M01 = 0, NewObj->s.M02 = 0,
        NewObj->s.M10 = 0, NewObj->s.M11 = 0, NewObj->s.M12 = 0,
        NewObj->s.M20 = 0, NewObj->s.M21 = 0, NewObj->s.M22 = 0;
    }

    /**
     * Sets this Matrix3 to identity.
     */
    inline static void Matrix3fSetIdentity(Matrix3fT* NewObj) {
        Matrix3fSetZero(NewObj);
        NewObj->s.M00 = 1.0f; //then set diagonal as 1
        NewObj->s.M11 = 1.0f;
        NewObj->s.M22 = 1.0f;
    }

    /**
      * Sets the value of this matrix to the matrix conversion of the
      * quaternion argument. 
      * @param q1 the quaternion to be converted 
      */
    //$hack this can be optimized some(if s == 0)
    inline static void Matrix3fSetRotationFromQuat4f(Matrix3fT* NewObj, const Quat4fT* q1) {
        GLfloat n, s;
        GLfloat xs, ys, zs;
        GLfloat wx, wy, wz;
        GLfloat xx, xy, xz;
        GLfloat yy, yz, zz;

        n = (q1->s.X * q1->s.X) + (q1->s.Y * q1->s.Y) + (q1->s.Z * q1->s.Z) + (q1->s.W * q1->s.W);
        s = (n > 0.0f) ? (2.0f / n) : 0.0f;

        xs = q1->s.X * s;  ys = q1->s.Y * s;  zs = q1->s.Z * s;
        wx = q1->s.W * xs; wy = q1->s.W * ys; wz = q1->s.W * zs;
        xx = q1->s.X * xs; xy = q1->s.X * ys; xz = q1->s.X * zs;
        yy = q1->s.Y * ys; yz = q1->s.Y * zs; zz = q1->s.Z * zs;

        NewObj->s.XX = 1.0f - (yy + zz); NewObj->s.YX =         xy - wz;  NewObj->s.ZX =         xz + wy;
        NewObj->s.XY =         xy + wz;  NewObj->s.YY = 1.0f - (xx + zz); NewObj->s.ZY =         yz - wx;
        NewObj->s.XZ =         xz - wy;  NewObj->s.YZ =         yz + wx;  NewObj->s.ZZ = 1.0f - (xx + yy);
    }

    /**
     * Sets the value of this matrix to the result of multiplying itself
     * with matrix m1. 
     * @param m1 the other matrix 
     */
    inline static void Matrix3fMulMatrix3f(Matrix3fT* NewObj, const Matrix3fT* m1) {
        Matrix3fT Result; //safe not to initialize

        // alias-safe way.
        Result.s.M00 = (NewObj->s.M00 * m1->s.M00) + (NewObj->s.M01 * m1->s.M10) + (NewObj->s.M02 * m1->s.M20);
        Result.s.M01 = (NewObj->s.M00 * m1->s.M01) + (NewObj->s.M01 * m1->s.M11) + (NewObj->s.M02 * m1->s.M21);
        Result.s.M02 = (NewObj->s.M00 * m1->s.M02) + (NewObj->s.M01 * m1->s.M12) + (NewObj->s.M02 * m1->s.M22);

        Result.s.M10 = (NewObj->s.M10 * m1->s.M00) + (NewObj->s.M11 * m1->s.M10) + (NewObj->s.M12 * m1->s.M20);
        Result.s.M11 = (NewObj->s.M10 * m1->s.M01) + (NewObj->s.M11 * m1->s.M11) + (NewObj->s.M12 * m1->s.M21);
        Result.s.M12 = (NewObj->s.M10 * m1->s.M02) + (NewObj->s.M11 * m1->s.M12) + (NewObj->s.M12 * m1->s.M22);

        Result.s.M20 = (NewObj->s.M20 * m1->s.M00) + (NewObj->s.M21 * m1->s.M10) + (NewObj->s.M22 * m1->s.M20);
        Result.s.M21 = (NewObj->s.M20 * m1->s.M01) + (NewObj->s.M21 * m1->s.M11) + (NewObj->s.M22 * m1->s.M21);
        Result.s.M22 = (NewObj->s.M20 * m1->s.M02) + (NewObj->s.M21 * m1->s.M12) + (NewObj->s.M22 * m1->s.M22);

        //copy result back to this
        *NewObj = Result;
    }

    inline static void Matrix4fSetRotationScaleFromMatrix4f(Matrix4fT* NewObj, const Matrix4fT* m1) {
        NewObj->s.XX = m1->s.XX; NewObj->s.YX = m1->s.YX; NewObj->s.ZX = m1->s.ZX;
        NewObj->s.XY = m1->s.XY; NewObj->s.YY = m1->s.YY; NewObj->s.ZY = m1->s.ZY;
        NewObj->s.XZ = m1->s.XZ; NewObj->s.YZ = m1->s.YZ; NewObj->s.ZZ = m1->s.ZZ;
    }

    /**
      * Performs SVD on this matrix and gets scale and rotation.
      * Rotation is placed into rot3, and rot4.
      * @param rot3 the rotation factor(Matrix3d). if null, ignored
      * @param rot4 the rotation factor(Matrix4) only upper 3x3 elements are changed. if null, ignored
      * @return scale factor
      */
    inline static GLfloat Matrix4fSVD(const Matrix4fT* NewObj, Matrix3fT* rot3, Matrix4fT* rot4) {
        GLfloat s, n;

        // this is a simple svd.
        // Not complete but fast and reasonable.
        // See comment in Matrix3d.

        s = sqrtf(
                ( (NewObj->s.XX * NewObj->s.XX) + (NewObj->s.XY * NewObj->s.XY) + (NewObj->s.XZ * NewObj->s.XZ) + 
                  (NewObj->s.YX * NewObj->s.YX) + (NewObj->s.YY * NewObj->s.YY) + (NewObj->s.YZ * NewObj->s.YZ) +
                  (NewObj->s.ZX * NewObj->s.ZX) + (NewObj->s.ZY * NewObj->s.ZY) + (NewObj->s.ZZ * NewObj->s.ZZ) ) / 3.0f );

        if (rot3) {   //if pointer not null
            //this->getRotationScale(rot3);
            rot3->s.XX = NewObj->s.XX; rot3->s.XY = NewObj->s.XY; rot3->s.XZ = NewObj->s.XZ;
            rot3->s.YX = NewObj->s.YX; rot3->s.YY = NewObj->s.YY; rot3->s.YZ = NewObj->s.YZ;
            rot3->s.ZX = NewObj->s.ZX; rot3->s.ZY = NewObj->s.ZY; rot3->s.ZZ = NewObj->s.ZZ;

            // zero-div may occur.

            n = 1.0f / sqrtf( (NewObj->s.XX * NewObj->s.XX) +
                                      (NewObj->s.XY * NewObj->s.XY) +
                                      (NewObj->s.XZ * NewObj->s.XZ) );
            rot3->s.XX *= n;
            rot3->s.XY *= n;
            rot3->s.XZ *= n;

            n = 1.0f / sqrtf( (NewObj->s.YX * NewObj->s.YX) +
                                      (NewObj->s.YY * NewObj->s.YY) +
                                      (NewObj->s.YZ * NewObj->s.YZ) );
            rot3->s.YX *= n;
            rot3->s.YY *= n;
            rot3->s.YZ *= n;

            n = 1.0f / sqrtf( (NewObj->s.ZX * NewObj->s.ZX) +
                                      (NewObj->s.ZY * NewObj->s.ZY) +
                                      (NewObj->s.ZZ * NewObj->s.ZZ) );
            rot3->s.ZX *= n;
            rot3->s.ZY *= n;
            rot3->s.ZZ *= n;
        }

        if (rot4) {   //if pointer not null
            if (rot4 != NewObj) {
                Matrix4fSetRotationScaleFromMatrix4f(rot4, NewObj);  // private method
            }

            // zero-div may occur.

            n = 1.0f / sqrtf( (NewObj->s.XX * NewObj->s.XX) +
                                      (NewObj->s.XY * NewObj->s.XY) +
                                      (NewObj->s.XZ * NewObj->s.XZ) );
            rot4->s.XX *= n;
            rot4->s.XY *= n;
            rot4->s.XZ *= n;

            n = 1.0f / sqrtf( (NewObj->s.YX * NewObj->s.YX) +
                                      (NewObj->s.YY * NewObj->s.YY) +
                                      (NewObj->s.YZ * NewObj->s.YZ) );
            rot4->s.YX *= n;
            rot4->s.YY *= n;
            rot4->s.YZ *= n;

            n = 1.0f / sqrtf( (NewObj->s.ZX * NewObj->s.ZX) +
                                      (NewObj->s.ZY * NewObj->s.ZY) +
                                      (NewObj->s.ZZ * NewObj->s.ZZ) );
            rot4->s.ZX *= n;
            rot4->s.ZY *= n;
            rot4->s.ZZ *= n;
        }

        return s;
    }

    inline static void Matrix4fSetRotationScaleFromMatrix3f(Matrix4fT* NewObj, const Matrix3fT* m1) {
        NewObj->s.XX = m1->s.XX; NewObj->s.YX = m1->s.YX; NewObj->s.ZX = m1->s.ZX;
        NewObj->s.XY = m1->s.XY; NewObj->s.YY = m1->s.YY; NewObj->s.ZY = m1->s.ZY;
        NewObj->s.XZ = m1->s.XZ; NewObj->s.YZ = m1->s.YZ; NewObj->s.ZZ = m1->s.ZZ;
    }

    inline static void Matrix4fMulRotationScale(Matrix4fT* NewObj, GLfloat scale) {
        NewObj->s.XX *= scale; NewObj->s.YX *= scale; NewObj->s.ZX *= scale;
        NewObj->s.XY *= scale; NewObj->s.YY *= scale; NewObj->s.ZY *= scale;
        NewObj->s.XZ *= scale; NewObj->s.YZ *= scale; NewObj->s.ZZ *= scale;
    }

    /**
      * Sets the rotational component (upper 3x3) of this matrix to the matrix
      * values in the T precision Matrix3d argument; the other elements of
      * this matrix are unchanged; a singular value decomposition is performed
      * on this object's upper 3x3 matrix to factor out the scale, then this
      * object's upper 3x3 matrix components are replaced by the passed rotation
      * components, and then the scale is reapplied to the rotational
      * components.
      * @param m1 T precision 3x3 matrix
      */
    inline static void Matrix4fSetRotationFromMatrix3f(Matrix4fT* NewObj, const Matrix3fT* m1) {
        GLfloat scale;

        scale = Matrix4fSVD(NewObj, NULL, NULL);

        Matrix4fSetRotationScaleFromMatrix3f(NewObj, m1);
        Matrix4fMulRotationScale(NewObj, scale);
    }

// 8<--Snip here if you have your own math types/funcs-->8 

    typedef struct {
            //Set new bounds
            Vector3fT   StVec;          //Saved click vector
            Vector3fT   EnVec;          //Saved drag vector
            GLfloat     AdjustWidth;    //Mouse bounds width
            GLfloat     AdjustHeight;   //Mouse bounds height
    } ArcBall_t;


/**                                                         **/
/*************************************************************/



//Arcball sphere constants:
//Diameter is       2.0f
//Radius is         1.0f
//Radius squared is 1.0f

inline void ArcBallSetBounds(ArcBall_t *a, GLfloat NewWidth, GLfloat NewHeight) {
    assert((NewWidth > 1.0f) && (NewHeight > 1.0f));

    //Set adjustment factor for width/height
    a->AdjustWidth  = 1.0f / ((NewWidth  - 1.0f) * 0.5f);
    a->AdjustHeight = 1.0f / ((NewHeight - 1.0f) * 0.5f);
}

void ArcBallMapToSphere(ArcBall_t *a, double x, double y, Vector3fT* NewVec) {
    //Adjust point coords and scale down to range of [-1 ... 1]
    x  =        (x * a->AdjustWidth)  - 1.0f;
    y  = 1.0f - (y * a->AdjustHeight);

    //Compute the square of the length of the vector to the point from the center
    GLfloat length = x * x + y * y;

    //If the point is mapped outside of the sphere... (length > radius squared)
    if (length > 1.0f) {
        GLfloat norm;

        norm = 1.0f / sqrtf(length); //Compute a normalizing factor (radius / sqrt(length))

        //Return the "normalized" vector, a point on the sphere
        NewVec->s.X = x * norm;
        NewVec->s.Y = y * norm;
        NewVec->s.Z = 0.0f;
    } else {   // else it's on the inside
        //Return a vector to a point mapped inside the sphere sqrt(radius squared - length)
        NewVec->s.X = x;
        NewVec->s.Y = y;
        NewVec->s.Z = sqrtf(1.0f - length);
    }
}

#ifdef DDT
//Create/Destroy
ArcBall_t::ArcBall_t(GLfloat NewWidth, GLfloat NewHeight) {
    //Clear initial values
    this->StVec.s.X     =
    this->StVec.s.Y     = 
    this->StVec.s.Z     = 

    this->EnVec.s.X     =
    this->EnVec.s.Y     = 
    this->EnVec.s.Z     = 0.0f;

    //Set initial bounds
    this->setBounds(NewWidth, NewHeight);
}
#endif

inline void ArcBallClick(ArcBall_t *a, double x, double y) {	// mouse down
    ArcBallMapToSphere(a, x, y, &(a->StVec)); //Map the point to the sphere
}

//Mouse drag, calculate rotation
inline void ArcBallDrag(ArcBall_t *a, double x, double y, Quat4fT* NewRot) {
    //Map the point to the sphere
    ArcBallMapToSphere(a, x, y, &(a->EnVec));

    //Return the quaternion equivalent to the rotation
    if (NewRot) {
        Vector3fT  Perp;

        //Compute the vector perpendicular to the begin and end vectors
        Vector3fCross(&Perp, &(a->StVec), &(a->EnVec));

        //Compute the length of the perpendicular vector
        if (Vector3fLength(&Perp) > ArcBallEpsilon) {    //if its non-zero
            //We're ok, so return the perpendicular vector as the transform after all
            NewRot->s.X = Perp.s.X;
            NewRot->s.Y = Perp.s.Y;
            NewRot->s.Z = Perp.s.Z;
            //In the quaternion values, w is cosine (theta / 2), where theta is rotation angle
            NewRot->s.W= Vector3fDot(&(a->StVec), &(a->EnVec));
        } else {                                    //if its zero
            //The begin and end vectors coincide, so return an identity transform
            NewRot->s.X = 0.0f;
            NewRot->s.Y = 0.0f;
            NewRot->s.Z = 0.0f;
            NewRot->s.W = 0.0f;
        }
    }
}

inline void ArcBallPrint(ArcBall_t *a) {
	printf("StVec=(%.2f,%.2f,%.2f),EnVec=(%.2f,%.2f,%.2f),AdjustWidt=%.2f,AdjustHeight=%.2f\n",
		a->StVec.T[0],a->StVec.T[1],a->StVec.T[2],
		a->EnVec.T[0],a->EnVec.T[1],a->EnVec.T[2],
		a->AdjustWidth,a->AdjustHeight
	);
}

inline void Matrix4Print(Matrix4fT *m) {
	printf("m=( (%.2f,%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f,%f), (%.2f,%.2f,%.2f,%.2f) )\n",
		m->M[0],m->M[1],m->M[2],m->M[3],
		m->M[4],m->M[5],m->M[6],m->M[7],
		m->M[8],m->M[9],m->M[10],m->M[11],
		m->M[12],m->M[13],m->M[14],m->M[15]
	);
}

#endif

