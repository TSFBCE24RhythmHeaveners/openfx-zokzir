/*
 * ColorCorrectionPlugin.cpp
 *
 * Example OFX Color Correction Plugin for Vegas Pro
 * Demonstrates gain, gamma, and saturation controls
 */

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxProperty.h"
#include "ofxParam.h"
#include "ofxUtilities.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// Plugin identifiers
#define kPluginName "Zokzir Color Correction"
#define kPluginGrouping "Color"
#define kPluginDescription "Basic color correction with gain, gamma, and saturation controls"
#define kPluginIdentifier "com.salkocsisfx.zokzir.colorcorrection"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

// Parameter names
#define kParamGain "gain"
#define kParamGainLabel "Gain"
#define kParamGainHint "Multiplies the image intensity"

#define kParamGamma "gamma"
#define kParamGammaLabel "Gamma"
#define kParamGammaHint "Applies gamma correction"

#define kParamSaturation "saturation"
#define kParamSaturationLabel "Saturation"
#define kParamSaturationHint "Adjusts color saturation"

#define kParamRGBGain "rgbGain"
#define kParamRGBGainLabel "RGB Gain"
#define kParamRGBGainHint "Individual gain for Red, Green, Blue channels"

using namespace ofx;

/**
 * @brief Process pixels for color correction
 */
template<typename T>
void processPixels(
    T* dst, const T* src,
    const OfxRectI& renderWindow,
    int dstRowBytes, int srcRowBytes,
    double gain, double gamma, double saturation,
    double rGain, double gGain, double bGain,
    double maxValue)
{
    int width = renderWindow.x2 - renderWindow.x1;
    int height = renderWindow.y2 - renderWindow.y1;

    for (int y = 0; y < height; y++) {
        T* dstRow = (T*)((char*)dst + y * dstRowBytes);
        const T* srcRow = (const T*)((const char*)src + y * srcRowBytes);

        for (int x = 0; x < width; x++) {
            int pixelIndex = x * 4; // RGBA

            // Read source pixels and normalize
            double r = srcRow[pixelIndex + 0] / maxValue;
            double g = srcRow[pixelIndex + 1] / maxValue;
            double b = srcRow[pixelIndex + 2] / maxValue;
            double a = srcRow[pixelIndex + 3] / maxValue;

            // Apply RGB gain
            r *= rGain;
            g *= gGain;
            b *= bGain;

            // Apply overall gain
            r *= gain;
            g *= gain;
            b *= gain;

            // Apply gamma
            if (gamma != 1.0) {
                r = std::pow(std::max(0.0, r), gamma);
                g = std::pow(std::max(0.0, g), gamma);
                b = std::pow(std::max(0.0, b), gamma);
            }

            // Apply saturation
            if (saturation != 1.0) {
                // Calculate luminance (Rec. 709)
                double luma = 0.2126 * r + 0.7152 * g + 0.0722 * b;

                // Interpolate between grayscale and color
                r = luma + saturation * (r - luma);
                g = luma + saturation * (g - luma);
                b = luma + saturation * (b - luma);
            }

            // Clamp and write output
            dstRow[pixelIndex + 0] = (T)(std::min(std::max(r, 0.0), 1.0) * maxValue);
            dstRow[pixelIndex + 1] = (T)(std::min(std::max(g, 0.0), 1.0) * maxValue);
            dstRow[pixelIndex + 2] = (T)(std::min(std::max(b, 0.0), 1.0) * maxValue);
            dstRow[pixelIndex + 3] = (T)(std::min(std::max(a, 0.0), 1.0) * maxValue);
        }
    }
}

/**
 * @brief Main rendering function
 */
static OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    // Get the render window
    PropertySet inArgsProps(inArgs);
    OfxRectI renderWindow;
    renderWindow.x1 = inArgsProps.getInt(kOfxImageEffectPropRenderWindow, 0);
    renderWindow.y1 = inArgsProps.getInt(kOfxImageEffectPropRenderWindow, 1);
    renderWindow.x2 = inArgsProps.getInt(kOfxImageEffectPropRenderWindow, 2);
    renderWindow.y2 = inArgsProps.getInt(kOfxImageEffectPropRenderWindow, 3);

    double time = inArgsProps.getDouble(kOfxPropTime);

    // Get clips
    OfxImageClipHandle sourceClip, outputClip;
    OfxPropertySetHandle sourceClipProps, outputClipProps;

    gImageEffectSuite->clipGetHandle(instance, kOfxImageEffectSimpleSourceClipName, &sourceClip, &sourceClipProps);
    gImageEffectSuite->clipGetHandle(instance, kOfxImageEffectOutputClipName, &outputClip, &outputClipProps);

    // Get images
    OfxPropertySetHandle sourceImg, outputImg;
    gImageEffectSuite->clipGetImage(sourceClip, time, nullptr, &sourceImg);
    gImageEffectSuite->clipGetImage(outputClip, time, nullptr, &outputImg);

    // Get parameters
    OfxParamSetHandle paramSet;
    gImageEffectSuite->getParamSet(instance, &paramSet);

    OfxParamHandle gainParam, gammaParam, saturationParam, rgbGainParam;
    OfxPropertySetHandle gainParamProps, gammaParamProps, saturationParamProps, rgbGainParamProps;

    gParameterSuite->paramGetHandle(paramSet, kParamGain, &gainParam, &gainParamProps);
    gParameterSuite->paramGetHandle(paramSet, kParamGamma, &gammaParam, &gammaParamProps);
    gParameterSuite->paramGetHandle(paramSet, kParamSaturation, &saturationParam, &saturationParamProps);
    gParameterSuite->paramGetHandle(paramSet, kParamRGBGain, &rgbGainParam, &rgbGainParamProps);

    // Get parameter values
    Param gain(gainParam);
    Param gamma(gammaParam);
    Param saturation(saturationParam);
    Param rgbGain(rgbGainParam);

    double gainValue, gammaValue, saturationValue;
    double rGain, gGain, bGain;

    gain.getValueAtTime(time, gainValue);
    gamma.getValueAtTime(time, gammaValue);
    saturation.getValueAtTime(time, saturationValue);
    rgbGain.getValueAtTime(time, rGain, gGain, bGain);

    // Get image properties
    PropertySet srcImgProps(sourceImg);
    PropertySet dstImgProps(outputImg);

    void* srcData = srcImgProps.getPointer(kOfxImagePropData);
    void* dstData = dstImgProps.getPointer(kOfxImagePropData);

    int srcRowBytes = srcImgProps.getInt(kOfxImagePropRowBytes);
    int dstRowBytes = dstImgProps.getInt(kOfxImagePropRowBytes);

    const char* pixelDepth = srcImgProps.getString(kOfxImageEffectPropPixelDepth);

    // Process based on bit depth
    if (strcmp(pixelDepth, kOfxBitDepthByte) == 0) {
        processPixels<unsigned char>(
            (unsigned char*)dstData, (const unsigned char*)srcData,
            renderWindow, dstRowBytes, srcRowBytes,
            gainValue, gammaValue, saturationValue,
            rGain, gGain, bGain, 255.0);
    }
    else if (strcmp(pixelDepth, kOfxBitDepthShort) == 0) {
        processPixels<unsigned short>(
            (unsigned short*)dstData, (const unsigned short*)srcData,
            renderWindow, dstRowBytes, srcRowBytes,
            gainValue, gammaValue, saturationValue,
            rGain, gGain, bGain, 65535.0);
    }
    else if (strcmp(pixelDepth, kOfxBitDepthFloat) == 0) {
        processPixels<float>(
            (float*)dstData, (const float*)srcData,
            renderWindow, dstRowBytes, srcRowBytes,
            gainValue, gammaValue, saturationValue,
            rGain, gGain, bGain, 1.0);
    }

    // Release images
    gImageEffectSuite->clipReleaseImage(sourceImg);
    gImageEffectSuite->clipReleaseImage(outputImg);

    return kOfxStatOK;
}

/**
 * @brief Describe the plugin
 */
