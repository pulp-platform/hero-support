/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                   		Imaging Division
 *------------------------------------------------------------------------------
 * $Id: image.c.rca 1.69 Tue Aug 14 14:03:02 2012 stephane.drouard@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: image.c.rca $
 * 
 *  Revision: 1.69 Tue Aug 14 14:03:02 2012 stephane.drouard@st.com
 *  Fixed crash on exception stack unwinding when the linked image is a local Python image.
 *
 *  Revision: 1.68 Wed Nov 24 13:57:18 2010 stephane.drouard@st.com
 *  Added IMG_LINK_CONST for IMG_CreateLinkToNathair().
 *
 *  Revision: 1.67 Thu Sep 23 14:25:47 2010 stephane.drouard@st.com
 *  IMG_image_t/CFGFILE_file_t: removed warning in destructor not to issue a warning in case of stack unwiding.
 *
 *  Revision: 1.66 Mon May 17 17:44:20 2010 stephane.drouard@st.com
 *  When compiled in C++, issue a warning when IMG_image_t or CFGFILE_file_t instance has not been freed (then free it).
 *
 *  Revision: 1.65 Tue May 12 18:06:49 2009 stephane.drouard@st.com
 *  IMG_Shift(): fixed right-shift rounding.
 *
 *  Revision: 1.64 Mon Apr 27 16:42:04 2009 sebastien.cleyet-merle@st.com
 *  Fix a warning appearing with g++
 *
 *  Revision: 1.63 Thu Mar  5 17:53:01 2009 stephane.drouard@st.com
 *  Fixed compile issue.
 *
 *  Revision: 1.62 Thu Mar  5 15:34:06 2009 stephane.drouard@st.com
 *  Reverted back previous modifications (GetConstData() and GetData() behave the same way).
 *  Added image address to lock ID.
 *
 *  Revision: 1.61 Mon Mar  2 17:00:26 2009 stephane.drouard@st.com
 *  Added linkType to CreateLinkToNathairImage in order to select the correct lock mechanism.
 *
 *  Revision: 1.60 Mon Feb 16 15:20:37 2009 stephane.drouard@st.com
 *  IMG_SortPixels now uses STL sort() when compiled in C++.
 *
 *  Revision: 1.59 Wed Jan 21 18:46:19 2009 stephane.drouard@st.com
 *  IMG_Clip/IMG_Mask: msg can now be inconditionally NULL.
 *
 *  Revision: 1.58 Thu Nov 20 11:16:28 2008 stephane.drouard@st.com
 *  IMG_CreateLinkToNathairImage(): uses the module name as lock ID.
 *  IMG_Free() / Nathair links: added a check that the Nathair image has not been modified while used by the module.
 *  IMG_SetBayerJogs(), IMG_Shift(), IMG_Crop(): now managed on links to Nathair images.
 *  IMG_Copy() / Nathair links: fixed Bayer jogs update.
 *
 *  Revision: 1.57 Thu Oct 16 11:56:22 2008 stephane.drouard@st.com
 *  IMG_Clip(), IMG_Mask(): moved summary message from DEBUG to INFO.
 *
 *  Revision: 1.56 Thu Oct 16 10:48:44 2008 stephane.drouard@st.com
 *  Replaced VERBOSE() by WARNING()/INFO()/DEBUG() (need to delete trailing "\n").
 *  GetAppName() now only available in standalone.
 *  SetAppName() renamed in SetModuleName() (Nathair only).
 *  error.c removed and merged with utils.c (need to clean before 1st rebuild).
 *  Reworked on the documentation.
 *
 *  Revision: 1.55 Fri Oct  3 10:42:05 2008 stephane.drouard@st.com
 *  IMG_Shift(): fixed right-shift round option with clipped values.
 *  IMG_Save(): now checks the image before creating the file.
 *
 *  Revision: 1.54 Tue Sep 30 16:35:23 2008 stephane.drouard@st.com
 *  Added YCbCr support.
 *  Fixed IMG_Check(), IMG_Clip() and IMG_Mask() (when bits == 2 or bits == 16).
 *
 *  Revision: 1.52 Mon Sep 29 17:36:50 2008 stephane.drouard@st.com
 *  IMG_Shift: added round option for right shift (IMG_SHIFT_ROUND).
 *
 *  Revision: 1.51 Fri Sep 19 16:56:21 2008 olivier.lemarchand@st.com
 *  just for trial
 *
 *  Revision: 1.50 Fri Sep 19 16:39:33 2008 olivier.lemarchand@st.com
 *  trialererer
 *
 *  Revision: 1.49 Wed Sep  3 16:00:48 2008 stephane.drouard@st.com
 *  IMG_Crop: replaced memcpy by memmove to get Valgrind happy.
 *
 *  Revision: 1.48 Mon Sep  1 14:24:43 2008 stephane.drouard@st.com
 *  Removed IMG_MAX_WIDTH/HEIGHT.
 *
 *  Revision: 1.47 Mon May 26 12:01:33 2008 stephane.drouard@st.com
 *  Accepted Nathair's YCbCr image type (as IMG_TYPE_RGB).
 *
 *  Revision: 1.46 Wed Mar  5 17:35:51 2008 stephane.drouard@st.com
 *  Fixed some Nathair functions not documented.
 *
 *  Revision: 1.45 Thu Feb 21 14:43:53 2008 stephane.drouard@st.com
 *  Fixed Nathair monochrome image (avoid getting jogs).
 *
 *  Revision: 1.44 Wed Feb 20 11:36:50 2008 stephane.drouard@st.com
 *  Nathair image link: check the data buffer is not changed.
 *
 *  Revision: 1.43 Thu Feb  7 17:35:52 2008 stephane.drouard@st.com
 *  Added Nathair support.
 *
 *  Revision: 1.42 Tue Jan 29 17:46:32 2008 stephane.drouard@st.com
 *  IMG_Clip() / IMG_Mask(): msg may be NULL if maxMsg is 0.
 *
 *  Revision: 1.41 Mon Jan 21 17:52:43 2008 stephane.drouard@st.com
 *  Replaced filename != "-" by fp != stdin/stdout.
 *
 *  Revision: 1.40 Wed Jan 16 14:43:51 2008 stephane.drouard@st.com
 *  Fixed IMG_Mask().
 *
 *  Revision: 1.39 Tue Jan  8 10:56:22 2008 stephane.drouard@st.com
 *  Added IMG_Mask().
 *  Minor documentation updates.
 *
 *  Revision: 1.38 Fri Dec 21 17:24:06 2007 stephane.drouard@st.com
 *  Added IMG_Clip().
 *
 *  Revision: 1.37 Mon Oct 22 19:14:21 2007 stephane.drouard@st.com
 *  Added support of empty images.
 *
 *  Revision: 1.36 Fri Sep 21 14:34:47 2007 stephane.drouard@st.com
 *  Renamed IMGSHIFT_COPY_MS_BITS as IMG_SHIFT_COPY_MS_BITS.
 *
 *  Revision: 1.35 Thu Aug 23 10:27:50 2007 olivier.lemarchand@st.com
 *  Transfered IMGSHIFT_COPY_MS_BITS into raid.h
 *
 *  Revision: 1.34 Thu Aug 23 10:19:13 2007 olivier.lemarchand@st.com
 *  Went back to default behaviour of IMG_shift() to ensure backward
 *  compatibility. It is possible to copy MS bits into LS bits when left shifting
 *  by using the IMGSHIFT_COPY_MS_BITS macro to be OR'ed with the bits
 *  value
 *
 *  Revision: 1.33 Thu Jul 19 11:05:25 2007 stephane.drouard@st.com
 *  Fixed warning under VisualC++.
 *
 *  Revision: 1.32 Thu Jul 19 09:52:38 2007 stephane.drouard@st.com
 *  IMG_Shift(): MS bits are copied into LS bits for left shifts.
 *
 *  Revision: 1.31 Fri Jun  1 11:21:45 2007 stephane.drouard@st.com
 *  Added IMG_HJOG(), IMG_VJOG(), IMG_BAYER_COLOR(), IMG_Duplicate() and IMG_GetBayerColorPosition().
 *  IMG_Crop(), IMG_Copy(), IMG_CopyBorder() now report the jogs to the destination image.
 *
 *  Revision: 1.30 Thu May 10 16:13:25 2007 stephane.drouard@st.com
 *  Fixed SoftIP image link support (bits per pixel is 24 for RGB images, not 8 as for RAID library).
 *
 *  Revision: 1.29 Fri Apr 27 09:46:24 2007 stephane.drouard@st.com
 *  IMG_Check(): updated error message (coordinate, value).
 *  Updated comments.
 *
 *  Revision: 1.28 Wed Apr 18 10:17:29 2007 stephane.drouard@st.com
 *  Added module parameter to TRY_RAID_LIBRARY().
 *
 *  Revision: 1.27 Fri Apr 13 17:23:08 2007 stephane.drouard@st.com
 *  Added SoftIP environment support.
 *  Updated comments.
 *  Changed IMG_Create(): now lets the buffer uninitialized (performance, not backward compatible).
 *  Changed IMG_Copy(): now supports overlapping regions.
 *
 *  Revision: 1.26 Wed Apr  4 16:31:06 2007 stephane.drouard@st.com
 *  Adapted to new RAID structure.
 *
 *  Revision: 1.25 Tue Apr  3 12:04:59 2007 stephane.drouard@st.com
 *  Added support of IMG_COLOR_2G (merge of 2 Bayer greens) for IMG_ExtractPlane().
 *
 *  Revision: 1.24 Fri Mar 23 17:16:55 2007 stephane.drouard@st.com
 *  Adjusted jog order (h then v).
 *
 *  Revision: 1.23 Fri Mar 23 16:43:12 2007 stephane.drouard@st.com
 *  Added IMG_Copy() and IMG_CopyBorder().
 *
 *  Revision: 1.21 Thu Mar 22 17:55:49 2007 stephane.drouard@st.com
 *  Added IMG_Crop().
 *
 *  Revision: 1.20 Tue Mar 20 18:12:13 2007 stephane.drouard@st.com
 *  Added IMG_ReplacePlane().
 *
 *  Revision: 1.19 Tue Mar 20 11:08:56 2007 stephane.drouard@st.com
 *  Added Bayer jogs support into an image.
 *
 *  Revision: 1.18 Mon Mar 19 18:06:07 2007 stephane.drouard@st.com
 *  Modified IMG_Create to pass image attributes.
 *
 *  Revision: 1.17 Mon Feb 26 17:23:07 2007 stephane.drouard@st.com
 *  Suppressed a warning.
 *
 *  Revision: 1.16 Wed Feb 21 15:53:20 2007 stephane.drouard@st.com
 *  Now checks data before saving them (IMG_Save()).
 *
 *  Revision: 1.15 Wed Feb 21 15:35:04 2007 stephane.drouard@st.com
 *  Added IMG_TYPE_LITTLE_ENDIAN for IMG_Load().
 *
 *  Revision: 1.14 Fri Feb  9 12:31:56 2007 stephane.drouard@st.com
 *  Shortened color names.
 *
 *  Revision: 1.13 Fri Feb  9 11:49:03 2007 stephane.drouard@st.com
 *  Added IMG_ExtractColorPlane() and IMG_ExtractBayerPlane().
 *
 *  Revision: 1.12 Wed Jan 31 18:56:43 2007 stephane.drouard@st.com
 *  Modified VERBOSE macro: "\n" is no more automatically appended.
 *
 *  Revision: 1.11 Tue Nov 28 18:43:25 2006 stephane.drouard@st.com
 *  Updated comments.
 *  Updated error messages.
 *
 *  Revision: 1.10 Tue Nov 28 15:34:52 2006 stephane.drouard@st.com
 *  Added VERBOSE() macro.
 *
 *  Revision: 1.9 Fri Nov 17 15:00:41 2006 stephane.drouard@st.com
 *  Removed unnecessary included files.
 *
 *  Revision: 1.8 Fri Nov 10 16:33:11 2006 stephane.drouard@st.com
 *  Added Linux support.
 *
 *  Revision: 1.7 Wed Oct 25 18:08:36 2006 stephane.drouard@st.com
 *  Removed bool_t, FALSE and TRUE, replaced by the C99's standard one (bool, false and true).
 *  Added CHECK() and SYSTEM_CHECK() macros.
 *  Rewrote CMDLINE macros.
 *  Added IMG_MAX_WIDTH/HEIGHT.
 *
 *  Revision: 1.6 Wed Sep 27 09:39:04 2006 stephane.drouard@st.com
 *  Fixed binary image load (first pixel data skipped when values correspond to spaces).
 *
 *  Revision: 1.5 Mon Sep 11 11:46:53 2006 stephane.drouard@st.com
 *  Removed IMG_Compare() and IMG_Crop() (implemented into their respective tools).
 *
 *  Revision: 1.4 Mon Aug 28 16:06:10 2006 stephane.drouard@st.com
 *  Renamed CMDLINE_OPTION_PARAM as CMDLINE_OPTION_STRING
 *  Renamed CMDLINE_OPTION_PARAM_SCAN as CMDLINE_OPTION_VALUE
 *  Renamed CMDLINE_PARAM as CMDLINE_PARAM_STRING
 *  Renamed CMDLINE_PARAM_SCAN as CMDLINE_PARAM_VALUE
 *  Added IMG_FORMAT_DEFAULT
 *
 *  Revision: 1.3 Wed Aug 23 15:22:55 2006 stephane.drouard@st.com
 *  Code cleanup
 *
 *  Revision: 1.2 Tue Aug 22 11:21:49 2006 olivier.lemarchand@st.com
 *  Added Synchronicity header.
 ******************************************************************************/
