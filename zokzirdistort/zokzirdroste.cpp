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

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <math.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "ofxsCoords.h"
#include "ofxsFilter.h"
#include "ofxsProcessing.H"
#include "ofxsMatrix2D.h"

#define kPluginName "Zokzir Droste"
#define kPluginGrouping "SalkocsisFX"
#define kPluginDescription \
  "Making droste effect"

#define kPluginIdentifier "com.salkocsisfx.zokzir.droste"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 1
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kSupportsMultipleClipPARs false
#define kSupportsMultipleClipDepths false
#define kRenderThreadSafety eRenderFullySafe

#define kParamLayering "layering"
#define kParamLayeringLabel "Layering"
#define kParamLayeringHint "Where we puts the scaled version, on the back or on the front"
#define kParamLayerOptionOnFront "On Front", "The scaled version is on the front", "onFront"
#define kParamLayerOptionOnBack "On Back", "The scaled version is on the back", "onBack"

enum LayeringEnum
{
  eLayeringOnFront,
  eLayeringOnBack,
};

#define kParamSpin "spin"
#define kParamSpinLabel "Spin"
#define kParamSpinHint "How many spin we need, 0: no spin, +: counter-clockwise, -: clockwise"

#define kParamRadius "radius"
#define kParamRadiusLabel "Radius"
#define kParamRadiusHint "To control how big the image"

#define kParamRatio "ratio"
#define kParamRatioLabel "Ratio"
#define kParamRatioHint "How much the next generation is scaled" \
  ", for example, 0.5 means the next generation is half from current"

#define kParamCenter "center"
#define kParamCenterLabel "Center"
#define kParamCenterHint "The location where the vanishing point is"

#define kParamPosition "position"
#define kParamPositionLabel "Position"
#define kParamPositionHint "Where we need to put the vanishing point"

#define kParamZoom "zoom"
#define kParamZoomLabel "Zoom"
#define kParamZoomHint "To zoom in / out the effects, 1 value represents zoom in to the next generation"

#define kParamRotation "rotation"
#define kParamRotationLabel "Rotation"
#define kParamRotationHint "To rotate the effect, 1 value represents 1 full rotation"

#define kParamEvolution "evolution"
#define kParamEvolutionLabel "Evolution"
#define kParamEvolutionHint "Like zooming but with the respect with the rotation, so it will perfectly loop, 1 value represents zooming and rotating to the next generation"

#define kParamMinDepth "minDepth"
#define kParamMinDepthLabel "Min Depth"
#define kParamMinDepthHint "If the image seems to be clipped, try to change this, will impact performance if the difference between Max Depth and Min Depth is large"

#define kParamMaxDepth "maxDepth"
#define kParamMaxDepthLabel "Max Depth"
#define kParamMaxDepthHint "If the image seems to be clipped, try to change this, will impact performance if the difference between Max Depth and Min Depth is large"

inline OfxPointD cExp(OfxPointD c) {
  double s = exp(c.x);
  return (OfxPointD){
    s * cos(c.y),
    s * sin(c.y)
  };
}

inline OfxPointD cLog(OfxPointD c) {
  return (OfxPointD){
    log(sqrt(c.x * c.x + c.y * c.y)),
    atan2(c.y, c.x)
  };
}

inline OfxPointD cRec(OfxPointD c) {
  double s = c.x * c.x + c.y * c.y;
  return (OfxPointD){
    c.x / s,
    -c.y / s
  };
}

inline OfxPointD cMul(OfxPointD a, OfxPointD b) {
  return (OfxPointD){
    a.x * b.x - a.y * b.y,
    a.x * b.y + a.y * b.x
  };
}

inline OfxPointD cDiv(OfxPointD a, OfxPointD b) {
  double s = b.x * b.x + b.y * b.y;
  return (OfxPointD){
    (a.x * b.x + a.y * b.y) / s,
    (a.y * b.x - a.x * b.y) / s
  };
}

inline OfxPointD cAdd(OfxPointD a, OfxPointD b) {
  return (OfxPointD) {
    a.x + b.x,
    a.y + b.y
  };
}

inline OfxPointD cSub(OfxPointD a, OfxPointD b) {
  return (OfxPointD) {
    a.x - b.x,
    a.y - b.y
  };
}

inline OfxPointD cMulS(OfxPointD c, double s) {
  return (OfxPointD) {
    c.x * s,
    c.y * s
  };
}

inline OfxPointD cDivS(OfxPointD c, double s) {
  return (OfxPointD) {
    c.x / s,
    c.y / s
  };
}

