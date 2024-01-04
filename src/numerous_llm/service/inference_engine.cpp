/* Copyright 2024 Tencent Inc.  All rights reserved.

==============================================================================*/

#include "numerous_llm/service/inference_engine.h"
#include <thread>

namespace numerous_llm {

InferenceEngine::InferenceEngine(Channel<std::pair<Status, Request>> &request_queue) : request_queue_(request_queue) {
  Initialize();
}

InferenceEngine::~InferenceEngine() {
  if (block_manager_) {
    delete block_manager_;
    block_manager_ = nullptr;
  }
}

Status InferenceEngine::Initialize() {
  std::shared_ptr<Environment> env = Singleton<Environment>::GetInstance();
  if (!env) {
    return Status(RET_INVALID_ARGUMENT, "The Environment is nullptr.");
  }

  context_.reset(new Context(env->GetTensorParallelSize(), env->GetPipeLineParallelSize()));

  // Initialize global block manager.
  BlockManagerConfig block_manager_config;
  Status status = env->GetBlockManagerConfig(block_manager_config);
  if (!status.OK()) {
    return Status(RET_INVALID_ARGUMENT, "Get block manager config error:" + status.ToString());
  }
  block_manager_ = new BlockManager(block_manager_config, context_);
  SetBlockManager(block_manager_);

  BatchManagerConfig batch_manager_config;
  status = env->GetBatchManagerConfig(batch_manager_config);
  if (!status.OK()) {
    return Status(RET_INVALID_ARGUMENT, "Get batch manager config error:" + status.ToString());
  }
  batch_manager_ = std::make_shared<BatchManager>(batch_manager_config, context_);

  // Load model instances.
  std::vector<ModelConfig> model_configs;
  status = env->GetModelList(model_configs);
  if (!status.OK()) {
    return Status(RET_INVALID_ARGUMENT, "Get model list error:" + status.ToString());
  }
  NLLM_LOG_INFO << "Get model instance size: " << model_configs.size();

  for (const ModelConfig &model_config : model_configs) {
    std::shared_ptr<ModelInstance> model_instance = std::make_shared<ModelInstance>(model_config, context_);
    model_instance->Load();

    // Register model instance.
    model_instances_.push_back(model_instance);
    batch_manager_->RegisterModelInstance(model_instance);
  }

  return Status();
}

Status InferenceEngine::FetchResult(int64_t req_id, std::vector<std::vector<int>> &tokens) {
  return batch_manager_->FetchResult(req_id, tokens);
}

Status InferenceEngine::HandleRequest(const Request &req) {
  NLLM_LOG_INFO << "Handle request id " << req.req_id << ", batch size " << req.tokens.size();
  Status handle_req_status =
      batch_manager_->Enqueue(req.req_id, req.model_name, req.tokens, req.sampling_configs, req.waiter);
  if (!handle_req_status.OK()) {
    return handle_req_status;
  }
  return Status();
}

Status InferenceEngine::HandleLoop() {
  NLLM_LOG_INFO << "Start handler";

  while (!terminated_) {
    Request req;
    std::pair<Status, Request> req_pair;
    request_queue_.Read(&req_pair);
    if (terminated_) {
      break;
    }

    Status status = req_pair.first;
    req = req_pair.second;

    if (status.GetCode() == RET_TERMINATED) {
      break;
    }

    HandleRequest(req);
  }

  return Status();
}

Status InferenceEngine::StartHandler() {
  handle_thread_ = std::thread(&InferenceEngine::HandleLoop, this);
  return Status();
}

Status InferenceEngine::Start() {
  // Start batch manager.
  batch_manager_->Start();

  // Start service handler.
  StartHandler();

  return Status();
}

Status InferenceEngine::Stop() {
  if (terminated_) {
    return Status();
  }

  terminated_ = true;
  handle_thread_.join();

  // Wait all request done.
  NLLM_LOG_INFO << "Waiting all running request.";
  Status status = batch_manager_->WaitAllDone();
  if (!status.OK()) {
    NLLM_LOG_ERROR << "Wait all requests done error:" << status.ToString();
  }

  // Stop the batch manger.
  NLLM_LOG_INFO << "Stop batch manager.";
  batch_manager_->Stop();

  return Status();
}

}  // namespace numerous_llm