#include "raid.h"

/**
 * @addtogroup IMG
 * @{
 */

/*
 * Assert image attributes.
 */
#if defined(SOFTIP)
  #define _SIGNATURE  0xDEADBEEF

  #define ASSERT_IMAGE(image) \
    assert(image != NULL); \
    assert(image->type == IMG_TYPE_GREY || image->type == IMG_TYPE_RGB \
      || image->type == IMG_TYPE_YCBCR); \
    assert(image->height >= 0); \
    assert(image->width >= 0); \
    assert(image->bits > 0 && image->bits <= 16); \
    assert(image->hJog == false || image->hJog == true); \
    assert(image->vJog == false || image->vJog == true); \
    assert(image->data != NULL); \
    assert(image->signature == _SIGNATURE); \
    assert(image->linkType >= IMG_LINK_IN && image->linkType <= IMG_LINK_OUT)
#else
  #define ASSERT_IMAGE(image) \
    assert(image != NULL); \
    assert(image->type == IMG_TYPE_GREY || image->type == IMG_TYPE_RGB \
      || image->type == IMG_TYPE_YCBCR); \
    assert(image->height >= 0); \
    assert(image->width >= 0); \
    assert(image->bits > 0 && image->bits <= 16); \
    assert(image->hJog == false || image->hJog == true); \
    assert(image->vJog == false || image->vJog == true); \
    assert(image->data != NULL)
#endif

/*
 * PGM / PPM file type signatures.
 */
#define FTYPE_PGM_ASCII   2
#define FTYPE_PGM_BINARY  5
#define FTYPE_PPM_ASCII   3
#define FTYPE_PPM_BINARY  6

/*
 * Color jog translation used by IMG_BAYER_COLOR().
 */
const char __IMG_bayerColors[2][2] = {
  { IMG_COLOR_GIR, IMG_COLOR_R },
  { IMG_COLOR_B, IMG_COLOR_GIB }
};

/*
 * Type strings.
 */
static const char *__IMG_types[] = {
  "Any",
  "Bayer",
  NULL,
  "RGB",
  NULL,
  NULL,
  NULL,
  "YCbCr"
};

#if defined(__cplusplus)
/*
 * Constructor.
 */
_IMG_image_t::_IMG_image_t()
{
  data = NULL;
}

/*
 * Destructor.
 */
_IMG_image_t::~_IMG_image_t()
{
  if (data != NULL)
    IMG_Free(this);
}
#endif

/*
 * Verifies an image could be Bayer.
 */
