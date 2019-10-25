#include "ofxMeshShape.h"

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
}

#pragma mark - Shape2D

vec3 Shape2D::getNormal() const
{
	return rotation_ * vec3(0,0,1);
}

void Shape2D::setRotation(float degrees, const vec3 &axis)
{
	rotation_ = rotate(quat(), degrees, axis);
}


#pragma mark - Rectangle

vec4 Rectangle::getPlate() const
{
	vec3 normal = getNormal();
	return glm::vec4(normal, -dot(normal, position));
}
vector<vec3> Rectangle::getVertices() const
{
	vector<vec3> vertices = getVerticesFromOrigin();
	for(auto &&v : vertices) {
		v = position + rotation_ * v;
	}
	return vertices;
}

vector<vec3> Rectangle::getVerticesFromOrigin() const
{
	vector<vec3> vertices;
	vertices.resize(4);
	switch(rectmode_) {
		case OF_RECTMODE_CORNER:
			vertices[0] = vec3(0,0,0);
			vertices[1] = vec3(width, 0, 0);
			vertices[2] = vec3(width, height, 0);
			vertices[3] = vec3(0, height, 0);
			break;
		case OF_RECTMODE_CENTER:
			vertices[0] = vec3(-width/2.f, -height/2.f, 0);
			vertices[1] = vec3(width/2.f, -height/2.f, 0);
			vertices[2] = vec3(width/2.f, height/2.f, 0);
			vertices[3] = vec3(-width/2.f, height/2.f, 0);
			break;
	}
	return vertices;
}
ofMesh Rectangle::getOutline() const
{
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_LINE_LOOP);
	mesh.addVertices(getVertices());
	setNormal(mesh, getNormal());
	return mesh;
}
ofMesh Rectangle::getOutline(float width_inner, float width_outer) const
{
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	vector<vec3> baseline = getVerticesFromOrigin();
	for(int i = 0; i < 4; ++i) {
		vec3 inner = vec3(((i==1||i==2) ? -1: 1)*width_inner, ((i==2||i==3) ? -1: 1)*width_inner, 0);
		vec3 outer = -vec3(((i==1||i==2) ? -1: 1)*width_outer, ((i==2||i==3) ? -1: 1)*width_outer, 0);
		mesh.addVertex(position + rotation_ * (baseline[i]+inner));
		mesh.addVertex(position + rotation_ * (baseline[i]+outer));
	}
	setNormal(mesh, getNormal());
	mesh.addIndices({0,1,2,3,4,5,6,7,0,1});
	return mesh;
}

ofMesh Rectangle::getFace() const
{
	ofMesh mesh = getOutline();
	mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	mesh.addIndices({0,1,3,2});
	return mesh;
}
