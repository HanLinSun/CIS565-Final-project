#pragma kernal pathtrace


// This is GPU code
[numthreads(256,256,1)]
void pathtrace()
{
    
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
}