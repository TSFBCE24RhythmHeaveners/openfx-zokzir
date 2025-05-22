// Copyright OpenFX and contributors to the OpenFX project.
// Copyright Hashory.
// SPDX-License-Identifier: BSD-3-Clause

/*
Ofx Example plugin that show a very simple plugin that inverts an image.

It is meant to illustrate certain features of the API, as opposed to
being a perfectly crafted piece of image processing software.

The main features are
- basic plugin definition
- basic property usage
- basic image access and rendering
*/

#include "ofxImageEffect.h"
#include "ofxPixels.h"
#include <cstring>
#include <new>

#if defined __APPLE__ || defined __linux__ || defined __FreeBSD__
#define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#define EXPORT OfxExport
#else
#error Not building on your operating system quite yet
#endif

// pointers to various bits of the host
OfxHost* gHost;
OfxImageEffectSuiteV1* gEffectHost = nullptr;
OfxPropertySuiteV1* gPropHost = nullptr;

// the name of the output clip
const int MAX_COLOR_VALUE = 255;

// look up a pixel in the image, does bounds checking to see if it is in the
// image rectangle
inline auto pixelAddress(OfxRGBAColourB* img, OfxRectI rect, int xCorad,
    int yCorad, int bytesPerLine) -> OfxRGBAColourB*
{
    if (xCorad < rect.x1 || xCorad >= rect.x2 || yCorad < rect.y1 || yCorad > rect.y2) {
        return nullptr;
    }
    auto* pix = (OfxRGBAColourB*)(((char*)img) + static_cast<ptrdiff_t>((yCorad - rect.y1) * bytesPerLine));
    pix += xCorad - rect.x1;
    return pix;
}

// throws this if it can't fetch an image
class NoImageEx { };

// the process code  that the host sees
static auto render(OfxImageEffectHandle instance,
    OfxPropertySetHandle inArgs,
    OfxPropertySetHandle /*outArgs*/) -> OfxStatus
{
    // get the render window and the time from the inArgs
    OfxTime time;
    OfxRectI renderWindow;
    OfxStatus status = kOfxStatOK;

    gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    gPropHost->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4,
        &renderWindow.x1);

    // fetch output clip
    OfxImageClipHandle outputClip;
    gEffectHost->clipGetHandle(instance, kOfxImageEffectOutputClipName,
        &outputClip, nullptr);

    OfxPropertySetHandle outputImg = nullptr;
    OfxPropertySetHandle sourceImg = nullptr;
    try {
        // fetch image to render into from that clip
        OfxPropertySetHandle outputImg;
        if (gEffectHost->clipGetImage(outputClip, time, nullptr, &outputImg) != kOfxStatOK) {
            throw NoImageEx();
        }

        // fetch output image info from that handle
        int dstRowBytes;
        OfxRectI dstRect;
        void* dstPtr;
        gPropHost->propGetInt(outputImg, kOfxImagePropRowBytes, 0, &dstRowBytes);
        gPropHost->propGetIntN(outputImg, kOfxImagePropBounds, 4, &dstRect.x1);
        gPropHost->propGetInt(outputImg, kOfxImagePropRowBytes, 0, &dstRowBytes);
        gPropHost->propGetPointer(outputImg, kOfxImagePropData, 0, &dstPtr);

        // fetch main input clip
        OfxImageClipHandle sourceClip;
        gEffectHost->clipGetHandle(instance, kOfxImageEffectSimpleSourceClipName,
            &sourceClip, nullptr);

        // fetch image at render time from that clip
        if (gEffectHost->clipGetImage(sourceClip, time, nullptr, &sourceImg) != kOfxStatOK) {
            throw NoImageEx();
        }

        // fetch image info out of that handle
        int srcRowBytes;
        OfxRectI srcRect;
        void* srcPtr;
        gPropHost->propGetInt(sourceImg, kOfxImagePropRowBytes, 0, &srcRowBytes);
        gPropHost->propGetIntN(sourceImg, kOfxImagePropBounds, 4, &srcRect.x1);
        gPropHost->propGetInt(sourceImg, kOfxImagePropRowBytes, 0, &srcRowBytes);
        gPropHost->propGetPointer(sourceImg, kOfxImagePropData, 0, &srcPtr);

        // cast data pointers to 8 bit RGBA
        auto* src = (OfxRGBAColourB*)srcPtr;
        auto* dst = (OfxRGBAColourB*)dstPtr;

        // and do some inverting
        for (int yCorad = renderWindow.y1; yCorad < renderWindow.y2; yCorad++) {
            if (gEffectHost->abort(instance) != 0) {
                break;
            }
            OfxRGBAColourB* dstPix = pixelAddress(dst, dstRect, renderWindow.x1, yCorad, dstRowBytes);

            for (int xCorad = renderWindow.x1; xCorad < renderWindow.x2; xCorad++) {
                OfxRGBAColourB* srcPix = pixelAddress(src, srcRect, xCorad, yCorad, srcRowBytes);

                if (srcPix != nullptr) {
                    dstPix->r = MAX_COLOR_VALUE - srcPix->r;
                    dstPix->g = MAX_COLOR_VALUE - srcPix->g;
                    dstPix->b = MAX_COLOR_VALUE - srcPix->b;
                    dstPix->a = MAX_COLOR_VALUE - srcPix->a;
                } else {
                    dstPix->r = 0;
                    dstPix->g = 0;
                    dstPix->b = 0;
                    dstPix->a = 0;
                }
                dstPix++;
            }
        }

        // we are finished with the source images so release them
    } catch (NoImageEx&) {
        // if we were interrupted, the failed fetch is fine, just return
        // kOfxStatOK otherwise, something weird happened
        if (gEffectHost->abort(instance) == 0) {
            status = kOfxStatFailed;
        }
    }

    if (sourceImg != nullptr) {
        gEffectHost->clipReleaseImage(sourceImg);
    }
    if (outputImg != nullptr) {
        gEffectHost->clipReleaseImage(outputImg);
    }

    // all was well
    return status;
}

