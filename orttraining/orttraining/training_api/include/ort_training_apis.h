// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace OrtTrainingApis {

ORT_API(const OrtTrainingApi*, GetTrainingApi, uint32_t version);

ORT_API_STATUS_IMPL(CreateTrainingSession, _In_ const OrtEnv* env, _In_ const OrtSessionOptions* options,
                    _Inout_ OrtCheckpointState* checkpoint_state, _In_ const ORTCHAR_T* train_model_path,
                    _In_ const ORTCHAR_T* eval_model_path, _In_ const ORTCHAR_T* optimizer_model_path,
                    _Outptr_ OrtTrainingSession** out);

ORT_API_STATUS_IMPL(TrainingSessionGetTrainingModelOutputCount, _In_ const OrtTrainingSession* sess, _Out_ size_t* out);

ORT_API_STATUS_IMPL(TrainingSessionGetEvalModelOutputCount, _In_ const OrtTrainingSession* sess, _Out_ size_t* out);

ORT_API_STATUS_IMPL(TrainingSessionGetTrainingModelOutputName, _In_ const OrtSession* sess, size_t index,
                    _Inout_ OrtAllocator* allocator, _Outptr_ char** output);

ORT_API_STATUS_IMPL(TrainingSessionGetEvalModelOutputName, _In_ const OrtSession* sess, size_t index,
                    _Inout_ OrtAllocator* allocator, _Outptr_ char** output);

ORT_API_STATUS_IMPL(ResetGrad, _Inout_ OrtTrainingSession* session);

ORT_API_STATUS_IMPL(TrainStep, _Inout_ OrtTrainingSession* session, _In_opt_ const OrtRunOptions* run_options,
                    size_t inputs_len, _In_reads_(input_len) const OrtValue* const* inputs,
                    size_t outputs_len, _Inout_updates_all_(outputs_len) OrtValue** outputs);

ORT_API_STATUS_IMPL(EvalStep, _In_ const OrtTrainingSession* session, _In_opt_ const OrtRunOptions* run_options,
                    size_t inputs_len, _In_reads_(input_len) const OrtValue* const* inputs,
                    size_t outputs_len, _Inout_updates_all_(outputs_len) OrtValue** outputs);

ORT_API_STATUS_IMPL(SetLearningRate, _Inout_ OrtTrainingSession* sess, _In_ float learning_rate);

ORT_API_STATUS_IMPL(GetLearningRate, _Inout_ OrtTrainingSession* sess, _Out_ float* learning_rate);

ORT_API_STATUS_IMPL(OptimizerStep, _Inout_ OrtTrainingSession* session, _In_opt_ const OrtRunOptions* run_options);

ORT_API_STATUS_IMPL(RegisterLinearLRScheduler, _Inout_ OrtTrainingSession* sess, _In_ const int64_t warmup_step_count,
                    _In_ const int64_t total_step_count, _In_ const float initial_lr);

ORT_API_STATUS_IMPL(SchedulerStep, _Inout_ OrtTrainingSession* sess);

ORT_API_STATUS_IMPL(LoadCheckpoint, _In_ const ORTCHAR_T* checkpoint_path,
                    _Outptr_ OrtCheckpointState** checkpoint_state);

ORT_API_STATUS_IMPL(SaveCheckpoint, _In_ const ORTCHAR_T* checkpoint_path, _In_ const OrtTrainingSession* session,
                    bool save_optimizer_state);

ORT_API_STATUS_IMPL(GetParametersSize, _Inout_ OrtTrainingSession* sess,
                    _Out_ size_t* out, bool trainable_only);

ORT_API_STATUS_IMPL(CopyParametersToBuffer, _Inout_ OrtTrainingSession* sess,
                    _Inout_ OrtValue* parameters_buffer, bool trainable_only);

ORT_API_STATUS_IMPL(CopyBufferToParameters, _Inout_ OrtTrainingSession* sess,
                    _Inout_ OrtValue* parameters_buffer, bool trainable_only);

ORT_API(void, ReleaseCheckpointState, _Frees_ptr_opt_ OrtCheckpointState* session);

ORT_API(void, ReleaseTrainingSession, _Frees_ptr_opt_ OrtTrainingSession* session);

ORT_API_STATUS_IMPL(ExportModelForInferencing, _Inout_ OrtTrainingSession* sess,
                    _In_ const ORTCHAR_T* inference_model_path, size_t graph_outputs_len,
                    _In_reads_(graph_outputs_len) const char* const* graph_output_names);

}  // namespace OrtTrainingApis
