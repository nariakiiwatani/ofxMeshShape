#include "ofxMeshShape.h"
#include "ofPath.h"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

using namespace ofx::meshshape;
using namespace std;
using namespace glm;

namespace {
	void setNormal(ofMesh &mesh, vec3 normal) {
		mesh.clearNormals();
		for(auto &&v : mesh.getVertices()) {
			mesh.addNormal(normal);
		}
	}
	// https://imagingsolution.blog.fc2.com/blog-entry-137.html
	bool getIntersection(vec2 p0, vec2 p1, vec2 q0, vec2 q1, vec2 &dst) {
		vec2 a1 = q1-q0;
		vec2 a2 = p1-p0;
		vec2 b1 = q0-p1;
		vec2 b2 = p0-q0;
		float s1 = (a1.x*b2.y - a1.y*b2.x)/2.f;
		float s2 = (a1.x*b1.y - a1.y*b1.x)/2.f;
		if(s1 + s2 == 0) {
			return false;
		}
		dst = p0 + a2 * s1 / (s1 + s2);
		return true;
	}
	bool getIntersection3D(vec3 p0, vec3 p1, vec3 q0, vec3 q1, vec3 &dst) {
		vec3 a1 = q1-q0;
		vec3 a2 = p1-p0;
		vec3 b1 = q0-p1;
		vec3 b2 = p0-q0;
		vec3 c1 = cross(a1,b2);
		vec3 c2 = cross(a1,b1);
		vec3 c3 = cross(-b1, b2);
		auto sign = [c3](const vec3 &src) {
			return ofSign(dot(c3, src));
		};
		float s1 = sign(c1)*length(c1)/2.f;
		float s2 = sign(c2)*length(c2)/2.f;
		if(s1 + s2 == 0) {
			return false;
		}
		dst = p0 + a2 * s1 / (s1 + s2);
		return true;
	}
}

#pragma mark - Shape

void Shape::setRotation(float degrees, const vec3 &axis)
{
	rotation_ = rotate(quat(), degrees, axis);
}

ofMesh Shape::getOutline() const
{
	ofMesh mesh;
	mesh.addVertices(getVertices());
	for(auto &&v : mesh.getVertices()) {
		v = rotation_*(v-anchor_) + anchor_;
	}
	return mesh;
}

#pragma mark - Shape2D

vec3 Shape2D::getNormal() const
{
	return rotation_ * vec3(0,0,-1);
}

glm::vec4 Shape2D::getPlate() const
{
	auto &&vertices = getVertices();
	vec3 normal = getNormal();
	return glm::vec4(normal, vertices.empty() ? 0 : dot(-normal, vertices.front()));
}

ofMesh Shape2D::getOutline() const
{
	ofMesh mesh = Shape::getOutline();
	mesh.setMode(isClosed() ? OF_PRIMITIVE_LINE_LOOP : OF_PRIMITIVE_LINE_STRIP);
	setNormal(mesh, getNormal());
	return mesh;
}

ofMesh Shape2D::getOutline(float width_inner, float width_outer) const
{
	auto getSlidedVertex = [](vec3 v0, vec3 v1, vec3 v2, float width_counterclockwise, vec3 normal) {
		vec3 axis = cross(v2-v1, v1-v0);
		int sign = ofSign(dot(normal, axis));
		vec3 move01 = sign*normalize(glm::rotate(v1-v0, half_pi<float>(), axis))*width_counterclockwise;
		vec3 move12 = sign*normalize(glm::rotate(v2-v1, half_pi<float>(), axis))*width_counterclockwise;
		vec3 intersection;
		return getIntersection3D(v0+move01, v1+move01, v1+move12, v2+move12, intersection)
			? intersection : v1+move01;
	};
	auto wrapIndex = [](int index, size_t size) {
		return (index+size)%size;
	};
	ofMesh mesh = getOutline();
	mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	auto vertices = mesh.getVertices();
	if(vertices.size() < 2) {
		return ofMesh();
	}
	mesh.clearVertices();
	mesh.clearIndices();
	size_t num = vertices.size();
	for(unsigned int i = 0; i < num; ++i) {
		vec3 v0 = vertices[wrapIndex(i-1, num)],
		v1 = vertices[wrapIndex(i, num)],
		v2 = vertices[wrapIndex(i+1, num)];
		vec3 inner = getSlidedVertex(v0, v1, v2, width_inner, getNormal());
		vec3 outer = getSlidedVertex(v0, v1, v2, -width_outer, getNormal());
		mesh.addVertex(inner);
		mesh.addVertex(outer);
		mesh.addIndices({i*2, i*2+1});
	}
	setNormal(mesh, getNormal());
	if(isClosed()) {
		mesh.addIndices({0,1});
	}
	return mesh;
}


#pragma mark - Rectangle

vector<vec3> Rectangle::getVertices() const
{
	vector<vec3> vertices(4, position);
	switch(rectmode_) {
		case OF_RECTMODE_CORNER:
			vertices[0] += vec3(0,0,0);
			vertices[1] += vec3(0, height, 0);
			vertices[2] += vec3(width, height, 0);
			vertices[3] += vec3(width, 0, 0);
			break;
		case OF_RECTMODE_CENTER:
			vertices[0] += vec3(-width/2.f, -height/2.f, 0);
			vertices[1] += vec3(-width/2.f, height/2.f, 0);
			vertices[2] += vec3(width/2.f, height/2.f, 0);
			vertices[3] += vec3(width/2.f, -height/2.f, 0);
			break;
	}
	return vertices;
}
ofMesh Rectangle::getFace() const
{
	ofMesh mesh = getOutline();
	mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	mesh.addIndices({0,1,3,2});
	return mesh;
}

#pragma mark - Contour

ofMesh Contour::getFace() const
{
	auto &&vertices = Shape::getOutline().getVertices();
	if(vertices.empty()) {
		return Shape2D::getFace();
	}
	ofPath path;
	path.moveTo(vertices[0]);
	for(auto &&v : vertices) {
		path.lineTo(v);
	}
	path.lineTo(vertices[0]);
	path.close();
	ofMesh mesh = path.getTessellation();
	setNormal(mesh, getNormal());
	return mesh;
}

