// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: kvstore.proto

#include "kvstore.pb.h"
#include "kvstore.grpc.pb.h"

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

static const char* KVStoreMaster_method_names[] = {
  "/KVStoreMaster/FetchNodeAddr",
};

std::unique_ptr< KVStoreMaster::Stub> KVStoreMaster::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< KVStoreMaster::Stub> stub(new KVStoreMaster::Stub(channel, options));
  return stub;
}

KVStoreMaster::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_FetchNodeAddr_(KVStoreMaster_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status KVStoreMaster::Stub::FetchNodeAddr(::grpc::ClientContext* context, const ::FetchNodeRequest& request, ::FetchNodeResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::FetchNodeRequest, ::FetchNodeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_FetchNodeAddr_, context, request, response);
}

void KVStoreMaster::Stub::async::FetchNodeAddr(::grpc::ClientContext* context, const ::FetchNodeRequest* request, ::FetchNodeResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::FetchNodeRequest, ::FetchNodeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_FetchNodeAddr_, context, request, response, std::move(f));
}

void KVStoreMaster::Stub::async::FetchNodeAddr(::grpc::ClientContext* context, const ::FetchNodeRequest* request, ::FetchNodeResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_FetchNodeAddr_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::FetchNodeResponse>* KVStoreMaster::Stub::PrepareAsyncFetchNodeAddrRaw(::grpc::ClientContext* context, const ::FetchNodeRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::FetchNodeResponse, ::FetchNodeRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_FetchNodeAddr_, context, request);
}

::grpc::ClientAsyncResponseReader< ::FetchNodeResponse>* KVStoreMaster::Stub::AsyncFetchNodeAddrRaw(::grpc::ClientContext* context, const ::FetchNodeRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncFetchNodeAddrRaw(context, request, cq);
  result->StartCall();
  return result;
}

KVStoreMaster::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreMaster_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreMaster::Service, ::FetchNodeRequest, ::FetchNodeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreMaster::Service* service,
             ::grpc::ServerContext* ctx,
             const ::FetchNodeRequest* req,
             ::FetchNodeResponse* resp) {
               return service->FetchNodeAddr(ctx, req, resp);
             }, this)));
}

KVStoreMaster::Service::~Service() {
}

::grpc::Status KVStoreMaster::Service::FetchNodeAddr(::grpc::ServerContext* context, const ::FetchNodeRequest* request, ::FetchNodeResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


static const char* KVStoreNode_method_names[] = {
  "/KVStoreNode/Execute",
  "/KVStoreNode/CheckHealth",
};

std::unique_ptr< KVStoreNode::Stub> KVStoreNode::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< KVStoreNode::Stub> stub(new KVStoreNode::Stub(channel, options));
  return stub;
}

KVStoreNode::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_Execute_(KVStoreNode_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_CheckHealth_(KVStoreNode_method_names[1], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status KVStoreNode::Stub::Execute(::grpc::ClientContext* context, const ::KVRequest& request, ::KVResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::KVRequest, ::KVResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_Execute_, context, request, response);
}

void KVStoreNode::Stub::async::Execute(::grpc::ClientContext* context, const ::KVRequest* request, ::KVResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::KVRequest, ::KVResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Execute_, context, request, response, std::move(f));
}

void KVStoreNode::Stub::async::Execute(::grpc::ClientContext* context, const ::KVRequest* request, ::KVResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Execute_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::KVResponse>* KVStoreNode::Stub::PrepareAsyncExecuteRaw(::grpc::ClientContext* context, const ::KVRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::KVResponse, ::KVRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_Execute_, context, request);
}

::grpc::ClientAsyncResponseReader< ::KVResponse>* KVStoreNode::Stub::AsyncExecuteRaw(::grpc::ClientContext* context, const ::KVRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncExecuteRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status KVStoreNode::Stub::CheckHealth(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_CheckHealth_, context, request, response);
}

void KVStoreNode::Stub::async::CheckHealth(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_CheckHealth_, context, request, response, std::move(f));
}

void KVStoreNode::Stub::async::CheckHealth(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_CheckHealth_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* KVStoreNode::Stub::PrepareAsyncCheckHealthRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_CheckHealth_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* KVStoreNode::Stub::AsyncCheckHealthRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncCheckHealthRaw(context, request, cq);
  result->StartCall();
  return result;
}

KVStoreNode::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreNode_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreNode::Service, ::KVRequest, ::KVResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreNode::Service* service,
             ::grpc::ServerContext* ctx,
             const ::KVRequest* req,
             ::KVResponse* resp) {
               return service->Execute(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KVStoreNode_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< KVStoreNode::Service, ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](KVStoreNode::Service* service,
             ::grpc::ServerContext* ctx,
             const ::google::protobuf::Empty* req,
             ::google::protobuf::Empty* resp) {
               return service->CheckHealth(ctx, req, resp);
             }, this)));
}

KVStoreNode::Service::~Service() {
}

::grpc::Status KVStoreNode::Service::Execute(::grpc::ServerContext* context, const ::KVRequest* request, ::KVResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status KVStoreNode::Service::CheckHealth(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


