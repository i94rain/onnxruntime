// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/rocm/math/gemm.h"

#include "core/providers/cpu/math/gemm_helper.h"
#include "core/providers/rocm/rocm_common.h"
#include "core/providers/rocm/shared_inc/fpgeneric.h"
#include "core/providers/rocm/tunable/gemm.h"


namespace onnxruntime {
namespace rocm {

using tunable::blas::BlasOp;

#define REGISTER_KERNEL_TYPED(T)                                  \
  ONNX_OPERATOR_VERSIONED_TYPED_KERNEL_EX(                        \
      Gemm,                                                       \
      kOnnxDomain,                                                \
      7,                                                          \
      8,                                                          \
      T,                                                          \
      kRocmExecutionProvider,                                     \
      (*KernelDefBuilder::Create())                               \
          .TypeConstraint("T", DataTypeImpl::GetTensorType<T>()), \
      Gemm<T>);                                                   \
  ONNX_OPERATOR_VERSIONED_TYPED_KERNEL_EX(                        \
      Gemm,                                                       \
      kOnnxDomain,                                                \
      9,                                                          \
      10,                                                         \
      T,                                                          \
      kRocmExecutionProvider,                                     \
      (*KernelDefBuilder::Create())                               \
          .TypeConstraint("T", DataTypeImpl::GetTensorType<T>()), \
      Gemm<T>);                                                   \
  ONNX_OPERATOR_VERSIONED_TYPED_KERNEL_EX(                        \
      Gemm,                                                       \
      kOnnxDomain,                                                \
      11,                                                         \
      12,                                                         \
      T,                                                          \
      kRocmExecutionProvider,                                     \
      (*KernelDefBuilder::Create())                               \
          .TypeConstraint("T", DataTypeImpl::GetTensorType<T>()), \
      Gemm<T>);                                                   \
  ONNX_OPERATOR_TYPED_KERNEL_EX(                                  \
      Gemm,                                                       \
      kOnnxDomain,                                                \
      13,                                                         \
      T,                                                          \
      kRocmExecutionProvider,                                     \
      (*KernelDefBuilder::Create())                               \
          .TypeConstraint("T", DataTypeImpl::GetTensorType<T>()), \
      Gemm<T>);

REGISTER_KERNEL_TYPED(float)
REGISTER_KERNEL_TYPED(double)
REGISTER_KERNEL_TYPED(MLFloat16)
REGISTER_KERNEL_TYPED(BFloat16)

template <typename T>
Status Gemm<T>::ComputeInternal(OpKernelContext* ctx) const {
  typedef typename ToHipType<T>::MappedType HipT;

  const auto* X = ctx->Input<Tensor>(0);
  const auto* W = ctx->Input<Tensor>(1);
  const auto* B = ctx->Input<Tensor>(2);
  // Bias could be missing. Treat as scalar 0 if that is the case.
  GemmHelper helper(X->Shape(), trans_A_, W->Shape(), trans_B_, B != nullptr ? B->Shape() : TensorShape({}));

  if (!helper.State().IsOK())
    return helper.State();

  int M = gsl::narrow_cast<int>(helper.M());
  int N = gsl::narrow_cast<int>(helper.N());
  int K = gsl::narrow_cast<int>(helper.K());
  auto* Y = ctx->Output(0, {M, N});
  HipT* out_data = reinterpret_cast<HipT*>(Y->MutableData<T>());

  HipT one = ToHipType<T>::FromFloat(1.0f);
  HipT zero = ToHipType<T>::FromFloat(0.0f);

  // broadcast bias if needed and is present
  if (beta_ != 0 && B != nullptr) {
    auto& b_shape = B->Shape();
    const HipT* b_data = reinterpret_cast<const HipT*>(B->Data<T>());

    if (b_shape.Size() == 1) {
      // if B is (), (1,) or (1, 1), broadcast the scalar
      ROCBLAS_RETURN_IF_ERROR(rocblasCopyHelper(
          Stream(),
          RocblasHandle(),
          M * N,
          b_data,
          0,
          out_data,
          1));
    } else if (b_shape.NumDimensions() == 1 || b_shape[0] == 1) {
      // B is (N,) or (1, N), broadcast using Y(N,M) = 1 * B(N,1) x ones(1,M) + 0 * Y
      ROCBLAS_RETURN_IF_ERROR(rocblasGemmHelper(
          RocblasHandle(),
          rocblas_operation_none,
          rocblas_operation_none,
          N, M, 1,
          /*alpha*/ &one,
          b_data, N,
          GetConstOnes<HipT>(M), 1,
          /*beta*/ &zero,
          out_data, N));
    } else if (b_shape.NumDimensions() == 2 && b_shape[1] == 1) {
      // B is (M, 1), broadcast using Y(N,M) = 1 * ones(N,1) x B(1,M) + 0 * Y
      ROCBLAS_RETURN_IF_ERROR(rocblasGemmHelper(
          RocblasHandle(),
          rocblas_operation_none,
          rocblas_operation_none,
          N, M, 1,
          /*alpha*/ &one,
          GetConstOnes<HipT>(N), N,
          b_data, 1,
          /*beta*/ &zero,
          out_data, N));
    } else {
      // B is (M, N), no broadcast needed.
      HIP_RETURN_IF_ERROR(hipMemcpyAsync(out_data, b_data, M * N * sizeof(T), hipMemcpyDeviceToDevice, Stream()));
    }
  }

  return tunable::blas::column_major::Gemm(
      IsTunableOpEnabled(), Stream(),
      RocblasHandle(),
      trans_B_ ? BlasOp::Trans : BlasOp::NonTrans,
      trans_A_ ? BlasOp::Trans : BlasOp::NonTrans,
      N, M, K,
      alpha_,
      reinterpret_cast<const HipT*>(W->Data<T>()),
      (trans_B_ ? K : N),
      reinterpret_cast<const HipT*>(X->Data<T>()),
      (trans_A_ ? M : K),
      // ideally we need to set the output buffer contents to 0 if bias is missing,
      // but passing 0 for beta is cheaper and it will ignore any junk in the output buffer
      B != nullptr ? beta_ : 0.0f,
      out_data, N);
}

}  // namespace rocm
}  // namespace onnxruntime
