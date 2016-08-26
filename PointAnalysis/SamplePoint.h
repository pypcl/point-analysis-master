#ifndef SAMPLEPOINT_H
#define SAMPLEPOINT_H

#include <Eigen/Core>
#include <array>

typedef double Real;

class SamplePoint
{
public:
	SamplePoint();
	~SamplePoint();
	SamplePoint(const SamplePoint &sample);
	SamplePoint(Eigen::Vector3f vertex, Eigen::Vector3f normal);
	SamplePoint(Eigen::Vector3f vertex, Eigen::Vector3f normal, int face_index, std::array<Real, 8> corner_weights);
	SamplePoint(Eigen::Vector3f vertex);
	SamplePoint(float x, float y, float z, float nx, float ny, float nz);
	SamplePoint(float x, float y, float z);

	float x() const;
	float y() const;
	float z() const;
	float nx() const;
	float ny() const;
	float nz() const;
	Eigen::Vector3f getVertex() const;
	Eigen::Vector3f getNormal() const;
	void setVertex(Eigen::Vector3f vertex);
	void setNormal(Eigen::Vector3f normal);
	double visibility() const;
	void setVisibility(double visibility);
	std::array<Real, 8> getCornerWeights() const;
	void setCornerWeights(std::array<Real, 8> corner_weights);
	int getFaceIndex() const;

	float & operator[](int index);

private:
	Eigen::Vector3f m_vertex;
	Eigen::Vector3f m_normal;
	double m_visibility;
	std::array<Real, 8> m_corner_weights;
	int m_face_index;
};

#endif