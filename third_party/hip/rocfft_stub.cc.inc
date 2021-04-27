// Auto-generated by tools/stub_codegen/generate.sh. Do not edit.

rocfft_status rocfft_setup() {
  return DynamicCall<decltype(rocfft_setup), &rocfft_setup>("rocfft_setup");
}

rocfft_status rocfft_cleanup() {
  return DynamicCall<decltype(rocfft_cleanup), &rocfft_cleanup>(
      "rocfft_cleanup");
}

rocfft_status rocfft_plan_create(rocfft_plan* plan,
                                 rocfft_result_placement placement,
                                 rocfft_transform_type transform_type,
                                 rocfft_precision precision, size_t dimensions,
                                 const size_t* lengths,
                                 size_t number_of_transforms,
                                 const rocfft_plan_description description) {
  return DynamicCall<decltype(rocfft_plan_create), &rocfft_plan_create>(
      "rocfft_plan_create", plan, placement, transform_type, precision,
      dimensions, lengths, number_of_transforms, description);
}

rocfft_status rocfft_execute(const rocfft_plan plan, void* in_buffer[],
                             void* out_buffer[], rocfft_execution_info info) {
  return DynamicCall<decltype(rocfft_execute), &rocfft_execute>(
      "rocfft_execute", plan, in_buffer, out_buffer, info);
}

rocfft_status rocfft_plan_destroy(rocfft_plan plan) {
  return DynamicCall<decltype(rocfft_plan_destroy), &rocfft_plan_destroy>(
      "rocfft_plan_destroy", plan);
}

rocfft_status rocfft_plan_description_set_data_layout(
    rocfft_plan_description description, const rocfft_array_type in_array_type,
    const rocfft_array_type out_array_type, const size_t* in_offsets,
    const size_t* out_offsets, const size_t in_strides_size,
    const size_t* in_strides, const size_t in_distance,
    const size_t out_strides_size, const size_t* out_strides,
    const size_t out_distance) {
  return DynamicCall<decltype(rocfft_plan_description_set_data_layout),
                     &rocfft_plan_description_set_data_layout>(
      "rocfft_plan_description_set_data_layout", description, in_array_type,
      out_array_type, in_offsets, out_offsets, in_strides_size, in_strides,
      in_distance, out_strides_size, out_strides, out_distance);
}

rocfft_status rocfft_get_version_string(char* buf, size_t len) {
  return DynamicCall<decltype(rocfft_get_version_string),
                     &rocfft_get_version_string>("rocfft_get_version_string",
                                                 buf, len);
}

rocfft_status rocfft_plan_get_work_buffer_size(const rocfft_plan plan,
                                               size_t* size_in_bytes) {
  return DynamicCall<decltype(rocfft_plan_get_work_buffer_size),
                     &rocfft_plan_get_work_buffer_size>(
      "rocfft_plan_get_work_buffer_size", plan, size_in_bytes);
}

rocfft_status rocfft_plan_get_print(const rocfft_plan plan) {
  return DynamicCall<decltype(rocfft_plan_get_print), &rocfft_plan_get_print>(
      "rocfft_plan_get_print", plan);
}

rocfft_status rocfft_plan_description_create(
    rocfft_plan_description* description) {
  return DynamicCall<decltype(rocfft_plan_description_create),
                     &rocfft_plan_description_create>(
      "rocfft_plan_description_create", description);
}

rocfft_status rocfft_plan_description_destroy(
    rocfft_plan_description description) {
  return DynamicCall<decltype(rocfft_plan_description_destroy),
                     &rocfft_plan_description_destroy>(
      "rocfft_plan_description_destroy", description);
}

rocfft_status rocfft_execution_info_create(rocfft_execution_info* info) {
  return DynamicCall<decltype(rocfft_execution_info_create),
                     &rocfft_execution_info_create>(
      "rocfft_execution_info_create", info);
}

rocfft_status rocfft_execution_info_destroy(rocfft_execution_info info) {
  return DynamicCall<decltype(rocfft_execution_info_destroy),
                     &rocfft_execution_info_destroy>(
      "rocfft_execution_info_destroy", info);
}

rocfft_status rocfft_execution_info_set_work_buffer(
    rocfft_execution_info info, void* work_buffer, const size_t size_in_bytes) {
  return DynamicCall<decltype(rocfft_execution_info_set_work_buffer),
                     &rocfft_execution_info_set_work_buffer>(
      "rocfft_execution_info_set_work_buffer", info, work_buffer,
      size_in_bytes);
}

rocfft_status rocfft_execution_info_set_stream(rocfft_execution_info info,
                                               void* stream) {
  return DynamicCall<decltype(rocfft_execution_info_set_stream),
                     &rocfft_execution_info_set_stream>(
      "rocfft_execution_info_set_stream", info, stream);
}