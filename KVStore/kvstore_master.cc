#include "kvstore_master.h"

namespace KVStore {
namespace {
using ::google::protobuf::Empty;
using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

const int kMessageSizeLimit = 1024 * 1024 * 1024;

unsigned long long click = 0;

// Verbose mode.
bool verbose_ = false;

void VerboseLog(std::string msg) {
  if (verbose_) {
    fprintf(stdout, "%s\n", msg.c_str());
  }
}

// Periodically check the health status of replicas.
void* ThreadFunc(void* args) {
  std::vector<Cluster>* clusters = (std::vector<Cluster>*)args;

  while (true) {
    VerboseLog("[Health Check] start checking " + std::to_string(click++) +
               " ... ... ");

    for (auto& cluster : (*clusters)) {
      // Check health of each replica node within a cluster.
      for (const auto& it : cluster.stubs) {
        ClientContext context;
        KVResponse check_health_res;
        Status res =
            it.second->CheckHealth(&context, Empty(), &check_health_res);

        pthread_mutex_lock(&cluster.lock);
        if (res.ok()) {
          if (check_health_res.status() == KVStatusCode::SUCCESS) {
            auto curr_status = cluster.status[it.first];
            if (curr_status == RUNNING) {
              // No status change.
              VerboseLog("[Health Check] node " + it.first + " in Cluster-" +
                         std::to_string(cluster.id) + " is RUNNING ... ");
            } else if (curr_status == SUSPENDED) {
              // Impossible.
              fprintf(
                  stderr,
                  "[Health Check] node %s in Cluster-%d was suspended before. "
                  "Impossible to become RUNNING without recovering.\n",
                  it.first.c_str(), cluster.id);
              abort();
            } else if (curr_status == RECOVERING) {
              cluster.status[it.first] = RUNNING;
              VerboseLog("[Health Check] node " + it.first + " in Cluster-" +
                         std::to_string(cluster.id) + " is RUNNING ... ");
            } else if (curr_status == CRASHED) {
              if (cluster.primary.empty()) {
                // Consider first node as init node.
                cluster.primary = it.first;
                cluster.status[it.first] = RUNNING;
              } else if (cluster.primary.compare(it.first) == 0) {
                // Impossible.
                fprintf(stderr,
                        "[Health Check] previous crased node cannot be a "
                        "primary node.\n");
                abort();
              } else {
                // Entering recovery mode.
                ClientContext ctx;
                KVRequest req;
                KVResponse res;
                req.mutable_recovery_request()->set_target_addr(it.first);
                Status grpc_status =
                    cluster.stubs[cluster.primary]->Execute(&ctx, req, &res);
                if (grpc_status.ok() && res.status() == KVStatusCode::SUCCESS) {
                  cluster.status[it.first] = RECOVERING;
                  VerboseLog("[Health Check] node " + it.first +
                             " in Cluster " + std::to_string(cluster.id) +
                             "is RECOVERING ... ");
                } else {
                  VerboseLog(
                      "[Health Check] recovery request is denied by primary "
                      "node, waiting for next round ... ");
                }
              }
            }
          } else if (check_health_res.status() == KVStatusCode::SUSPENDED) {
            auto curr_status = cluster.status[it.first];
            if (curr_status == RUNNING) {
              // Impossible.
              fprintf(stderr,
                      "[Health Check] node %s in Cluster-%d next status is "
                      "SUSPENDED but current status is RUNNING. Status changes "
                      "from RUNNINg to SUSPENDED should be done during Suspend "
                      "request from Console.\n",
                      it.first.c_str(), cluster.id);
              abort();
            } else if (curr_status == SUSPENDED) {
              VerboseLog("[Health Check] node " + it.first + " in Cluster-" +
                         std::to_string(cluster.id) + " is SUSPENDED ... ");
            } else if (curr_status == RECOVERING) {
              // Impossible.
              fprintf(stderr,
                      "[Health Check] node %s in Cluster-%d next status is "
                      "SUSPENDED but current status is RECOVERING. Impossible "
                      "case.\n",
                      it.first.c_str(), cluster.id);
              abort();
            } else if (curr_status == CRASHED) {
              // Impossible.
              fprintf(stderr,
                      "[Health Check] node %s in Cluster-%d next status is "
                      "SUSPENDED but current status is CRASHED. Impossible "
                      "case.\n",
                      it.first.c_str(), cluster.id);
              abort();
            }
          } else if (check_health_res.status() == KVStatusCode::RECOVERING) {
            auto curr_status = cluster.status[it.first];
            if (curr_status == RUNNING) {
              cluster.status[it.first] = RECOVERING;
              VerboseLog("[Health Check] node " + it.first + " in Cluster-" +
                         std::to_string(cluster.id) + " is RECOVERING ... ");
            } else if (curr_status == SUSPENDED) {
              // Impossible.
              fprintf(stderr,
                      "[Health Check] node %s in Cluster-%d next status is "
                      "RECOVERING but current status is SUSPENDED. Suspend to "
                      "recovering should be done in Revive request.\n",
                      it.first.c_str(), cluster.id);
              abort();
            } else if (curr_status == RECOVERING) {
              VerboseLog("[Health Check] node " + it.first + " in Cluster-" +
                         std::to_string(cluster.id) + " is RECOVERING ... ");
            } else if (curr_status == CRASHED) {
              // Impossible.
              fprintf(stderr,
                      "[Health Check] node %s in Cluster-%d next status is "
                      "RECOVERING but current status is CRASHED. Crashed to "
                      "recovering should be done when first time master node "
                      "hears back from replica nodes.\n",
                      it.first.c_str(), cluster.id);
              abort();
            }
          }
        } else {
          // GRPC failed, node becomes crashed.
          cluster.status[it.first] = CRASHED;
          VerboseLog("[Health Check] node " + it.first + " in Cluster-" +
                     std::to_string(cluster.id) + " is CRASHED ... ");

          if (cluster.primary.compare(it.first) == 0) {
            cluster.primary.clear();
            // Find next available node immediately.
            for (const auto& entry : cluster.status) {
              if (entry.second == RUNNING) {
                cluster.primary = entry.first;
                VerboseLog("[Health Check] primay node in Cluster-" +
                           std::to_string(cluster.id) + " changes to " +
                           entry.first);
                break;
              }
            }
          }
        }
        pthread_mutex_unlock(&cluster.lock);
      }

      if (cluster.primary.empty()) {
        VerboseLog("[Health Check] no KVStore nodes in Cluster-" +
                   std::to_string(cluster.id) + " is available now ... ");
      } else {
        VerboseLog("[Health Check] primary node in Cluster-" +
                   std::to_string(cluster.id) + " is " + cluster.primary);
      }
    }
    VerboseLog("\n");
    sleep(1);
  }
}

}  // namespace

KVStoreMasterImpl::KVStoreMasterImpl() {}

void KVStoreMasterImpl::ReadConfig() {
  FILE* file = fopen(kServerlistPath, "r");
  if (file == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", kServerlistPath,
            strerror(errno));
    exit(-1);
  }

