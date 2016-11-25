#define ACCELERATION 0.01f
#define FRICTION (ACCELERATION * 0.0001f)

float rand(uint2 *seed)
{
	seed->x = 36969 * (seed->x & 65535) + (seed->x >> 16);
    seed->y = 18000 * (seed->y & 65535) + (seed->y >> 16);
	uint result = (seed->x << 16) + seed->y;
	return (float)result / (float)0xffffffffu;
}

__kernel void initPositions(__global float2 *pos, __global float2 *vel, uint random)
{
	int index = get_global_id(0);
	uint2 seed = (uint2)(random*(333 + index), random / (33 + index));

	pos[index].x = rand(&seed) * 2.f - 1.f;
	pos[index].y = rand(&seed) * 2.f - 1.f;
}

__kernel void updatePositions(__global float2 *pos, __global float2 *vel, float delta, __global float2 *attractor, uint attractorCount)
{
	int index = get_global_id(0);

	float speed = length(vel[index]);
	for (int i = 0; i < attractorCount; i++) {
		float2 dir = normalize(attractor[i] - pos[index]);
		float len = length(attractor[i] - pos[index]);

		if (len > 0.f) {
			float2 speed = dir * ACCELERATION * (float)min(10.0f, 1.0f / len) * delta;

			if (length(attractor[i] - pos[index]) < length(speed)) {
				speed = length(attractor[i] - pos[index]);
			}
			vel[index] += speed;
		}
	}

	pos[index] += vel[index];

	float friction = FRICTION * delta;
	if (friction < fast_length(vel[index])) {
		//vel[index] += friction * -(vel[index] / speed);
		vel[index] *= 1.0f - (delta * 0.3f);
	}
	else {
		vel[index].x = 0.f;
		vel[index].y = 0.f;
	}

/*	if (pos[index].x < -1.f || pos[index].x > 1.f) {
		pos[index].x = clamp(pos[index].x, -1.f, 1.f);
		vel[index].x = -vel[index].x;
	}
	if (pos[index].y < -1.f || pos[index].y > 1.f) {
		pos[index].y = clamp(pos[index].y, -1.f, 1.f);
		vel[index].y = -vel[index].y;
	}*/
}
