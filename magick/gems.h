/*
  Graphic Gems - Graphic Support Methods.
*/
#ifndef _GEMS_H
#define _GEMS_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Graphic gems define declarations.
*/
extern MagickExport double
  ExpandAffine(const AffineMatrix *),
  Permutate(int,int);

extern MagickExport PixelPacket
  CompositeOver(const PixelPacket *,const double,const PixelPacket *,
    const double);

extern MagickExport int
  GetOptimalKernelWidth(const double,const double),
  GetOptimalKernelWidth1D(const double,const double),
  GetOptimalKernelWidth2D(const double,const double);

extern MagickExport PixelPacket
  InterpolateColor(Image *,const double,const double);

extern MagickExport Quantum
  GenerateNoise(const Quantum,const NoiseType);

extern MagickExport void
  Contrast(const int,Quantum *,Quantum *,Quantum *),
  HSLTransform(const double,const double,const double,Quantum *,Quantum *,
    Quantum *),
  Hull(const int,const int,const int,const unsigned int,const unsigned int,
    Quantum *,Quantum *),
  IdentityAffine(AffineMatrix *),
  Modulate(double,double,double,Quantum *,Quantum *,Quantum *),
  TransformHSL(const Quantum,const Quantum,const Quantum,double *,double *,
    double *),
  Upsample(const unsigned int,const unsigned int,const unsigned int,
    unsigned char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