inline void over(float *dst, float *src, float *out) {
  out[3] = src[3] + dst[3] * (1. - src[3]); // alpha
  if (out[3] != 0.) {
    for (int i=0; i<3; i++) {
      out[i] = (src[i] * src[3] + dst[i] * dst[3] * (1. - src[3])) / out[3];
    }
  } else {
    out[0] = out[1] = out[2] = 0.;
  }
}

// Base class for the RGBA and the Alpha processor
class DrosteBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg;

  LayeringEnum  _layering;
  int           _spin;
  double        _radius;
  double        _ratio;
  OfxPointD     _center;
  OfxPointD     _position;
  double        _zoom;
  double        _rotation;
  double        _evolution;
  int           _minDepth;
  int           _maxDepth;

  OFX::RenderArguments _args;
public :
  /** @brief no arg ctor */
  DrosteBase(OFX::ImageEffect &instance)
    : OFX::ImageProcessor(instance)
    , _srcImg(NULL)
    , _layering(eLayeringOnFront)
    , _spin(1)
    , _radius(500.)
    , _ratio(0.5)
    , _center((OfxPointD){0., 0.})
    , _position((OfxPointD){0., 0.})
    , _zoom(0.)
    , _rotation(0.)
    , _evolution(0.)
    , _minDepth(-2)
    , _maxDepth(2)
  {        
  }

  /** @brief set the src image */
  void setSrcImg(OFX::Image *v) {_srcImg = v;}

  void setRenderArguments(const OFX::RenderArguments &args) {
    _args = args;
  }

  void setValues(
    LayeringEnum layering, 
    int spin, 
    double radius, 
    double ratio, 
    OfxPointD center, 
    OfxPointD position,
    double zoom,
    double rotation,
    double evolution,
    int minDepth,
    int maxDepth)
  {
    _layering   = layering;
    _spin       = spin;
    _radius     = radius;
    _ratio      = ratio;
    _center     = center;
    _position   = position;
    _zoom       = zoom;
    _rotation   = rotation;
    _evolution  = evolution;
    _minDepth   = minDepth;
    _maxDepth   = maxDepth;
  }
};

// template to do the RGBA processing
template <class PIX, int nComponents, int max>
class Droste : public DrosteBase {
public :
  // ctor
  Droste(OFX::ImageEffect &instance) 
    : DrosteBase(instance)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    OfxPointD renderScale = _dstImg->getRenderScale();
    double par = _dstImg->getPixelAspectRatio();

    OfxRectD bounds;
    OFX::Coords::toCanonical(_dstImg->getBounds(), renderScale, par, &bounds);

    const double two_pi = 2.0 * OFX::ofxsPi();
    const double r2 = _radius;
    const double r1 = r2 * _ratio;
    const double scale = log(r2 / r1);
    const double angle = atan2(_spin * scale, two_pi);

    double cos_angle = cos(angle);
    OfxPointD complex_angle = cExp((OfxPointD) {0, angle});

    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {

        OfxPointD t_canonical;
        OFX::Coords::toCanonicalSub((OfxPointD){x, y}, renderScale, par, &t_canonical);

        float dst[4] = {0., 0., 0., 0.};
        for (int i=_minDepth; i<=_maxDepth; i++) {
          int depth;
          if (_layering == eLayeringOnFront) {
            depth = i;
          } else if (_layering == eLayeringOnBack) {
            depth = _maxDepth + _minDepth - i;
          } else {
            depth = i;
          }

          OfxPointD c = t_canonical;

          // 10. Translate to position
          c = cSub(c, _position);

          // 9. Take the tiled strips back to ordinary space
          c = cLog(c);

          // 8. To zoom
          c.x -= scale * _zoom;

          // 7. Make spiral
          c = cDiv(cDivS(c, cos_angle), complex_angle);

          // 6. To rotate
          c.y -= two_pi * fmod(_rotation, 1.);

          // 5. Evolution (zoom with looping)
          c.x -= scale * fmod(_evolution, 1.);

          // 4. Tile the strips
          c.x = fmod(c.x, scale);

          // 3. Offset the depth
          c.x += scale * (double) depth;

          // 2. Convert to strip
          c = cMulS(cExp(c), r1);

          // 1. Take from center
          c = cAdd(c, _center);

          OfxPointD t_pixel;
          OFX::Coords::toPixelSub(c, renderScale, par, &t_pixel);

          float src[4];
          OFX::ofxsFilterInterpolate2D<PIX, nComponents, OFX::eFilterCubic, false>(t_pixel.x, t_pixel.y, _srcImg, true, src);

          float out[4];
          over(dst, src, out);

          // copy out to src
          for (int i=0; i<4; i++) {
            dst[i] = out[i];
          }
        }

        for (int c = 0; c < nComponents; c++) {
          dstPix[c] = dst[c] * max;
        }

        // increment the dst pixel
        dstPix += nComponents;
      }
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class DrostePlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *_dstClip;
  OFX::Clip *_srcClip;

