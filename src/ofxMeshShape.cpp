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

ofMesh Shape2D::getOutline(float width_inner, float width_outer, ofPrimitiveMode mode) const
{
	assert(mode == OF_PRIMITIVE_TRIANGLES || mode == OF_PRIMITIVE_TRIANGLE_STRIP);
	auto getSlidedVertex = [](vec3 v0, vec3 v1, vec3 v2, float width_counterclockwise, vec3 normal) {
		vec3 axis = cross(v2-v1, v1-v0);
		if(length(axis) == 0) axis = normal;
		int sign = ofSign(dot(normal, axis));
		vec3 move01 = sign*normalize(glm::rotate(v1-v0, half_pi<float>(), axis))*width_counterclockwise;
		vec3 move12 = sign*normalize(glm::rotate(v2-v1, half_pi<float>(), axis))*width_counterclockwise;
		vec3 intersection;
		return getIntersection3D(v0+move01, v1+move01, v1+move12, v2+move12, intersection)
			? intersection : v1+move01;
	};
	ofMesh mesh = getOutline();
	auto vertices = mesh.getVertices();
	if(vertices.size() < 2) {
		return ofMesh();
	}
	mesh.clearVertices();
	mesh.clearIndices();
	size_t num = vertices.size();
	auto wrapIndex = [num](int index) {
		return (index+num)%num;
	};
	if(mesh.getMode() == OF_PRIMITIVE_LINE_STRIP_ADJACENCY) {
		num -= 2;
	}
	for(unsigned int i = 0; i < num; ++i) {
		vec3 v0 = vertices[wrapIndex(i+0)],
		v1 = vertices[wrapIndex(i+1)],
		v2 = vertices[wrapIndex(i+2)];
		vec3 inner = getSlidedVertex(v0, v1, v2, width_inner, getNormal());
		vec3 outer = getSlidedVertex(v0, v1, v2, -width_outer, getNormal());
		mesh.addVertex(inner);
		mesh.addVertex(outer);
	}
	setNormal(mesh, getNormal());
	switch(mode) {
		case OF_PRIMITIVE_TRIANGLE_STRIP:
			for(unsigned int i = 0; i < num; ++i) {
				mesh.addIndices({i*2, i*2+1});
			}
			if(isClosed()) {
				mesh.addIndices({0,1});
			}
			break;
		case OF_PRIMITIVE_TRIANGLES: {
			auto indices = [](unsigned int base) -> vector<unsigned int> {
				return {base, base+1, base+2, base+2, base+1, base+3};
			};
			for(unsigned int i = 0; i < num-1; ++i) {
				mesh.addIndices(indices(i*2));
			}
			if(isClosed()) {
				auto ids = indices((num-1)*2);
				ids[2] = ids[3] = 0;
				ids[5] = 1;
				mesh.addIndices(ids);
			}
		}	break;
		default:
			assert(false);
			break;
	}
	mesh.setMode(mode);
	return mesh;
}


#pragma mark - Rectangle

vector<vec3> Rectangle::getVertices() const
{
	vec3 pos = position;
	switch(rectmode_) {
		case OF_RECTMODE_CORNER:
			break;
		case OF_RECTMODE_CENTER:
			pos.x -= width/2.f;
			pos.y -= height/2.f;
			break;
	}
	vector<vec3> vertices(4, pos);
	vertices[0] += vec3(0,0,0);
	vertices[1] += vec3(0, height, 0);
	vertices[2] += vec3(width, height, 0);
	vertices[3] += vec3(width, 0, 0);
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
	auto vertices = Shape::getOutline().getVertices();
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

#pragma mark - AdjacencyLine

vector<vec3> AdjacencyLine::getVertices() const
{
	vector<vec3> vertices;
	auto &&src = Contour::getVertices();
	vertices.reserve(src.size()+2);
	vertices.push_back(rotation_*(lead_-anchor_)+anchor_);
	vertices.insert(std::end(vertices), std::begin(src), std::end(src));
	vertices.push_back(rotation_*(trail_-anchor_)+anchor_);
	return vertices;
}

ofMesh AdjacencyLine::getOutline() const
{
	ofMesh mesh = Contour::getOutline();
	mesh.setMode(OF_PRIMITIVE_LINE_STRIP_ADJACENCY);
	return mesh;
}

#pragma mark - Grid
std::vector<glm::vec3> Grid::getVertices() const
{
	vec3 pos = position;
	switch(rectmode_) {
		case OF_RECTMODE_CORNER:
			break;
		case OF_RECTMODE_CENTER:
			pos.x -= width/2.f;
			pos.y -= height/2.f;
			break;
	}
	vector<vec3> vertices((div_u_+2)*(div_v_+2), pos);
	vec3 d = vec3(width/(float)(div_u_+1), height/(float)(div_v_+1), 0);
	for(int y = 0; y < div_v_+2; ++y) {
		for(int x = 0; x < div_u_+2; ++x) {
			vertices[x+y*(div_u_+2)] += d*vec2(x,y);
		}
	}
	return vertices;
}

ofMesh Grid::getOutline(float width_inner, float width_outer, ofPrimitiveMode mode) const
{
	assert(mode == OF_PRIMITIVE_TRIANGLES);
	ofMesh mesh = ((Rectangle)*this).getOutline(0, width_outer, OF_PRIMITIVE_TRIANGLES);
	Rectangle child;
	child.setAnchor(getAnchor());
	child.setRotation(getRotation());
	child.setRectMode(OF_RECTMODE_CORNER);
	child.width = width/(float)(div_u_+1);
	child.height = height/(float)(div_v_+1);
	vec3 pos = position;
	switch(rectmode_) {
		case OF_RECTMODE_CORNER:
			break;
		case OF_RECTMODE_CENTER:
			pos -= vec3(width, height, 0)/2.f;
			break;
	}
	for(int y = 0; y < div_v_+1; ++y) {
		child.position.y = pos.y+y*child.height;
		for(int x = 0; x < div_u_+1; ++x) {
			child.position.x = pos.x+x*child.width;
			mesh.append(child.getOutline(width_inner/2.f, 0, OF_PRIMITIVE_TRIANGLES));
		}
	}
	return mesh;
}
