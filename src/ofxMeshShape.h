#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include "ofMesh.h"
#include "ofPolyline.h"
#include "ofRectangle.h"

namespace ofx {
namespace meshshape {
class Shape {
public:
	virtual ofMesh getOutline() const { return ofMesh(); }
	virtual ofMesh getFace() const { return ofMesh(); }
};
class Shape2D : public Shape {
public:
	void setRotation(float degrees, const glm::vec3 &axis);
	glm::vec3 getNormal() const;
	virtual glm::vec4 getPlate() const { return glm::vec4(getNormal(),0); }
	virtual ofMesh getOutline(float width_inner, float width_outer) const { return ofMesh(); }
protected:
	glm::quat rotation_;
};
class Rectangle : public Shape2D, public ofRectangle {
public:
	void setRectMode(ofRectMode mode) { rectmode_ = mode; }
	glm::vec4 getPlate() const;
	ofMesh getOutline() const;
	ofMesh getFace() const;
	ofMesh getOutline(float width_inner, float width_outer) const;
private:
	std::vector<glm::vec3> getVertices() const;
	std::vector<glm::vec3> getVerticesFromOrigin() const;
	
	ofRectMode rectmode_=OF_RECTMODE_CORNER;
};
}
}
