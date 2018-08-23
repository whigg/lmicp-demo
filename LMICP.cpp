// LMICP.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "projection_parameters.h"
#include "lm-icp.h"
#include <iostream>
#include "eigen/Eigen/Dense"
#include "opencv/cv.hpp"

int main()
{
	const int N_d = 4;
	const int N_p = 12;

	// Parameters
	float r_11 = 1;
	float r_12 = 0;
	float r_13 = 0;
	float r_21 = 0;
	float r_22 = 1;
	float r_23 = 0;
	RotationMatrix R = RotationMatrix(r_11, r_12, r_13, r_21, r_22, r_23);
	

	float f = 100;
	ProjectionMatrix P = ProjectionMatrix(f);
	

	float tau_x = 0;
	float tau_y = 0;
	float tau_z = -50;
	CameraDisplacementVector vec_tau = CameraDisplacementVector(tau_x, tau_y, tau_z);

	float o_x = 320;
	float o_y = 240;
	OpticalAxisVector vec_o = OpticalAxisVector(o_x, o_y);

	// Sample data points

	float xI_1 = 480;
	float yI_1 = 272;
	float xI_2 = 576;
	float yI_2 = 332;
	float xI_3 = 320;
	float yI_3 = 332;
	float xI_4 = 220;
	float yI_4 = 280;
	Eigen::VectorXf vec_xI(N_d * 2);
	vec_xI(0) = xI_1;
	vec_xI(1) = yI_1;
	vec_xI(2) = xI_2;
	vec_xI(3) = yI_2;
	vec_xI(4) = xI_3;
	vec_xI(5) = yI_3;
	vec_xI(6) = xI_4;
	vec_xI(7) = yI_4;
	
	// Original Points
	float x_1 = 0;
	float y_1 = -50;
	float z_1 = -50;
	float x_2 = -50;
	float y_2 = 50;
	float z_2 = -50;
	float x_3 = 50;
	float y_3 = 50;
	float z_3 = -50;
	float x_4 = 75;
	float y_4 = 25;
	float z_4 = -50;

	Eigen::VectorXf pointsToProject(N_d * 3);
	pointsToProject(0) = x_1;
	pointsToProject(1) = y_1;
	pointsToProject(2) = z_1;
	pointsToProject(3) = x_2;
	pointsToProject(4) = y_2;
	pointsToProject(5) = z_2;
	pointsToProject(6) = x_3;
	pointsToProject(7) = y_3;
	pointsToProject(8) = z_3;
	pointsToProject(9) = x_4;
	pointsToProject(10) = y_4;
	pointsToProject(11) = z_4;

	auto originalParams = ProjectionParameters(&R, f, &vec_tau, &vec_o);
	auto params = ProjectionParameters(&R, f, &vec_tau, &vec_o);

	ProjectionParameters fitted_params = FitByLmIcp(params, N_d, pointsToProject, vec_xI);

	// Draw
	cv::Mat im = cv::Mat::zeros(480, 640, CV_8UC3);

	Eigen::VectorXf originalProjectedPoints = originalParams.Project(N_d, pointsToProject);
	cv::circle(im, cv::Point(originalProjectedPoints[0], originalProjectedPoints[1]), 4, cv::Scalar(0, 255, 0), 3, CV_AA);
	cv::circle(im, cv::Point(originalProjectedPoints[2], originalProjectedPoints[3]), 4, cv::Scalar(0, 255, 128), 3, CV_AA);
	cv::circle(im, cv::Point(originalProjectedPoints[4], originalProjectedPoints[5]), 4, cv::Scalar(0, 255, 192), 3, CV_AA);
	cv::circle(im, cv::Point(originalProjectedPoints[6], originalProjectedPoints[7]), 4, cv::Scalar(0, 255, 192), 3, CV_AA);

	cv::circle(im, cv::Point(xI_1, yI_1), 8, cv::Scalar(255, 0, 0), 3, CV_AA);
	cv::circle(im, cv::Point(xI_2, yI_2), 8, cv::Scalar(255, 128, 0), 3, CV_AA);
	cv::circle(im, cv::Point(xI_3, yI_3), 8, cv::Scalar(255, 192, 0), 3, CV_AA);
	cv::circle(im, cv::Point(xI_4, yI_4), 8, cv::Scalar(255, 192, 0), 3, CV_AA);

	Eigen::VectorXf icpProjectedPoints = fitted_params.Project(N_d, pointsToProject);
	printf("Result: (%f, %f), (%f, %f), (%f, %f)", icpProjectedPoints[0], icpProjectedPoints[1], icpProjectedPoints[2],
		icpProjectedPoints[3], icpProjectedPoints[4], icpProjectedPoints[5]);
	cv::circle(im, cv::Point(icpProjectedPoints[0], icpProjectedPoints[1]), 4, cv::Scalar(0, 0, 255), 3, CV_AA);
	cv::circle(im, cv::Point(icpProjectedPoints[2], icpProjectedPoints[3]), 4, cv::Scalar(128, 0, 255), 3, CV_AA);
	cv::circle(im, cv::Point(icpProjectedPoints[4], icpProjectedPoints[5]), 4, cv::Scalar(192, 0, 255), 3, CV_AA);
	cv::circle(im, cv::Point(icpProjectedPoints[6], icpProjectedPoints[7]), 4, cv::Scalar(192, 0, 255), 3, CV_AA);

	cv::imshow("test", im);
	cv::waitKey(0);

    return 0;
}

