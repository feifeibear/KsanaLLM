/* Copyright 2023 Tencent Inc.  All rights reserved.

==============================================================================*/

#include "ksana_llm/samplers/base/base_sampling.h"

namespace ksana_llm {

Status BaseSampling::Forward(float* logits, const uint32_t* offsets, uint32_t* output_token,
                             const SamplingConfig* sampling_config, SamplingDevideParameter sampling_devide_parameter,
                             const ModelConfig* model_config, Stream& stream) {
  // the same for all type sampling

  // specialized sampling for different type
  STATUS_CHECK_RETURN(
      RunSampling(logits, offsets, output_token, sampling_config, sampling_devide_parameter, model_config, stream));
  return Status();
}

}  // namespace ksana_llm
