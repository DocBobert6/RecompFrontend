// [wcw hdr] Composites the UI (rendered offscreen in sRGB) into an scRGB swap chain: linear light,
// scaled so UI white sits at the paper white brightness.

struct Input {
    float4x4 transform;
    float2 translation;
    float hdrScale;
};

[[vk::push_constant]]
ConstantBuffer<Input> gInput : register(b0, space0);

SamplerState gSampler : register(s1, space0);
Texture2D<float4> gTexture : register(t2, space1);

void PSMain(
    in float4 iColor : COLOR,
    in float2 iUV : TEXCOORD,
    out float4 oColor : SV_TARGET
)
{
    float4 color = gTexture.Sample(gSampler, iUV) * iColor;

    // The offscreen UI was blended over transparent black, so its color is premultiplied by alpha.
    float3 srgb = (color.a > 0.0f) ? saturate(color.rgb / color.a) : float3(0.0f, 0.0f, 0.0f);
    float3 linearColor = lerp(pow((srgb + 0.055f) / 1.055f, 2.4f), srgb / 12.92f, step(srgb, 0.04045f));
    oColor = float4(linearColor * gInput.hdrScale, color.a);
}
