#include "obb.h"

OBB::OBB(QObject *parent)
	: QObject(parent), m_count(0)
{

}

OBB::OBB(QVector3D xAxis, QVector3D yAxis, QVector3D zAxis, QVector3D centroid, 
	double xLength, double yLength, double zLength, int label, QObject *parent)
	: QObject(parent), m_count(0)
{
	x_axis = xAxis;
	y_axis = yAxis;
	z_axis = zAxis;

	x_length = xLength;
	y_length = yLength;
	z_length = zLength;
	m_centroid = centroid;
	m_label = label;

	if (m_label >= 0 && m_label <= 10)
	{
		m_color.setX(COLORS[m_label][0]);
		m_color.setY(COLORS[m_label][1]);
		m_color.setZ(COLORS[m_label][2]);
	}
	else
	{
		m_color.setX(COLORS[0][0]);
		m_color.setY(COLORS[0][1]);
		m_color.setZ(COLORS[0][2]);
	}
}

OBB::~OBB()
{

}

QVector3D OBB::getXAxis()
{
	return QVector3D(x_axis);
}

QVector3D OBB::getYAxis()
{
	return QVector3D(y_axis);
}

QVector3D OBB::getZAxis()
{
	return QVector3D(z_axis);
}

QVector3D OBB::getScale()
{
	return QVector3D(x_length, y_length, z_length);
}

QVector<QVector3D> OBB::getAxes()
{
	QVector<QVector3D> axes(3);
	axes[0] = QVector3D(x_axis);
	axes[1] = QVector3D(y_axis);
	axes[2] = QVector3D(z_axis);

	return axes;
}

void OBB::triangulate()
{
	QMatrix4x4 translate_matrices[8];
	translate_matrices[0].setToIdentity();
	translate_matrices[0].translate(x_length / 2.0 * x_axis);
	translate_matrices[0].translate(z_length / 2.0 * z_axis);
	translate_matrices[0].translate(y_length / 2.0  * y_axis);
	translate_matrices[1].setToIdentity();
	translate_matrices[1].translate(-x_length / 2.0  * x_axis);
	translate_matrices[1].translate(z_length / 2.0  * z_axis);
	translate_matrices[1].translate(y_length / 2.0  * y_axis);
	translate_matrices[2].setToIdentity();
	translate_matrices[2].translate(-x_length / 2.0  * x_axis);
	translate_matrices[2].translate(-y_length / 2.0  * y_axis);
	translate_matrices[2].translate(z_length / 2.0  * z_axis);
	translate_matrices[3].setToIdentity();
	translate_matrices[3].translate(x_length / 2.0  * x_axis);
	translate_matrices[3].translate(-y_length / 2.0  * y_axis);
	translate_matrices[3].translate(z_length / 2.0  * z_axis);
	translate_matrices[4].setToIdentity();
	translate_matrices[4].translate(x_length / 2.0  * x_axis);
	translate_matrices[4].translate(y_length / 2.0  * y_axis);
	translate_matrices[4].translate(-z_length / 2.0  * z_axis);
	translate_matrices[5].setToIdentity();
	translate_matrices[5].translate(-x_length / 2.0  * x_axis);
	translate_matrices[5].translate(y_length / 2.0  * y_axis);
	translate_matrices[5].translate(-z_length / 2.0  * z_axis);
	translate_matrices[6].setToIdentity();
	translate_matrices[6].translate(-x_length / 2.0  * x_axis);
	translate_matrices[6].translate(-y_length / 2.0  * y_axis);
	translate_matrices[6].translate(-z_length / 2.0  * z_axis);
	translate_matrices[7].setToIdentity();
	translate_matrices[7].translate(x_length / 2.0  * x_axis);
	translate_matrices[7].translate(-y_length / 2.0  * y_axis);
	translate_matrices[7].translate(-z_length / 2.0  * z_axis);

	QVector3D vertices[8];
	for (int i = 0; i < 8; i++)
		vertices[i] = translate_matrices[i] * m_centroid;

	QVector3D face_normals[6] = {
		x_axis, y_axis, z_axis, -x_axis, -y_axis, -z_axis
	};

	m_data.resize(192);

	add(vertices[0], vertices[1], vertices[3], face_normals[2]);
	add(vertices[1], vertices[2], vertices[3], face_normals[2]);
	add(vertices[0], vertices[3], vertices[7], face_normals[0]);
	add(vertices[0], vertices[7], vertices[4], face_normals[0]);
	add(vertices[0], vertices[4], vertices[1], face_normals[1]);
	add(vertices[1], vertices[4], vertices[5], face_normals[1]);
	add(vertices[4], vertices[7], vertices[6], face_normals[5]);
	add(vertices[4], vertices[6], vertices[5], face_normals[5]);
	add(vertices[1], vertices[5], vertices[2], face_normals[3]);
	add(vertices[2], vertices[5], vertices[6], face_normals[3]);
	add(vertices[2], vertices[7], vertices[3], face_normals[4]);
	add(vertices[2], vertices[6], vertices[7], face_normals[4]);
}

void OBB::add(QVector3D v0, QVector3D v1, QVector3D v2, QVector3D normal)
{
	float * triangle = m_data.data() + m_count;
	/* The fist vertex of the triangle */
	triangle[0] = v0.x();
	triangle[1] = v0.y();
	triangle[2] = v0.z();
	/* The second vertex of the triangle */
	triangle[3] = v1.x();
	triangle[4] = v1.y();
	triangle[5] = v1.z();
	/* The third vertex of the triangle */
	triangle[6] = v2.x();
	triangle[7] = v2.y();
	triangle[8] = v2.z();
	/* The normal vector of the triangle facet */
	triangle[9] = normal.x();
	triangle[10] = normal.y();
	triangle[11] = normal.z();

	m_count += 12;
}