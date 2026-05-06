//Rachid Tazi BX26

/*
Actually working on lab2:
- implement indirect lighting part for point light sources
- add antialiasing
- optional : soft shadows
*/


#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>
#include <cmath>
#include <random>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323856
#endif

#include <omp.h>
#include <map>
#include <string>
#include <fstream>

static std::default_random_engine engine[32];
static std::uniform_real_distribution<double> uniform(0, 1);

double sqr(double x) { return x * x; };

class Vector {
public:
	explicit Vector(double x = 0, double y = 0, double z = 0) {
		data[0] = x;
		data[1] = y;
		data[2] = z;
	}
	double norm2() const {
		return data[0] * data[0] + data[1] * data[1] + data[2] * data[2];
	}
	double norm() const {
		return sqrt(norm2());
	}
	void normalize() {
		double n = norm();
		data[0] /= n;
		data[1] /= n;
		data[2] /= n;
	}
	double operator[](int i) const { return data[i]; };
	double& operator[](int i) { return data[i]; };
	double data[3];
};

Vector operator+(const Vector& a, const Vector& b) {
	return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}
Vector operator-(const Vector& a, const Vector& b) {
	return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
Vector operator*(const double a, const Vector& b) {
	return Vector(a * b[0], a * b[1], a * b[2]);
}
Vector operator*(const Vector& a, const double b) {
	return Vector(a[0] * b, a[1] * b, a[2] * b);
}

Vector operator*(const Vector& a, const Vector& b) {
	return Vector(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}

Vector operator/(const Vector& a, const double b) {
	return Vector(a[0] / b, a[1] / b, a[2] / b);
}
double dot(const Vector& a, const Vector& b) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vector cross(const Vector& a, const Vector& b) {
	return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}

class Ray {
public:
	Ray(const Vector& origin, const Vector& unit_direction) : O(origin), u(unit_direction) {};
	Vector O, u;
};

class Object {
public:
	Object(const Vector& albedo, bool mirror = false, bool transparent = false) : albedo(albedo), mirror(mirror), transparent(transparent) {};

	virtual bool intersect(const Ray& ray, Vector& P, double& t, Vector& N) const = 0;

	Vector albedo;
	bool mirror, transparent;
};

Vector random_cos(const Vector& N, int tid) {
	double r1 = uniform(engine[tid]);
	double r2 = uniform(engine[tid]);

	double x = cos(2.0 * M_PI * r1) * sqrt(1.0 - r2);
	double y = sin(2.0 * M_PI * r1) * sqrt(1.0 - r2);
	double z = sqrt(r2);

	Vector t1;
	if (fabs(N[0]) <= fabs(N[1]) && fabs(N[0]) <= fabs(N[2])) {
		t1 = Vector(0.0, -N[2], N[1]);
	}
	else if (fabs(N[1]) <= fabs(N[0]) && fabs(N[1]) <= fabs(N[2])) {
		t1 = Vector(-N[2], 0.0, N[0]);
	}
	else {
		t1 = Vector(-N[1], N[0], 0.0);
	}

	t1.normalize();
	Vector t2 = cross(N, t1);
	Vector direction = x * t1 + y * t2 + z * N;
	direction.normalize();
	return direction;
}

class Sphere : public Object {
public:
	Sphere(const Vector& center, double radius, const Vector& albedo, bool mirror = false, bool transparent = false) : ::Object(albedo, mirror, transparent), C(center), R(radius) {};

	// returns true iif there is an intersection between the ray and the sphere
	// if there is an intersection, also computes the point of intersection P, 
	// t>=0 the distance between the ray origin and P (i.e., the parameter along the ray)
	// and the unit normal N
	bool intersect(const Ray& ray, Vector& P, double& t, Vector& N) const {
		// TODO (lab 1) : compute the intersection (just true/false at the begining of lab 1, then P, t and N as well)

		Vector oc = ray.O - this->C;
		double a = dot(ray.u, oc);
		double b = dot(oc, oc) - sqr(R);
		double delta = sqr(a) - b;

		if (delta < 0) {
			return false;
		}

		Vector co = this->C - ray.O;
		double t1 = dot(ray.u, co) - sqrt(delta);
		double t2 = dot(ray.u, co) + sqrt(delta);

		if (t2 < 0) {
			return false;
		}

		if (t1 >= 0) {
			t = t1;
		}
		else {
			t = t2;
		}

		P = ray.O + t * ray.u;
		N = (P - this->C);
		N.normalize();

		return true;
	}

	double R;
	Vector C;
};



// Class only used in labs 3 and 4 
class TriangleIndices {
public:
	TriangleIndices(int vtxi = -1, int vtxj = -1, int vtxk = -1, int ni = -1, int nj = -1, int nk = -1, int uvi = -1, int uvj = -1, int uvk = -1, int group = -1) {
		vtx[0] = vtxi; vtx[1] = vtxj; vtx[2] = vtxk;
		uv[0] = uvi; uv[1] = uvj; uv[2] = uvk;
		n[0] = ni; n[1] = nj; n[2] = nk;
		this->group = group;
	};
	int vtx[3]; // indices within the vertex coordinates array --> vertices[vtx[0]] : A
	int uv[3];  // indices within the uv coordinates array
	int n[3];   // indices within the normals array
	int group;  // face group
};

// Class only used in labs 3 and 4 

class TriangleMesh : public Object {
public:
	TriangleMesh(const Vector& albedo, bool mirror = false, bool transparent = false) : ::Object(albedo, mirror, transparent) {};

	//we add a structure to handle nodes in BVH tree
	struct BVHNode {
		//bounding box of the node
		Vector Bmin, Bmax;
		//the box includes a starting_triangle and a ending one
		int start_index;
		int end_index;
		int child_left;
		int child_right;

		BVHNode() {
			start_index = 0;
			end_index = 0;
			child_left = -1; //no child
			child_right = -1;
		}
	};



	// first scale and then translate the current object
	void scale_translate(double s, const Vector& t) {
		for (int i = 0; i < vertices.size(); i++) {
			vertices[i] = vertices[i] * s + t;
		}
	}

	//compute bounding box
	void computeBoundingBox() {
		Bmin = Vector(1e10, 1e10, 1e10);
		Bmax = Vector(-1e10, -1e10, -1e10);

		for (int i = 0; i < vertices.size(); i++) {
			for (int j = 0; j < 3; j++) {
				Bmin[j] = std::min(Bmin[j], vertices[i][j]);
				Bmax[j] = std::max(Bmax[j], vertices[i][j]);
			}
		}
	}


	// read an .obj file
	void readOBJ(const char* obj) {
		std::ifstream f(obj);
		if (!f) return;

		std::map<std::string, int> mtls;
		int curGroup = -1, maxGroup = -1;

		// OBJ indices are 1-based and can be negative (relative), this normalizes them
		auto resolveIdx = [](int i, int size) {
			return i < 0 ? size + i : i - 1;
			};

		auto setFaceVerts = [&](TriangleIndices& t, int i0, int i1, int i2) {
			t.vtx[0] = resolveIdx(i0, vertices.size());
			t.vtx[1] = resolveIdx(i1, vertices.size());
			t.vtx[2] = resolveIdx(i2, vertices.size());
			};
		auto setFaceUVs = [&](TriangleIndices& t, int j0, int j1, int j2) {
			t.uv[0] = resolveIdx(j0, uvs.size());
			t.uv[1] = resolveIdx(j1, uvs.size());
			t.uv[2] = resolveIdx(j2, uvs.size());
			};
		auto setFaceNormals = [&](TriangleIndices& t, int k0, int k1, int k2) {
			t.n[0] = resolveIdx(k0, normals.size());
			t.n[1] = resolveIdx(k1, normals.size());
			t.n[2] = resolveIdx(k2, normals.size());
			};

		std::string line;
		while (std::getline(f, line)) {
			// Trim trailing whitespace
			line.erase(line.find_last_not_of(" \r\t\n") + 1);
			if (line.empty()) continue;

			const char* s = line.c_str();

			if (line.rfind("usemtl ", 0) == 0) {
				std::string matname = line.substr(7);
				auto result = mtls.emplace(matname, maxGroup + 1);
				if (result.second) {
					curGroup = ++maxGroup;
				}
				else {
					curGroup = result.first->second;
				}
			}
			else if (line.rfind("vn ", 0) == 0) {
				Vector v;
				sscanf(s, "vn %lf %lf %lf", &v[0], &v[1], &v[2]);
				normals.push_back(v);
			}
			else if (line.rfind("vt ", 0) == 0) {
				Vector v;
				sscanf(s, "vt %lf %lf", &v[0], &v[1]);
				uvs.push_back(v);
			}
			else if (line.rfind("v ", 0) == 0) {
				Vector pos, col;
				if (sscanf(s, "v %lf %lf %lf %lf %lf %lf", &pos[0], &pos[1], &pos[2], &col[0], &col[1], &col[2]) == 6) {
					for (int i = 0; i < 3; i++) col[i] = std::min(1.0, std::max(0.0, col[i]));
					vertexcolors.push_back(col);
				}
				else {
					sscanf(s, "v %lf %lf %lf", &pos[0], &pos[1], &pos[2]);
				}
				vertices.push_back(pos);
			}
			else if (line[0] == 'f') {
				int i[4], j[4], k[4], offset, nn;
				const char* cur = s + 1;
				TriangleIndices t;
				t.group = curGroup;

				// Try each face format: v/vt/vn, v/vt, v//vn, v
				if ((nn = sscanf(cur, "%d/%d/%d %d/%d/%d %d/%d/%d%n", &i[0], &j[0], &k[0], &i[1], &j[1], &k[1], &i[2], &j[2], &k[2], &offset)) == 9) {
					setFaceVerts(t, i[0], i[1], i[2]);
					setFaceUVs(t, j[0], j[1], j[2]);
					setFaceNormals(t, k[0], k[1], k[2]);
				}
				else if ((nn = sscanf(cur, "%d/%d %d/%d %d/%d%n", &i[0], &j[0], &i[1], &j[1], &i[2], &j[2], &offset)) == 6) {
					setFaceVerts(t, i[0], i[1], i[2]);
					setFaceUVs(t, j[0], j[1], j[2]);
				}
				else if ((nn = sscanf(cur, "%d//%d %d//%d %d//%d%n", &i[0], &k[0], &i[1], &k[1], &i[2], &k[2], &offset)) == 6) {
					setFaceVerts(t, i[0], i[1], i[2]);
					setFaceNormals(t, k[0], k[1], k[2]);
				}
				else if ((nn = sscanf(cur, "%d %d %d%n", &i[0], &i[1], &i[2], &offset)) == 3) {
					setFaceVerts(t, i[0], i[1], i[2]);
				}
				else continue;

				indices.push_back(t);
				cur += offset;

				// Fan triangulation for polygon faces (4+ vertices)
				while (*cur && *cur != '\n') {
					TriangleIndices t2;
					t2.group = curGroup;
					if ((nn = sscanf(cur, " %d/%d/%d%n", &i[3], &j[3], &k[3], &offset)) == 3) {
						setFaceVerts(t2, i[0], i[2], i[3]);
						setFaceUVs(t2, j[0], j[2], j[3]);
						setFaceNormals(t2, k[0], k[2], k[3]);
					}
					else if ((nn = sscanf(cur, " %d/%d%n", &i[3], &j[3], &offset)) == 2) {
						setFaceVerts(t2, i[0], i[2], i[3]);
						setFaceUVs(t2, j[0], j[2], j[3]);
					}
					else if ((nn = sscanf(cur, " %d//%d%n", &i[3], &k[3], &offset)) == 2) {
						setFaceVerts(t2, i[0], i[2], i[3]);
						setFaceNormals(t2, k[0], k[2], k[3]);
					}
					else if ((nn = sscanf(cur, " %d%n", &i[3], &offset)) == 1) {
						setFaceVerts(t2, i[0], i[2], i[3]);
					}
					else {
						cur++;
						continue;
					}

					indices.push_back(t2);
					cur += offset;
					i[2] = i[3]; j[2] = j[3]; k[2] = k[3];
				}
			}
		}
	}


	// TODO ray-mesh intersection (labs 3 and 4)
	/*
	bool intersectBox(const Ray& ray) const {
		double tmin = -std::numeric_limits<double>::infinity();
		double tmax = std::numeric_limits<double>::infinity();

		for (int i = 0; i < 3; i++) {
			double t0 = (Bmin[i] - ray.O[i]) / ray.u[i];
			double t1 = (Bmax[i] - ray.O[i]) / ray.u[i];
			if (t0 > t1) {
				std::swap(t0, t1);
			}

			tmin = std::max(tmin, t0);
			tmax = std::min(tmax, t1);
			if (tmin > tmax) return false;
		}
		return tmax >= 0;
	}
	*/


	//BVH 

	/*
	methods i have to add :
	- compute_bbox(starting_triangle, ending_triangle)
	- compute_diag(node->bbox)
	- get_longest(diag)
	- compute_barycenter(indices[i])
	*/

	//small method to compute a barycenter for a triangle
	Vector compute_barycenter(int i) {
		Vector A = vertices[indices[i].vtx[0]];
		Vector B = vertices[indices[i].vtx[1]];
		Vector C = vertices[indices[i].vtx[2]];

		return (A + B + C) / 3.0;
	}

	//method to compute BBox from starting triangle to ending triangle excluded
	BVHNode compute_bbox(int start_index, int end_index) {
		BVHNode node;

		node.start_index = start_index;
		node.end_index = end_index;
		node.Bmin = Vector(1e10, 1e10, 1e10);
		node.Bmax = Vector(-1e10, -1e10, -1e10);

		for (int i = start_index; i < end_index; i++) {
			for (int k = 0; k < 3; k++) {
				Vector V = vertices[indices[i].vtx[k]];

				for (int d = 0; d < 3; d++) {
					node.Bmin[d] = std::min(node.Bmin[d], V[d]);
					node.Bmax[d] = std::max(node.Bmax[d], V[d]);
				}
			}
		}

		return node;
	}

	//size of a bbox in x,y,z
	Vector compute_diag(BVHNode& node) {
		return node.Bmax - node.Bmin;
	}

	//get longest axis (0 = x, 1 = y, 2 = z)
	int get_longest(const Vector& diag) const {
		int longest_axis = 0;

		if (diag[1] > diag[longest_axis]) {
			longest_axis = 1;
		}
		if (diag[2] > diag[longest_axis]) {
			longest_axis = 2;
		}
		return longest_axis;
	}
	//we want to slice the bbox in the longest direction

	//intersect but for a given box
	//t_box gives distance to the box
	bool intersectBox(const Ray& ray, const Vector& bmin, const Vector& bmax, double& t_box) const {
		double tmin = -std::numeric_limits<double>::infinity();
		double tmax = std::numeric_limits<double>::infinity();

		for (int i = 0; i < 3; i++) {
			double t0 = (bmin[i] - ray.O[i]) / ray.u[i];
			double t1 = (bmax[i] - ray.O[i]) / ray.u[i];

			if (t0 > t1) {
				std::swap(t0, t1);
			}

			tmin = std::max(tmin, t0);
			tmax = std::min(tmax, t1);

			if (tmin > tmax) {
				return false;
			}
		}

		t_box = tmin;
		return tmax >= 0;
	}

	//build recursively the BVH
	/*
	code from the textbook
	create bounding box around triangles, find longest axis, compute center, separate triangles, 2 children recursively
	*/

	int buildBVHRecursive(int start_index, int end_index) {
		BVHNode node = compute_bbox(start_index, end_index);
		//bbox from starting triangle included to ending_triangle excluded

		int node_id = nodes.size();
		nodes.push_back(node);

		Vector diag = compute_diag(nodes[node_id]);

		Vector middle_diag = nodes[node_id].Bmin + diag * 0.5;

		int longest_axis = get_longest(diag);
		int pivot_index = start_index;

		for (int i = start_index; i < end_index; i++) {
			Vector barycenter = compute_barycenter(i);
			//the swap below guarentees triangles whose barycenter are smaller than middle diag are before pivot index

			if (barycenter[longest_axis] < middle_diag[longest_axis]) {
				std::swap(indices[i], indices[pivot_index]);
				pivot_index++;
			}
		}
		//stopping criterion
		if (pivot_index <= start_index ||
			pivot_index >= end_index - 1 ||
			end_index - start_index < 5) {
			return node_id;
		}

		nodes[node_id].child_left = buildBVHRecursive(start_index, pivot_index);
		nodes[node_id].child_right = buildBVHRecursive(pivot_index, end_index);

		return node_id;
	}

	//buildBVH
	void buildBVH() {
		nodes.clear();
		buildBVHRecursive(0, indices.size());
	}

	bool intersect(const Ray& ray, Vector& P, double& t, Vector& N) const {

		//empty bvh ?
		if (nodes.empty()) return false;

		bool hit = false;
		double best_t = std::numeric_limits<double>::infinity();
		//stack of nodes
		std::vector<int> stack;
		stack.push_back(0); //root = 0

		while (!stack.empty()) {

			int node_id = stack.back();
			stack.pop_back();
			const BVHNode& node = nodes[node_id];
			double t_box;

			//test bounding box of the node
			if (!intersectBox(ray, node.Bmin, node.Bmax, t_box)) continue;
			if (t_box > best_t) continue;

			//no child ?
			if (node.child_left == -1 && node.child_right == -1) {

				//test triangles of the node
				for (int i = node.start_index; i < node.end_index; i++) {

					Vector A = vertices[indices[i].vtx[0]];
					Vector B = vertices[indices[i].vtx[1]];
					Vector C = vertices[indices[i].vtx[2]];

					Vector e1 = B - A;
					Vector e2 = C - A;
					Vector n = cross(e1, e2);

					double denom = dot(ray.u, n);
					if (fabs(denom) < 1e-10) continue;

					double beta = dot(e2, cross((A - ray.O), ray.u)) / denom;
					double gamma = -dot(e1, cross((A - ray.O), ray.u)) / denom;
					double alpha = 1 - beta - gamma;

					double t_tmp = dot(A - ray.O, n) / denom;

					if (alpha >= 0 && beta >= 0 && gamma >= 0 &&
						beta <= 1 && gamma <= 1 &&
						t_tmp >= 1e-6) {

						if (t_tmp < best_t) {
							best_t = t_tmp;
							P = ray.O + t_tmp * ray.u;
							n.normalize();
							N = n;
							hit = true;
						}
					}
				}
			}
			else {
				//add children
				if (node.child_left != -1)
					stack.push_back(node.child_left);

				if (node.child_right != -1)
					stack.push_back(node.child_right);
			}
		}

		t = best_t;
		return hit;
	}

	/*
	bool old_intersect(const Ray& ray, Vector& P, double& t, Vector& N) const {
		// lab 3 : for each triangle, compute the ray-triangle intersection with Moller-Trumbore algorithm

		
		//point P within triangle A, B, C if P = a A + b B + c C, a,b,c in [0,1]
		//and a + b + c = 1

		//bounding box
		if (!intersectBox(ray)) return false;


		bool hit = false;
		//we keep the nearest triangle 
		double best_t = std::numeric_limits<double>::infinity();
		Vector best_P;
		Vector best_N;


		//for each triangle
		for (int i = 0; i < indices.size(); ++i) {
			//take from vertices, points A,B,C
			Vector A = vertices[indices[i].vtx[0]];
			Vector B = vertices[indices[i].vtx[1]];
			Vector C = vertices[indices[i].vtx[2]];

			Vector e1 = B - A;
			Vector e2 = C - A;

			Vector n = cross(e1, e2);

			double denom = dot(ray.u, n);
			if (fabs(denom) < 1e-10) continue;

			double beta = dot(e2, cross((A - ray.O), ray.u)) / denom;
			double gamma = -dot(e1, cross((A - ray.O), ray.u)) / denom;
			double alpha = 1 - beta - gamma;

			double t_tmp = dot(A - ray.O, n) / denom;

			if (alpha >= 0 && 0 <= beta && beta <= 1 && 0 <= gamma && gamma <= 1 && t_tmp >= 1e-6) {
				if (t_tmp < best_t) {
					best_t = t_tmp;
					best_P = ray.O + t_tmp * ray.u;
					n.normalize();
					best_N = n;
					hit = true;
				}

			}
		}

		if (hit) {
			t = best_t;
			P = best_P;
			N = best_N;
		}

		// lab 3 : once done, speed it up by first checking against the mesh bounding box
		//box around the cat



		// lab 4 : recursively apply the bounding-box test from a BVH datastructure


		return hit;
	}
	*/

	std::vector<TriangleIndices> indices;
	std::vector<Vector> vertices;
	std::vector<Vector> normals;
	std::vector<Vector> uvs;
	std::vector<Vector> vertexcolors;
	//bounding box
	Vector Bmin, Bmax;
	//bvh nodes 
	std::vector<BVHNode> nodes;
};


class Scene {
public:
	Scene() {};
	void addObject(const Object* obj) {
		objects.push_back(obj);
	}

	// returns true iif there is an intersection between the ray and any object in the scene
	// if there is an intersection, also computes the point of the *nearest* intersection P, 
	// t>=0 the distance between the ray origin and P (i.e., the parameter along the ray)
	// and the unit normal N. 
	// Also returns the index of the object within the std::vector objects in object_id
	bool intersect(const Ray& ray, Vector& P, double& t, Vector& N, int& object_id) const {

		// TODO (lab 1): iterate through the objects and check the intersections with all of them, 
		// and keep the closest intersection, i.e., the one if smallest positive value of t
		bool is_intersection = false;
		double t_res = std::numeric_limits<double>::infinity();
		Vector P_res;
		Vector N_res;

		for (int i = 0; i < objects.size(); i++) {
			if (objects[i]->intersect(ray, P, t, N)) {
				is_intersection = true;

				if (t >= 0 && t < t_res) {
					t_res = t;
					object_id = i;
					P_res = P;
					N_res = N;
				}
			}

		}

		t = t_res;
		P = P_res;
		N = N_res;
		return is_intersection;
	}


	// return the radiance (color) along ray
	Vector getColor(const Ray& ray, int recursion_depth) {

		if (recursion_depth >= max_light_bounce) return Vector(0, 0, 0);

		// TODO (lab 1) : if intersect with ray, use the returned information to compute the color ; otherwise black 
		// in lab 1, the color only includes direct lighting with shadows

		Vector P, N;
		double t;
		int object_id;

		if (!intersect(ray, P, t, N, object_id)) {
			return Vector(0, 0, 0);
		}

		if (objects[object_id]->mirror) {
			//lab 2
			// return getColor in the reflected direction, with recursion_depth+1 (recursively)
			Vector reflected_direction = ray.u - 2 * dot(ray.u, N) * N;
			reflected_direction.normalize();

			Ray reflected_ray(P + 1e-4 * N, reflected_direction);
			return getColor(reflected_ray, recursion_depth + 1);
		}

		if (objects[object_id]->transparent) {
			//lab 2
			// return getColor in the refraction direction, with recursion_depth+1 (recursively)


			return Vector(0, 0, 0);
		}

		//---------------------
		//diffuse surfaces

		Vector Lo(0., 0., 0.);

		//direct
		Vector to_light = this->light_position - P;
		double distance_to_light2 = dot(to_light, to_light);
		double distance_to_light = sqrt(distance_to_light2);
		Vector wi = to_light / distance_to_light;

		// test if there is a shadow by sending a new ray
		// if there is no shadow, compute the formula with dot products etc.
		//shadow ray 
		Ray shadow_ray(P + 1e-4 * N, wi);

		Vector shadow_P, shadow_N;
		double shadow_t;
		int shadow_object_id;

		double visibility = 1.0;
		if (intersect(shadow_ray, shadow_P, shadow_t, shadow_N, shadow_object_id)) {
			if (shadow_t < distance_to_light) {
				visibility = 0.0;
			}
		}

		double attenuation = light_intensity / (4.0 * M_PI * distance_to_light2);
		Vector material = objects[object_id]->albedo / M_PI;
		double solid_angle = dot(N, wi);

		Lo = Lo + attenuation * material * solid_angle * visibility;


		// TODO (lab 2) : add indirect lighting component with a recursive call
		/*
		rendering equation :
		getColor depends on the point x in the scene , the ray direction -wo,
		monte carlo integration
		sampling strategies
		*/

		//indirect 
		int tid = omp_get_thread_num();
		Vector random_dir = random_cos(N, tid);
		Ray random_ray(P + 1e-4 * N, random_dir);

		Lo = Lo + objects[object_id]->albedo * getColor(random_ray, recursion_depth + 1);

		return Lo;
	}

	std::vector<const Object*> objects;

	Vector camera_center, light_position;
	double fov, gamma, light_intensity;
	int max_light_bounce;
	int max_path_length;
	int nb_paths;
};


int main() {
	int W = 512;
	int H = 512;

	for (int i = 0; i < 32; i++) {
		engine[i].seed(i);
	}
	Sphere left_sphere(Vector(-20, 0, 20), 10., Vector(0.8, 0.2, 0.4), false, false);
	Sphere center_sphere(Vector(0, 0, 0), 10., Vector(0.8, 0.8, 0.8), true, false);
	Sphere wall_left(Vector(-1000, 0, 0), 940, Vector(0.5, 0.8, 0.1));
	Sphere wall_right(Vector(1000, 0, 0), 940, Vector(0.9, 0.2, 0.3));
	Sphere wall_front(Vector(0, 0, -1000), 940, Vector(0.1, 0.6, 0.7));
	Sphere wall_behind(Vector(0, 0, 1000), 940, Vector(0.8, 0.2, 0.9));
	Sphere ceiling(Vector(0, 1000, 0), 940, Vector(0.3, 0.5, 0.3));
	Sphere floor(Vector(0, -1000, 0), 990, Vector(0.6, 0.5, 0.7));

	Scene scene;
	scene.camera_center = Vector(0, 0, 55);
	scene.light_position = Vector(-10, 20, 40);
	scene.light_intensity = 3E7;
	scene.fov = 60 * M_PI / 180.;
	scene.gamma = 2.2;    // TODO (lab 1) : play with gamma ; typically, gamma = 2.2
	scene.max_light_bounce = 15;
	scene.nb_paths = 32;
	scene.max_path_length = 3;

	//scene.addObject(&center_sphere);
	//scene.addObject(&left_sphere);

	//CAT
	TriangleMesh cat(Vector(0.4, 0.4, 0.4), false, false);
	cat.readOBJ("objects/cat.obj");
	cat.scale_translate(0.5, Vector(0., -10., 0.));
	cat.computeBoundingBox();
	cat.buildBVH();
	scene.addObject(&cat);


	scene.addObject(&wall_left);
	scene.addObject(&wall_right);
	scene.addObject(&wall_front);
	scene.addObject(&wall_behind);
	scene.addObject(&ceiling);
	scene.addObject(&floor);



	std::vector<unsigned char> image(W * H * 3, 0);

#pragma omp parallel for schedule(dynamic, 1)
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			Vector color(0., 0., 0.);

			// TODO (lab 1) : correct ray_direction so that it goes through each pixel (j, i)			
			/*
			double X = j - W / 2 + 0.5;
			double Y = H / 2 - i - 0.5;
			double Z = -(W) / (2 * tan(scene.fov/2));
			Vector ray_direction(X, Y, Z);
			ray_direction.normalize();

			Ray ray(scene.camera_center, ray_direction);
			*/


			// TODO (lab 2) : add Monte Carlo / averaging of random ray contributions here
			// TODO (lab 2) : add antialiasing by altering the ray_direction here

			/*
			discontinuity btwn adjacent pixels -> aliasing
			camera sensor cells have an area, arranged in a Bayer pattern
			twice as many "green cells" than red and blue

			We can directly emulate an RGB-senstive pixel array
			Integrate radiance that reaches the camera sensor
			Li,j = Int[Li(x,wi(x)) hi,j(x)dx]

			*/

			for (int k = 0; k < scene.nb_paths; k++) {
				int tid = omp_get_thread_num();
				double r1 = uniform(engine[tid]);
				double r2 = uniform(engine[tid]);
				double x_boxMuller = sqrt(-2 * log(r1)) * cos(2 * M_PI * r2) * 0.2;
				double y_boxMuller = sqrt(-2 * log(r1)) * sin(2 * M_PI * r2) * 0.2;

				double X = j - W / 2 + 0.5 + x_boxMuller;
				double Y = H / 2 - i - 0.5 + y_boxMuller;
				double Z = -(W) / (2 * tan(scene.fov / 2));
				Vector rand_dir(X, Y, Z);
				rand_dir.normalize();

				//depth of field
				double focal_distance = 55.0;
				double aperture = 1.0;

				//focus point
				Vector focus_point = scene.camera_center + focal_distance * rand_dir;

				//random point for aperture
				double rand_aperture = aperture * sqrt(uniform(engine[tid]));
				double theta_aperture = 2.0 * M_PI * uniform(engine[tid]);
				Vector random_camera_origin(scene.camera_center[0] + rand_aperture * cos(theta_aperture), scene.camera_center[1] + rand_aperture * sin(theta_aperture), scene.camera_center[2]);

				//direction
				Vector dof_dir = focus_point - random_camera_origin;
				dof_dir.normalize();

				Ray ray(random_camera_origin, dof_dir);
				color = color + scene.getColor(ray, 0);

			}


			image[i * W * 3 + j * 3 + 0] = std::min(255., std::max(0., 255. * std::pow((color[0] / scene.nb_paths) / 255., 1. / scene.gamma)));
			image[i * W * 3 + j * 3 + 1] = std::min(255., std::max(0., 255. * std::pow((color[1] / scene.nb_paths) / 255., 1. / scene.gamma)));
			image[i * W * 3 + j * 3 + 2] = std::min(255., std::max(0., 255. * std::pow((color[2] / scene.nb_paths) / 255., 1. / scene.gamma)));
			// TODO (lab 2) : add depth of field effect by altering the ray origin (and direction) 
		}
	}
	stbi_write_png("image.png", W, H, 3, &image[0], 0);

	return 0;
}
