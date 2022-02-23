#include "Movable.h"
#include <iostream>
Movable::Movable()
{
	Tout = Eigen::Affine3d::Identity();
	Tin = Eigen::Affine3d::Identity();
}

Movable::Movable(const Movable& mov)
{
	Tout = mov.Tout;
	Tin = mov.Tin;
}

Eigen::Matrix4f Movable::MakeTransScale()
{
	return (Tout.matrix() * Tin.matrix()).cast<float>();
}

Eigen::Matrix4d Movable::MakeTransScaled()
{
	return (Tout.matrix() * Tin.matrix());
}

Eigen::Matrix4d Movable::MakeTransd()
{
	Eigen::Matrix4d mat = Eigen::Matrix4d::Identity();
	mat.col(3) << Tin.translation(), 1;

	return (Tout.matrix() * mat);
}

void Movable::MyTranslate(Eigen::Vector3d amt, bool preRotation)
{

	if (preRotation)
		Tout.pretranslate(amt);
	else
		Tout.translate(amt);
}
//angle in radians
void Movable::MyRotate(Eigen::Vector3d rotAxis, double angle)
{
	Tout.rotate(Eigen::AngleAxisd(angle, rotAxis.normalized()));
}

void Movable::MyRotate(const Eigen::Matrix3d& rot)
{
	Tout.rotate(rot);
}

//Ass 3 comment
void Movable::MyRotate(Eigen::Vector3d rotAxis, double angle, bool zAxis)
{
	Eigen::Matrix3d mat = Tout.rotation().matrix();
	mat.transposeInPlace();
	if (zAxis)
		Tout.rotate(Eigen::AngleAxisd(angle, mat * rotAxis));
	else
		Tout.rotate(Eigen::AngleAxisd(angle, rotAxis));
}
//end Ass3 comment


void Movable::MyScale(Eigen::Vector3d amt)
{
	Tin.scale(amt);
}

void Movable::SetCenterOfRotation(Eigen::Vector3d amt)
{
	Tout.pretranslate(amt);
	Tin.pretranslate(-amt);

}

void Movable::MyTranslateInSystem(Eigen::Matrix3d rot, Eigen::Vector3d amt)
{
	Tout.pretranslate(rot.transpose() * amt);
}

void Movable::RotateInSystem(Eigen::Vector3d rotAxis, double angle)
{
	Tout.rotate(Eigen::AngleAxisd(angle, Tout.rotation().transpose() *
		rotAxis.normalized())); //we will multiply the vector with the rotate matrix

}

// --------------------------------------------------Project--------------------------------------------------------------------

Eigen::Quaterniond Movable::GetRotationQ()
{
	return Eigen::Quaterniond(Tout.rotation());
}

void Movable::RotateInSystem(Eigen::Matrix4d Mat, Eigen::Vector3d rotAxis, double angle)
{
	Eigen::Vector3d v = Mat.transpose().block<3, 3>(0, 0) * rotAxis; //transpose instead of inverse
	MyRotate(v.normalized(), angle);
}
// ----------------------------------------------------Project-----------------------------------------------------------------------

//Project comment

void Movable::MyRotate(const Eigen::Quaterniond rot)
{
	Tout.rotate(rot);
}

void Movable::RotateInSystem(const Eigen::Matrix3d& mat, const Eigen::Quaterniond rot)
{
	Eigen::Quaterniond q = Eigen::Quaterniond(mat);
	MyRotate((q * rot.conjugate()) * q.conjugate());
}

//end Project comment
























