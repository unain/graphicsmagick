/*
  ImageMagick Exception Methods.
*/
#ifndef _MAGICK_ERROR_H
#define _MAGICK_ERROR_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Exception define definitions.
*/
#define ThrowBinaryException(code,reason,description) \
{ \
  if (image != (Image *) NULL) \
    ThrowException(&image->exception,code,reason,description); \
  return(False); \
}
#define ThrowImageException(code,reason,description) \
{ \
  ThrowException(exception,code,reason,description); \
  return((Image *) NULL); \
}
#define ThrowReaderException(code,reason,image) \
{ \
  if ((image) == (Image *) NULL) \
    ThrowException(exception,code,reason,(char *) NULL); \
  else \
    { \
      ThrowException(exception,code,reason,(image)->filename); \
      CloseBlob(image); \
      DestroyImages(image); \
    } \
  return((Image *) NULL); \
}
#define ThrowWriterException(code,reason,image) \
{ \
  assert(image != (Image *) NULL); \
  ThrowException(&(image)->exception,code,reason,(image)->filename); \
  if (image_info->adjoin) \
    while ((image)->previous != (Image *) NULL) \
      (image)=(image)->previous; \
  CloseBlob(image); \
  return(False); \
}

/*
  Typedef declarations.
*/
typedef struct _ExceptionInfo
{
  char
    *reason,
    *description;

  ExceptionType
    severity;

  unsigned long
    signature;
} ExceptionInfo;

/*
  Exception typedef declarations.
*/
typedef void
  (*ErrorHandler)(const ExceptionType,const char *,const char *);

typedef void
  (*FatalErrorHandler)(const ExceptionType,const char *,const char *);

typedef void
  (*WarningHandler)(const ExceptionType,const char *,const char *);

/*
  Exception declarations.
*/
extern MagickExport ErrorHandler
  SetErrorHandler(ErrorHandler);

extern MagickExport FatalErrorHandler
  SetFatalErrorHandler(FatalErrorHandler);

extern MagickExport void
  CatchException(const ExceptionInfo *),
  DestroyExceptionInfo(ExceptionInfo *),
  GetExceptionInfo(ExceptionInfo *),
  MagickError(const ExceptionType,const char *,const char *),
  MagickFatalError(const ExceptionType,const char *,const char *),
  MagickWarning(const ExceptionType,const char *,const char *),
  SetExceptionInfo(ExceptionInfo *,ExceptionType),
  ThrowException(ExceptionInfo *,const ExceptionType,const char *,const char *);

extern MagickExport WarningHandler
  SetWarningHandler(WarningHandler);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