static OfxStatus describe(OfxImageEffectHandle descriptor)
{
    OfxPropertySetHandle effectProps;
    gImageEffectSuite->getPropertySet(descriptor, &effectProps);

    PropertySet props(effectProps);

    // Plugin properties
    props.setString(kOfxPropLabel, kPluginName);
    props.setString(kOfxImageEffectPluginPropGrouping, kPluginGrouping);
    props.setString(kOfxImageEffectPluginPropGrouping, kPluginDescription);

    // Supported contexts
    const char* contexts[] = { kOfxImageEffectContextFilter };
    props.setStringN(kOfxImageEffectPropSupportedContexts, 1, contexts);

    // Supported pixel depths
    const char* pixelDepths[] = { kOfxBitDepthByte, kOfxBitDepthShort, kOfxBitDepthFloat };
    props.setStringN(kOfxImageEffectPropSupportedPixelDepths, 3, pixelDepths);

    // Other properties
    props.setInt(kOfxImageEffectPropSupportsTiles, 1);
    props.setInt(kOfxImageEffectPropSupportsMultiResolution, 1);
    props.setInt(kOfxImageEffectPropTemporalClipAccess, 0);
    props.setString(kOfxImageEffectPropRenderThreadSafety, kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

/**
 * @brief Describe the plugin in a specific context
 */
static OfxStatus describeInContext(OfxImageEffectHandle descriptor, OfxPropertySetHandle inArgs)
{
    PropertySet inArgsProps(inArgs);
    const char* context = inArgsProps.getString(kOfxImageEffectPropContext);

    // Define clips
    OfxPropertySetHandle clipProps;

    // Source clip
    gImageEffectSuite->clipDefine(descriptor, kOfxImageEffectSimpleSourceClipName, &clipProps);
    PropertySet sourceClipProps(clipProps);
    const char* rgbaComponents[] = { kOfxImageComponentRGBA };
    sourceClipProps.setStringN(kOfxImageClipPropSupportedComponents, 1, rgbaComponents);

    // Output clip
    gImageEffectSuite->clipDefine(descriptor, kOfxImageEffectOutputClipName, &clipProps);
    PropertySet outputClipProps(clipProps);
    outputClipProps.setStringN(kOfxImageClipPropSupportedComponents, 1, rgbaComponents);

    // Define parameters
    OfxParamSetHandle paramSet;
    gImageEffectSuite->getParamSet(descriptor, &paramSet);

    OfxPropertySetHandle paramProps;

    // Gain parameter
    gParameterSuite->paramDefine(paramSet, kOfxParamTypeDouble, kParamGain, &paramProps);
    PropertySet gainProps(paramProps);
    gainProps.setString(kOfxPropLabel, kParamGainLabel);
    gainProps.setString(kOfxParamPropHint, kParamGainHint);
    gainProps.setDouble(kOfxParamPropDefault, 1.0);
    gainProps.setDouble(kOfxParamPropMin, 0.0);
    gainProps.setDouble(kOfxParamPropMax, 4.0);
    gainProps.setDouble(kOfxParamPropDisplayMin, 0.0);
    gainProps.setDouble(kOfxParamPropDisplayMax, 2.0);
    gainProps.setInt(kOfxParamPropAnimates, 1);

    // Gamma parameter
    gParameterSuite->paramDefine(paramSet, kOfxParamTypeDouble, kParamGamma, &paramProps);
    PropertySet gammaProps(paramProps);
    gammaProps.setString(kOfxPropLabel, kParamGammaLabel);
    gammaProps.setString(kOfxParamPropHint, kParamGammaHint);
    gammaProps.setDouble(kOfxParamPropDefault, 1.0);
    gammaProps.setDouble(kOfxParamPropMin, 0.1);
    gammaProps.setDouble(kOfxParamPropMax, 4.0);
    gammaProps.setDouble(kOfxParamPropDisplayMin, 0.2);
    gammaProps.setDouble(kOfxParamPropDisplayMax, 3.0);
    gammaProps.setInt(kOfxParamPropAnimates, 1);

    // Saturation parameter
    gParameterSuite->paramDefine(paramSet, kOfxParamTypeDouble, kParamSaturation, &paramProps);
    PropertySet saturationProps(paramProps);
    saturationProps.setString(kOfxPropLabel, kParamSaturationLabel);
    saturationProps.setString(kOfxParamPropHint, kParamSaturationHint);
    saturationProps.setDouble(kOfxParamPropDefault, 1.0);
    saturationProps.setDouble(kOfxParamPropMin, 0.0);
    saturationProps.setDouble(kOfxParamPropMax, 4.0);
    saturationProps.setDouble(kOfxParamPropDisplayMin, 0.0);
    saturationProps.setDouble(kOfxParamPropDisplayMax, 2.0);
    saturationProps.setInt(kOfxParamPropAnimates, 1);

    // RGB Gain parameter
    gParameterSuite->paramDefine(paramSet, kOfxParamTypeRGB, kParamRGBGain, &paramProps);
    PropertySet rgbGainProps(paramProps);
    rgbGainProps.setString(kOfxPropLabel, kParamRGBGainLabel);
    rgbGainProps.setString(kOfxParamPropHint, kParamRGBGainHint);
    double defaultRGB[] = { 1.0, 1.0, 1.0 };
    rgbGainProps.setDoubleN(kOfxParamPropDefault, 3, defaultRGB);
    rgbGainProps.setDouble(kOfxParamPropMin, 0.0);
    rgbGainProps.setDouble(kOfxParamPropMax, 4.0);
    rgbGainProps.setDouble(kOfxParamPropDisplayMin, 0.0);
    rgbGainProps.setDouble(kOfxParamPropDisplayMax, 2.0);
    rgbGainProps.setInt(kOfxParamPropAnimates, 1);

    return kOfxStatOK;
}

/**
 * @brief Create an instance
 */
static OfxStatus createInstance(OfxImageEffectHandle instance)
{
    return kOfxStatOK;
}

/**
 * @brief Destroy an instance
 */
static OfxStatus destroyInstance(OfxImageEffectHandle instance)
{
    return kOfxStatOK;
}

/**
 * @brief Main entry point
 */
static OfxStatus mainEntry(const char *action, const void *handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (strcmp(action, kOfxActionLoad) == 0) {
        return kOfxStatOK;
    }
    else if (strcmp(action, kOfxActionUnload) == 0) {
        return kOfxStatOK;
    }
    else if (strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect);
    }
    else if (strcmp(action, kOfxActionDescribeInContext) == 0) {
        return describeInContext(effect, inArgs);
    }
    else if (strcmp(action, kOfxActionCreateInstance) == 0) {
        return createInstance(effect);
    }
    else if (strcmp(action, kOfxActionDestroyInstance) == 0) {
        return destroyInstance(effect);
    }
    else if (strcmp(action, kOfxImageEffectActionRender) == 0) {
        return render(effect, inArgs, outArgs);
    }
    else if (strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        return kOfxStatReplyDefault;
    }
    else if (strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) {
        return kOfxStatReplyDefault;
    }
    else if (strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        return kOfxStatOK;
    }
    else if (strcmp(action, kOfxImageEffectActionIsIdentity) == 0) {
        return kOfxStatReplyDefault;
    }
    else if (strcmp(action, kOfxActionBeginInstanceEdit) == 0) {
        return kOfxStatOK;
    }
    else if (strcmp(action, kOfxActionEndInstanceEdit) == 0) {
        return kOfxStatOK;
    }
    else if (strcmp(action, kOfxActionInstanceChanged) == 0) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

/**
 * @brief Set host callback
 */
static void setHostFunc(OfxHost *host)
{
    initializeSuites(host);
}

/**
 * @brief Plugin descriptor
 */
static OfxPlugin colorCorrectionPlugin = {
    kOfxImageEffectPluginApi,
    kOfxImageEffectPluginApiVersion,
    kPluginIdentifier,
    kPluginVersionMajor,
    kPluginVersionMinor,
    setHostFunc,
    mainEntry
};

/**
 * @brief Number of plugins in this binary
 */
OfxExport int OfxGetNumberOfPlugins(void)
{
    return 1;
}

/**
 * @brief Get the nth plugin
 */
OfxExport OfxPlugin *OfxGetPlugin(int nth)
{
    if (nth == 0) {
        return &colorCorrectionPlugin;
    }
    return nullptr;
}