  OFX::ChoiceParam   *_layering;
  OFX::IntParam      *_spin;
  OFX::DoubleParam   *_radius;
  OFX::DoubleParam   *_ratio;
  OFX::Double2DParam *_center;
  OFX::Double2DParam *_position;
  OFX::DoubleParam   *_zoom;
  OFX::DoubleParam   *_rotation;
  OFX::DoubleParam   *_evolution;
  OFX::IntParam      *_minDepth;
  OFX::IntParam      *_maxDepth;

public :
  /** @brief ctor */
  DrostePlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , _dstClip(NULL)
    , _srcClip(NULL)
    , _layering(NULL)
    , _spin(NULL)
    , _radius(NULL)
    , _ratio(NULL)
    , _center(NULL)
    , _position(NULL)
    , _zoom(NULL)
    , _rotation(NULL)
    , _evolution(NULL)
    , _minDepth(NULL)
    , _maxDepth(NULL)
  {
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    _layering   = fetchChoiceParam(kParamLayering);
    _spin       = fetchIntParam(kParamSpin);
    _radius     = fetchDoubleParam(kParamRadius);
    _ratio      = fetchDoubleParam(kParamRatio);
    _center     = fetchDouble2DParam(kParamCenter);
    _position   = fetchDouble2DParam(kParamPosition);
    _zoom       = fetchDoubleParam(kParamZoom);
    _rotation   = fetchDoubleParam(kParamRotation);
    _evolution  = fetchDoubleParam(kParamEvolution);
    _minDepth   = fetchIntParam(kParamMinDepth);
    _maxDepth   = fetchIntParam(kParamMaxDepth);
  }

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  /* set up and run a processor */
  void setupAndProcess(DrosteBase &, const OFX::RenderArguments &args);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from


/* set up and run a processor */
void
DrostePlugin::setupAndProcess(DrosteBase &processor, const OFX::RenderArguments &args)
{
  // get a dst image
  std::auto_ptr<OFX::Image> dst(_dstClip->fetchImage(args.time));
  OFX::BitDepthEnum dstBitDepth       = dst->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();

  // fetch main input image
  std::auto_ptr<OFX::Image> src(_srcClip->fetchImage(args.time));

  // make sure bit depths are sane
  if(src.get()) {
    OFX::BitDepthEnum    srcBitDepth      = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1); // HACK!! need to throw an sensible exception here!
  }

  // get parameters
  LayeringEnum layering = (LayeringEnum) _layering->getValueAtTime(args.time);
  int spin              = _spin->getValueAtTime(args.time);
  double radius         = _radius->getValueAtTime(args.time);
  double ratio          = _ratio->getValueAtTime(args.time);
  OfxPointD center      = _center->getValueAtTime(args.time);
  OfxPointD position    = _position->getValueAtTime(args.time);
  double zoom           = _zoom->getValueAtTime(args.time);
  double rotation       = _rotation->getValueAtTime(args.time);
  double evolution      = _evolution->getValueAtTime(args.time);
  int minDepth          = _minDepth->getValueAtTime(args.time);
  int maxDepth          = _maxDepth->getValueAtTime(args.time);

  // set the images
  processor.setDstImg(dst.get());
  processor.setSrcImg(src.get());

  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // set RenderArguments
  processor.setRenderArguments(args);

