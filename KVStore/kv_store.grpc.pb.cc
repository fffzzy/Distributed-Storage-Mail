// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: kv_store.proto

#include "kv_store.pb.h"
#include "kv_store.grpc.pb.h"

#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>

static const char* KVStoreService_method_names[] = {
  "/KVStoreService/Put",
  "/KVStoreService/Get",
  "/KVStoreService/CPut",
  "/KVStoreService/Delete",
};

std::unique_ptr< KVStoreService::Stub> KVStoreService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< KVStoreService::Stub> stub(new KVStoreService::Stub(channel, options));
  return stub;
}

KVStoreService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_Put_(KVStoreService_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_Get_(KVStoreService_method_names[1], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_CPut_(KVStoreService_method_names[2], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_Delete_(KVStoreService_method_names[3], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status KVStoreService::Stub::Put(::grpc::ClientContext* context, const ::KVPutRequest& request, ::KVPutResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::KVPutRequest, ::KVPutResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_Put_, context, request, response);
}

void KVStoreService::Stub::async::Put(::grpc::ClientContext* context, const ::KVPutRequest* request, ::KVPutResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::KVPutRequest, ::KVPutResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Put_, context, request, response, std::move(f));
}

void KVStoreService::Stub::async::Put(::grpc::ClientContext* context, const ::KVPutRequest* request, ::KVPutResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Put_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::KVPutResponse>* KVStoreService::Stub::PrepareAsyncPutRaw(::grpc::ClientContext* context, const ::KVPutRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::KVPutResponse, ::KVPutRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_Put_, context, request);
}

::grpc::ClientAsyncResponseReader< ::KVPutResponse>* KVStoreService::Stub::AsyncPutRaw(::grpc::ClientContext* context, const ::KVPutRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncPutRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status KVStoreService::Stub::Get(::grpc::ClientContext* context, const ::KVGetRequest& request, ::KVGetResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::KVGetRequest, ::KVGetResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_Get_, context, request, response);
}

void KVStoreService::Stub::async::Get(::grpc::ClientContext* context, const ::KVGetRequest* request, ::KVGetResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::KVGetRequest, ::KVGetResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Get_, context, request, response, std::move(f));
}

void KVStoreService::Stub::async::Get(::grpc::ClientContext* context, const ::KVGetRequest* request, ::KVGetResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Get_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::KVGetResponse>* KVStoreService::Stub::PrepareAsyncGetRaw(::grpc::ClientContext* context, const ::KVGetRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::KVGetResponse, ::KVGetRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_Get_, context, request);
}

::grpc::ClientAsyncResponseReader< ::KVGetResponse>* KVStoreService::Stub::AsyncGetRaw(::grpc::ClientContext* context, const ::KVGetRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncGetRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status KVStoreService::Stub::CPut(::grpc::ClientContext* context, const ::KVCPutRequest& request, ::KVCPutResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::KVCPutRequest, ::KVCPutResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_CPut_, context, request, response);
}

void KVStoreService::Stub::async::CPut(::grpc::ClientContext* context, const ::KVCPutRequest* request, ::KVCPutResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::KVCPutRequest, ::KVCPutResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_CPut_, context, request, response, std::move(f));
}

void KVStoreService::Stub::async::CPut(::grpc::ClientContext* context, const ::KVCPutRequest* request, ::KVCPutResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_CPut_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::KVCPutResponse>* KVStoreService::Stub::PrepareAsyncCPutRaw(::grpc::ClientContext* context, const ::KVCPutRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::KVCPutResponse, ::KVCPutRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_CPut_, context, request);
}

::grpc::ClientAsyncResponseReader< ::KVCPutResponse>* KVStoreService::Stub::AsyncCPutRaw(::grpc::ClientContext* context, const ::KVCPutRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncCPutRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status KVStoreService::Stub::Delete(::grpc::ClientContext* context, const ::KVDeleteRequest& request, ::KVDeleteResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::KVDeleteRequest, ::KVDeleteResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_Delete_, context, request, response);
}

void KVStoreService::Stub::async::Delete(::grpc::ClientContext* context, const ::KVDeleteRequest* request, ::KVDeleteResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::KVDeleteRequest, ::KVDeleteResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Delete_, context, request, response, std::move(f));
}

void KVStoreService::Stub::async::Delete(::grpc::ClientContext* context, const ::KVDeleteRequest* request, ::KVDeleteResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Delete_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::KVDeleteResponse>* KVStoreService::Stub::PrepareAsyncDeleteRaw(::grpc::ClientContext* context, const ::KVDeleteRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::KVDeleteResponse, ::KVDeleteRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_Delete_, context, request);
}

::grpc::ClientAsyncResponseReader< ::KVDeleteResponse>* KVStoreService::Stub::AsyncDeleteRaw(::grpc::ClientContext* context, const ::KVDeleteRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncDeleteRaw(context, request, cq);
  result->StartCall();
  return result;
}

KVStoreService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreService::Service, ::KVPutRequest, ::KVPutResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::KVPutRequest* req,
             ::KVPutResponse* resp) {
               return service->Put(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreService_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreService::Service, ::KVGetRequest, ::KVGetResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::KVGetRequest* req,
             ::KVGetResponse* resp) {
               return service->Get(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreService_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreService::Service, ::KVCPutRequest, ::KVCPutResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::KVCPutRequest* req,
             ::KVCPutResponse* resp) {
               return service->CPut(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreService_method_names[3],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreService::Service, ::KVDeleteRequest, ::KVDeleteResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::KVDeleteRequest* req,
             ::KVDeleteResponse* resp) {
               return service->Delete(ctx, req, resp);
             }, this)));
}

KVStoreService::Service::~Service() {
}

::grpc::Status KVStoreService::Service::Put(::grpc::ServerContext* context, const ::KVPutRequest* request, ::KVPutResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status KVStoreService::Service::Get(::grpc::ServerContext* context, const ::KVGetRequest* request, ::KVGetResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status KVStoreService::Service::CPut(::grpc::ServerContext* context, const ::KVCPutRequest* request, ::KVCPutResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status KVStoreService::Service::Delete(::grpc::ServerContext* context, const ::KVDeleteRequest* request, ::KVDeleteResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