bool IMG_SeemsBayer(const IMG_image_t *image)
{
  #if defined(NATHAIR)
  if (image->nathairImage != NULL)
    return image->nathairImage->GetDataType() == Image::Bayer;
  #endif
  return image->type == IMG_TYPE_GREY
      && (image->width & 1) == 0 && (image->height & 1) == 0;
}

/*
 * Checks an image is Bayer.
 */
#define CHECK_BAYER(image)  CHECK(IMG_SeemsBayer(image), "Not a Bayer image")

/**
 * Computes the minimum number of bits fitting a value.
 *
 * @param maxValue      Value [0,65535].
 * @return              Number of bits [1,16].
 */
int IMG_GetBits(int maxValue)
{
  int i;

  assert(maxValue >= 0 && maxValue <= 65535);

  for (i = 1; maxValue >= (1 << i); i++) {}
  return i;
}

/**
 * Creates an image.
 *
 * @param image         [out] @ref IMG_image_t "Image".
 * @param width         Image width in pixels.
 * @param height        Image height in pixels.
 * @param type          @ref IMG_TYPE "Image type".
 * @param bits          Number of bits per color channel [1,16].
 *
 * @note Bayer jobs are cleared, call IMG_SetBayerJogs() to possibly adjust
 *       them.
 * @note The data buffer is allocated but not initialized.
 * @note Call IMG_Free() after use of the image.
 */
void IMG_Create(IMG_image_t *image, int width, int height, int type, int bits)
{
  assert(image != NULL);
  assert(width >= 0);
  assert(height >= 0);
  assert(type == IMG_TYPE_GREY || type == IMG_TYPE_RGB
    || type == IMG_TYPE_YCBCR);
  assert(bits > 0 && bits <= 16);

  #if defined(__cplusplus)
  if (image->data != NULL) {
    WARNING("RAID image not explicitly freed");
    IMG_Free(image);
  }
  #endif

  memset(image, 0, sizeof(IMG_image_t));
  image->width = width;
  image->height = height;
  image->type = type;
  image->bits = bits;
  image->data = (IMG_data_t*)malloc(
    MAX(IMG_PIXELS(*image), 1) * IMG_CHANNELS(*image) * sizeof(IMG_data_t));
  SYSTEM_CHECK(image->data != NULL, NULL);

  #if defined(SOFTIP)
  image->signature = _SIGNATURE;
  #endif

  ASSERT_IMAGE(image);
}

#if defined(SOFTIP) || defined(DOXYGEN)

/**
 * Creates an image as a link to a SoftIP image.
 *
 * @par Specific to the SoftIP environment.
 *
 * @param image         [out] @ref IMG_image_t "Image".
 * @param softIpImage   SoftIP image to link to.
 * @param linkType      @ref IMG_LINK_SOFTIP "Link type".
 *
 * @note Operations which change the image structure are not allowed on links,
 *       as they would also need to change the linked SoftIP image, which is
 *       not currently supported (IMG_Shift(), IMG_Crop()).
 * @note SoftIP images do not embed Bayer jog information. Bayer jobs are
 *       cleared, call IMG_SetBayerJogs() to adjust them.
 * @note Call IMG_Free() after use of the image.
 *
 * See TRY_RAID_LIBRARY() for an example.
 */
void IMG_CreateLinkToSoftIpImage(IMG_image_t *image, const TVImage *softIpImage,
  int linkType)
{
  int i, size;
  IMG_data_t *data;
  unsigned char *bdata;
  unsigned short *wdata;

  assert(image != NULL);
  assert(softIpImage != NULL);
  assert(linkType >= IMG_LINK_IN && linkType <= IMG_LINK_OUT);

  switch (softIpImage->mType) {
  case VIMAGE_GREY:
  case VIMAGE_BAYER:
    if (softIpImage->mBitsPerPixel <= 8) {
      IMG_Create(image, softIpImage->mWidth, softIpImage->mHeight,
        IMG_TYPE_GREY, softIpImage->mBitsPerPixel);

      if (linkType != IMG_LINK_OUT) {
        size = IMG_PIXELS(*image);
        data = image->data;
        bdata = (unsigned char*)softIpImage->mpBuffer;
        for (i = 0; i < size; i++)
          *data++ = *bdata++;
      }
    }
    else {
      memset(image, 0, sizeof(IMG_image_t));
      image->width = softIpImage->mWidth;
      image->height = softIpImage->mHeight;
      image->type = IMG_TYPE_GREY;
      image->bits = softIpImage->mBitsPerPixel;
      image->data = (IMG_data_t*)softIpImage->mpBuffer;
    }
    break;

  case VIMAGE_RGB:
    assert((softIpImage->mBitsPerPixel % 3) == 0);

    if (softIpImage->mBitsPerPixel <= 24) {
      IMG_Create(image, softIpImage->mWidth, softIpImage->mHeight,
        IMG_TYPE_RGB, softIpImage->mBitsPerPixel / 3);

      if (linkType != IMG_LINK_OUT) {
        size = IMG_PIXELS(*image) * 3;
        data = image->data;
        bdata = (unsigned char*)softIpImage->mpBuffer;
        for (i = 0; i < size; i++)
          *data++ = *bdata++;
      }
    }
    else {
      memset(image, 0, sizeof(IMG_image_t));
      image->width = softIpImage->mWidth;
      image->height = softIpImage->mHeight;
      image->type = IMG_TYPE_RGB;
      image->bits = softIpImage->mBitsPerPixel / 3;
      image->data = (IMG_data_t*)softIpImage->mpBuffer;
    }
    break;

  case VIMAGE_BGR:
    assert((softIpImage->mBitsPerPixel % 3) == 0);

    IMG_Create(image, softIpImage->mWidth, softIpImage->mHeight,
      IMG_TYPE_RGB, softIpImage->mBitsPerPixel / 3);

    size = IMG_PIXELS(*image);
    data = image->data;

    if (linkType != IMG_LINK_OUT) {
      if (softIpImage->mBitsPerPixel <= 24) {
        bdata = (unsigned char*)softIpImage->mpBuffer;
        for (i = 0; i < size; i++) {
          *data++ = bdata[2];
          *data++ = bdata[1];
          *data++ = bdata[0];
          bdata += 3;
        }
      }
      else {
        wdata = (unsigned short*)softIpImage->mpBuffer;
        for (i = 0; i < size; i++) {
          *data++ = wdata[2];
          *data++ = wdata[1];
          *data++ = wdata[0];
          wdata += 3;
        }
      }
    }
    break;

  default:
    CHECK(false, "Unsupported SoftIP image type");
  }

  image->signature = _SIGNATURE;
  image->softIpImage = softIpImage;
  image->linkType = linkType;

  ASSERT_IMAGE(image);
}

#endif // defined(SOFTIP)

#if defined(NATHAIR) || defined(DOXYGEN)

#define _GET_LOCK_ID(image, var) \
        char var[200]; snprintf(var, 199, "%s-%x", GetModuleName(), (int)image)

/**
 * Creates an image as a link to a Nathair image.
 *
 * @par Specific to the Nathair environment.
 *
 * @param image         [out] @ref IMG_image_t "Image".
 * @param nathairImage  Nathair image to link to.
 * @param type          Expected @ref IMG_TYPE "image type".\n
 *                      An @ref ERROR "error is raised" if the image type is not
 *                      the expected one.\n
 *                      It should also be OR'ed with the appropriate
 *                      @ref IMG_LINK_NATHAIR "link type".
 *
 * @note Call IMG_Free() after use of the image.
 */