//  describe the plugin in context
static auto describeInContext(OfxImageEffectHandle effect,
    OfxPropertySetHandle /*inArgs*/) -> OfxStatus
{
    OfxPropertySetHandle props;
    // define the single output clip in both contexts
    gEffectHost->clipDefine(effect, kOfxImageEffectOutputClipName, &props);

    // set the component types we can handle on out output
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0,
        kOfxImageComponentRGBA);

    // define the single source clip in both contexts
    gEffectHost->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);

    // set the component types we can handle on our main input
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0,
        kOfxImageComponentRGBA);

    return kOfxStatOK;
}

////////////////////////////////////////////////////////////////////////////////
// the plugin's description routine
static auto describe(OfxImageEffectHandle effect) -> OfxStatus
{
    // get the property handle for the plugin
    OfxPropertySetHandle effectProps;
    gEffectHost->getPropertySet(effect, &effectProps);

    // say we cannot support multiple pixel depths and let the clip preferences
    // action deal with it all.
    gPropHost->propSetInt(effectProps,
        kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);

    // set the bit depths the plugin can handle
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths,
        0, kOfxBitDepthByte);

    // set plugin label and the group it belongs to
    gPropHost->propSetString(effectProps, kOfxPropLabel, 0, "OFX Invert Example");
    gPropHost->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0,
        "OFX Example");

    // define the contexts we can be used in
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 0,
        kOfxImageEffectContextFilter);

    return kOfxStatOK;
}

////////////////////////////////////////////////////////////////////////////////
// Called at load
static auto onLoad() -> OfxStatus
{
    // fetch the host suites out of the global host pointer
    if (gHost == nullptr) {
        return kOfxStatErrMissingHostFeature;
    }

    gEffectHost = (OfxImageEffectSuiteV1*)gHost->fetchSuite(
        gHost->host, kOfxImageEffectSuite, 1);
    gPropHost = (OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host,
        kOfxPropertySuite, 1);
    if (gEffectHost == nullptr || gPropHost == nullptr) {
        return kOfxStatErrMissingHostFeature;
    }
    return kOfxStatOK;
}

////////////////////////////////////////////////////////////////////////////////
// The main entry point function
static auto pluginMain(const char* action, const void* handle,
    OfxPropertySetHandle inArgs,
    OfxPropertySetHandle outArgs) -> OfxStatus
{
    try {
        // cast to appropriate type
        auto* effect = (OfxImageEffectHandle)handle;

        if (strcmp(action, kOfxActionLoad) == 0) {
            return onLoad();
        }
        if (strcmp(action, kOfxActionDescribe) == 0) {
            return describe(effect);
        }
        if (strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
            return describeInContext(effect, inArgs);
        }
        if (strcmp(action, kOfxImageEffectActionRender) == 0) {
            return render(effect, inArgs, outArgs);
        }
    } catch (std::bad_alloc) {
        // catch memory
        // std::cout << "OFX Plugin Memory error." << std::endl;
        return kOfxStatErrMemory;
    } catch (const std::exception&) {
        // standard exceptions
        // std::cout << "OFX Plugin error: " << e.what() << std::endl;
        return kOfxStatErrUnknown;
    } catch (int err) {
        // ho hum, gone wrong somehow
        return err;
    } catch (...) {
        // everything else
        // std::cout << "OFX Plugin error" << std::endl;
        return kOfxStatErrUnknown;
    }

    // other actions to take the default value
    return kOfxStatReplyDefault;
}

// function to set the host structure
static void setHostFunc(OfxHost* hostStruct) { gHost = hostStruct; }

////////////////////////////////////////////////////////////////////////////////
// the plugin struct
static OfxPlugin basicPlugin = { kOfxImageEffectPluginApi,
    1,
    "uk.co.thefoundry.OfxInvertExample",
    1,
    0,
    setHostFunc,
    pluginMain };

// the two mandated functions
EXPORT auto OfxGetPlugin(int nth) -> OfxPlugin*
{
    if (nth == 0) {
        return &basicPlugin;
    }
    return nullptr;
}

EXPORT auto OfxGetNumberOfPlugins(void) -> int { return 1; }