  char line[1000];
  int count = 0;
  while (fgets(line, sizeof(line), file)) {
    char* addr_line = strtok(line, ",\r\n");
    std::string addr_str = std::string(addr_line, strlen(addr_line));
    if (count == 0) {
      master_addr_ = addr_str;
    } else {
      int cluster_id = (count - 1) / kNumOfReplicas;
      if (count % kNumOfReplicas == 1) {
        clusters_.push_back(Cluster());
        clusters_[cluster_id].id = cluster_id;
      }
      clusters_[cluster_id].status[addr_str] = CRASHED;
      clusters_[cluster_id].stubs[addr_str] = KVStoreNode::NewStub(
          grpc::CreateChannel(addr_str, grpc::InsecureChannelCredentials()));

      ChannelArguments args;
      // 5GB incoming message size.
      args.SetMaxReceiveMessageSize(kMessageSizeLimit);
      clusters_[cluster_id].stubs[addr_str] =
          KVStoreNode::NewStub(grpc::CreateCustomChannel(
              addr_str, grpc::InsecureChannelCredentials(), args));
    }
    count++;
  }

  if ((count - 1) % kNumOfReplicas != 0) {
    fprintf(stderr,
            "Server list should have one master node and %d number of "
            "replicas for each cluster, but have %d nodes in total \n",
            kNumOfReplicas, count);
    exit(-1);
  }