void IMG_CreateLinkToNathairImage(IMG_image_t *image, Image *nathairImage,
  int type)
{
  bool linkConst = type & IMG_LINK_CONST;

  assert(image != NULL);
  assert(nathairImage != NULL);

  type &= ~IMG_LINK_CONST;
  assert(type == IMG_TYPE_ANY || type == IMG_TYPE_GREY || type == IMG_TYPE_RGB
    || type == IMG_TYPE_YCBCR);

  #if defined(__cplusplus)
  if (image->data != NULL) {
    WARNING("RAID image not explicitly freed");
    IMG_Free(image);
  }
  #endif

  memset(image, 0, sizeof(IMG_image_t));
  image->width = nathairImage->GetWidth();
  image->height = nathairImage->GetHeight();
  image->bits = nathairImage->GetBitsPerValue();
  image->nathairImage = nathairImage;

  switch (nathairImage->GetDataType()) {
  case Image::Mono:
    CHECK(type == IMG_TYPE_ANY || type == IMG_TYPE_GREY,
      "Not an expected %s image", __IMG_types[type]);
    image->type = IMG_TYPE_GREY;
    break;

  case Image::Bayer:
    CHECK(type == IMG_TYPE_ANY || type == IMG_TYPE_GREY,
      "Not an expected %s image", __IMG_types[type]);
    image->type = IMG_TYPE_GREY;
    CHECK_BAYER(image);
    switch (nathairImage->GetFirstBayerChannelType()) {
    case Image::GreenOnRed:
      break;
    case Image::Red:
      image->hJog = true;
      break;
    case Image::Blue:
      image->vJog = true;
      break;
    case Image::GreenOnBlue:
      image->hJog = true;
      image->vJog = true;
      break;
    default:
      CHECK(false, "Invalid Bayer color");
    }
    break;

  case Image::RGB:
    CHECK(type == IMG_TYPE_ANY || type == IMG_TYPE_RGB,
      "Not an expected %s image", __IMG_types[type]);
    image->type = IMG_TYPE_RGB;
    break;

  case Image::YCbCr:
    CHECK(type == IMG_TYPE_ANY || type == IMG_TYPE_YCBCR,
      "Not an expected %s image", __IMG_types[type]);
    image->type = IMG_TYPE_YCBCR;
    break;

  default:
    CHECK(false, "Invalid or unsupported image type");
  }

  _GET_LOCK_ID(image, lockID);
  if (linkConst)
    image->data = (IMG_data_t*)nathairImage->GetConstData(lockID);
  else
    image->data = (IMG_data_t*)nathairImage->GetData(lockID);

  ASSERT_IMAGE(image);
}

#endif // defined(NATHAIR)

/**
 * Duplicates an image.
 *
 * @param image1        Source @ref IMG_image_t "image".
 * @param image2        [out] Destination @ref IMG_image_t "image".
 *
 * @note Call IMG_Free() after use of the destination image.
 */
void IMG_Duplicate(const IMG_image_t *image1, IMG_image_t *image2)
{
  ASSERT_IMAGE(image1);

  IMG_Create(image2, image1->width, image1->height, image1->type, image1->bits);
  IMG_Copy(image1, 0, 0, image1->width, image1->height, image2, 0, 0);
}

/**
 * Frees allocated storage of an image.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 *
 * @par SoftIP and Nathair environments
 * It also releases a link to a @ref IMG_CreateLinkToSoftIpImage "SoftIP image"
 * or @ref IMG_CreateLinkToNathairImage "Nathair image".
 */
void IMG_Free(IMG_image_t *image)
{
  #if defined(SOFTIP)
  if (image->signature != _SIGNATURE)
    return; // Not initialized.

  ASSERT_IMAGE(image);

  image->signature = ~_SIGNATURE;

  if (image->softIpImage != NULL) {
    int i, size;
    IMG_data_t *data;
    unsigned char *bdata;
    unsigned short *wdata;

    assert(image->width == image->softIpImage->mWidth);
    assert(image->height == image->softIpImage->mHeight);

    switch (image->softIpImage->mType) {
    case VIMAGE_GREY:
    case VIMAGE_BAYER:
      assert(image->bits == image->softIpImage->mBitsPerPixel);

      if (image->softIpImage->mBitsPerPixel <= 8) {
        if (image->linkType != IMG_LINK_IN) {
          size = IMG_PIXELS(*image);
          data = image->data;
          bdata = (unsigned char*)image->softIpImage->mpBuffer;
          for (i = 0; i < size; i++)
            *bdata++ = (unsigned char)*data++;
        }
        free(image->data);
      }
      else
        assert(image->data == image->softIpImage->mpBuffer);
      break;

    case VIMAGE_RGB:
      assert(image->bits == image->softIpImage->mBitsPerPixel / 3);

      if (image->softIpImage->mBitsPerPixel <= 24) {
        if (image->linkType != IMG_LINK_IN) {
          size = IMG_PIXELS(*image) * 3;
          data = image->data;
          bdata = (unsigned char*)image->softIpImage->mpBuffer;
          for (i = 0; i < size; i++)
            *bdata++ = (unsigned char)*data++;
        }
        free(image->data);
      }
      else
        assert(image->data == image->softIpImage->mpBuffer);
      break;

    case VIMAGE_BGR:
      assert(image->bits == image->softIpImage->mBitsPerPixel / 3);

      if (image->linkType != IMG_LINK_IN) {
        size = IMG_PIXELS(*image);
        data = image->data;

        if (image->softIpImage->mBitsPerPixel <= 24) {
          bdata = (unsigned char*)image->softIpImage->mpBuffer;
          for (i = 0; i < size; i++) {
            *bdata++ = (unsigned char)data[2];
            *bdata++ = (unsigned char)data[1];
            *bdata++ = (unsigned char)data[0];
            data += 3;
          }
        }
        else {
          wdata = (unsigned short*)image->softIpImage->mpBuffer;
          for (i = 0; i < size; i++) {
            *wdata++ = (unsigned char)data[2];
            *wdata++ = (unsigned char)data[1];
            *wdata++ = (unsigned char)data[0];
            data += 3;
          }
        }
      }
      free(image->data);
      break;

    default:
      assert(false);
    }

    image->softIpImage = NULL;
  }
  else
    free(image->data);
  #elif defined(NATHAIR)
  ASSERT_IMAGE(image);
  if (image->nathairImage != NULL) {
    // On exception stack unwiding, it may happen that the Python image object
    // linked to this structure is deleted before the structure destructor is
    // called, causing a crash at the Unlock() call (if the image is local).
    // Checking the image is valid seems OK, even though this is not a clean
    // fix, as the image object is already destroyed...
    if (image->nathairImage->IsValidImage()) {
      _GET_LOCK_ID(image, lockID);
      image->nathairImage->Unlock(lockID);
    }
    image->nathairImage = NULL;
  }
  else
    free(image->data);
  #else
  ASSERT_IMAGE(image);
  free(image->data);
  #endif // defined(SOFTIP) / defined(NATHAIR)

  image->data = NULL;
}

/**
 * Loads an image header from a file.
 *
 * An @ref ERROR "error is raised" if the file is not recognized or the header
 * contains invalid data.
 *
 * @param image         [out] @ref IMG_image_t "Image".
 * @param filename      Name of the file to load image from.\n
 *                      If the file name is "-", the image is read from the
 *                      standard input.
 * @param type          Expected @ref IMG_TYPE "image type".\n
 *                      An @ref ERROR "error is raised" if the image type is not
 *                      the expected one.
 * @param fileType      [out] File type (one of FTYPE_...).\n
 *                      May be @p NULL if this value is not requested.
 * @return              The file pointer.\n
 *                      To be closed after use, except if standard input.
 *
 * @note PNM format does not embed Bayer jog information. Bayer jobs are
 *       cleared, call IMG_SetBayerJogs() to possibly adjust them.
 * @note When @p type == #IMG_TYPE_ANY on a 3-channel image (PPM), the image
 *       type is set to #IMG_TYPE_RGB.
 */
