/*
 * Copyright (c) 1992,1993-1995 by Argonaut Technologies Limited. All rights reserved.
 *
 * $Id: angles.h 1.2 1998/05/07 15:50:35 jon Exp $
 * $Locker: $
 *
 */
#ifndef _ANGLES_H_
#define _ANGLES_H_

/*
 * Angles are in turns, i.e. [0, 1],
 */
typedef br_scalar br_angle;

#define BR_ANGLE_DEG(deg)   ((br_angle)((deg) / 360.0f))
#define BR_ANGLE_RAD(rad)   ((br_angle)((rad) / (2.0f * PI)))

#define BrAngleToDegree(a)  ((br_scalar)((a) * 360.0))
#define BrDegreeToAngle(d)  ((br_angle)((d) / 360.0))
#define BrAngleToRadian(a)  ((br_scalar)((a) * (2.0 * PI)))
#define BrRadianToAngle(r)  ((br_angle)((r) / (2.0 * PI)))
#define BrDegreeToRadian(d) ((br_scalar)((d) * (PI / 180.0)))
#define BrRadianToDegree(r) ((br_scalar)((r) * (180.0 / PI)))

#define BrAngleToScalar(a)  (a)
#define BrScalarToAngle(s)  (s)


#define BR_SIN(a)           ((br_scalar)sinf(BrAngleToRadian(a)))
#define BR_COS(a)           ((br_scalar)cosf(BrAngleToRadian(a)))
#define BR_ASIN(a)          BrRadianToAngle(asinf(a))
#define BR_ACOS(a)          BrRadianToAngle(acosf(a))
#define BR_ATAN2(a, b)      BrRadianToAngle(atan2f((a), (b)))
#define BR_ATAN2FAST(a, b)  BrRadianToAngle(atan2f((a), (b)))

/*
 * Fields that go into br_euler.order
 */
// clang-format off
enum {
    BR_EULER_FIRST       = 0x03,
        BR_EULER_FIRST_X = 0x00,
        BR_EULER_FIRST_Y = 0x01,
        BR_EULER_FIRST_Z = 0x02,

    BR_EULER_PARITY          = 0x04,
        BR_EULER_PARITY_EVEN = 0x00,
        BR_EULER_PARITY_ODD  = 0x04,

    BR_EULER_REPEAT         = 0x08,
        BR_EULER_REPEAT_NO  = 0x00,
        BR_EULER_REPEAT_YES = 0x08,

    BR_EULER_FRAME              = 0x10,
        BR_EULER_FRAME_STATIC   = 0x00,
        BR_EULER_FRAME_ROTATING = 0x10
};
// clang-format on

/*
 * Various possible orders
 */
#define BR_EULER_ORDER(a, p, r, f) (BR_EULER_FIRST_##a | BR_EULER_PARITY_##p | BR_EULER_REPEAT_##r | BR_EULER_FRAME_##f)

// clang-format off
enum {
    BR_EULER_XYZ_S = BR_EULER_ORDER(X, EVEN, NO,  STATIC),
    BR_EULER_XYX_S = BR_EULER_ORDER(X, EVEN, YES, STATIC),
    BR_EULER_XZY_S = BR_EULER_ORDER(X, ODD,  NO,  STATIC),
    BR_EULER_XZX_S = BR_EULER_ORDER(X, ODD,  YES, STATIC),
    BR_EULER_YZX_S = BR_EULER_ORDER(Y, EVEN, NO,  STATIC),
    BR_EULER_YZY_S = BR_EULER_ORDER(Y, EVEN, YES, STATIC),
    BR_EULER_YXZ_S = BR_EULER_ORDER(Y, ODD,  NO,  STATIC),
    BR_EULER_YXY_S = BR_EULER_ORDER(Y, ODD,  YES, STATIC),
    BR_EULER_ZXY_S = BR_EULER_ORDER(Z, EVEN, NO,  STATIC),
    BR_EULER_ZXZ_S = BR_EULER_ORDER(Z, EVEN, YES, STATIC),
    BR_EULER_ZYX_S = BR_EULER_ORDER(Z, ODD,  NO,  STATIC),
    BR_EULER_ZYZ_S = BR_EULER_ORDER(Z, ODD,  YES, STATIC),

    BR_EULER_ZYX_R = BR_EULER_ORDER(X, EVEN, NO,  ROTATING),
    BR_EULER_XYX_R = BR_EULER_ORDER(X, EVEN, YES, ROTATING),
    BR_EULER_YZX_R = BR_EULER_ORDER(X, ODD,  NO,  ROTATING),
    BR_EULER_XZX_R = BR_EULER_ORDER(X, ODD,  YES, ROTATING),
    BR_EULER_XZY_R = BR_EULER_ORDER(Y, EVEN, NO,  ROTATING),
    BR_EULER_YZY_R = BR_EULER_ORDER(Y, EVEN, YES, ROTATING),
    BR_EULER_ZXY_R = BR_EULER_ORDER(Y, ODD,  NO,  ROTATING),
    BR_EULER_YXY_R = BR_EULER_ORDER(Y, ODD,  YES, ROTATING),
    BR_EULER_YXZ_R = BR_EULER_ORDER(Z, EVEN, NO,  ROTATING),
    BR_EULER_ZXZ_R = BR_EULER_ORDER(Z, EVEN, YES, ROTATING),
    BR_EULER_XYZ_R = BR_EULER_ORDER(Z, ODD,  NO,  ROTATING),
    BR_EULER_ZYZ_R = BR_EULER_ORDER(Z, ODD,  YES, ROTATING)
};
// clang-format on

/*
 * A triple of euler angles and a description of how they are to
 * be applied - loosely based on -
 * 	"Euler Angle Conversion" Ken Shoemake, Graphics Gems IV pp. 222
 */
typedef struct br_euler {
    br_angle  a;
    br_angle  b;
    br_angle  c;
    br_uint_8 order;
} br_euler;

#endif
