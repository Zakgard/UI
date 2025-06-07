// HPBarCompute.hlsl
cbuffer HPBarCB : register(b0)
{
    float2 TextureSize; // ������ �������� � �������� (������, ������)
    float Time; // ����� ��� ��������
    float Health; // ������� �������� (0.0 - 1.0)
    float4 ColorStart; // ���� ������ ��������� (��������, ������)
    float4 ColorEnd; // ���� ����� ��������� (��������, �������)
    float SparkleSpeed; // �������� �������� ������
    float SparkleIntensity; // ������������� ������
    float2 StarPosMin; // ����������� ������� ������ ����� (��������������� ����������)
    float2 StarPosMax; // ������������ ������� ������ ����� (��������������� ����������)
};

RWTexture2D<float4> OutputTexture : register(u0);

// ��������� ���������� ����� �� ������ seed
float Random(float2 uv, float seed)
{
    return frac(sin(dot(uv + seed, float2(12.9898, 78.233))) * 43758.5453);
}

// ��������� ���������� ������� 2D
float2 Random2(float2 uv, float seed)
{
    return frac(sin(float2(dot(uv + seed, float2(127.1, 311.7)),
                    dot(uv + seed, float2(269.5, 183.3)))) * 43758.5453);
}

// ��������� ���������� ����� (RGB) �� ������ seed
float3 RandomColor(float2 uv, float seed)
{
    return float3(
        Random(uv, seed),
        Random(uv, seed + 1.0),
        Random(uv, seed + 2.0)
    );
}

// ������� ��� ��������� ��������� ����� � ������
float4 SpawnStar(float2 pixelPos, float seed)
{
    float starCount = floor(Random(float2(0.0, seed), seed) * 40.0 + 20.0); // 20-60 �����
    float starEffect = 0.0;
    float3 starColor = float3(0, 0, 0); // �������� ���� ������ (�����������)

    // ������� ������ � ���������� �����������
    float2 minPos = StarPosMin * TextureSize;
    float2 maxPos = float2(min(StarPosMax.x, Health), StarPosMax.y) * TextureSize;

    float aspectRatio = TextureSize.x / TextureSize.y;

    for (int i = 0; i < starCount; i++)
    {
        // ���������� ��������� ������
        float2 starPos = Random2(float2(float(i), seed), seed + i * 0.1);
        starPos = lerp(minPos, maxPos, starPos);

        // ��������� �������� ����� �������
        float moveSpeed = 50.0;
        starPos.x += fmod(Time * moveSpeed + i * 100.0, maxPos.x - minPos.x);
        starPos.y += sin(Time * 0.5 + i * 0.3) * 5.0; // ��������� ��������� �� Y

        // ��������� ������ ������ (10-30 ��������)
        float starSize = lerp(10.0, 30.0, Random(float2(float(i), seed), seed + i * 0.2));

        // ������������ ���������� ��� ������� �����
        float2 adjustedPixelPos = pixelPos;
        float2 adjustedStarPos = starPos;
        adjustedPixelPos.x /= aspectRatio;
        adjustedStarPos.x /= aspectRatio;

        // ���������� �� ������ ������
        float dist = length(adjustedPixelPos - adjustedStarPos);

        // ������� ������ �������� (������ ���������)
        float star = smoothstep(starSize, starSize * 0.5, dist);
        star *= (sin(Time * 0.5 + i * 0.5) * 0.3 + 0.7); // ��������

        // ���������� ��������� ���� ��� ������
        float3 color = RandomColor(float2(float(i), seed), seed + i * 0.5);
        color = saturate(color); // ������������ �������� [0, 1]

        // ��������� ����� ������ � ����� ������
        starEffect += star;
        starColor += star * color; // �������� ���� �� ������������� ������
    }

    // ����������� ���� (����� �� ���� ���������)
    starColor = saturate(starColor);
    return float4(starColor, starEffect * 3.0); // ���������� ���� + ����� (�������������)
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= (uint) TextureSize.x || DTid.y >= (uint) TextureSize.y)
        return;

    float2 pixelPos = float2(DTid.x, DTid.y);
    float2 uv = pixelPos / TextureSize;
    float4 outputColor = float4(0, 0, 0, 0);

    float clampedHealth = clamp(Health, 0.0, 1.0);

    if (uv.x <= clampedHealth)
    {
        // �������� ��������
        float t = uv.x / clampedHealth;
        outputColor = lerp(ColorStart, ColorEnd, t);

        // �����
        float sparkle = Random(uv, Time * SparkleSpeed);
        sparkle = pow(sparkle, 10.0) * SparkleIntensity;
        outputColor.rgb += float3(sparkle, sparkle, sparkle);

        // ���������
        float pulse = sin(Time * 2.0 + uv.x * 5.0) * 0.05 + 0.95;
        outputColor.rgb *= pulse;

        // ��������� ������ �� ��������� ������
        float4 starEffect = SpawnStar(pixelPos, sin(Time * 0.05));
        outputColor.rgb += starEffect.rgb; // ��������� ���� �����
        outputColor.a = 1.0;
    }

    OutputTexture[DTid.xy] = outputColor;
}