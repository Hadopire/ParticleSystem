#define ACCELERATION 0.7f
#define MOUSE_COLOR_DIST 0.1f

float rand(uint2 *);

float rand(uint2 *seed)
{
	seed->x = 36969 * (seed->x & 65535) + (seed->x >> 16);
    seed->y = 18000 * (seed->y & 65535) + (seed->y >> 16);
	uint result = (seed->x << 16) + seed->y;
	return (float)result / (float)0xffffffffu;
}

__kernel void initPositions(__global float4 *pos, uint random, __global float4 *colors)
{
	int index = get_global_id(0);
	uint2 seed = (uint2)(random*(333 + index), random / (33 + index));

	pos[index].x = rand(&seed) - 0.5f;
	pos[index].y = rand(&seed) - 0.5f;
	pos[index].z = rand(&seed) - 0.5f;
	pos[index].w = 1.f;

	colors[index] = (float4)(0.65f, 0.15f, 0.01f, 0.15f);
}

__kernel void initPositionsInSphere(__global float4 *pos, uint random, __global float4 *colors)
{
	int index = get_global_id(0);
	uint2 seed = (uint2)(random*(333 + index), random / (33 + index));

	float u = rand(&seed);
	float v = rand(&seed);
	float theta = 2 * M_PI * u;
	float phi = acos(2.f * v - 1.f);
	pos[index].x = 0.5f * sin(phi) * cos(theta);
	pos[index].y = 0.5f * sin(phi) * sin(theta);
	pos[index].z = 0.5f * sin(rand(&seed));
	pos[index].w = 1.f;
	colors[index] = (float4)(0.65f, 0.15f, 0.01f, 0.15f);
}

__kernel void updatePositions(__global float4 *pos, __global float4 *vel, float delta, __global float4 *attractors, uint attractorCount, float4 mousePosition, __global float4 *colors)
{
	int index = get_global_id(0);

	for (uint i = 0; i < attractorCount; i++) {
		float4 attractor = (float4)(attractors[i].x, attractors[i].y, attractors[i].z, 1.f);
		float4 dir = normalize(attractor - pos[index]) * attractors[i].w;
		float len = fast_length(attractor - pos[index]);

		if (len > 0.f) {
			float4 speed = dir * ACCELERATION * (float)min(10.0f, 1.0f / len);
			vel[index] += speed * delta;
		}
	}

	pos[index] += vel[index] * delta;

	float4 mouseDif = min(MOUSE_COLOR_DIST, fast_length(mousePosition - pos[index]));
	float4 secondary = (float4)(1.0f, 0.5f, 0.25f, 0.5f) * min(1.f, (MOUSE_COLOR_DIST - mouseDif) / MOUSE_COLOR_DIST);
	float4 primary = (float4)(0.65f, 0.15f, 0.01f, 0.15f) * min(1.f, mouseDif / MOUSE_COLOR_DIST);
	colors[index] = secondary + primary;

	if (fast_length(vel[index]) > 0.01f * delta) {
		if (attractorCount > 0) {
			vel[index] *= 1.0f - (delta * 0.03f);
		}
		else {
			vel[index] *= 1.0f - (delta * 0.8f);
		}
	}
	else {
		vel[index].x = 0.f;
		vel[index].y = 0.f;
		vel[index].z = 0.f;
	}

}
