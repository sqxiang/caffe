// Copyright 2014 BVLC and contributors.

#include <algorithm>
#include <vector>

#include "caffe/layer.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype>
void PowerLayer<Dtype>::SetUp(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  NeuronLayer<Dtype>::SetUp(bottom, top);
  power_ = this->layer_param_.power_param().power();
  scale_ = this->layer_param_.power_param().scale();
  shift_ = this->layer_param_.power_param().shift();
  diff_scale_ = power_  * scale_;
}

// Compute y = (shift + scale * x)^power
template <typename Dtype>
Dtype PowerLayer<Dtype>::Forward(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  Dtype* top_data = (*top)[0]->mutable_data();
  const int count = bottom[0]->count();
  // Special case where we can ignore the input: scale or power is 0.
  if (diff_scale_ == Dtype(0)) {
    Dtype value = (power_ == 0) ? Dtype(1) : pow(shift_, power_);
    this->device_->set(count, value, top_data);
    return Dtype(0);
  }
  const Dtype* bottom_data = bottom[0]->const_data();
  this->device_->copy(count, bottom_data, top_data);
  if (scale_ != Dtype(1)) {
    this->device_->scal(count, scale_, top_data);
  }
  if (shift_ != Dtype(0)) {
    this->device_->add_scalar(count, shift_, top_data);
  }
  if (power_ != Dtype(1)) {
    this->device_->powx(count, top_data, power_, top_data);
  }
  return Dtype(0);
}

template <typename Dtype>
void PowerLayer<Dtype>::Backward(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  if (propagate_down[0]) {
    Dtype* bottom_diff = (*bottom)[0]->mutable_diff();
    const int count = (*bottom)[0]->count();
    const Dtype* top_diff = top[0]->const_diff();
    if (diff_scale_ == Dtype(0) || power_ == Dtype(1)) {
      this->device_->set(count, diff_scale_, bottom_diff);
    } else {
      const Dtype* bottom_data = (*bottom)[0]->const_data();
      // Compute dy/dx = scale * power * (shift + scale * x)^(power - 1)
      //               = diff_scale * y / (shift + scale * x)
      if (power_ == Dtype(2)) {
        // Special case for y = (shift + scale * x)^2
        //     -> dy/dx = 2 * scale * (shift + scale * x)
        //              = diff_scale * shift + diff_scale * scale * x
        this->device_->axpby(count, diff_scale_ * scale_, bottom_data,
            Dtype(0), bottom_diff);
        if (shift_ != Dtype(0)) {
          this->device_->add_scalar(count, diff_scale_ * shift_, bottom_diff);
        }
      } else if (shift_ == Dtype(0)) {
        // Special case for y = (scale * x)^power
        //     -> dy/dx = scale * power * (scale * x)^(power - 1)
        //              = scale * power * (scale * x)^power * (scale * x)^(-1)
        //              = power * y / x
        const Dtype* top_data = top[0]->const_data();
        this->device_->div(count, top_data, bottom_data, bottom_diff);
        this->device_->scal(count, power_, bottom_diff);
      } else {
        this->device_->copy(count, bottom_data, bottom_diff);
        if (scale_ != Dtype(1)) {
          this->device_->scal(count, scale_, bottom_diff);
        }
        if (shift_ != Dtype(0)) {
          this->device_->add_scalar(count, shift_, bottom_diff);
        }
        const Dtype* top_data = top[0]->const_data();
        this->device_->div(count, top_data, bottom_diff, bottom_diff);
        if (diff_scale_ != Dtype(1)) {
          this->device_->scal(count, diff_scale_, bottom_diff);
        }
      }
    }
    if (diff_scale_ != Dtype(0)) {
      this->device_->mul(count, top_diff, bottom_diff, bottom_diff);
    }
  }
}

INSTANTIATE_CLASS(PowerLayer);


}  // namespace caffe
