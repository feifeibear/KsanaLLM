/* Copyright 2024 Tencent Inc.  All rights reserved.

==============================================================================*/
#pragma once

#include <torch/script.h>

#include "numerous_llm/endpoints/local/local_endpoint.h"
#include "numerous_llm/service/inference_engine.h"
#include "numerous_llm/utils/channel.h"

namespace numerous_llm {

// The serving implementation.
class ServingImpl {
 public:
  ServingImpl();
  ~ServingImpl() {}

  // Start the inference server.
  Status Start();

  // Stop the inference server.
  Status Stop();

  // Handle serving request.
  Status Handle(const std::string &model_name, const std::vector<std::vector<int>> &tokens,
                const std::vector<SamplingConfig> &sampling_configs, std::vector<std::vector<int>> &output_tokens);

 private:
  // The inference engine.
  std::shared_ptr<InferenceEngine> inference_engine_ = nullptr;

  // The rpc endpoint of this service.
  std::shared_ptr<LocalEndpoint> endpoint_ = nullptr;

  // channel for endpoint and inference server
  Channel<std::pair<Status, Request>> request_queue_;
};

}  // namespace numerous_llm