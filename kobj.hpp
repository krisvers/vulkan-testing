#ifndef KRISVERS_KOBJ_HPP
#define KRISVERS_KOBJ_HPP

#include <cstdint>

struct kobj_face_t {
	uint32_t v1, v2, v3;
	uint32_t vt1, vt2, vt3;
	uint32_t vn1, vn2, vn3;
};

struct kobj_t {
	float * vertices;
	float * normals;
	float * uvs;
	kobj_face_t * faces;
	uint32_t vcount;
	uint32_t uvcount;
	uint32_t ncount;
	uint32_t fcount;
};

int kobj_load(kobj_t * out_obj, void * buffer, size_t length);
void kobj_destroy(kobj_t * obj);

#endif