#include "pathtraceHeader.hlsli"
//Random seed generation

RWTexture2D<float4> RenderTarget;

unsigned int utilhash(unsigned int a)
{
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}

void TriangleIntersect()
{
    
}

void scatterRay()
{
    
}

void generateRayFromCamera()
{
    
}

void getPointonRay(const Ray &r,float t)
{
    
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID ):SV_Target
{
}