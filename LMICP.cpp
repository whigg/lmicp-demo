// LMICP.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "projection_parameters.h"
#include <iostream>
#include "eigen/Eigen/Dense"
#include "opencv/cv.hpp"

Eigen::Vector2f Project(Eigen::Vector3f x, Eigen::Matrix<float, 3, 3> R, float f, Eigen::Vector3f tau, Eigen::Vector2f o)
{
	Eigen::Vector2f out;

	Eigen::Matrix<float, 2, 3> P;
	P << f / x[2], 0.0f, 0.0f,
		0.0f, -f / x[2], 0.0f;

	out = P * (R * x + tau) + o;
	return out;
}

int main()
{
	const int N_d = 3;
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
	Eigen::VectorXf vec_xI(N_d * 2);
	vec_xI(0) = xI_1;
	vec_xI(1) = yI_1;
	vec_xI(2) = xI_2;
	vec_xI(3) = yI_2;
	vec_xI(4) = xI_3;
	vec_xI(5) = yI_3;
	
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

	auto originalParams = ProjectionParameters(&R, f, &vec_tau, &vec_o);
	auto params = ProjectionParameters(&R, f, &vec_tau, &vec_o);
	/*
	Eigen::VectorXf testProjectedPoints = params.Project(N_d, pointsToProject);

	printf("(%f, %f), ", testProjectedPoints[0], testProjectedPoints[1]);
	printf("(%f, %f), ", testProjectedPoints[2], testProjectedPoints[3]);
	printf("(%f, %f)\n", testProjectedPoints[4], testProjectedPoints[5]);
	*/
	
	
	// LM-ICP
	const float epsilon = 0.1;
	const int max_iterations = 100;
	float lambda = 0.001;
	float max_lambda = 10.0f;
	Eigen::VectorXf vec_a = params.GetParamsAsVector();	// set a = a_0
	int k = 0;
	while (k < max_iterations && lambda < max_lambda)
	{
		// Compute e_k (Error Vector)
		Eigen::VectorXf projectedPoints = params.Project(N_d, pointsToProject);

		Eigen::VectorXf X(N_d);
		Eigen::VectorXf Y(N_d);
		Eigen::VectorXf vec_e_k(N_d);
		for (int i = 0; i < N_d; i++)
		{
			X(i) = vec_xI(i * 2 + 0) - projectedPoints(i * 2 + 0);
			Y(i) = vec_xI(i * 2 + 1) - projectedPoints(i * 2 + 1);
			vec_e_k(i) = sqrt(X(i) * X(i) + Y(i) * Y(i));
		}

		// Compute Jacobian Matrix
		// For experiment and limiting parameters into o_x & o_y, now J is {Nd×2}

		Eigen::MatrixXf J(N_d, N_p);

		for (int i = 0; i < N_d; i++)
		{
			float sqrtPowXPlusPowY = sqrt(X(i)*X(i) + Y(i)*Y(i));
			float x_i = pointsToProject(i * 3 + 0);
			float y_i = pointsToProject(i * 3 + 1);
			float z_i = pointsToProject(i * 3 + 2);
			// r1-r6
			J(i, 0) = -X(i) / sqrtPowXPlusPowY * f / z_i * x_i;
			J(i, 1) = -X(i) / sqrtPowXPlusPowY * f / z_i * y_i;
			J(i, 2) = -X(i) * f / sqrtPowXPlusPowY;
			J(i, 3) = Y(i) / sqrtPowXPlusPowY * f / z_i * x_i;
			J(i, 4) = Y(i) / sqrtPowXPlusPowY * f / z_i * y_i;
			J(i, 5) = Y(i) * f / sqrtPowXPlusPowY;
			// f
			J(i, 6) = 0;
			// tau
			J(i, 7) = -X(i) / sqrtPowXPlusPowY * f / z_i;	// ∂Ei/∂τ_x, using constant f 
			J(i, 8) = Y(i) * f / sqrtPowXPlusPowY / z_i;		// ∂Ei/∂τ_y, using constant f
			J(i, 9) = 0;															// ∂Ei/∂τ_z
			// o
			J(i, 10) = -X(i) / sqrtPowXPlusPowY;	// ∂Ei/∂o_x
			J(i, 11) = -Y(i) / sqrtPowXPlusPowY;	// ∂Ei/∂o_y
		}
		Eigen::MatrixXf JT = J.transpose();

		while (lambda < max_lambda)
		{
			Eigen::MatrixXf lambdaI = Eigen::MatrixXf::Identity(N_p, N_p) * lambda;
			Eigen::VectorXf vec_a_k = vec_a - (JT * J + lambdaI).inverse() * JT * vec_e_k;
			// Compute New Errors
			Eigen::VectorXf vec_e_k_candidate(N_d);
			auto newParams = params.createFromVector(vec_a_k);
			Eigen::VectorXf newProjectedPoints = newParams.Project(N_d, pointsToProject);
			for (int i = 0; i < N_d; i++)
			{
				X(i) = vec_xI(i * 2 + 0) - newProjectedPoints(i * 2 + 0);
				Y(i) = vec_xI(i * 2 + 1) - newProjectedPoints(i * 2 + 1);
				vec_e_k_candidate(i) = sqrt(X(i) * X(i) + Y(i) * Y(i));
			}
			std::cout << "lambda=" << lambda << std::endl;
			std::cout << "e_k:" << std::endl;
			std::cout << vec_e_k << std::endl;
			std::cout << "new e_k:" << std::endl;
			std::cout << vec_e_k_candidate << std::endl;
			std::cout << "new a:" << std::endl;
			std::cout << vec_a_k << std::endl;

			if (vec_e_k.norm() > vec_e_k_candidate.norm())
			{
				std::cout << "break!" << std::endl << std::endl;
				params = params.createFromVector(vec_a_k);
				vec_a = params.GetParamsAsVector();
				break;
			}
			lambda = lambda * 2;
		}
		k++;
	}
	/*
	float X_1 = (xI_1 - x_proj_1[0]);
	float Y_1 = (yI_1 - x_proj_1[1]);
	float e_1 = sqrt(X_1*X_1 + Y_1*Y_1);
	printf("e1 = %f(x_diff=%f, y_diff=%f)\n", e_1, X_1, Y_1);
	float X_2 = (xI_2 - x_proj_2[0]);
	float Y_2 = (yI_2 - x_proj_2[1]);
	float e_2 = sqrt(X_2*X_2 + Y_2*Y_2);
	printf("e2 = %f(x_diff=%f, y_diff=%f)\n", e_2, X_2, Y_2);
	float X_3 = (xI_3 - x_proj_3[0]);
	float Y_3 = (yI_3 - x_proj_3[1]);
	float e_3 = sqrt(X_3*X_3 + Y_3 * Y_3);
	printf("e3 = %f\n", e_3);

	Eigen::VectorXf vec_e(3);
	vec_e << e_1, e_2, e_3;
	Eigen::VectorXf vec_X(3);
	vec_X << X_1, X_2, X_3;
	Eigen::VectorXf vec_Y(3);
	vec_X << Y_1, Y_2, Y_3;

	Eigen::VectorXf vec_xi(3);
	vec_xi << x_1, x_2, x_3;
	//printf("<<%f, %f, %f>>", vec_xi(0), vec_xi(1), vec_xi(2));
	Eigen::VectorXf vec_yi(3);
	vec_xi << y_1, y_2, y_3;
	Eigen::VectorXf vec_zi(3);
	vec_xi << z_1, z_2, z_3;

	const int N_p = 2;
	const float epsilon = 0.1;
	Eigen::VectorXf e_k(N_d);
	Eigen::Vector2f a(o_x, o_y);
	while (vec_e.norm() > epsilon)
	{
		float lambda = 0.001;

		// Compute e_k
		Eigen::VectorXf e_k(N_d);
		for (int i = 0; i < N_d; i++)
		{
			Eigen::Vector3f to_project = Eigen::Vector3f(vec_xi(i), vec_yi(i), vec_zi(i));
			std::cout << to_project << std::endl;
			Eigen::Vector2f projected = Project(to_project, R, f, tau, a);
			e_k(i) = sqrt(
				(vec_xI[i] - projected(0))*(vec_xI[i] - projected(0)) +
				(vec_yI[i] - projected(1))*(vec_yI[i] - projected(1))
			);
			//printf("<xi(i)=%f, yi(i)=%f>", vec_xi[i], vec_yi[i]);
			printf("proj:(%f, %f)[%f, diff=%f, %f]", projected[0], projected[1], e_k(i), vec_xI(i) - projected(0), vec_yI(i) - projected(1));
		}

		Eigen::Matrix<float, N_d, N_p> J;
		for (int i = 0; i < N_d; i++)
		{
			//J(i, 0) = vec_X(i) / vec_e(i) * f / vec_zi(i) * vec_xi(i); // ∂Ei/∂r_11
			//J(i, 1) = vec_X(i) / vec_e(i) * f / vec_zi(i) * vec_yi(i); // ∂Ei/∂r_12
			J(i, 0) = -vec_X(i) / vec_e(i);		//∂Ei/∂o_x
			J(i, 1) = -vec_X(i) / vec_e(i);		//∂Ei/∂o_y
		}

		break;	// Test
		
	}
	
	*/
	// Draw
	cv::Mat im = cv::Mat::zeros(480, 640, CV_8UC3);

	Eigen::VectorXf originalProjectedPoints = originalParams.Project(N_d, pointsToProject);
	cv::circle(im, cv::Point(originalProjectedPoints[0], originalProjectedPoints[1]), 4, cv::Scalar(0, 255, 0), 3, CV_AA);
	cv::circle(im, cv::Point(originalProjectedPoints[2], originalProjectedPoints[3]), 4, cv::Scalar(0, 255, 128), 3, CV_AA);
	cv::circle(im, cv::Point(originalProjectedPoints[4], originalProjectedPoints[5]), 4, cv::Scalar(0, 255, 192), 3, CV_AA);

	cv::circle(im, cv::Point(xI_1, yI_1), 8, cv::Scalar(255, 0, 0), 3, CV_AA);
	cv::circle(im, cv::Point(xI_2, yI_2), 8, cv::Scalar(255, 128, 0), 3, CV_AA);
	cv::circle(im, cv::Point(xI_3, yI_3), 8, cv::Scalar(255, 192, 0), 3, CV_AA);

	Eigen::VectorXf icpProjectedPoints = params.Project(N_d, pointsToProject);
	printf("Result: (%f, %f), (%f, %f), (%f, %f)", icpProjectedPoints[0], icpProjectedPoints[1], icpProjectedPoints[2],
		icpProjectedPoints[3], icpProjectedPoints[4], icpProjectedPoints[5]);
	cv::circle(im, cv::Point(icpProjectedPoints[0], icpProjectedPoints[1]), 4, cv::Scalar(0, 0, 255), 3, CV_AA);
	cv::circle(im, cv::Point(icpProjectedPoints[2], icpProjectedPoints[3]), 4, cv::Scalar(128, 0, 255), 3, CV_AA);
	cv::circle(im, cv::Point(icpProjectedPoints[4], icpProjectedPoints[5]), 4, cv::Scalar(192, 0, 255), 3, CV_AA);

	cv::imshow("test", im);
	cv::waitKey(0);

    return 0;
}

