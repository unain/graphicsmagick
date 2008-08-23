/*
% Copyright (C) 2004, 2008 GraphicsMagick Group
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
% Interfaces to support simple iterative pixel read/update access within
% an image or between two images.
%
% Written by Bob Friesenhahn, March 2004, Updated for rows 2008.
%
*/

#include "magick/studio.h"
#include "magick/monitor.h"
#include "magick/pixel_cache.h"
#include "magick/pixel_iterator.h"
#include "magick/utility.h"

typedef struct _ThreadViewSet
{ 
  ViewInfo
    *views;

 unsigned int
    nviews;
} ThreadViewSet;

static void DestroyThreadViewSet(ThreadViewSet *view_set)
{
  unsigned int
    i;
  
  if (view_set != (ThreadViewSet *) NULL)
    {
      if (view_set->views != (ViewInfo *) NULL)
        {
          for (i=0; i < view_set->nviews; i++)
            {
              if (view_set->views[i] != (ViewInfo *) NULL)
                {
                  CloseCacheView(view_set->views[i]);
                  view_set->views[i]=(ViewInfo *) NULL;
                }
            }
        }
      view_set->nviews=0;
      MagickFreeMemory(view_set->views);
      MagickFreeMemory(view_set);
    }
}
static ThreadViewSet *AllocateThreadViewSet(Image *image,ExceptionInfo *exception)
{
  ThreadViewSet
    *view_set;
  
  unsigned int
    i;
  
  MagickPassFail
    status=MagickPass;
  
  view_set=MagickAllocateMemory(ThreadViewSet *,sizeof(ThreadViewSet));
  if (view_set == (ThreadViewSet *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
                      UnableToAllocateCacheView);
  view_set->nviews=1;
#if defined(_OPENMP)
  /*
    omp_get_max_threads() returns the # threads which will be used in team by default.
    omp_get_num_threads() returns the # of threads in current team (1 in main thread).
  */
  view_set->nviews=omp_get_max_threads();
#endif /* defined(_OPENMP) */
  
  view_set->views=MagickAllocateMemory(ViewInfo *,view_set->nviews*sizeof(ViewInfo *));
  if (view_set->views == (ViewInfo *) NULL)
    {
      ThrowException(exception,CacheError,UnableToAllocateCacheView,
                     image->filename);
      status=MagickFail;
    }

  if (view_set->views != (ViewInfo *) NULL)
    for (i=0; i < view_set->nviews; i++)
      {
        view_set->views[i]=OpenCacheView(image);
        if (view_set->views[i] == (ViewInfo *) NULL)
          {
            ThrowException(exception,CacheError,UnableToAllocateCacheView,
                           image->filename);
            status=MagickFail;
          }
      }
  
  if (status == MagickFail)
    {
      DestroyThreadViewSet(view_set);
      view_set=(ThreadViewSet *) NULL;
    }

  return view_set;
}
static ViewInfo *AccessThreadView(ThreadViewSet *view_set)
{
  ViewInfo
    *view;

  unsigned int
    thread_num=0;

#if defined(_OPENMP)
  thread_num=omp_get_thread_num();
#endif /* defined(_OPENMP) */
  assert(thread_num < view_set->nviews);
  view=view_set->views[thread_num];

  return view;
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n i t i a l i z e P i x e l I t e r a t o r O p t i o n s               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializePixelIteratorOptions() assigns default options to a user-provided
%  PixelIteratorOptions structure.  This function should always be used
%  to initialize the PixelIteratorOptions structure prior to making any
%  changes to it.
%
%  The format of the InitializePixelIteratorOptions method is:
%
%      void InitializePixelIteratorOptions(PixelIteratorOptions *options,
%                                          ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o options: pointer to PixelIteratorOptions structure to initialize.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport void
InitializePixelIteratorOptions(PixelIteratorOptions *options,
                               ExceptionInfo *exception)
{
  ARG_NOT_USED(exception);
  assert(options != (PixelIteratorOptions *) NULL);
  memset(options,0,sizeof(PixelIteratorOptions));
  options->signature=MagickSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e M o n o R e a d                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% PixelIterateMonoRead() iterates through a region of an image and invokes a
% user-provided callback function (of type PixelRowIteratorMonoReadCallback)
% for a row of pixels. This is useful to support simple operations such as
% statistics computation.
%
%  The format of the PixelIterateMonoRead method is:
%
%      MagickPassFail PixelIterateMonoRead(
%                                 PixelIteratorMonoReadCallback call_back,
%                                 const PixelIteratorOptions *options,
%                                 const char *description,
%                                 void *mutable_data,
%                                 const void *immutable_data,
%                                 const long x,
%                                 const long y,
%                                 const unsigned long columns,
%                                 const unsigned long rows,
%                                 const Image *image,
%                                 ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which is passed the
%       address of pixels from each image.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o x: The horizontal ordinate of the top left corner of the region.
%
%    o y: The vertical ordinate of the top left corner of the region.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o image: The address of the Image.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
MagickExport MagickPassFail
PixelIterateMonoRead(PixelIteratorMonoReadCallback call_back,
                     const PixelIteratorOptions *options,
                     const char *description,
                     void *mutable_data,
                     const void *immutable_data,
                     const long x,
                     const long y,
                     const unsigned long columns,
                     const unsigned long rows,
                     const Image *image,
                     ExceptionInfo *exception)
{

  MagickPassFail
    status = MagickPass;

  register long
    row;

  unsigned long
    row_count=0;

  ThreadViewSet
    *view_set;

  view_set=AllocateThreadViewSet((Image *) image,exception);
  if (view_set == (ThreadViewSet *) NULL)
    return MagickFail;

#pragma omp parallel for schedule(static,64)
  for (row=y; row < (long) (y+rows); row++)
    {
      MagickBool
        thread_status;

      const PixelPacket
        *pixels;

      const IndexPacket
        *indexes;

      ViewInfo
        *view;

      thread_status=status;
      if (thread_status == MagickFail)
        continue;

#pragma omp critical (pixel_cache)
      {
        view=AccessThreadView(view_set);
        pixels=AcquireCacheView(view,x, row, columns, 1, exception);
        if (!pixels)
          thread_status=MagickFail;
        indexes=GetCacheViewIndexes(view);
      }
      if (thread_status != MagickFail)
        thread_status=(call_back)(mutable_data,immutable_data,image,pixels,indexes,columns,exception);

#pragma omp critical
      {
        row_count++;
        if (QuantumTick(row_count,rows))
          if (!MagickMonitor(description,row_count,rows,exception))
            thread_status=MagickFail;

        if (thread_status == MagickFail)
          status=MagickFail;
      }
    }

  DestroyThreadViewSet(view_set);
 
  return (status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e M o n o M o d i f y                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIterateMonoModify() iterates through a region of an image and invokes
%  a user-provided callback function (of type PixelIteratorMonoReadCallback)
%  for a region of pixels. This is useful to support simple operations such as
%  level shifting, colorspace translation, or thresholding.
%
%  The format of the PixelIterateMonoModify method is:
%
%      MagickPassFail PixelIterateMonoModify(
%                              PixelIteratorMonoModifyCallback call_back,
%                              const PixelIteratorOptions *options,
%                              const char *description,
%                              void *mutable_data,
%                              const void *immutable_data,
%                              const long x,
%                              const long y,
%                              const unsigned long columns,
%                              const unsigned long rows,
%                              Image *image,
%                              ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which is passed the
%       address of pixels from each image.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o x: The horizontal ordinate of the top left corner of the region.
%
%    o y: The vertical ordinate of the top left corner of the region.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o image: The address of the Image.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
MagickExport MagickPassFail
PixelIterateMonoModify(PixelIteratorMonoModifyCallback call_back,
                       const PixelIteratorOptions *options,
                       const char *description,
                       void *mutable_data,
                       const void *immutable_data,
                       const long x,
                       const long y,
                       const unsigned long columns,
                       const unsigned long rows,
                       Image *image,
                       ExceptionInfo *exception)
{
  MagickPassFail
    status = MagickPass;

  register long
    row;

  unsigned long
    row_count=0;

  ThreadViewSet
    *view_set=(ThreadViewSet *) NULL;

  view_set=AllocateThreadViewSet(image,exception);
  if (view_set == (ThreadViewSet *) NULL)
    return MagickFail;

#pragma omp parallel for schedule(static,64)
  for (row=y; row < (long) (y+rows); row++)
    {
      MagickBool
        thread_status;

      PixelPacket
        *pixels;

      IndexPacket
        *indexes;

      ViewInfo
        *view;

      thread_status=status;
      if (thread_status == MagickFail)
        continue;

#pragma omp critical (pixel_cache)
      {
        view=AccessThreadView(view_set);
        pixels=GetCacheView(view, x, row, columns, 1);
        if (!pixels)
          thread_status=MagickFail;
        indexes=GetCacheViewIndexes(view);
      }
      if (thread_status != MagickFail)
        thread_status=(call_back)(mutable_data,immutable_data,image,pixels,indexes,columns,exception);

#pragma omp critical (pixel_cache)
      if (thread_status != MagickFail)
        if (!SyncCacheView(view))
          {
            thread_status=MagickFail;
            CopyException(exception,&image->exception);
          }

#pragma omp critical
      {
        row_count++;
        if (QuantumTick(row_count,rows))
          if (!MagickMonitor(description,row_count,rows,exception))
            thread_status=MagickFail;

        if (thread_status == MagickFail)
          status=MagickFail;
      }
    }

  DestroyThreadViewSet(view_set);

  return (status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e D u a l R e a d                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIterateDualRead() iterates through pixel regions of two images and
%  invokes a user-provided callback function (of type
%  PixelIteratorDualReadCallback) for each row of pixels. This is useful to
%  support operations such as image comparison.
%
%  The format of the PixelIterateDualModify method is:
%
%      MagickPassFail PixelIterateDualRead(
%                                PixelIteratorDualReadCallback call_back,
%                                const PixelIteratorOptions *options,
%                                const char *description,
%                                void *mutable_data,
%                                const void *immutable_data,
%                                const unsigned long columns,
%                                const unsigned long rows,
%                                const Image *first_image,
%                                const long first_x,
%                                const long first_y,
%                                const Image *second_image,
%                                const long second_x,
%                                const long second_y,
%                                ExceptionInfo *exception);
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which is passed the
%       address of pixels from each image.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o first_image: The address of the first Image.
%
%    o first_x: The horizontal ordinate of the top left corner of the first region.
%
%    o first_y: The vertical ordinate of the top left corner of the first region.
%
%    o second_image: The address of the second Image.
%
%    o second_x: The horizontal ordinate of the top left corner of the second region.
%
%    o second_y: The vertical ordinate of the top left corner of the second region.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
MagickExport MagickPassFail
PixelIterateDualRead(PixelIteratorDualReadCallback call_back,
                     const PixelIteratorOptions *options,
                     const char *description,
                     void *mutable_data,
                     const void *immutable_data,
                     const unsigned long columns,
                     const unsigned long rows,
                     const Image *first_image,
                     const long first_x,
                     const long first_y,
                     const Image *second_image,
                     const long second_x,
                     const long second_y,
                     ExceptionInfo *exception)
{
  MagickPassFail
    status = MagickPass;

  register long
    row;

  unsigned long
    row_count=0;

  ThreadViewSet
    *first_view_set,
    *second_view_set;

  first_view_set=AllocateThreadViewSet((Image *) first_image,exception);
  second_view_set=AllocateThreadViewSet((Image *) second_image,exception);
  if ((first_view_set == (ThreadViewSet *) NULL) ||
      (second_view_set == (ThreadViewSet *) NULL))
    {
      DestroyThreadViewSet(first_view_set);
      DestroyThreadViewSet(second_view_set);
      return MagickFail;
    }

#pragma omp parallel for schedule(static,64)
  for (row=0; row < (long) rows; row++)
    {
      MagickBool
        thread_status;

      long
        first_row,
        second_row;

      const PixelPacket
        *first_pixels,
        *second_pixels;

      const IndexPacket
        *first_indexes,
        *second_indexes;

      ViewInfo
        *first_view,
        *second_view;

      thread_status=status;
      if (thread_status == MagickFail)
        continue;

      first_row=first_y+row;
      second_row=second_y+row;
#pragma omp critical (pixel_cache)
      {
        first_view=AccessThreadView(first_view_set);
        first_pixels=AcquireCacheView(first_view, first_x, first_row, columns, 1, exception);
        if (!first_pixels)
          thread_status=MagickFail;
        first_indexes=GetCacheViewIndexes(first_view);

        second_view=AccessThreadView(second_view_set);
        second_pixels=AcquireCacheView(second_view, second_x, second_row, columns, 1, exception);
        if (!second_pixels)
          thread_status=MagickFail;
        second_indexes=GetCacheViewIndexes(second_view);
      }

      if (thread_status != MagickFail)
        thread_status=(call_back)(mutable_data,immutable_data,
                                  first_image,first_pixels,first_indexes,
                                  second_image,second_pixels,second_indexes,
                                  columns, exception);

#pragma omp critical
      {
        row_count++;
        if (QuantumTick(row_count,rows))
          if (!MagickMonitor(description,row_count,rows,exception))
            thread_status=MagickFail;

        if (thread_status == MagickFail)
          status=MagickFail;
      }
    }

  DestroyThreadViewSet(second_view_set);
  DestroyThreadViewSet(first_view_set);

  return (status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e D u a l M o d i f y                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIterateDualModify() iterates through pixel regions of two images and
%  invokes a user-provided callback function (of type
%  PixelIteratorDualModifyCallback) for each row of pixels. This is useful to
%  support operations such as composition.
%
%  The format of the PixelIterateDualModify method is:
%
%      MagickPassFail PixelIterateDualModify(
%                                PixelIteratorDualModifyCallback call_back,
%                                const PixelIteratorOptions *options,
%                                const char *description,
%                                void *mutable_data,
%                                const void *immutable_data,
%                                const unsigned long columns,
%                                const unsigned long rows,
%                                const Image *source_image,
%                                const long source_x,
%                                const long source_y,
%                                Image *update_image,
%                                const long update_x,
%                                const long update_y,
%                                ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which reads from
%      a region of source pixels and updates a region of destination pixels.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o source_image: The address of the constant source Image.
%
%    o source_x: The horizontal ordinate of the top left corner of the source region.
%
%    o source_y: The vertical ordinate of the top left corner of the source region.
%
%    o update_image: The address of the update Image.
%
%    o update_x: The horizontal ordinate of the top left corner of the update region.
%
%    o update_y: The vertical ordinate of the top left corner of the update region.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
static MagickPassFail
PixelIterateDualImplementation(PixelIteratorDualModifyCallback call_back,
                               const PixelIteratorOptions *options,
                               const char *description,
                               void *mutable_data,
                               const void *immutable_data,
                               const unsigned long columns,
                               const unsigned long rows,
                               const Image *source_image,
                               const long source_x,
                               const long source_y,
                               Image *update_image,
                               const long update_x,
                               const long update_y,
                               ExceptionInfo *exception,
                               MagickBool set)
{
  MagickPassFail
    status = MagickPass;

  register long
    row;

  unsigned long
    row_count=0;

  ThreadViewSet
    *source_view_set,
    *update_view_set;

  source_view_set=AllocateThreadViewSet((Image *) source_image,exception);
  update_view_set=AllocateThreadViewSet(update_image,exception);
  if ((source_view_set == (ThreadViewSet *) NULL) ||
      (update_view_set == (ThreadViewSet *) NULL))
    {
      DestroyThreadViewSet(source_view_set);
      DestroyThreadViewSet(update_view_set);
      return MagickFail;
    }

#pragma omp parallel for schedule(static,64)
  for (row=0; row < (long) rows; row++)
    {
      MagickBool
        thread_status;

      const PixelPacket
        *source_pixels;

      const IndexPacket
        *source_indexes;

      PixelPacket
        *update_pixels;

      IndexPacket
        *update_indexes;

      ViewInfo
        *source_view,
        *update_view;

      register long
        source_row,
        update_row;

      thread_status=status;
      if (thread_status == MagickFail)
        continue;

      source_row=source_y+row;
      update_row=update_y+row;

#pragma omp critical (pixel_cache)
      {
        source_view=AccessThreadView(source_view_set);
        source_pixels=AcquireCacheView(source_view, source_x, source_row,
                                       columns, 1, exception);
        if (!source_pixels)
          thread_status=MagickFail;
        source_indexes=GetCacheViewIndexes(source_view);

        update_view=AccessThreadView(update_view_set);
        if (set)
          update_pixels=SetCacheView(update_view, update_x, update_row, columns, 1);
        else
          update_pixels=GetCacheView(update_view, update_x, update_row, columns, 1);
        if (!update_pixels)
          {
            CopyException(exception,&update_image->exception);
            thread_status=MagickFail;
          }
        update_indexes=GetCacheViewIndexes(update_view);
      }

      if (thread_status != MagickFail)
        thread_status=(call_back)(mutable_data,immutable_data,
                                  source_image,source_pixels,source_indexes,
                                  update_image,update_pixels,update_indexes,
                                  columns,exception);

#pragma omp critical (pixel_cache)
      if (thread_status != MagickFail)
        if (!SyncCacheView(update_view))
          {
            thread_status=MagickFail;
            CopyException(exception,&update_image->exception);
          }

#pragma omp critical
      {
        row_count++;
        if (QuantumTick(row_count,rows))
          if (!MagickMonitor(description,row_count,rows,exception))
            thread_status=MagickFail;

        if (thread_status == MagickFail)
          status=MagickFail;
      }
    }

  DestroyThreadViewSet(update_view_set);
  DestroyThreadViewSet(source_view_set);

  return (status);
}

MagickExport MagickPassFail
PixelIterateDualModify(PixelIteratorDualModifyCallback call_back,
                       const PixelIteratorOptions *options,
                       const char *description,
                       void *mutable_data,
                       const void *immutable_data,
                       const unsigned long columns,
                       const unsigned long rows,
                       const Image *source_image,
                       const long source_x,
                       const long source_y,
                       Image *update_image,
                       const long update_x,
                       const long update_y,
                       ExceptionInfo *exception)
{
  return PixelIterateDualImplementation
    (call_back,options,description,mutable_data,immutable_data,columns,rows,
     source_image,source_x,source_y,update_image,update_x,update_y,exception,
     MagickFalse);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e D u a l N e w                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIterateDualNew() iterates through pixel regions of two images and
%  invokes a user-provided callback function (of type
%  PixelIteratorDualNewCallback) for each row of pixels. This is used if a
%  new output image is created based on an input image.  The difference from
%  PixelIterateDualModify() is that the output pixels are not initialized so
%  it is more efficient when outputting a new image.
%
%  The format of the PixelIterateDualNew method is:
%
%      MagickPassFail PixelIterateDualNew(
%                                PixelIteratorDualNewCallback call_back,
%                                const PixelIteratorOptions *options,
%                                const char *description,
%                                void *mutable_data,
%                                const void *immutable_data,
%                                const unsigned long columns,
%                                const unsigned long rows,
%                                const Image *source_image,
%                                const long source_x,
%                                const long source_y,
%                                Image *new_image,
%                                const long new_x,
%                                const long new_y,
%                                ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which reads from
%      a region of source pixels and initializes a region of destination pixels.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o source_image: The address of the constant source Image.
%
%    o source_x: The horizontal ordinate of the top left corner of the source region.
%
%    o source_y: The vertical ordinate of the top left corner of the source region.
%
%    o new_image: The address of the new Image.
%
%    o new_x: The horizontal ordinate of the top left corner of the new region.
%
%    o new_y: The vertical ordinate of the top left corner of the new region.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
MagickExport MagickPassFail
PixelIterateDualNew(PixelIteratorDualNewCallback call_back,
                    const PixelIteratorOptions *options,
                    const char *description,
                    void *mutable_data,
                    const void *immutable_data,
                    const unsigned long columns,
                    const unsigned long rows,
                    const Image *source_image,
                    const long source_x,
                    const long source_y,
                    Image *new_image,
                    const long new_x,
                    const long new_y,
                    ExceptionInfo *exception)
{
  return PixelIterateDualImplementation
    (call_back,options,description,mutable_data,immutable_data,columns,rows,
     source_image,source_x,source_y,new_image,new_x,new_y,exception,
     MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e T r i p l e M o d i f y                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIterateTripleModify() iterates through pixel regions of three images
%  and invokes a user-provided callback function (of type
%  PixelIteratorTripleModifyCallback) for each row of pixels.  The first two
%  images are read-only, while the third image is read-write for update.
%  Access of the first two images is done lock-step using the same coordinates.
%  This is useful to support operations such as image differencing.
%
%  The format of the PixelIterateTripleModify method is:
%
%      MagickPassFail PixelIterateTripleModify(
%                                PixelIteratorTripleModifyCallback call_back,
%                                const PixelIteratorOptions *options,
%                                const char *description,
%                                void *mutable_data,
%                                const void *immutable_data,
%                                const unsigned long columns,
%                                const unsigned long rows,
%                                const Image *source1_image,
%                                const Image *source2_image,
%                                const long source_x,
%                                const long source_y,
%                                Image *update_image,
%                                const long update_x,
%                                const long update_y,
%                                ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which reads from
%      a region of source pixels and updates a region of destination pixels.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o source1_image: The address of the constant source 1 Image.
%
%    o source2_image: The address of the constant source 2 Image.
%
%    o source_x: The horizontal ordinate of the top left corner of the source regions.
%
%    o source_y: The vertical ordinate of the top left corner of the source regions.
%
%    o update_image: The address of the update Image.
%
%    o update_x: The horizontal ordinate of the top left corner of the update region.
%
%    o update_y: The vertical ordinate of the top left corner of the update region.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
static MagickPassFail
PixelIterateTripleImplementation(PixelIteratorTripleModifyCallback call_back,
                                 const PixelIteratorOptions *options,
                                 const char *description,
                                 void *mutable_data,
                                 const void *immutable_data,
                                 const unsigned long columns,
                                 const unsigned long rows,
                                 const Image *source1_image,
                                 const Image *source2_image,
                                 const long source_x,
                                 const long source_y,
                                 Image *update_image,
                                 const long update_x,
                                 const long update_y,
                                 ExceptionInfo *exception,
                                 MagickBool set)
{
  MagickPassFail
    status = MagickPass;

  register long
    row;

  unsigned long
    row_count=0;

  ThreadViewSet
    *source1_view_set,
    *source2_view_set,
    *update_view_set;

  source1_view_set=AllocateThreadViewSet((Image *) source1_image,exception);
  source2_view_set=AllocateThreadViewSet((Image *) source2_image,exception);
  update_view_set=AllocateThreadViewSet(update_image,exception);
  if ((source1_view_set == (ThreadViewSet *) NULL) ||
      (source2_view_set == (ThreadViewSet *) NULL) ||
      (update_view_set == (ThreadViewSet *) NULL))
    {
      DestroyThreadViewSet(source1_view_set);
      DestroyThreadViewSet(source2_view_set);
      DestroyThreadViewSet(update_view_set);
      return MagickFail;
    }

#pragma omp parallel for schedule(static,64)
  for (row=0; row < (long) rows; row++)
    {
      MagickBool
        thread_status;

      const PixelPacket
        *source1_pixels,
        *source2_pixels;

      const IndexPacket
        *source1_indexes,
        *source2_indexes;

      PixelPacket
        *update_pixels;

      IndexPacket
        *update_indexes;

      ViewInfo
        *source1_view,
        *source2_view,
        *update_view;

      long
        source_row,
        update_row;

      thread_status=status;
      if (thread_status == MagickFail)
        continue;

      source_row=source_y+row;
      update_row=update_y+row;

#pragma omp critical (pixel_cache)
      {
        /*
          First image (read only).
        */
        source1_view=AccessThreadView(source1_view_set);
        source1_pixels=AcquireCacheView(source1_view, source_x, source_row,
                                        columns, 1, exception);
        if (!source1_pixels)
          thread_status=MagickFail;
        source1_indexes=GetCacheViewIndexes(source1_view);

        /*
          Second image (read only).
        */
        source2_view=AccessThreadView(source2_view_set);
        source2_pixels=AcquireCacheView(source2_view, source_x, source_row,
                                        columns, 1, exception);
        if (!source2_pixels)
          thread_status=MagickFail;
        source2_indexes=GetCacheViewIndexes(source2_view);

        /*
          Third image (read/write).
        */
        update_view=AccessThreadView(update_view_set);
        if (set)
          update_pixels=SetCacheView(update_view, update_x, update_row, columns, 1);
        else
          update_pixels=GetCacheView(update_view, update_x, update_row, columns, 1);
        if (!update_pixels)
          {
            CopyException(exception,&update_image->exception);
            thread_status=MagickFail;
          }
        update_indexes=GetCacheViewIndexes(update_view);
      }

      if (thread_status != MagickFail)
        status=(call_back)(mutable_data,immutable_data,
                           source1_image,source1_pixels,source1_indexes,
                           source2_image,source2_pixels,source2_indexes,
                           update_image,update_pixels,update_indexes,
                           columns,exception);

#pragma omp critical (pixel_cache)
      if (!SyncCacheView(update_view))
        {
          if (thread_status != MagickFail)
            {
              thread_status=MagickFail;
              CopyException(exception,&update_image->exception);
            }
        }

#pragma omp critical
      {
        row_count++;
        if (QuantumTick(row_count,rows))
          if (!MagickMonitor(description,row_count,rows,exception))
            thread_status=MagickFail;

        if (thread_status == MagickFail)
          status=MagickFail;
      }
    }

  DestroyThreadViewSet(source1_view_set);
  DestroyThreadViewSet(source2_view_set);
  DestroyThreadViewSet(update_view_set);

  return (status);
}

MagickExport MagickPassFail
PixelIterateTripleModify(PixelIteratorTripleModifyCallback call_back,
                         const PixelIteratorOptions *options,
                         const char *description,
                         void *mutable_data,
                         const void *immutable_data,
                         const unsigned long columns,
                         const unsigned long rows,
                         const Image *source1_image,
                         const Image *source2_image,
                         const long source_x,
                         const long source_y,
                         Image *update_image,
                         const long update_x,
                         const long update_y,
                         ExceptionInfo *exception)
{
  return PixelIterateTripleImplementation
    (call_back,options,description,mutable_data,immutable_data,columns,rows,
     source1_image,source2_image,source_x,source_y,
     update_image,update_x,update_y,
     exception,MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l I t e r a t e D u a l N e w                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelIterateTripleNew() iterates through pixel regions of three images
%  and invokes a user-provided callback function (of type
%  PixelIteratorTripleNewCallback) for each row of pixels. The first two
%  images are read-only, while the third image is read-write for update.
%  Access of the first two images is done lock-step using the same coordinates.
%  This is used if a new output image is created based on two input images.
%  The difference from PixelIterateTripleModify() is that the output pixels
%  are not initialized so it is more efficient when outputting a new image.
%
%  The format of the PixelIterateTripleNew method is:
%
%      MagickPassFail PixelIterateTripleNew(
%                                PixelIteratorTripleNewCallback call_back,
%                                 const PixelIteratorOptions *options,
%                                const char *description,
%                                void *mutable_data,
%                                const void *immutable_data,
%                                const unsigned long columns,
%                                const unsigned long rows,
%                                const Image *source1_image,
%                                const Image *source2_image,
%                                const long source_x,
%                                const long source_y,
%                                Image *new_image,
%                                const long new_x,
%                                const long new_y,
%                                ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o call_back: A user-provided C callback function which reads from
%      a region of source pixels and initializes a region of destination pixels.
%
%    o options: Pixel iterator execution options (may be NULL).
%
%    o description: textual description of operation being performed.
%
%    o mutable_data: User-provided mutable context data.
%
%    o immutable_data: User-provided immutable context data.
%
%    o columns: Width of pixel region
%
%    o rows: Height of pixel region
%
%    o source1_image: The address of the constant source 1 Image.
%
%    o source2_image: The address of the constant source 2 Image.
%
%    o source_x: The horizontal ordinate of the top left corner of the source regions.
%
%    o source_y: The vertical ordinate of the top left corner of the source regions.
%
%    o new_image: The address of the new Image.
%
%    o new_x: The horizontal ordinate of the top left corner of the new region.
%
%    o new_y: The vertical ordinate of the top left corner of the new region.
%
%    o exception: If an error is reported, this argument is updated with the reason.
%
*/
MagickExport MagickPassFail
PixelIterateTripleNew(PixelIteratorTripleNewCallback call_back,
                      const PixelIteratorOptions *options,
                      const char *description,
                      void *mutable_data,
                      const void *immutable_data,
                      const unsigned long columns,
                      const unsigned long rows,
                      const Image *source1_image,
                      const Image *source2_image,
                      const long source_x,
                      const long source_y,
                      Image *new_image,
                      const long new_x,
                      const long new_y,
                      ExceptionInfo *exception)
{
  return PixelIterateTripleImplementation
    (call_back,options,description,mutable_data,immutable_data,columns,rows,
     source1_image,source2_image,source_x,source_y,
     new_image,new_x,new_y,
     exception,MagickTrue);
}