  if (master_addr_.empty()) {
    fprintf(stderr, "No master address is found.");
    exit(-1);
  }
  fclose(file);

  // Show cluster information.
  if (verbose_) {
    fprintf(stderr, "========== Cluster Informations ==========\n");
    for (auto const& cluster : clusters_) {
      fprintf(stderr, "Cluster-%d primary node: %s\n", cluster.id,
              cluster.primary.c_str());
      for (auto const& it : cluster.stubs) {
        std::cout << "Replica node: " << it.first << std::endl;
      }
    }
    fprintf(stderr, "==========================================\n\n");
  }
}

void KVStoreMasterImpl::CheckReplicaHealth() {
  pthread_t health_checker_thread;
  if (pthread_create(&health_checker_thread, NULL, &ThreadFunc, &clusters_) <
      0) {
    fprintf(stderr,
            "Failed to create a pthread to periodically check the health of "
            "replica nodes.\n");
    exit(-1);
  };
}

std::string KVStoreMasterImpl::GetAddr() { return master_addr_; }

Status KVStoreMasterImpl::FetchNodeAddr(ServerContext* context,
                                        const FetchNodeRequest* request,
                                        FetchNodeResponse* response) {
  int hash_code = GetDigest(request->row(), request->col()) % clusters_.size();
  pthread_mutex_lock(&clusters_[hash_code].lock);
  std::string primary_addr = clusters_[hash_code].primary;
  pthread_mutex_unlock(&clusters_[hash_code].lock);
  if (primary_addr.empty()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_error_message("-ERR service unavailable.");
  } else {
    response->set_status(KVStatusCode::SUCCESS);
    response->set_addr(primary_addr);
  }
  return Status::OK;
}

Status KVStoreMasterImpl::NotifyRecoveryFinished(
    ServerContext* context, const NotifyRecoveryFinishedRequest* request,
    KVResponse* response) {
  std::string target_addr = request->target_addr();

  bool found = false;
  for (auto& cluster : clusters_) {
    bool found = false;
    pthread_mutex_lock(&cluster.lock);
    for (auto& node : cluster.status) {
      if (node.first.compare(target_addr) == 0) {
        found = true;
        assert(node.second == RECOVERING);
        node.second = RUNNING;
        response->set_status(KVStatusCode::SUCCESS);
        break;
      }
    }
    pthread_mutex_unlock(&cluster.lock);
    if (found) {
      break;
    }
  }

  if (!found) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("-ERR " + target_addr + " not found.");
  }
  return Status::OK;
}

Status KVStoreMasterImpl::PollStatus(ServerContext* context,
                                     const PollStatusRequest* request,
                                     PollStatusResponse* response) {
  for (auto& cluster : clusters_) {
    pthread_mutex_lock(&cluster.lock);
    PollStatusResponse_Cluster* cluster_info = response->add_clusters();
    for (auto& it : cluster.status) {
      PollStatusResponse_Cluster_Node* node_info = cluster_info->add_nodes();
      if (it.first.compare(cluster.primary) == 0) {
        node_info->set_is_primary(true);
      } else {
        node_info->set_is_primary(false);
      }
      node_info->set_address(it.first);
      switch (it.second) {
        case RUNNING: {
          node_info->set_state("RUNNING");
          break;
        }
        case SUSPENDED: {
          node_info->set_state("SUSPENDED");
          break;
        }
        case RECOVERING: {
          node_info->set_state("RECOVERING");
          break;
        }
        case CRASHED: {
          node_info->set_state("CRASHED");
          break;
        }
      }
    }
    pthread_mutex_unlock(&cluster.lock);
  }
  return Status::OK;
};

