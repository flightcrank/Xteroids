
#include <math.h>

#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

//Add two vectors
static inline Vector3 v3_add(Vector3 a, Vector3 b) {
	
	return (Vector3) {a.x + b.x, a.y + b.y, a.z + b.z};
}

//Add a scalar to a vector
static inline Vector3 v3_add_s(Vector3 a, float s) {
	
	return (Vector3) {a.x + s, a.y + s, a.z + s};
}

//Subtract 2 vectors
static inline Vector3 v3_sub(Vector3 a, Vector3 b) {
	
	return (Vector3) {a.x - b.x, a.y - b.y, a.z - b.z};
}

//Subtract a scalar from a vector
static inline Vector3 v3_sub_s(Vector3 a, float s) {
	
	return (Vector3) {a.x - s, a.y - s, a.z - s};
}

//Multiply two vectors
static inline Vector3 v3_multi(Vector3 a, Vector3 b) {
	
	return (Vector3) {a.x * b.x, a.y * b.y, a.z * b.z};
}

//Multiply a vector with a scalar
static inline Vector3 v3_multi_s(Vector3 a, float s) {
	
	return (Vector3) {a.x * s, a.y * s, a.z * s};
}

//Divied a vector with a vector
static inline Vector3 v3_div(Vector3 a, Vector3 b) {

	if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f) {
		
		return (Vector3) {0.0f, 0.0f, 0.0f};

	} else {

		return (Vector3) {a.x / b.x, a.y / b.y, a.z / b.z};
	}
}

//Divide a vector with a scalar
static inline Vector3 v3_div_s(Vector3 a, float s) {

	if (s == 0.0f) {
		
		return (Vector3) {0.0f, 0.0f, 0.0f};

	} else {

		float recip = 1.0f / s;

		return (Vector3) {a.x * recip, a.y * recip, a.z * recip};
	}
}

//find the magnitude of a vector
static inline float v3_magnitude(Vector3 v) {

	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

//spherical limit
static inline Vector3 v3_limit_mag(Vector3 v, float limit) {

	float mag_sq = v.x*v.x + v.y*v.y + v.z*v.z;
	float max_sq = limit * limit;

	if (mag_sq > max_sq) {
		
		float mag = sqrtf(mag_sq);
		float scale = limit / mag;	// Scale the vector back to the limit while keeping direction

		return (Vector3){v.x * scale, v.y * scale, v.z * scale};
	}
	
	
	return v;	//return vector unchanged
}

//box limit
static inline Vector3 v3_limit_s(Vector3 v, float s) {

	return (Vector3) {CLAMP(v.x, -s,s), CLAMP(v.y, -s,s), CLAMP(v.z, -s,s)};
}

//normalise vector
static inline Vector3 v3_normalise(Vector3 v) {

	float mag = v3_magnitude(v);
    
	if (mag > 0.0f) { 

		float inv_mag = 1.0f / mag; //use inverse to avoid 3 divisions
		
		return (Vector3){v.x * inv_mag, v.y * inv_mag, v.z * inv_mag};
	}

	return (Vector3){0, 0, 0};
}


//static inline Vector3 v3_limit(Vector3 v, Vector3 a) {
//
//	return NULL;
//}


static inline Vector3 v3_rotate(Vector3 v, Vector3 a) {
 
	float cx = cosf(a.x), sx = sinf(a.x);
	float cy = cosf(a.y), sy = sinf(a.y);
	float cz = cosf(a.z), sz = sinf(a.z);

	Vector3 res;

	// X-axis
	res.x = v.x * (cy * cz) + v.y * (sx * sy * cz - cx * sz) + v.z * (cx * sy * cz + sx * sz);

	// Y-axis
	res.y = v.x * (cy * sz) + v.y * (sx * sy * sz + cx * cz) + v.z * (cx * sy * sz - sx * cz);

	// Z-axis
	res.z = v.x * (-sy) + v.y * (sx * cy) + v.z * (cx * cy);

	return res;
}