static FILE *_LoadHeader(IMG_image_t *image, const char *filename, int type,
  int *fileType)
{
  FILE *fp;
  int value;
  char buffer[10];

  #define SKIP_COMMENTS(fp) while (fscanf(fp, " %[#]%*[^\n]", buffer) > 0) {}

  assert(image != NULL);
  assert(filename != NULL);
  assert(type == IMG_TYPE_ANY || type == IMG_TYPE_GREY || type == IMG_TYPE_RGB
    || type == IMG_TYPE_YCBCR);

  if (strcmp(filename, "-") == 0) {
    INFO("Loading image from standard input...");
    fp = stdin;
    filename = "STDIN";
  }
  else {
    INFO("Loading image from '%s'...", filename);
    fp = fopen(filename, "rb");
    SYSTEM_CHECK(fp != NULL, filename);
  }

  memset(image, 0, sizeof(IMG_image_t));

  // Read file header.
  CHECK(fscanf(fp, "P%d", &value) == 1,
    "%s: Invalid image file header (Px not found)", filename);
  switch (value) {
  case FTYPE_PGM_ASCII:
  case FTYPE_PGM_BINARY:
    CHECK(type == IMG_TYPE_ANY || type == IMG_TYPE_GREY,
      "%s: Not an expected %s image", filename, __IMG_types[type]);
    image->type = IMG_TYPE_GREY;
    break;
  case FTYPE_PPM_ASCII:
  case FTYPE_PPM_BINARY:
    CHECK(type == IMG_TYPE_ANY || type == IMG_TYPE_RGB
        || type == IMG_TYPE_YCBCR,
      "%s: Not an expected %s image", filename, __IMG_types[type]);
    image->type = type == IMG_TYPE_ANY ? IMG_TYPE_RGB : type;
    break;
  default:
    CHECK(false, "%s: Invalid or unsupported image type", filename);
  }
  if (fileType != NULL)
    *fileType = value;

  SKIP_COMMENTS(fp);
  CHECK(fscanf(fp, "%d", &image->width) == 1,
    "%s: Invalid image file header (width not found)", filename);
  CHECK(image->width >= 0, "%s: Invalid image width (must be >= 0)", filename);

  SKIP_COMMENTS(fp);
  CHECK(fscanf(fp, "%d", &image->height) == 1,
    "%s: Invalid image file header (height not found)", filename);
  CHECK(image->height >= 0, "%s: Invalid image height (must be >= 0)",
    filename);

  SKIP_COMMENTS(fp);
  CHECK(fscanf(fp, "%d", &value) == 1,
    "%s: Invalid image file header (max color value not found)", filename);
  CHECK(value > 0 && value <= 65535,
    "%s: Invalid image data max value (must be in the range [1,65535]",
    filename);

  // No comments after max value, just 1 whitespace (blank, TAB, CR or LF).
  *buffer = (char)fgetc(fp);
  CHECK(*buffer == ' ' || *buffer == '\t' || *buffer == '\r' || *buffer == '\n',
    "%s: Invalid image file header (invalid header separator)", filename);

  image->bits = IMG_GetBits(value);

  #if defined(SOFTIP)
  image->signature = _SIGNATURE;
  #endif

  return fp;
}

/**
 * Loads an image header from a file.
 *
 * An @ref ERROR "error is raised" if the file is not recognized or the header
 * contains invalid data.
 *
 * @param image         [out] @ref IMG_image_t "Image".
 * @param filename      Name of the file to load header from.\n
 *                      If the file name is "-", the image is read from the
 *                      standard input.
 * @param type          Expected @ref IMG_TYPE "image type".\n
 *                      An @ref ERROR "error is raised" if the image type is not
 *                      the expected one.
 *
 * @note All @p image members are set according to the header, except
 *       @p data which is set to @p NULL.
 * @note PNM format does not embed Bayer jog information. Bayer jobs are
 *       cleared, call IMG_SetBayerJogs() to possibly adjust them.
 * @note When @p type == #IMG_TYPE_ANY on a 3-channel image (PPM), the image
 *       type is set to #IMG_TYPE_RGB.
 */
void IMG_LoadHeader(IMG_image_t *image, const char *filename, int type)
{
  FILE *fp;

  fp = _LoadHeader(image, filename, type, NULL);
  if (fp != stdin)
    SYSTEM_CHECK(fclose(fp) == 0, filename);
}

/**
 * Loads an image from a file.
 *
 * An @ref ERROR "error is raised" if the file is not recognized or contains
 * invalid data.
 *
 * @param image         [out] @ref IMG_image_t "Image".
 * @param filename      Name of the file to load image from.\n
 *                      If the file name is "-", the image is read from the
 *                      standard input.
 * @param type          Expected @ref IMG_TYPE "image type".\n
 *                      An @ref ERROR "error is raised" if the image type is not
 *                      the expected one.
 *
 * @note PNM format does not embed Bayer jog information. Bayer jobs are
 *       cleared, call IMG_SetBayerJogs() to possibly adjust them.
 * @note When @p type == #IMG_TYPE_ANY on a 3-channel image (PPM), the image
 *       type is set to #IMG_TYPE_RGB.
 * @note Call IMG_Free() after use of the image.
 */
void IMG_Load(IMG_image_t *image, const char *filename, int type)
{
  FILE *fp;
  int i, fileType, size, sum = 0;
  IMG_data_t *data;
  uint8_t *bdata;
  bool swap;

  swap = (bool)(type & IMG_TYPE_LITTLE_ENDIAN);
  type &= ~IMG_TYPE_LITTLE_ENDIAN;

  fp = _LoadHeader(image, filename, type, &fileType);
  IMG_Create(image, image->width, image->height, image->type, image->bits);

  size = IMG_PIXELS(*image) * IMG_CHANNELS(*image);
  data = image->data;

  if (fp == stdin)
    filename = "STDIN";

  // Read data.
  switch (fileType) {
  case FTYPE_PGM_ASCII:
  case FTYPE_PPM_ASCII:
    CHECK(!swap, "%s: Can't swap an ASCII image", filename);
    sum = 0;
    for (i = 0; i < size; i++)
      sum += fscanf(fp, "%hd", data++);
    break;
  case FTYPE_PGM_BINARY:
  case FTYPE_PPM_BINARY:
    if (image->bits <= 8) {
      CHECK(!swap, "%s: Can't swap an 8-bit image", filename);
      sum = (int)fread(data, 1, size, fp);
      // Extend to 16-bit data.
      bdata = (uint8_t*)data + size;
      data += size;
      for (i = 0; i < size; i++)
        *--data = *--bdata;
    }
    else {
      sum = (int)fread(data, 2, size, fp);
      // Consider endianness (both for input file and CPU).
      bdata = (uint8_t*)data;
      if (swap)
        for (i = 0; i < size; i++, bdata += 2)
          *data++ = ((IMG_data_t)bdata[1] << 8) | bdata[0];
      else
        for (i = 0; i < size; i++, bdata += 2)
          *data++ = ((IMG_data_t)bdata[0] << 8) | bdata[1];
    }
    break;
  }

  CHECK(sum == size, "%s: Not enough pixel data", filename);

  if (fp != stdin)
    SYSTEM_CHECK(fclose(fp) == 0, filename);

  IMG_Check(image, filename);
}

/**
 * Saves an image into a file.
 *
 * @param image         @ref IMG_image_t "Image".
 * @param filename      Name of the file to dump image to.\n
 *                      If the file name is "-", the image is written to the
 *                      standard output.
 * @param format        @ref IMG_FORMAT "File format".
 *
 * @note Data are checked prior to being saved (IMG_Check()).
 */
void IMG_Save(const IMG_image_t *image, const char *filename, int format)
{
  FILE *fp;
  int i, size, fileType;
  IMG_data_t *data;

  ASSERT_IMAGE(image);
  assert(filename != NULL);
  assert(format == IMG_FORMAT_PNM_BINARY || format == IMG_FORMAT_PNM_ASCII);

  IMG_Check(image, filename);

  if (strcmp(filename, "-") == 0) {
    INFO("Saving image to standard output...");
    fp = stdout;
    filename = "STDOUT";
  }
  else {
    INFO("Saving image to '%s'...", filename);
    fp = fopen(filename, "wb");
    SYSTEM_CHECK(fp != NULL, filename);
  }

  size = IMG_PIXELS(*image) * IMG_CHANNELS(*image);
  data = image->data;

  fileType = image->type == IMG_TYPE_GREY
    ? (format == IMG_FORMAT_PNM_ASCII ? FTYPE_PGM_ASCII : FTYPE_PGM_BINARY)
    : (format == IMG_FORMAT_PNM_ASCII ? FTYPE_PPM_ASCII : FTYPE_PPM_BINARY);

  // Write the header.
  SYSTEM_CHECK(fprintf(fp, "P%d\n%d %d\n%d\n", fileType, image->width,
    image->height, (1 << image->bits) - 1) >= 0, filename);

  // Write the data.
  switch (fileType) {
  case FTYPE_PGM_ASCII:
    for (i = 0; i < size; i++)
      fprintf(fp, "%d\n", *data++);
    break;
  case FTYPE_PPM_ASCII:
    for (i = 0; i < size; i += 3, data += 3)
      fprintf(fp, "%d %d %d\n", data[0], data[1], data[2]);
    break;
  case FTYPE_PGM_BINARY:
  case FTYPE_PPM_BINARY:
    if (image->bits <= 8) {
      for (i = 0; i < size; i++)
        fputc(*data++, fp);
    }
    else {
      //TODO: could be done instead in big endian.
      // fwrite(data, 2, size, fp);
      for (i = 0; i < size; i++) {
        fputc(*data >> 8, fp);
        fputc(*data++ & 0xFF, fp);
      }
    }
    break;
  }

  if (fp != stdout)
    SYSTEM_CHECK(fclose(fp) == 0, filename);
}

