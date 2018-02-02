/* Copyright (c) 2017, Waterloo Autonomous Vehicles Laboratory (WAVELab),
 * Waterloo Intelligent Systems Engineering Lab (WISELab),
 * University of Waterloo.
 *
 * Refer to the accompanying LICENSE file for license information.
 *
 * ############################################################################
 ******************************************************************************
 |                                                                            |
 |                         /\/\__/\_/\      /\_/\__/\/\                       |
 |                         \          \____/          /                       |
 |                          '----________________----'                        |
 |                              /                \                            |
 |                            O/_____/_______/____\O                          |
 |                            /____________________\                          |
 |                           /    (#UNIVERSITY#)    \                         |
 |                           |[**](#OFWATERLOO#)[**]|                         |
 |                           \______________________/                         |
 |                            |_""__|_,----,_|__""_|                          |
 |                            ! !                ! !                          |
 |                            '-'                '-'                          |
 |       __    _   _  _____  ___  __  _  ___  _    _  ___  ___   ____  ____   |
 |      /  \  | | | ||_   _|/ _ \|  \| |/ _ \| \  / |/ _ \/ _ \ /     |       |
 |     / /\ \ | |_| |  | |  ||_||| |\  |||_|||  \/  |||_||||_|| \===\ |====   |
 |    /_/  \_\|_____|  |_|  \___/|_| \_|\___/|_|\/|_|\___/\___/ ____/ |____   |
 |                                                                            |
 ******************************************************************************
 * ############################################################################
 *
 * File: ref_ekf.hpp
 * Desc: File containing the function for pose composition with uncertainty.
 *
 * References:
 *       [1] J. Blanco, “A tutorial on se (3) transformation parameterizations
 *           and on-manifold optimization,” Univ. Malaga, Tech. Rep, no. 3,
 *           pp. 1–56, 2014.
 * Auth: Chunshang Li and Jordan Hu
 *
 * ############################################################################
 */

#ifndef REF_EKF_POSE_COV_COMP_HPP_
#define REF_EKF_POSE_COV_COMP_HPP_

#include <Eigen/Core>
#include <Eigen/Dense>

namespace wave {

typedef Eigen::Matrix<double, 7, 1> Vector7;
typedef Eigen::Matrix<double, 6, 1> Vector6;
typedef Eigen::Matrix<double, 4, 1> Vector4;
typedef Eigen::Matrix<double, 3, 1> Vector3;
typedef Eigen::Matrix<double, 6, 6, Eigen::RowMajor> Matrix6x6;
typedef Eigen::Matrix<double, 6, 7, Eigen::RowMajor> Matrix6x7;
typedef Eigen::Matrix<double, 7, 6, Eigen::RowMajor> Matrix7x6;
typedef Eigen::Matrix<double, 4, 4, Eigen::RowMajor> Matrix4x4;
typedef Eigen::Matrix<double, 3, 4, Eigen::RowMajor> Matrix3x4;
typedef Eigen::Matrix<double, 7, 7, Eigen::RowMajor> Matrix7x7;
typedef Eigen::Matrix<double, 3, 7, Eigen::RowMajor> Matrix3x7;
typedef Eigen::Matrix<double, 3, 3, Eigen::RowMajor> Matrix3x3;

struct PoseWithCovariance {
    Vector3 position;
    Matrix3x3 rotation_matrix;
    Matrix6x6 covariance;

    PoseWithCovariance();
    PoseWithCovariance(Vector6 &p, Matrix6x6 &q);
    PoseWithCovariance(Vector3 &p, Matrix3x3 &r, Matrix6x6 &q);
    Vector3 getPosition() const;
    Vector3 getYPR() const;
    Vector4 getQuaternion() const;
    Vector7 getPoseQuaternion() const;
    Eigen::Affine3d getTransformMatrix() const;
};

/*
 * Pose composition based on [1]
 */
PoseWithCovariance composePose(PoseWithCovariance &p1,
                               PoseWithCovariance &p);

/*
 * Normalizes Quaternion where qr^2 + qx^2 + qy^2 + qz^2 = 1
 * Equation(1.6)
 * @return: Vector4 [qw, qx, qy, qz]
 */
Vector4 normalizeQuat(const Vector4 q);

/*
 * Converts Quaternion to Yaw Pitch Roll angles
 * Equation(2.9) and Equation(2.10)
 * Note v(0) is yaw, v(1) is pitch, v(2) is roll
 * @return: Vector4 [qw, qx, qy, qz]
 */
Vector3 quatToYPR(const Vector4 &q);

/*
 * Converts Yaw Pitch Roll angles to Quaternion
 * Equation(2.3), Equation(2.4), Equation(2.5), Equation(2.6)
 * @return: Vector3 [yaw, pitch, roll]
 */
Vector4 yprToQuat(const Vector3 &ypr);

/*
 * Converts Yaw Pitch Roll angles to Rotation Matrix
 * Equation(2.18)
 * @return: Matrix3x3
 */
Matrix3x3 yprToRotMatrix(const Vector3 &ypr);

/*
 * Converts Rotation Matrix to Yaw Pitch Roll angles
 * Equation(2.20), Equation(2.21), Equation(2.22), and Equation(2.23)
 * @return: Vector3 [yaw, pitch, roll]
 */
Vector3 rotMatrixToYPR(const Matrix3x3 &p);

/*
 * Converts Rotation Matrix to Quaternion
 * According to [1], fastest way to convert from Rotation Matrix to Quat is to
 * convert Quaternion -> Euler Angles -> Rotation Matrix
 * @return: Vector4 [qw, qx, qy, qz]
 */
Vector4 rotMatrixToQuat(const Matrix3x3 &p);

/// the jacobian of quaternion normalization function
/// quat in the form of [qr, qx, qt, qz]
/// Equation (1.7)
Matrix4x4 jacobian_Quat_Norm_wrt_q(const Vector4 &q);

// the jacobian of normalized quaternion to rpy function
// Equation (2.9) to Equation (2.10)
Matrix3x4 jacobian_Quat_Norm_to_Rpy_wrt_q(const Vector4 &q);

/// the jocobian of p7 to p6 conversion
/// Equation (2.12)
Matrix6x7 jacobian_p7_to_p6_wrt_p(const Vector7 &p);

/// jacobian of composing a point to a p7
/// Equation (3.8)
Matrix3x7 jacobian_p7_Point_Composition_wrt_p(const Vector7 &p,
                                              const Vector3 &a);
/// jacobian of the composition of p7 poses
/// Equation (5.8)
Matrix7x7 jacobian_p7_p7_Composition_wrt_p1(const Vector7 &p1,
                                            const Vector7 &p2);

/// jacobian of composing a point to a p7
/// Equation (3.10)
Matrix3x3 jacobian_p7_Point_Composition_wrt_a(const Vector7 &p,
                                              const Vector3 &a);

/// jacobian of the composition of p7 poses
/// Equation (5.9)
Matrix7x7 jacobian_p7_p7_Composition_wrt_p2(const Vector7 &p1,
                                            const Vector7 &p2);

/// jacobian of converting a p6 to a p7
/// Equation (2.8)
Matrix7x6 jacobian_p6_to_p7_wrt_p(const Vector6 &p);
}  // namespace pose_cov_comp


#endif
