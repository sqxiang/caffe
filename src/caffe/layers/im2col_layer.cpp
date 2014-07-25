// Copyright 2014 BVLC and contributors.

#include <vector>

#include "caffe/layer.hpp"
#include "caffe/vision_layers.hpp"
#include "caffe/common.hpp"

namespace caffe {

template <typename Dtype>
void Im2colLayer<Dtype>::SetUp(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  Layer<Dtype>::SetUp(bottom, top);
  ConvolutionParameter conv_param = this->layer_param_.convolution_param();
  CHECK(!conv_param.has_kernel_size() !=
      !(conv_param.has_kernel_h() && conv_param.has_kernel_w()))
      << "Filter size is kernel_size OR kernel_h and kernel_w; not both";
  CHECK(conv_param.has_kernel_size() ||
      (conv_param.has_kernel_h() && conv_param.has_kernel_w()))
      << "For non-square filters both kernel_h and kernel_w are required.";
  CHECK((!conv_param.has_pad() && conv_param.has_pad_h()
      && conv_param.has_pad_w())
      || (!conv_param.has_pad_h() && !conv_param.has_pad_w()))
      << "pad is pad OR pad_h and pad_w are required.";
  CHECK((!conv_param.has_stride() && conv_param.has_stride_h()
      && conv_param.has_stride_w())
      || (!conv_param.has_stride_h() && !conv_param.has_stride_w()))
      << "Stride is stride OR stride_h and stride_w are required.";
  if (conv_param.has_kernel_size()) {
    kernel_h_ = kernel_w_ = conv_param.kernel_size();
  } else {
    kernel_h_ = conv_param.kernel_h();
    kernel_w_ = conv_param.kernel_w();
  }
  CHECK_GT(kernel_h_, 0) << "Filter dimensions cannot be zero.";
  CHECK_GT(kernel_w_, 0) << "Filter dimensions cannot be zero.";
  if (!conv_param.has_pad_h()) {
    pad_h_ = pad_w_ = conv_param.pad();
  } else {
    pad_h_ = conv_param.pad_h();
    pad_w_ = conv_param.pad_w();
  }
  if (!conv_param.has_stride_h()) {
    stride_h_ = stride_w_ = conv_param.stride();
  } else {
    stride_h_ = conv_param.stride_h();
    stride_w_ = conv_param.stride_w();
  }
  channels_ = bottom[0]->channels();
  height_ = bottom[0]->height();
  width_ = bottom[0]->width();
  (*top)[0]->Reshape(
      bottom[0]->num(), channels_ * kernel_h_ * kernel_w_,
      (height_ + 2 * pad_h_ - kernel_h_) / stride_h_ + 1,
      (width_ + 2 * pad_w_ - kernel_w_) / stride_w_ + 1);
}

template <typename Dtype>
Dtype Im2colLayer<Dtype>::Forward(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  const Dtype* bottom_data = bottom[0]->const_data();
  Dtype* top_data = (*top)[0]->mutable_data();
  for (int n = 0; n < bottom[0]->num(); ++n) {
    this->device_->im2col(
        bottom_data + bottom[0]->offset(n), channels_, height_,
        width_, kernel_h_, kernel_w_, pad_h_, pad_w_,
        stride_h_, stride_w_, top_data + (*top)[0]->offset(n));
  }
  return Dtype(0.);
}

template <typename Dtype>
void Im2colLayer<Dtype>::Backward(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, vector<Blob<Dtype>*>* bottom) {
  const Dtype* top_diff = top[0]->const_diff();
  Dtype* bottom_diff = (*bottom)[0]->mutable_diff();
  for (int n = 0; n < top[0]->num(); ++n) {
    this->device_->col2im(
        top_diff + top[0]->offset(n), channels_, height_, width_,
        kernel_h_, kernel_w_, pad_h_, pad_w_,
        stride_h_, stride_w_, bottom_diff + (*bottom)[0]->offset(n));
  }
}

INSTANTIATE_CLASS(Im2colLayer);

}  // namespace caffe