/**
 * Sets the jogs of a Bayer image.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 * @param hJog          Horizontal jog.
 * @param vJog          Vertical jog.
 *
 * @see IMG_HJOG(), IMG_VJOG().
 */
void IMG_SetBayerJogs(IMG_image_t *image, bool hJog, bool vJog)
{
  ASSERT_IMAGE(image);
  CHECK_BAYER(image);

  #if defined(NATHAIR)
  if (image->nathairImage != NULL)
    image->nathairImage->SetFirstBayerChannelType(
      vJog ? (hJog ? Image::GreenOnBlue : Image::Blue)
           : (hJog ? Image::Red : Image::GreenOnRed));
  #endif

  image->hJog = hJog ? true : false;
  image->vJog = vJog ? true : false;
}

/**
 * Builds an error message for an invalid value.
 *
 * @param buffer        [out] Buffer.
 * @param image         @ref IMG_image_t "Image".
 * @param position      Linear position in the buffer.
 * @param msg           String to be displayed in front of the error message.\n
 *                      May be NULL.
 */
static char *_GetInvalidValueMsg(char *buffer, const IMG_image_t *image,
  int position, const char *msg)
{
  const char *rgb[3] = {"red", "green", "blue"};
  const char *ycbcr[3] = {"Y", "Cb", "Cr"};
  char *p = buffer;

  if (msg != NULL)
      p += sprintf(p, "%s: ", msg);

  if (image->type != IMG_TYPE_GREY)
    sprintf(p, "Invalid channel value [%d > %d] at (%d,%d,%s)",
      image->data[position], (1 << image->bits) - 1,
      (position / 3) % image->width, (position / 3) / image->width,
      (image->type == IMG_TYPE_RGB ? rgb : ycbcr)[position % 3]);
  else
    sprintf(p, "Invalid pixel value [%d > %d] at (%d,%d)",
      image->data[position], (1 << image->bits) - 1,
      position % image->width, position / image->width);

  return buffer;
}

/**
 * Checks color data fit the expected range of bits.
 *
 * An @ref ERROR "error is raised" if a value is invalid.
 *
 * @param image         @ref IMG_image_t "Image".
 * @param msg           String to be displayed in front of the error message.\n
 *                      Typically a name representing the image (file name)
 *                      or a step within an algorithm.
 */
void IMG_Check(const IMG_image_t *image, const char *msg)
{
  int i, size;
  IMG_data_t *data, maxValue;
  char buffer[200];

  ASSERT_IMAGE(image);
  assert(msg != NULL);

  if (image->bits == sizeof(IMG_data_t) * 8)
    return; // No need to check.

  size = IMG_PIXELS(*image) * IMG_CHANNELS(*image);
  data = image->data;
  maxValue = (IMG_data_t)((1 << image->bits) - 1);

  for (i = 0; i < size; i++, data++)
    if (*data > maxValue)
      CHECK(false, "%s", _GetInvalidValueMsg(buffer, image, i, msg));
}

/**
 * Clips color data to the expected range of bits.
 *
 * In @ref VERBOSE "debug verbose level", messages can be displayed containing
 * the position and the original value of data fixed.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 * @param maxMsg        Max number of messages to display.\n
 *                      -1 to display all messages.
 * @param msg           String to be displayed in front of the message.\n
 *                      Typically a name representing the image (file name)
 *                      or a step within an algorithm.\n
 *                      May be @p NULL.
 */
void IMG_Clip(IMG_image_t *image, int maxMsg, const char *msg)
{
  int i, size, fixed;
  IMG_data_t *data, maxValue;
  char buffer[200];

  ASSERT_IMAGE(image);

  if (image->bits == sizeof(IMG_data_t) * 8)
    return; // No need to clip.

  size = IMG_PIXELS(*image) * IMG_CHANNELS(*image);
  data = image->data;
  maxValue = (IMG_data_t)((1 << image->bits) - 1);

  fixed = 0;
  for (i = 0; i < size; i++, data++) {
    if (*data > maxValue) {
      if (++fixed <= maxMsg || maxMsg < 0)
        DEBUG("%s", _GetInvalidValueMsg(buffer, image, i, msg));
      *data = maxValue;
    }
  }

  if (fixed > 0)
  {
    char *p = buffer;
    if (msg != NULL)
      p += sprintf(p, "%s: ", msg);
    p += sprintf(p, "%d value%s clipped", fixed, fixed > 1 ? "s" : "");
    if (maxMsg > 0 && fixed > maxMsg)
      p += sprintf(p, " (%d first displayed)", maxMsg);
    INFO("%s", buffer);
  }
}

/**
 * Masks color data to the expected range of bits.
 *
 * In @ref VERBOSE "debug verbose level", messages can be displayed containing
 * the position and the original value of data fixed.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 * @param maxMsg        Max number of messages to display.\n
 *                      -1 to display all messages.
 * @param msg           String to be displayed in front of the message.\n
 *                      Typically a name representing the image (file name)
 *                      or a step within an algorithm.\n
 *                      May be @p NULL.
 */
void IMG_Mask(IMG_image_t *image, int maxMsg, const char *msg)
{
  int i, size, fixed;
  IMG_data_t *data, maxValue;
  char buffer[200];

  ASSERT_IMAGE(image);

  if (image->bits == sizeof(IMG_data_t) * 8)
    return; // No need to mask.

  size = IMG_PIXELS(*image) * IMG_CHANNELS(*image);
  data = image->data;
  maxValue = (IMG_data_t)((1 << image->bits) - 1);

  fixed = 0;
  for (i = 0; i < size; i++, data++) {
    if (*data > maxValue) {
      if (++fixed <= maxMsg || maxMsg < 0)
        DEBUG("%s", _GetInvalidValueMsg(buffer, image, i, msg));
      *data &= maxValue;
    }
  }

  if (fixed > 0)
  {
    char *p = buffer;
    if (msg != NULL)
      p += sprintf(p, "%s: ", msg);
    p += sprintf(p, "%d value%s masked", fixed, fixed > 1 ? "s" : "");
    if (maxMsg > 0 && fixed > maxMsg)
      p += sprintf(p, " (%d first displayed)", maxMsg);
    INFO("%s", buffer);
  }
}

/**
 * Shifts (left or right) data to a new range of bits.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 * @param bits          Number of bits per color channel.\n
 *                      Can be OR'ed with @ref IMG_SHIFT "options".
 * @return              @p true if the image data were shifted,\n
 *                      @p false if the image range of bits was already the
 *                      requested one.
 *
 * @par SoftIP environment
 * Cannot be applied on a link to a
 * @ref IMG_CreateLinkToSoftIpImage "SoftIP image".
 */
