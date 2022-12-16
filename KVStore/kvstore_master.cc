#include "kvstore_master.h"

namespace KVStore {
namespace {
using ::google::protobuf::Empty;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Verbose mode.
bool verbose_ = false;

// Acquire lock to access cluster information.
pthread_mutex_t cluster_lock_;

// Periodically check the health status of replicas.
void* ThreadFunc(void* args) {
  std::vector<Cluster>* clusters = (std::vector<Cluster>*)args;

  while (true) {
    pthread_mutex_lock(&cluster_lock_);
    if (verbose_) {
      fprintf(stderr, "[Health Check] start checking ... ... \n");
    }

    for (auto& cluster : (*clusters)) {
      auto& isAlive = cluster.isAlive;
      // Check health of each replica node within a cluster.
      for (const auto& it : cluster.stubs) {
        ClientContext context;
        Empty empty_req;
        KVResponse check_health_res;
        Status status =
            it.second->CheckHealth(&context, empty_req, &check_health_res);
        if (status.ok()) {
          isAlive[it.first] = true;
          if (verbose_) {
            fprintf(stderr, "[Health Check] node %s in Cluster-%d is OK ...\n",
                    it.first.c_str(), cluster.id);
          }

        } else {
          isAlive[it.first] = false;
          if (verbose_) {
            fprintf(stderr,
                    "[Health Check] node %s in Cluster-%d Crashed ...\n",
                    it.first.c_str(), cluster.id);
          }

          if (cluster.primary.compare(it.first) == 0) {
            cluster.primary.clear();
          }
        }
      }
      // If primary node is done, select a new one.
      if (cluster.primary.empty()) {
        for (const auto& entry : isAlive) {
          if (entry.second) {
            cluster.primary = entry.first;
            if (verbose_) {
              fprintf(stderr,
                      "[Health Check] primary node in Cluster-%d changes to %s "
                      "...\n",
                      cluster.id, entry.first.c_str());
            }

            break;
          }
        }
      }
      if (verbose_) {
        if (cluster.primary.empty()) {
          fprintf(stderr,
                  "[Health Check] no KVStore nodes in Cluster-%d is available "
                  "... \n",
                  cluster.id);
        } else {
          fprintf(stderr, "[Health Check] primary node in Cluster-%d is %s\n",
                  cluster.id, cluster.primary.c_str());
        }
      }
    }
    if (verbose_) {
      fprintf(stderr, "\n");
    }
    pthread_mutex_unlock(&cluster_lock_);
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
      clusters_[cluster_id].isAlive[addr_str] = false;
      clusters_[cluster_id].stubs[addr_str] = KVStoreNode::NewStub(
          grpc::CreateChannel(addr_str, grpc::InsecureChannelCredentials()));
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
  pthread_mutex_lock(&cluster_lock_);
  std::string primary_addr = clusters_[hash_code].primary;
  pthread_mutex_unlock(&cluster_lock_);
  if (primary_addr.empty()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_error_message("-ERR service unavailable.");
  } else {
    response->set_status(KVStatusCode::SUCCESS);
    response->set_addr(primary_addr);
  }
  return Status::OK;
}

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