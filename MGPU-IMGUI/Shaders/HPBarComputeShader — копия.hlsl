// HPBarCompute.hlsl
cbuffer HPBarCB : register(b0)
{
    float2 TextureSize; // ������ �������� (������, ������)
    float Time; // ����� ��� ��������
    float Health; // ������� �������� (0.0 - 1.0)
    float4 ColorStart; // ���� ������ ��������� (��������, ������)
    float4 ColorEnd; // ���� ����� ��������� (��������, �������)
    float SparkleSpeed; // �������� �������� ������
    float SparkleIntensity; // ������������� ������
};

RWTexture2D<float4> OutputTexture : register(u0);

float3 Random(float2 uv, float seed)
{
    return frac(sin(dot(uv + seed, float2(12.9898, 78.233))) * 43758.5453);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // ���������, ��� ���������� � �������� ��������
    if (DTid.x >= (uint) TextureSize.x || DTid.y >= (uint) TextureSize.y)
        return;

    float2 uv = float2(DTid.x, DTid.y) / TextureSize;
    float4 outputColor = float4(0, 0, 0, 0); // ���������� ��� ��� ������ �����

    // ���������� ����������� ����� ���� (����� �������)
    if (uv.x <= Health)
    {
        // �������� �� ColorStart � ColorEnd
        float t = uv.x / Health; // ����������� �� ����������� �����
        outputColor = lerp(ColorStart, ColorEnd, t);

        // ��������� ������������� �����
        float sparkle = Random(uv, Time * SparkleSpeed).x;
        sparkle = pow(sparkle, 10.0) * SparkleIntensity; // ������ ����� �������
        outputColor.rgb += float3(sparkle, sparkle, sparkle);

        // ��������� ������ ���������
        float pulse = sin(Time * 2.0 + uv.x * 5.0) * 0.05 + 0.95;
        outputColor.rgb *= pulse;

        outputColor.a = 1.0; // ������ ��������������
    }

    // ���������� ���������
    OutputTexture[DTid.xy] = outputColor;
}