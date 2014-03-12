#include "likely/opencv.hpp"

namespace likely
{

int typeToDepth(likely_type type)
{
    switch (likely_data(type)) {
      case likely_type_u8:  return CV_8U;
      case likely_type_i8:  return CV_8S;
      case likely_type_u16: return CV_16U;
      case likely_type_i16: return CV_16S;
      case likely_type_i32: return CV_32S;
      case likely_type_f32: return CV_32F;
      case likely_type_f64: return CV_64F;
    }
    assert(!"Unsupported matrix depth.");
    return 0;
}

likely_type depthToType(int depth)
{
    switch (depth) {
      case CV_8U:  return likely_type_u8;
      case CV_8S:  return likely_type_i8;
      case CV_16U: return likely_type_u16;
      case CV_16S: return likely_type_i16;
      case CV_32S: return likely_type_i32;
      case CV_32F: return likely_type_f32;
      case CV_64F: return likely_type_f64;
    }
    assert(!"Unsupported matrix depth.");
    return likely_type_null;
}

cv::Mat toCvMat(likely_const_mat m)
{
    return cv::Mat((int)m->rows, (int)m->columns, CV_MAKETYPE(typeToDepth(m->type), (int)m->channels), (void*)m->data);
}

likely_mat fromCvMat(const cv::Mat &src)
{
    if (!src.isContinuous() || !src.data)
        return likely_new(likely_type_null, 0, 0, 0, 0, NULL);
    return likely_new(depthToType(src.depth()), src.channels(), src.cols, src.rows, 1, src.data);
}

} // namespace likely