bool IMG_Shift(IMG_image_t *image, int bits)
{
  int i, size, shift;
  IMG_data_t *data;
  bool copyMSbits;
  bool round;

  copyMSbits = (bool)(bits & IMG_SHIFT_COPY_MS_BITS);
  round = (bool)(bits & IMG_SHIFT_ROUND);
  bits &= ~(IMG_SHIFT_COPY_MS_BITS | IMG_SHIFT_ROUND);

  ASSERT_IMAGE(image);
  assert(bits > 0 && bits <= 16);

  #if defined(SOFTIP)
  CHECK(image->softIpImage == NULL, "Cannot shift a link to a SoftIP image");
  #endif

  if (image->bits == bits)
    return false;

  #if defined(NATHAIR)
  if (image->nathairImage != NULL)
    image->nathairImage->SetBitsPerValue(bits);
  #endif

  size = IMG_PIXELS(*image) * IMG_CHANNELS(*image);
  data = image->data;

  INFO("Shifting image from %d to %d bits...", image->bits, bits);

  if (image->bits > bits) {
    shift = image->bits - bits;
    if (round) {
      // Add "0.5" before shifting.
      int offset = 1 << (shift - 1);
      IMG_data_t maxValue = (IMG_data_t)((1 << bits) - 1);
      for (i = 0; i < size; i++, data++) {
        *data = (IMG_data_t)((*data + offset) >> shift);
        if (*data > maxValue)
          *data = maxValue;
      }
    }
    else {
      // Do not perform rounding.
      for (i = 0; i < size; i++, data++)
        *data >>= shift;
    }
  }
  else {
    shift = bits - image->bits;
    if (copyMSbits) {
      // Copy the MS bits to the LS bits
      if (image->bits >= shift) {
        // Copy the MS bits to the LS bits.
        int msbShift = image->bits - shift;
        for (i = 0; i < size; i++, data++)
          *data = (IMG_data_t)((*data << shift) | (*data >> msbShift));
      }
      else {
        // The width of the data is less than the requested shift.
        // So data has to be copied several times to fill the LS bits.
        int j, n = shift / image->bits;
        for (i = 0; i < size; i++, data++) {
          *data <<= shift;
          for (j = 0; j < n; j++)
            *data |= *data >> image->bits;
        }
      }
    }
    else {
      // Do not copy MB bits: keep 0's
      for (i = 0; i < size; i++, data++)
        *data <<= shift;
    }
  }

  image->bits = bits;
  return true;
}

/**
 * Crops an image.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 * @param left          Number of pixels to crop on the left border.
 * @param top           Number of pixels to crop on the top border.
 * @param right         Number of pixels to crop on the right border.
 * @param bottom        Number of pixels to crop on the bottom border.
 *
 * For #IMG_TYPE_GREY images, @ref IMG_SetBayerJogs "Bayer jogs" are adjusted
 * accordingly.
 *
 * @par SoftIP environment
 * Cannot be applied on a link to a
 * @ref IMG_CreateLinkToSoftIpImage "SoftIP image".
 */
void IMG_Crop(IMG_image_t *image, int left, int top, int right, int bottom)
{
  int width, height;

  ASSERT_IMAGE(image);

  assert(left >= 0);
  assert(top >= 0);
  assert(right >= 0);
  assert(bottom >= 0);

  #if defined(SOFTIP)
  CHECK(image->softIpImage == NULL, "Cannot crop a link to a SoftIP image");
  #endif

  if (left + top + right + bottom == 0)
    return; // Nothing to do.

  #if defined(NATHAIR)
  if (image->nathairImage != NULL) {
    _GET_LOCK_ID(image, lockID);
    image->nathairImage->Unlock(lockID);
    image->nathairImage->CropBorders(top, right, bottom, left);

    image->data = (IMG_data_t*)image->nathairImage->GetData(lockID);
    image->width = image->nathairImage->GetWidth();
    image->height = image->nathairImage->GetHeight();

    if (image->nathairImage->GetDataType() == Image::Bayer)
      IMG_SetBayerJogs(image, IMG_HJOG(*image, left), IMG_VJOG(*image, top));
    return;
  }
  #endif

  width = image->width - left - right;
  height = image->height - top - bottom;
  CHECK(width > 0 && height > 0,
    "Cropping parameters result in an empty image");

  if (left + right == 0) {
    if (top > 0)
      memmove(image->data, IMG_PIXEL(*image, 0, top),
        IMG_OFFSET(*image, 0, height) * sizeof(IMG_data_t));
  }
  else {
    IMG_data_t *src = IMG_PIXEL(*image, left, top);
    IMG_data_t *dst = IMG_PIXEL(*image, 0, 0);
    int srcLen = image->width * IMG_CHANNELS(*image);
    int dstLen = width * IMG_CHANNELS(*image);
    int i;

    for (i = 0; i < height; i++, src += srcLen, dst += dstLen)
      memmove(dst, src, dstLen * sizeof(IMG_data_t));
  }

  image->width = width;
  image->height = height;
  image->data = (IMG_data_t*)realloc(image->data,
    IMG_PIXELS(*image) * IMG_CHANNELS(*image) * sizeof(IMG_data_t));
  SYSTEM_CHECK(image->data != NULL, NULL);

  if (IMG_SeemsBayer(image))
    IMG_SetBayerJogs(image, IMG_HJOG(*image, left), IMG_VJOG(*image, top));
}

/**
 * Copies a rectangular region of an image into another one.
 *
 * @param image1        Source @ref IMG_image_t "image".
 * @param x1            X position within the source image.
 * @param y1            Y position within the source image.
 * @param width         Width of the rectangular region.
 * @param height        Height of the rectangular region.
 * @param image2        [in/out] Destination @ref IMG_image_t "image".\n
 *                      Can be @p image1, overlapping regions are allowed.
 * @param x2            X position within the destination image.
 * @param y2            Y position within the destination image.
 *
 * For #IMG_TYPE_GREY images, when (x2,y2)=(0,0), @ref IMG_SetBayerJogs
 * "Bayer jogs" of the destination image are set according to (x1,y1).
 *
 * @note If @p width or @p height are negative, nothing is copied.
 */
void IMG_Copy(const IMG_image_t *image1, int x1, int y1, int width, int height,
  IMG_image_t *image2, int x2, int y2)
{
  IMG_data_t *src, *dst;
  int i, srcLen, dstLen, lineLen;

  ASSERT_IMAGE(image1);
  ASSERT_IMAGE(image2);

  assert(x1 >= 0);
  assert(y1 >= 0);
  assert(x2 >= 0);
  assert(y2 >= 0);

  if (width <= 0 || height <= 0)
    return;

  CHECK(image1->type == image2->type,
    "Both images must be of the same type");
  CHECK(image1->bits == image2->bits,
    "Both images must be of the same bit range");
  CHECK(x1 + width <= image1->width && y1 + height <= image1->height,
    "Rectangle outside the source image");
  CHECK(x2 + width <= image2->width && y2 + height <= image2->height,
    "Rectangle outside the destination image");

  src = IMG_PIXEL(*image1, x1, y1);
  dst = IMG_PIXEL(*image2, x2, y2);
  srcLen = image1->width * IMG_CHANNELS(*image1);
  dstLen = image2->width * IMG_CHANNELS(*image2);
  lineLen = width * IMG_CHANNELS(*image1) * sizeof(IMG_data_t);

  if (image1 == image2) {
    if (y2 > y1) {
      src += (width - 1) * srcLen;
      dst += (width - 1) * dstLen;
      srcLen = -srcLen;
      dstLen = -dstLen;
    }
    for (i = 0; i < height; i++, src += srcLen, dst += dstLen)
      memmove(dst, src, lineLen);
  }
  else
    for (i = 0; i < height; i++, src += srcLen, dst += dstLen)
      memcpy(dst, src, lineLen);

  if (IMG_SeemsBayer(image2) && x2 == 0 && y2 == 0)
    IMG_SetBayerJogs(image2, IMG_HJOG(*image1, x1), IMG_VJOG(*image1, y1));
}

/**
 * Copies an image border into another image.
 *
 * @param image1        Source @ref IMG_image_t "image".
 * @param width         Width of the border.
 * @param height        Height of the border.
 * @param image2        [in/out] Destination @ref IMG_image_t "image".
 *
 * For #IMG_TYPE_GREY images, @ref IMG_SetBayerJogs "Bayer jogs" of the
 * destination image are set according to the source image.
 *
 * @note If @p width or @p height are negative, nothing is copied.
 */
void IMG_CopyBorder(const IMG_image_t *image1, int width, int height,
  IMG_image_t *image2)
{
  ASSERT_IMAGE(image1);
  ASSERT_IMAGE(image2);

  assert(width >= 0);
  assert(height >= 0);

  width = MIN(width, image1->width / 2);
  height = MIN(height, image1->height / 2);

  IMG_Copy(image1, 0, 0,
           image1->width, height,
           image2, 0, 0);
  IMG_Copy(image1, 0, height,
           width, image1->height - 2 * height,
           image2, 0, height);
  IMG_Copy(image1, image1->width - width, height,
           width, image1->height - 2 * height,
           image2, image1->width - width, height);
  IMG_Copy(image1, 0, image1->height - height,
           image1->width, height,
           image2, 0, image1->height - height);
}