Status KVStoreMasterImpl::Suspend(ServerContext* context,
                                  const SuspendRequest* request,
                                  KVResponse* response) {
  int cluster_id = request->cluster_id();
  std::string node_addr = request->node_addr();

  bool found = false;
  pthread_mutex_lock(&clusters_[cluster_id].lock);
  for (auto& it : clusters_[cluster_id].status) {
    if (it.first.compare(node_addr) == 0) {
      found = true;
      ClientContext context;
      KVRequest req;
      KVResponse res;
      req.mutable_suspend_request()->set_target_addr(node_addr);
      Status grpc_status =
          clusters_[cluster_id].stubs[node_addr]->Execute(&context, req, &res);
      if (grpc_status.ok()) {
        it.second = SUSPENDED;
        response->set_status(KVStatusCode::SUCCESS);
        if (clusters_[cluster_id].primary.compare(node_addr) == 0) {
          for (auto& entry : clusters_[cluster_id].status) {
            if (entry.second == RUNNING) {
              clusters_[cluster_id].primary = entry.first;
              break;
            }
          }
        }
      } else {
        response->set_status(KVStatusCode::FAILURE);
        response->set_message(res.message());
      }
      break;
    }
  }
  pthread_mutex_unlock(&clusters_[cluster_id].lock);

  if (!found) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message(node_addr + " not found.");
  }
  return Status::OK;
};

Status KVStoreMasterImpl::Revive(ServerContext* context,
                                 const ReviveRequest* request,
                                 KVResponse* response) {
  int cluster_id = request->cluster_id();
  std::string node_addr = request->node_addr();

  bool found = false;
  pthread_mutex_lock(&clusters_[cluster_id].lock);
  std::string primary_node = clusters_[cluster_id].primary;
  for (auto& it : clusters_[cluster_id].status) {
    if (it.first.compare(node_addr) == 0) {
      found = true;
      if (primary_node.compare(it.first) == 0) {
        fprintf(stderr, "Impossible to revive primary node.\n");
        response->set_status(KVStatusCode::FAILURE);
        response->set_message("Impossible to revive primary node.");
        return Status::OK;
      }

      if (it.second != SUSPENDED) {
        fprintf(stderr,
                "cannot revivie a node which is not in SUSPENDED status.\n");
        response->set_status(KVStatusCode::FAILURE);
        response->set_message(
            "cannot revivie a node which is not in SUSPENDED status.");
        return Status::OK;
      }

      ClientContext context;
      KVRequest req;
      KVResponse res;
      req.mutable_recovery_request()->set_target_addr(node_addr);

      Status grpc_status = clusters_[cluster_id].stubs[primary_node]->Execute(
          &context, req, &res);
      if (grpc_status.ok()) {
        if (res.status() == KVStatusCode::SUCCESS) {
          it.second = RECOVERING;
          response->set_status(KVStatusCode::SUCCESS);
          VerboseLog("[Revive] reviving node " + node_addr + " in Cluster-" +
                     std::to_string(cluster_id));
        } else {
          response->set_status(KVStatusCode::FAILURE);
          response->set_message(res.message());
        }
      } else {
        response->set_status(KVStatusCode::FAILURE);
        response->set_message("Primary node " + primary_node +
                              " isn't responding ... \n");
      }
      break;
    }
  }
  pthread_mutex_unlock(&clusters_[cluster_id].lock);

  if (!found) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message(node_addr + " not found.");
  }
  return Status::OK;
};

}  // namespace KVStore

int main(int argc, char** argv) {
  /* read from command line */
  int opt;
  while ((opt = getopt(argc, argv, "v")) != -1) {
    switch (opt) {
      case 'v':
        KVStore::verbose_ = true;
        break;
      default:
        fprintf(stderr, "[Command Line Format] ./kvstore_node [-v]\n");
        exit(-1);
        break;
    }
  }

  KVStore::KVStoreMasterImpl master;
  master.ReadConfig();

  master.CheckReplicaHealth();

  ::grpc::ServerBuilder builder;
  // 1 GB incoming message size.
  builder.SetMaxReceiveMessageSize(KVStore::kMessageSizeLimit);

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(master.GetAddr(), grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&master);
  // Finally assemble the server.
  std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << master.GetAddr() << std::endl
            << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return 0;
}