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
	void setRotation(float degrees, const glm::vec3 &axis);
	void setRotation(const glm::quat &rotation) { rotation_ = rotation; }
	void setAnchor(const glm::vec3 &anchor) { anchor_ = anchor; }
	virtual ofMesh getOutline() const;
	virtual ofMesh getFace() const { return ofMesh(); }
	glm::vec3 getAnchor() const { return anchor_; }
	glm::quat getRotation() const { return rotation_; }
protected:
	virtual std::vector<glm::vec3> getVertices() const=0;
	glm::vec3 anchor_;
	glm::quat rotation_;
};
class Shape2D : public Shape {
public:
	virtual glm::vec3 getNormal() const;
	virtual bool isClosed() const=0;
	virtual glm::vec4 getPlate() const;
	virtual ofMesh getOutline() const;
	virtual ofMesh getOutline(float width_inner, float width_outer, ofPrimitiveMode mode) const;
};
class Rectangle : public Shape2D, public ofRectangle {
public:
	void setRectMode(ofRectMode mode) { rectmode_ = mode; }
	bool isClosed() const { return true; }
	ofMesh getFace() const;
protected:
	virtual std::vector<glm::vec3> getVertices() const;
	ofRectMode rectmode_=OF_RECTMODE_CORNER;
};
class Grid : public Rectangle {
public:
	void setDiv(unsigned int u, unsigned int v) { div_u_ = u, div_v_ = v; }
	ofMesh getOutline(float width_inner, float width_outer, ofPrimitiveMode mode) const;
	ofMesh getOutline(float width_inner, float width_outer) const;
	ofMesh getOutline(float width_outline_inner, float width_outline_outer, float width_inner) const;
protected:
	virtual std::vector<glm::vec3> getVertices() const;
	unsigned int div_u_=0, div_v_=0;
};
class Contour : public Shape2D, public ofPolyline {
public:
	bool isClosed() const { return ofPolyline::isClosed(); }
	ofMesh getFace() const;
protected:
	virtual std::vector<glm::vec3> getVertices() const { return ofPolyline::getVertices(); }
};
class AdjacencyLine : public Contour {
public:
	void setLead(const glm::vec3 &pos) { lead_ = pos; }
	void setTrail(const glm::vec3 &pos) { trail_ = pos; }
	bool isClosed() const { return false; }
	ofMesh getOutline() const;
	using Contour::getOutline;
protected:
	virtual std::vector<glm::vec3> getVertices() const;
	glm::vec3 lead_, trail_;
};
}
}