/**
 * Gets the (x, y) position of a color of a Bayer image.
 *
 * @param image         Source @ref IMG_image_t "image".
 * @param color         @ref IMG_COLOR "Color plane".
 * @param x             [out] X position, either 0 or 1.
 * @param y             [out] Y position, either 0 or 1.
 *
 * @note For #IMG_COLOR_G and #IMG_COLOR_2G, the first green pixel position is
 *       returned (Y position is always 0).
 * @note This function takes care of @ref IMG_SetBayerJogs "Bayer jogs".
 */
void IMG_GetBayerColorPosition(const IMG_image_t *image, int color,
  int *x, int *y)
{
  ASSERT_IMAGE(image);
  CHECK_BAYER(image);

  // Jog translation:
  //     HV       XY     XY     XY       XY         XY
  // in: 00  gir: 00  r: 10  b: 01  gib: 11  1st g: 00
  //     01       01     11     00       10         10
  //     10       10     00     11       01         10
  //     11       11     01     10       00         00
  switch (color) {
  case IMG_COLOR_GIR:
    *x = image->hJog ? 1 : 0;
    *y = image->vJog ? 1 : 0;
    break;
  case IMG_COLOR_R:
    *x = image->hJog ? 0 : 1;
    *y = image->vJog ? 1 : 0;
    break;
  case IMG_COLOR_B:
    *x = image->hJog ? 1 : 0;
    *y = image->vJog ? 0 : 1;
    break;
  case IMG_COLOR_GIB:
    *x = image->hJog ? 0 : 1;
    *y = image->vJog ? 0 : 1;
    break;
  case IMG_COLOR_G: // Average greens.
  case IMG_COLOR_2G: // Both greens.
    *x = (image->hJog ^ image->vJog) ? 1 : 0;
    *y = 0;
    break;
  default:
    assert(false);
  }
}

/**
 * Extracts a color plane from an image.
 *
 * @param image         @ref IMG_image_t "Image".
 * @param color         @ref IMG_COLOR "Color plane".
 * @param plane         [out] Color plane @ref IMG_image_t "image".\n
 *                      Its type is #IMG_TYPE_GREY.
 *
 * This function applies to both RGB and Bayer images:
 *  - RGB:
 *     - The output plane is the same size as the image.
 *  - Bayer:
 *     - The output plane is 1/4 size (1/2 width, 1/2 height) of the image
 *       except for #IMG_COLOR_2G (1/2 width, 1 height).
 *     - If @p color is #IMG_COLOR_G, the output plane is the average of the 2
 *       greens GIR and GIB.
 *     - If @p color is #IMG_COLOR_2G, the output plane is the merge of the 2
 *       greens GIR and GIB.
 *
 * @note This function takes care of @ref IMG_SetBayerJogs "Bayer jogs".
 * @note Call IMG_Free() after use of the plane image.
 */
void IMG_ExtractPlane(const IMG_image_t *image, int color, IMG_image_t *plane)
{
  int i;
  IMG_data_t *data, *cdata;

  ASSERT_IMAGE(image);
  assert(plane != NULL);

  if (image->type == IMG_TYPE_RGB || image->type == IMG_TYPE_YCBCR) {
    int size;

    assert(color >= IMG_COLOR_R && color <= IMG_COLOR_B);

    IMG_Create(plane, image->width, image->height, IMG_TYPE_GREY, image->bits);
    cdata = plane->data;

    size = IMG_PIXELS(*image);
    data = image->data + color;

    for (i = 0; i < size; i++, data += 3)
      *cdata++ = *data;
  }
  else {
    int j, width, height, x, y;

    IMG_GetBayerColorPosition(image, color, &x, &y);

    IMG_Create(plane, image->width / 2,
      color == IMG_COLOR_2G ? image->height : image->height / 2, IMG_TYPE_GREY,
      image->bits);

    width = image->width;
    height = image->height;
    data = IMG_PIXEL(*image, x, y);
    cdata = plane->data;

    if (color == IMG_COLOR_G) { // Average greens.
      IMG_data_t *data2 = IMG_PIXEL(*image, x == 0 ? 1 : 0, 1);

      for (i = 0; i < height; i += 2, data += width, data2 += width)
        for (j = 0; j < width; j += 2, data += 2, data2 += 2)
          *cdata++ = (*data + *data2) / 2;
    }
    else if (color == IMG_COLOR_2G) { // Both greens.
      IMG_data_t *data2 = IMG_PIXEL(*image, x == 0 ? 1 : 0, 1);

      for (i = 0; i < height; i += 2, data += width, data2 += width) {
        for (j = 0; j < width; j += 2, data += 2)
          *cdata++ = *data;
        for (j = 0; j < width; j += 2, data2 += 2)
          *cdata++ = *data2;
      }
    }
    else {
      for (i = 0; i < height; i += 2, data += width)
        for (j = 0; j < width; j += 2, data += 2)
          *cdata++ = *data;
    }
  }
}

/**
 * Replaces a color plane of an image.
 *
 * @param image         [in/out] @ref IMG_image_t "Image".
 * @param color         @ref IMG_COLOR "Color plane".
 * @param plane         Color plane @ref IMG_image_t "image".\n
 *                      Its type must be #IMG_TYPE_GREY.
 *
 * This function applies to both RGB and Bayer images:
 *  - RGB:
 *     - The plane must be the same size as the image.
 *  - Bayer:
 *     - The plane must be 1/4 size of the image.
 *     - If @p color is #IMG_COLOR_G, the provided plane replaces the 2 green
 *       GIR and GIB.
 *
 * @note This function takes care of @ref IMG_SetBayerJogs "Bayer jogs".
 */
void IMG_ReplacePlane(IMG_image_t *image, int color, const IMG_image_t *plane)
{
  int i;
  IMG_data_t *data, *cdata;

  ASSERT_IMAGE(image);
  ASSERT_IMAGE(plane);
  CHECK(plane->type == IMG_TYPE_GREY, "Plane must be of type grey");

  if (image->type == IMG_TYPE_RGB || image->type == IMG_TYPE_YCBCR) {
    int size;

    assert(color >= IMG_COLOR_R && color <= IMG_COLOR_B);

    CHECK(plane->width == image->width && plane->height == image->height,
      "Plane must be the same size as the image");

    size = IMG_PIXELS(*image);
    data = image->data + color;
    cdata = plane->data;

    for (i = 0; i < size; i++, data += 3)
      *data = *cdata++;
  }
  else {
    int j, width, height, x, y;

    assert(color != IMG_COLOR_2G);

    CHECK(plane->width == image->width / 2
        && plane->height == image->height / 2,
      "Plane must be 1/4 of the image");

    IMG_GetBayerColorPosition(image, color, &x, &y);

    width = image->width;
    height = image->height;
    data = IMG_PIXEL(*image, x, y);
    cdata = plane->data;

    if (color == IMG_COLOR_G) { // Average greens.
      IMG_data_t *data2 = IMG_PIXEL(*image, x == 0 ? 1 : 0, 1);

      for (i = 0; i < height; i += 2, data += width, data2 += width)
        for (j = 0; j < width; j += 2, data += 2, data2 += 2)
          *data = *data2 = *cdata++;
    }
    else {
      for (i = 0; i < height; i += 2, data += width)
        for (j = 0; j < width; j += 2, data += 2)
          *data = *cdata++;
    }
  }
}

#if !defined(__cplusplus)

static int __IMG_SortPixel(const void *a, const void *b)
{
  return *(const IMG_data_t*)a - *(const IMG_data_t*)b;
}

/**
 * Sorts pixels.
 *
 * @param pixels        [in/out] Pixel array to sort.
 * @param n             Number of pixels in the array.
 */
void IMG_SortPixels(IMG_data_t *pixels, int n)
{
  qsort(pixels, n, sizeof(IMG_data_t), __IMG_SortPixel);
}

#endif // !defined(__cplusplus)

/** @} */
