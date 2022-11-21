#define RENDER_WIDTH 2560
#define RENDER_HEIGHT 1440

#define MAX_RAYS 12
#define SHADOW_MAX_RAYS 4

#define PIXELS_COUNT (RENDER_WIDTH*RENDER_HEIGHT) // 1920x1080 : 2 073 600

#define MAX_TEXTURE_SIZE 512
#define TEXTURE_SIZE 400 // 440, can't go up else the aabbs buffer is upper mem limit
#define HALF_TEXTURE_SIZE (TEXTURE_SIZE/2.0)
#define AABB_SIZE 2
#define AABBS_SIZE (TEXTURE_SIZE/AABB_SIZE)
#define AABBS_COUNT (AABBS_SIZE*AABBS_SIZE*AABBS_SIZE) // 10 648 000 aabbs
#define AABB_TEXEL_COUNT (AABB_SIZE/2+1)
#define AABBS_PERPIXEL_COUNT (AABBS_COUNT/PIXELS_COUNT+1)

//#define MAX_DOTS_COUNT (AABBS_COUNT*AABB_SIZE*AABB_SIZE*AABB_SIZE) // 85 184 000 dots
//#define MAX_DOTS_PER_PIXEL (MAX_DOTS_COUNT/PIXELS_COUNT)

// No need to animate max_dots
#define DOTS_PER_PIXEL 12 //16
#define DOTS_COUNT (DOTS_PER_PIXEL*PIXELS_COUNT)	// 33 177 600 dots

#define BOUNDS 350.0
#define AABB_BOUNDS (BOUNDS / AABBS_SIZE)

#define RAY_MARCHING_SAMPLE_COUNT 8 //(8*RENDER_HEIGHT/1080) << crash in quad HD, shader limit reached to the max.

#define LIGHT_COUNT 6

// -> clear AABBS_PERPIXEL_COUNT aabbs per pixel (AABB_TEXEL_COUNT*AABB_TEXEL_COUNT*AABB_TEXEL_COUNT cubic texture fetch, +1 we need the borders)
//
// double & quadruple buffered :
//
//.build with aabbs 1
//.update aabbs 0 using texture 0
//.generate dots in texture 1 & clear dots in texture 2
//.raytrace using aabbs 1 & texture 3
//
//.build with aabbs 0
//.update aabbs 1 using texture 1
//.generate dots in texture 2 & clear dots in texture 3
//.raytrace using aabbs 0 & texture 0
//
//.build with aabbs 1
//.update aabbs 0 using texture 2
//.generate dots in texture 3 & clear dots in texture 0
//.raytrace using aabbs 1 & texture 1
//
//.build with aabbs 0
//.update aabbs 1 using texture 3
//.generate dots in texture 0 & clear dots in texture 1
//.raytrace using aabbs 0 & texture 2

#define FOV 100.0
#define PI 3.14159265359