  // set parameters
  processor.setValues(
    layering,
    spin,
    radius,
    ratio,
    center,
    position,
    zoom,
    rotation,
    evolution,
    minDepth,
    maxDepth
  );

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

// the overridden render function
void
DrostePlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = _dstClip->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = _dstClip->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth) {
case OFX::eBitDepthUByte : {      
  Droste<unsigned char, 4, 255> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  Droste<unsigned short, 4, 65535> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  Droste<float, 4, 1> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;
default :
  OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
  else {
    switch(dstBitDepth) {
case OFX::eBitDepthUByte : {
  Droste<unsigned char, 1, 255> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  Droste<unsigned short, 1, 65535> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  Droste<float, 1, 1> fred(*this);
  setupAndProcess(fred, args);
                           }                          
                           break;
default :
  OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}

mDeclarePluginFactory(DrostePluginFactory, {}, {});

using namespace OFX;
void DrostePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  // basic labels
  desc.setLabels("Droste", "Droste", "Droste");
  desc.setPluginGrouping("alijaya");

  // add the supported contexts, only filter at the moment
  desc.addSupportedContext(eContextFilter);

  // add supported pixel depths
  desc.addSupportedBitDepth(eBitDepthUByte);
  desc.addSupportedBitDepth(eBitDepthUShort);
  desc.addSupportedBitDepth(eBitDepthFloat);

  // set a few flags
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(true);
  desc.setTemporalClipAccess(false);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);

}

void DrostePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
  // Source clip only in the filter context
  // create the mandated source clip
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  {
    ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamLayering);
    param->setLabel(kParamLayeringLabel);
    param->setHint(kParamLayeringHint);
    assert(param->getNOptions() == eLayeringOnFront);
    param->appendOption(kParamLayerOptionOnFront);
    assert(param->getNOptions() == eLayeringOnBack);
    param->appendOption(kParamLayerOptionOnBack);
    param->setDefault(eLayeringOnFront);
  }

  {
    IntParamDescriptor *param = desc.defineIntParam(kParamSpin);
    param->setLabel(kParamSpinLabel);
    param->setHint(kParamSpinHint);
    param->setDefault(1);
    param->setDisplayRange(-10, 10);
  }

  {
    DoubleParamDescriptor *param = desc.defineDoubleParam(kParamRadius);
    param->setLabel(kParamRadiusLabel);
    param->setHint(kParamRadiusHint);
    param->setDefault(500.);
    param->setDisplayRange(0., 1000.);
    param->setDoubleType(eDoubleTypeX);
  }

  {
    DoubleParamDescriptor *param = desc.defineDoubleParam(kParamRatio);
    param->setLabel(kParamRatioLabel);
    param->setHint(kParamRatioHint);
    param->setDefault(0.5);
    param->setDisplayRange(0., 1.);
    param->setDoubleType(eDoubleTypeScale);
  }

  {
    Double2DParamDescriptor *param = desc.defineDouble2DParam(kParamCenter);
    param->setLabel(kParamCenterLabel);
    param->setHint(kParamCenterHint);
    param->setDoubleType(eDoubleTypeXYAbsolute);
  }

  {
    Double2DParamDescriptor *param = desc.defineDouble2DParam(kParamPosition);
    param->setLabel(kParamPositionLabel);
    param->setHint(kParamPositionHint);
    param->setDoubleType(eDoubleTypeXYAbsolute);
  }

  {
    DoubleParamDescriptor *param = desc.defineDoubleParam(kParamZoom);
    param->setLabel(kParamZoomLabel);
    param->setHint(kParamZoomHint);
    param->setDefault(0.);
    param->setDisplayRange(-10., 10.);
    param->setDoubleType(eDoubleTypeScale);
  }

  {
    DoubleParamDescriptor *param = desc.defineDoubleParam(kParamRotation);
    param->setLabel(kParamRotationLabel);
    param->setHint(kParamRotationHint);
    param->setDefault(0.);
    param->setDisplayRange(-1., 1.);
    param->setDoubleType(eDoubleTypeAngle);
  }

  {
    DoubleParamDescriptor *param = desc.defineDoubleParam(kParamEvolution);
    param->setLabel(kParamEvolutionLabel);
    param->setHint(kParamEvolutionHint);
    param->setDefault(0.);
    param->setDisplayRange(-10., 10.);
    param->setDoubleType(eDoubleTypeScale);
  }

  {
    IntParamDescriptor *param = desc.defineIntParam(kParamMinDepth);
    param->setLabel(kParamMinDepthLabel);
    param->setHint(kParamMinDepthHint);
    param->setDefault(-2);
    param->setDisplayRange(-10, 10);
  }

  {
    IntParamDescriptor *param = desc.defineIntParam(kParamMaxDepth);
    param->setLabel(kParamMaxDepthLabel);
    param->setHint(kParamMaxDepthHint);
    param->setDefault(2);
    param->setDisplayRange(-10, 10);
  }

}

OFX::ImageEffect* DrostePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
  return new DrostePlugin(handle);
}

static DrostePluginFactory p("com.alijaya.droste", 1, 0);
mRegisterPluginFactoryInstance(p)
